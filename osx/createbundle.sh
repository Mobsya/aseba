#/bin/bash -ex

DEST=$1
BUILD_DIR=$2
IDENTITY="$3"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"





# Make the top Level Bundle Out of the Launcher Bundle
mkdir -p "$DEST"
cp -R "$BUILD_DIR"/thymio-launcher.app/* "$DEST"

realpath() {
    [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
}

add_to_group() {
    defaults write "$1" "com.apple.security.application-groups" -array "P97H86YL8K.ThymioSuite"
}

#Make sure the launcher is retina ready
defaults write $(realpath "$DEST/Contents/Info.plist") NSPrincipalClass -string NSApplication
defaults write $(realpath "$DEST/Contents/Info.plist") NSHighResolutionCapable -string True
add_to_group $(realpath "$DEST/Contents/Info.plist")

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
cp -R "${BUILD_DIR}/AsebaStudio.app" "$APPS_DIR/"
cp -R "${BUILD_DIR}/AsebaPlayground.app" "$APPS_DIR/"
cp -R "${BUILD_DIR}/ThymioVPLClassic.app" "$APPS_DIR/"

for app in "AsebaStudio" "AsebaPlayground" "ThymioVPLClassic"
do
    cp -r "${BUILD_DIR}/$app.app" "$APPS_DIR/"
    defaults write $(realpath "$APPS_DIR/$app.app/Contents/Info.plist") NSPrincipalClass -string NSApplication
    defaults write $(realpath "$APPS_DIR/$app.app/Contents/Info.plist") NSHighResolutionCapable -string True
    add_to_group $(realpath "$APPS_DIR/$app.app/Contents/Info.plist")
done

for app in "AsebaStudio" "AsebaPlayground" "ThymioVPLClassic"
do
    echo "Signing $APPS_DIR/$app.app/ with $DIR/inherited.entitlements"
    codesign --verbose -f -s "$IDENTITY" --deep $(realpath "$APPS_DIR/$app.app/")
    #--entitlements "$DIR/inherited.entitlements"
done

for fw in $(ls "$DEST/Contents/Frameworks")
do
    echo "Signing $DEST/Contents/Frameworks/$fw"
    codesign --verbose -f -s "$IDENTITY" --deep $(realpath "$DEST/Contents/Frameworks/$fw")
done

for plugin in $(find $DEST/Contents/PlugIns -name '*.dylib')
do
    echo "Signing $plugin"
    codesign --verbose -f -s "$IDENTITY" --deep $(realpath "$plugin")
done

for binary in "thymio-device-manager" "asebacmd" "thymiownetconfig-cli"
do
    echo "Signing $BINUTILS_DIR/$binary with $DIR/inherited.entitlements"
    codesign --verbose -f -s "$IDENTITY" \
    --deep $(realpath "$BINUTILS_DIR/$binary")
    #--entitlements "$DIR/inherited.entitlements"
done

echo "Signing $DEST with $DIR/launcher.entitlements"
codesign --verbose --verify -f -s "$IDENTITY"  $(realpath "$BINUTILS_DIR/thymio-launcher")
#--entitlements "$DIR/launcher.entitlements"