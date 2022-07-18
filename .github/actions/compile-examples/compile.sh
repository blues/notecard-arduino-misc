set -e
SKETCH="$1"
BOARD="$2"
LIBS="$3"

HOME=/home/blues
IFS=';'
read -ra libraries <<< "$LIBS"
for lib in "${libraries[@]}"; do arduino-cli lib install $lib; done

arduino-cli compile \
      --build-property compiler.cpp.extra_flags='-Wno-unused-parameter -Werror' \
      --fqbn "$BOARD" \
      --log-level trace \
      --verbose \
      --warnings all \
      "$SKETCH"
