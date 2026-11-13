#!/usr/bin/env bash
# Builds a macOS installer: Scope-<version>-mac.dmg containing Scope-Installer.pkg,
# which installs the VST3, AU and Standalone app to the standard locations.
# Usage: packaging/mac/make_installer.sh <artefacts dir> <version> <out dir>
set -euo pipefail

ART="${1:?artefacts dir, e.g. build/Scope_artefacts/Release}"
VERSION="${2:?version}"
OUT="${3:?output dir}"
NAME="Scope"
ID="com.yourbrand.scope"
HERE="$(cd "$(dirname "$0")" && pwd)"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$OUT" "$WORK/pkgs"

component() { # $1 = bundle path, $2 = install location, $3 = id suffix
    local root="$WORK/root-$3"
    mkdir -p "$root"
    cp -R "$1" "$root/"
    pkgbuild --root "$root" --identifier "$ID.$3" --version "$VERSION" \
             --install-location "$2" "$WORK/pkgs/$NAME-$3.pkg"
}

component "$ART/VST3/$NAME.vst3"      "/Library/Audio/Plug-Ins/VST3"       vst3
component "$ART/AU/$NAME.component"   "/Library/Audio/Plug-Ins/Components" au
component "$ART/Standalone/$NAME.app" "/Applications"                      app

sed "s/@VERSION@/$VERSION/g" "$HERE/distribution.xml" > "$WORK/distribution.xml"
productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK/pkgs" \
             --resources "$HERE/resources" "$WORK/$NAME-Installer.pkg"

# Disk image with the installer and a short guide
DMGROOT="$WORK/dmg"
mkdir -p "$DMGROOT"
cp "$WORK/$NAME-Installer.pkg" "$DMGROOT/"
cp "$HERE/resources/HOW TO INSTALL.txt" "$DMGROOT/"
hdiutil create -volname "$NAME $VERSION" -srcfolder "$DMGROOT" -ov -format UDZO \
        "$OUT/$NAME-$VERSION-mac.dmg"
echo "Created $OUT/$NAME-$VERSION-mac.dmg"
