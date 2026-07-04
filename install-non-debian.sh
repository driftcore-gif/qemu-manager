#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════
# QEMU Manager — Universal Installer for Non-Debian Linux Distributions
# Supports 30+ distro families / package managers
# Usage: sudo bash install-non-debian.sh [--arch ARCH] [--prefix PREFIX]
# ═══════════════════════════════════════════════════════════════════════
set -euo pipefail

VERSION="10.0-1"
REPO="driftcore-gif/qemu-manager"
PREFIX="${PREFIX:-/usr/local}"

RED='\033[0;31m' GRN='\033[0;32m' YEL='\033[1;33m' CYN='\033[0;36m' RST='\033[0m'
info()  { echo -e "${CYN}[INFO]${RST}  $*"; }
ok()    { echo -e "${GRN}[OK]${RST}    $*"; }
warn()  { echo -e "${YEL}[WARN]${RST}  $*"; }
die()   { echo -e "${RED}[ERROR]${RST} $*" >&2; exit 1; }

detect_arch() {
    case "$(uname -m)" in
        x86_64|amd64)        echo "amd64" ;;
        aarch64|arm64)       echo "arm64" ;;
        armv7*|armhf|armv6*) echo "armhf" ;;
        i386|i486|i586|i686) echo "i386"  ;;
        *) die "Unsupported architecture: $(uname -m)" ;;
    esac
}

detect_distro() {
    if   [ -f /etc/os-release ];   then . /etc/os-release; echo "${ID:-unknown}|${ID_LIKE:-}|${VERSION_ID:-}"
    elif [ -f /etc/redhat-release ]; then echo "rhel||"
    elif [ -f /etc/arch-release ];   then echo "arch||"
    elif [ -f /etc/gentoo-release ]; then echo "gentoo||"
    else echo "unknown||"; fi
}

install_deps() {
    local id="$1" like="$2"
    info "Installing GTK4 dependencies for: $id"

    # 1. Arch / Manjaro / EndeavourOS / Garuda / ArcoLinux / CachyOS / Parabola / Hyperbola
    if echo "$id $like" | grep -qiE "arch|manjaro|endeavouros|garuda|arcolinux|cachyos|parabola|hyperbola"; then
        pacman -Sy --needed --noconfirm gtk4 glib2 pango cairo graphene 2>/dev/null || warn "pacman: some packages failed"

    # 2. Fedora (including Nobara / Ultramarine / Bazzite / Aurora)
    elif echo "$id $like" | grep -qiE "fedora|nobara|ultramarine|bazzite|aurora"; then
        ( command -v dnf5 &>/dev/null && dnf5 install -y gtk4 glib2 pango cairo ) || \
        dnf install -y gtk4 glib2 pango cairo || warn "dnf: some packages failed"

    # 3. RHEL / CentOS / AlmaLinux / RockyLinux / Oracle Linux / EuroLinux
    elif echo "$id $like" | grep -qiE "rhel|centos|almalinux|rocky|oracle|eurolinux"; then
        ( command -v dnf &>/dev/null && dnf install -y gtk4 glib2 pango cairo ) || \
        yum install -y gtk4 glib2 pango cairo || warn "yum/dnf: some packages failed"

    # 4. openSUSE Leap / Tumbleweed / SLES / GeckoLinux / OpenMandriva
    elif echo "$id $like" | grep -qiE "opensuse|suse|sles|gecko|openmandriva"; then
        zypper --non-interactive install gtk4 glib2-tools pango cairo graphene || warn "zypper: some packages failed"

    # 5. Gentoo / Funtoo / Calculate / Sabayon (now Entropy)
    elif echo "$id $like" | grep -qiE "gentoo|funtoo|calculate|sabayon"; then
        emerge --ask=n "x11-libs/gtk+:4" dev-libs/glib x11-libs/pango x11-libs/cairo || warn "emerge: some packages failed"

    # 6. Alpine Linux / postmarketOS / Crystal Linux
    elif echo "$id $like" | grep -qiE "alpine|postmarketos|crystal"; then
        apk add --no-cache gtk4.0 glib pango cairo || warn "apk: some packages failed"

    # 7. Void Linux / Artix (runit/s6/openrc with Void repos)
    elif echo "$id $like" | grep -qiE "void"; then
        xbps-install -Sy gtk4 glib pango cairo || warn "xbps: some packages failed"

    # 8. Solus
    elif echo "$id $like" | grep -qiE "solus"; then
        eopkg install -y gtk4-devel glib2-devel pango-devel cairo-devel || warn "eopkg: some packages failed"

    # 9. NixOS
    elif echo "$id $like" | grep -qiE "nixos"; then
        warn "NixOS detected. Add to /etc/nixos/configuration.nix:"
        echo "  environment.systemPackages = with pkgs; [ gtk4 glib pango cairo ];"
        warn "Or run: nix-env -iA nixpkgs.gtk4 nixpkgs.glib nixpkgs.pango nixpkgs.cairo"

    # 10. Clear Linux (Intel)
    elif echo "$id $like" | grep -qiE "clearlinux|clear"; then
        swupd bundle-add gtk-libs || warn "swupd: bundle add failed"

    # 11. Mageia
    elif echo "$id $like" | grep -qiE "mageia"; then
        urpmi --auto lib64gtk4.0 lib64glib2.0_0 lib64pango1.0_0 lib64cairo2 || \
        dnf install -y gtk4 glib2 pango cairo || warn "mageia: some packages failed"

    # 12. PCLinuxOS
    elif echo "$id $like" | grep -qiE "pclinuxos"; then
        apt-get install -y lib64gtk4.0 lib64glib2.0_0 || warn "pclinuxos: some packages failed"

    # 13. Slackware / Salix / Slackel
    elif echo "$id $like" | grep -qiE "slackware|salix|slackel"; then
        warn "Slackware family detected. Install via SlackBuilds.org:"
        warn "  gtk4, glib, pango, cairo — https://slackbuilds.org"

    # 14. KaOS
    elif echo "$id $like" | grep -qiE "kaos"; then
        pacman -Sy --needed --noconfirm gtk4 glib2 pango cairo || warn "kaos pacman: failed"

    # 15. Artix Linux (arch-based but systemd-free)
    elif echo "$id $like" | grep -qiE "artix"; then
        pacman -Sy --needed --noconfirm gtk4 glib2 pango cairo graphene || warn "artix: failed"

    # 16. MX Linux / antiX (Debian-based but apt may not handle GTK4 sources well)
    elif echo "$id $like" | grep -qiE "mxlinux|antix|mx"; then
        apt-get install -y libgtk-4-1 libglib2.0-0 libpango-1.0-0 libcairo2 || warn "mx: some packages failed"

    # 17. Trisquel / gNewSense (fully free distros)
    elif echo "$id $like" | grep -qiE "trisquel|gnewsense"; then
        apt-get install -y libgtk-4-1 libglib2.0-0 libpango-1.0-0 || warn "trisquel: some packages failed"

    # 18. Chimera Linux (musl-based)
    elif echo "$id $like" | grep -qiE "chimera"; then
        apk add gtk4.0 glib pango cairo || warn "chimera apk: failed"

    # 19. Haiku (Linux binary compatibility layer — experimental)
    elif echo "$id $like" | grep -qiE "haiku"; then
        warn "Haiku detected — GTK4 support is experimental. Try: pkgman install gtk4"

    # 20. Termux (Android pseudo-distro)
    elif [ -d /data/data/com.termux ] || echo "$PREFIX" | grep -q termux; then
        info "Termux environment detected"
        pkg install -y gtk4 glib pango || warn "Termux: some packages failed"

    # 21. Guix System
    elif echo "$id $like" | grep -qiE "guix"; then
        warn "Guix detected. Run: guix install gtk+ glib pango cairo"

    # 22. Bedrock Linux (meta-distro — detect via brl)
    elif command -v brl &>/dev/null; then
        warn "Bedrock Linux detected. Use your stratum's package manager to install GTK4."
        brl list 2>/dev/null || true

    # 23. OpenWrt / embedded (unlikely but handle gracefully)
    elif echo "$id $like" | grep -qiE "openwrt|lede"; then
        warn "OpenWrt detected — GTK4 is not available on embedded targets."
        warn "QEMU Manager requires a desktop Linux environment."
        exit 1

    # 24. Raspberry Pi OS (Debian-based — handled by standard apt but flag it)
    elif echo "$id $like" | grep -qiE "raspbian|raspberrypi"; then
        apt-get install -y libgtk-4-1 libglib2.0-0 libpango-1.0-0 || warn "raspbian: failed"

    # 25. Parrot OS / Kali (Debian-based security distros)
    elif echo "$id $like" | grep -qiE "parrot|kali"; then
        apt-get install -y libgtk-4-1 libglib2.0-0 libpango-1.0-0 || warn "kali/parrot: failed"

    # 26. Pop!_OS (Ubuntu-based)
    elif echo "$id $like" | grep -qiE "pop|popos"; then
        apt-get install -y libgtk-4-1 libglib2.0-0 libpango-1.0-0 libcairo2 || warn "pop: failed"

    # 27. Linux Mint / LMDE
    elif echo "$id $like" | grep -qiE "linuxmint|lmde"; then
        apt-get install -y libgtk-4-1 libglib2.0-0 libpango-1.0-0 libcairo2 || warn "mint: failed"

    # 28. elementary OS
    elif echo "$id $like" | grep -qiE "elementary"; then
        apt-get install -y libgtk-4-1 libglib2.0-0 libpango-1.0-0 || warn "elementary: failed"

    # 29. deepin
    elif echo "$id $like" | grep -qiE "deepin"; then
        apt-get install -y libgtk-4-1 libglib2.0-0 libpango-1.0-0 || warn "deepin: failed"

    # 30. Unknown — try everything
    else
        warn "Unknown distro ($id). Trying available package managers..."
        if   command -v pacman     &>/dev/null; then pacman -Sy --needed --noconfirm gtk4 glib2 pango cairo
        elif command -v dnf5       &>/dev/null; then dnf5 install -y gtk4 glib2 pango cairo
        elif command -v dnf        &>/dev/null; then dnf install -y gtk4 glib2 pango cairo
        elif command -v yum        &>/dev/null; then yum install -y gtk4 glib2 pango cairo
        elif command -v zypper     &>/dev/null; then zypper -n install gtk4 glib2 pango cairo
        elif command -v emerge     &>/dev/null; then emerge --ask=n gtk+:4 glib pango cairo
        elif command -v apk        &>/dev/null; then apk add --no-cache gtk4.0 glib pango cairo
        elif command -v xbps-install &>/dev/null; then xbps-install -Sy gtk4 glib pango cairo
        elif command -v eopkg      &>/dev/null; then eopkg install -y gtk4 glib2 pango cairo
        elif command -v swupd      &>/dev/null; then swupd bundle-add gtk-libs
        elif command -v apt-get    &>/dev/null; then apt-get install -y libgtk-4-1 libglib2.0-0 libpango-1.0-0
        else warn "No supported package manager found. Install GTK4 manually."; fi
    fi
}

download_and_extract() {
    local arch="$1"
    local url="https://github.com/${REPO}/releases/download/v${VERSION}/qemu-manager_${VERSION}_${arch}.deb"
    local workdir="/tmp/qemu-manager-install-$$"
    mkdir -p "$workdir/extracted"

    info "Downloading v${VERSION} [${arch}]..."
    if command -v curl &>/dev/null; then
        curl -fsSL --progress-bar "$url" -o "$workdir/pkg.deb"
    elif command -v wget &>/dev/null; then
        wget -q --show-progress "$url" -O "$workdir/pkg.deb"
    else
        die "curl or wget required. Install one and retry."
    fi

    info "Extracting..."
    if command -v dpkg-deb &>/dev/null; then
        dpkg-deb --extract "$workdir/pkg.deb" "$workdir/extracted"
    elif command -v ar &>/dev/null; then
        cd "$workdir"
        ar x pkg.deb
        if   [ -f data.tar.xz  ]; then tar -xJf data.tar.xz -C extracted
        elif [ -f data.tar.gz  ]; then tar -xzf data.tar.gz -C extracted
        elif [ -f data.tar.zst ]; then tar --use-compress-program=zstd -xf data.tar.zst -C extracted
        else die "Unknown .deb data archive format"; fi
        cd - >/dev/null
    else
        die "dpkg-deb or ar (binutils) required for extraction."
    fi
    echo "$workdir"
}

do_install() {
    local arch="$1"
    local workdir; workdir=$(download_and_extract "$arch")
    local BIN_DIR="$PREFIX/bin"
    local APP_DIR="$PREFIX/share/applications"
    local PIX_DIR="$PREFIX/share/pixmaps"
    local ICO_DIR="$PREFIX/share/icons/hicolor/256x256/apps"

    mkdir -p "$BIN_DIR" "$APP_DIR" "$PIX_DIR" "$ICO_DIR"

    cp -f "$workdir/extracted/usr/bin/qemu-manager" "$BIN_DIR/qemu-manager"
    chmod 755 "$BIN_DIR/qemu-manager"

    # .desktop
    local desk="$workdir/extracted/usr/share/applications/qemu-manager.desktop"
    [ -f "$desk" ] && sed "s|Exec=qemu-manager|Exec=$BIN_DIR/qemu-manager|" "$desk" > "$APP_DIR/qemu-manager.desktop"

    # Icon
    for p in "$workdir/extracted/usr/share/pixmaps/qemu-manager.png" \
              "$workdir/extracted/usr/share/icons/hicolor/256x256/apps/qemu-manager.png"; do
        if [ -f "$p" ]; then cp -f "$p" "$PIX_DIR/"; cp -f "$p" "$ICO_DIR/"; break; fi
    done

    command -v update-desktop-database &>/dev/null && update-desktop-database "$APP_DIR" 2>/dev/null || true
    command -v gtk-update-icon-cache   &>/dev/null && gtk-update-icon-cache -f -t "$PREFIX/share/icons/hicolor" 2>/dev/null || true

    # Cleanup workdir
    find "$workdir" -maxdepth 0 -type d -exec rm -rf {} + 2>/dev/null || true

    ok "Installed: $BIN_DIR/qemu-manager"
}

do_uninstall() {
    local BIN_DIR="$PREFIX/bin"
    info "Removing QEMU Manager..."
    rm -f "$BIN_DIR/qemu-manager"
    rm -f "$PREFIX/share/applications/qemu-manager.desktop"
    rm -f "$PREFIX/share/pixmaps/qemu-manager.png"
    rm -f "$PREFIX/share/icons/hicolor/256x256/apps/qemu-manager.png"
    command -v update-desktop-database &>/dev/null && update-desktop-database "$PREFIX/share/applications" 2>/dev/null || true
    ok "Uninstalled."
}

# ── Arg parsing ─────────────────────────────────────────────────────────
UNINSTALL=0; SKIP_DEPS=0; OVERRIDE_ARCH=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --arch)      OVERRIDE_ARCH="$2"; shift 2 ;;
        --prefix)    PREFIX="$2"; shift 2 ;;
        --uninstall) UNINSTALL=1; shift ;;
        --skip-deps) SKIP_DEPS=1; shift ;;
        --help|-h)
            echo "Usage: sudo bash install-non-debian.sh [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --arch ARCH        Force architecture: amd64 | arm64 | armhf | i386"
            echo "  --prefix PATH      Install prefix (default: /usr/local)"
            echo "  --uninstall        Remove QEMU Manager"
            echo "  --skip-deps        Skip GTK4 dependency installation"
            echo "  --help             Show this help"
            echo ""
            echo "Supported distros (30+):"
            echo "  Arch, Manjaro, EndeavourOS, Garuda, ArcoLinux, CachyOS, Parabola"
            echo "  Fedora, Nobara, Ultramarine, Bazzite, RHEL, CentOS, AlmaLinux, Rocky"
            echo "  openSUSE, SLES, Mageia, PCLinuxOS, openMandriva"
            echo "  Gentoo, Funtoo, Calculate, Void, Solus, KaOS, Artix, Chimera"
            echo "  Alpine, postmarketOS, NixOS, Guix, Clear Linux, Bedrock"
            echo "  Trisquel, MX Linux, antiX, Pop!_OS, Mint, elementary, deepin, Parrot, Kali"
            echo "  Termux (Android), Raspberry Pi OS, Slackware"
            exit 0 ;;
        *) warn "Unknown option: $1"; shift ;;
    esac
done

echo ""
echo -e "${CYN}╔══════════════════════════════════════════════════════════════╗${RST}"
echo -e "${CYN}║  QEMU Manager v${VERSION} — Universal Installer (Non-Debian)   ║${RST}"
echo -e "${CYN}╚══════════════════════════════════════════════════════════════╝${RST}"
echo ""

IFS='|' read -r DISTRO_ID DISTRO_LIKE DISTRO_VER <<< "$(detect_distro)"
ARCH="${OVERRIDE_ARCH:-$(detect_arch)}"
info "Distro  : $DISTRO_ID (like: ${DISTRO_LIKE:-none}) ${DISTRO_VER}"
info "Arch    : $ARCH"
info "Prefix  : $PREFIX"
echo ""

[ "$UNINSTALL" -eq 1 ] && { do_uninstall; exit 0; }
[ "$SKIP_DEPS" -eq 0 ] && install_deps "$DISTRO_ID" "$DISTRO_LIKE"
do_install "$ARCH"

echo ""
echo -e "${GRN}Done! Run: qemu-manager${RST}"
echo ""
