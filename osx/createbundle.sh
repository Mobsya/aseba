#/bin/bash -ex

DEST=$1
BUILD_DIR=$2
IDENTITY="$3"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [[ $DEST =~ \.dmg$ ]]; then
    DMG="$1"
    DMG_DIR="$(mktemp -d)"
    DEST="$DMG_DIR/ThymioSuite.app"
fi

#hdiutil create -size 50m -fs HFS+ -volname Widget /path/to/save/widget.dmg


# Make the top Level Bundle Out of the Launcher Bundle
mkdir -p "$DEST"
cp -R "$BUILD_DIR"/thymio-launcher.app/* "$DEST"
rm -r "$DEST/Contents/Frameworks/QtWebEngine.framework"
rm -r "$DEST/Contents/Frameworks/QtWebEngineCore.framework"

if [ -d "$BUILD_DIR/scratch" ]; then
    cp -R "$BUILD_DIR/scratch" "$DEST/Contents/Resources"
fi

if [ -d "$BUILD_DIR/thymio_blockly" ]; then
    cp -R "$BUILD_DIR/thymio_blockly" "$DEST/Contents/Resources"
fi
if [ -d "$BUILD_DIR/vpl3-thymio-suite" ]; then
    cp -R "$BUILD_DIR/vpl3-thymio-suite" "$DEST/Contents/Resources"
fi

realpath() {
    [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
}

add_to_group() {
    defaults write "$1" "com.apple.security.application-groups" -array "P97H86YL8K.ThymioSuite"
}

sign() {
    if [ -z "$IDENTITY" ]; then
        echo "Identity not provided, not signing"
    else
        codesign --verify --options=runtime --verbose --timestamp -f -s "$IDENTITY" "$@"
    fi
}

defaults write $(realpath "$DEST/Contents/Info.plist") NSPrincipalClass -string NSApplication
defaults write $(realpath "$DEST/Contents/Info.plist") NSHighResolutionCapable -string False
add_to_group $(realpath "$DEST/Contents/Info.plist")
chmod 644 $(realpath "$DEST/Contents/Info.plist")

APPS_DIR="$DEST/Contents/Applications"
BINUTILS_DIR="$DEST/Contents/Helpers"
MAIN_DIR="$DEST/Contents/MacOS"

#Copy the binaries we need
mkdir -p "$MAIN_DIR"
mkdir -p "$BINUTILS_DIR"
for binary in "thymio-device-manager" "thymio2-firmware-upgrader"
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
    defaults write $(realpath "$APPS_DIR/$app.app/Contents/Info.plist") NSHighResolutionCapable -string False
    add_to_group $(realpath "$APPS_DIR/$app.app/Contents/Info.plist")

    plutil -replace CFBundleURLTypes -xml "
        <array>
            <dict>
                <key>CFBundleTypeRole</key>
                <string>Viewer</string>
                <key>CFBundleURLName</key>
                <string>org.mobsy.CustomURLScheme</string>
                <key>CFBundleURLSchemes</key>
                <array>
                    <string>mobsya</string>
                    </array>
            </dict>
        </array>
    " $(realpath "$APPS_DIR/$app.app/Contents/Info.plist")



    defaults read $(realpath "$APPS_DIR/$app.app/Contents/Info.plist")
	chmod 644 $(realpath "$APPS_DIR/$app.app/Contents/Info.plist")

done

for app in "AsebaStudio" "AsebaPlayground" "ThymioVPLClassic"
do
    echo "Signing $APPS_DIR/$app.app/ with $DIR/inherited.entitlements"
    sign --deep $(realpath "$APPS_DIR/$app.app/")
done

for fw in $(ls "$DEST/Contents/Frameworks")
do
    echo "Signing $DEST/Contents/Frameworks/$fw"
    sign $(realpath "$DEST/Contents/Frameworks/$fw")
done

for plugin in $(find $DEST/Contents/PlugIns -name '*.dylib')
do
    echo "Signing $plugin"
    sign $(realpath "$plugin")
done

for binary in "thymio-device-manager" "thymio2-firmware-upgrader"
do
    echo "Signing $BINUTILS_DIR/$binary with $DIR/inherited.entitlements"
    sign -i org.mobsya.ThymioLauncher.$binary $(realpath "$BINUTILS_DIR/$binary")
done

echo "Signing $DEST with $DIR/launcher.entitlements"
sign $(realpath "$MAIN_DIR/thymio-launcher")

if [ -n "$DMG" ]; then
    test -f "$1" && rm "$DMG"
    "$DIR/dmg/create-dmg" \
    --volname "Thymio Suite" \
    --volicon "$DIR/../menu/osx/launcher.icns" \
    --background "$DIR/background.png" \
    --window-pos 200 120 \
    --window-size 620 470 \
    --icon-size 100 \
    --icon "ThymioSuite.app" 100 300 \
    --hide-extension "ThymioSuite.app" \
    --app-drop-link 500 300 \
    "$DMG" \
    "$DMG_DIR/ThymioSuite.app"

    sign -f "$1"
fi