#!/usr/bin/env bash
set -euo pipefail
repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_dir"
node tests/models.test.cjs
node tests/actions.test.cjs
for test_name in crypto_test storage_test mpris_test; do
  test_dir="build/${test_name//_/-}"
  mkdir -p "$test_dir"
  qmake6 -o "$test_dir/Makefile" "tests/$test_name.pro"
  make -C "$test_dir" -j"${YUNJIAN_JOBS:-4}" > "$test_dir/build.log" 2>&1
done
./build/crypto-test/crypto-test tests/transport-vectors.json
ffmpeg -hide_banner -loglevel error -f lavfi -i sine=frequency=220:sample_rate=44100 -t 2 -c:a pcm_s16le -y build/test-audio.wav
YUNJIAN_TEST_AUDIO="$repo_dir/build/test-audio.wav" ./build/storage-test/storage-test
QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software ./build/mpris-test/mpris-test
if [[ "${YUNJIAN_NATIVE_TESTS:-0}" == 1 ]]; then
  mkdir -p build/native-test
  qmake6 -o build/native-test/Makefile tests/native_test.pro
  make -C build/native-test -j"${YUNJIAN_JOBS:-4}" > build/native-test/build.log 2>&1
  ffmpeg -hide_banner -loglevel error -f lavfi -i testsrc2=size=640x360:rate=24 -f lavfi -i sine=frequency=440:sample_rate=48000 -t 12 -c:v libx264 -pix_fmt yuv420p -c:a aac -y build/test-media.mp4
  YUNJIAN_TEST_MEDIA="$repo_dir/build/test-media.mp4" ./build/native-test/native-test
fi
