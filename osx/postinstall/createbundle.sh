#/bin/bash -ex

DEST=$1
BUILD_DIR=$2


# Make the top Level Bundle Out of the Launcher Bundle
mkdir -p "$DEST"
cp -r "$BUILD_DIR"/thymio-launcher.app/* "$DEST"

realpath() {
    [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
}

#Make sure the launcher is retina ready
defaults write $(realpath "$DEST/Contents/Info.plist") NSPrincipalClass -string NSApplication
defaults write $(realpath "$DEST/Contents/Info.plist") NSHighResolutionCapable -string True

APPS_DIR="$DEST/Contents/Applications"
BINUTILS_DIR="$DEST/Contents/MacOs"

#Copy the binaries we need
mkdir -p "$BINUTILS_DIR"
for binary in "thymio-device-manager" "asebacmd" "thymiownetconfig-cli"
do
    cp -r "$BUILD_DIR/$binary" "$BINUTILS_DIR"
done

#Copy the Applications
mkdir -p "$APPS_DIR/"
cp -r "${BUILD_DIR}/AsebaStudio.app" "$APPS_DIR/"
cp -r "${BUILD_DIR}/AsebaPlayground.app" "$APPS_DIR/"
cp -r "${BUILD_DIR}/ThymioVPLClassic.app" "$APPS_DIR/"

