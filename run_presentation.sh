#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
EXECUTABLE="$BUILD_DIR/compiler"

echo "=== Compiler152Project Presentation Script ==="

# Build using CMake if needed
if [ ! -x "$EXECUTABLE" ]; then
  echo "Building project..."
  mkdir -p "$BUILD_DIR"
  cd "$BUILD_DIR"
  cmake ..
  make
  cd "$PROJECT_ROOT"
else
  echo "Using existing build: $EXECUTABLE"
fi

# Present files to run
TEST_FILES=(
  "test/test.py"
  "test/case1.py"
  "test/case2.py"
  "test/test_irgen.py"
)

for test_file in "${TEST_FILES[@]}"; do
  echo
  echo "--- Running $test_file ---"
  "$EXECUTABLE" "$PROJECT_ROOT/$test_file"
done

echo
echo "=== Presentation complete ==="
