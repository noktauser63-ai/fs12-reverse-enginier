#!/usr/bin/env bash
# Kullanim: make_apk.sh <cikis.apk> <assets_dir> <abi:libapp.so> [<abi:libapp.so> ...]
# Gereken: Android SDK build-tools (aapt2, zipalign, apksigner) + platforms/android-XX/android.jar
set -euo pipefail
OUT=$(realpath -m "$1"); ASSETS=$(realpath "$2"); shift 2
LIBS=()
for e in "$@"; do LIBS+=("${e%%:*}:$(realpath "${e#*:}")"); done   # cd'den once mutlak yola cevir
HERE=$(cd "$(dirname "$0")/.." && pwd)
SDK="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"
BT=$(ls -d "$SDK"/build-tools/* | sort -V | tail -1)
JAR=$(ls -d "$SDK"/platforms/android-*/android.jar | sort -V | tail -1)
echo "build-tools: $BT  android.jar: $JAR"
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
"$BT/aapt2" compile --dir "$HERE/packaging/res" -o "$W/res.zip"      # launcher ikonu (kapak)
"$BT/aapt2" link -o "$W/base.apk" -I "$JAR" --manifest "$HERE/packaging/AndroidManifest.xml" "$W/res.zip" \
    --min-sdk-version 24 --target-sdk-version 35 --version-code 1 --version-name 0.1-smoke
mkdir -p "$W/pkg"; cd "$W/pkg"
for e in "${LIBS[@]}"; do abi=${e%%:*}; so=${e#*:}; mkdir -p "lib/$abi"; cp "$so" "lib/$abi/libapp.so"; done
mkdir -p assets; cp -r "$ASSETS"/. assets/
cp ../base.apk ../unsigned.apk   # aapt2 ciktisi (manifest + resources.arsc) oldugu gibi korunur
zip -q -X -0 -r ../unsigned.apk lib          # .so dosyalari sikistirilmaz (extractNativeLibs=false)
zip -q -X -r ../unsigned.apk assets
cd "$W"
"$BT/zipalign" -f -P 16 4 unsigned.apk aligned.apk
[ -f "$HERE/packaging/debug.keystore" ] || keytool -genkeypair -keystore "$W/ks" -storepass android -keypass android \
    -alias fs12 -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=FS12 Debug" >/dev/null 2>&1
KS="$W/ks"; [ -f "$HERE/packaging/debug.keystore" ] && KS="$HERE/packaging/debug.keystore"
"$BT/apksigner" sign --ks "$KS" --ks-pass pass:android --key-pass pass:android --ks-key-alias fs12 --out "$OUT" aligned.apk
"$BT/apksigner" verify --verbose "$OUT" | head -5
"$BT/zipalign" -c -P 16 4 "$OUT" && echo "16 KB hiza: OK"
ls -l "$OUT"
