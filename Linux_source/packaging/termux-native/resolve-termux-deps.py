import re, sys, json

def parse_packages(path, repo_url):
    pkgs = {}
    with open(path, encoding='utf-8', errors='replace') as f:
        content = f.read()
    blocks = content.split("\n\n")
    for b in blocks:
        b = b.strip()
        if not b: continue
        fields = {}
        cur_key = None
        for line in b.split("\n"):
            if line.startswith(" ") and cur_key:
                fields[cur_key] += "\n" + line
                continue
            if ":" in line:
                k, v = line.split(":", 1)
                cur_key = k.strip()
                fields[cur_key] = v.strip()
        if "Package" in fields:
            name = fields["Package"]
            deps = []
            if "Depends" in fields:
                for d in fields["Depends"].split(","):
                    d = d.strip()
                    if not d: continue
                    # take first alternative, strip version constraint
                    alt = d.split("|")[0].strip()
                    pkgname = re.split(r'[\s(]', alt)[0].strip()
                    if pkgname:
                        deps.append(pkgname)
            pkgs[name] = {
                "version": fields.get("Version",""),
                "filename": fields.get("Filename",""),
                "depends": deps,
                "repo": repo_url,
            }
    return pkgs

main = parse_packages("/app/termux-native/repo/main-aarch64-Packages", "https://packages.termux.dev/apt/termux-main/")
x11 = parse_packages("/app/termux-native/repo/x11-Packages", "https://packages.termux.dev/apt/termux-x11/")

all_pkgs = {}
all_pkgs.update(main)
all_pkgs.update(x11)  # x11 takes precedence for overlapping names like gtk4

print(f"main pkgs: {len(main)}, x11 pkgs: {len(x11)}, total unique: {len(all_pkgs)}")

def resolve(root):
    seen = set()
    order = []
    missing = set()
    def visit(name):
        if name in seen: return
        seen.add(name)
        if name not in all_pkgs:
            missing.add(name)
            return
        for d in all_pkgs[name]["depends"]:
            visit(d)
        order.append(name)
    visit(root)
    return order, missing

order, missing = resolve("gtk4")
print(f"\nTotal packages needed (incl. gtk4): {len(order)}")
print("Missing (not found in either repo):", missing)
with open("/app/termux-native/resolved_pkgs.json","w") as f:
    json.dump({"order": order, "pkgs": {k: all_pkgs[k] for k in order}}, f, indent=2)
for p in order:
    print(" -", p, all_pkgs[p]["version"])
