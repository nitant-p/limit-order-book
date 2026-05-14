#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR_INPUT="${1:-$(pwd)}"
BUILD_DIR="$(cd "${BUILD_DIR_INPUT}" && pwd)"
GCOV_BIN="$(command -v gcov || true)"

if [[ -z "${GCOV_BIN}" ]]; then
  echo "gcov is not available on PATH."
  exit 1
fi

OBJ_DIR="${BUILD_DIR}/CMakeFiles/order_book_lib.dir/src"
if [[ ! -d "${OBJ_DIR}" ]]; then
  echo "Expected object directory not found: ${OBJ_DIR}"
  exit 1
fi

mkdir -p "${BUILD_DIR}/coverage"
pushd "${BUILD_DIR}/coverage" >/dev/null

echo "Generating gcov reports from: ${OBJ_DIR}"
: > coverage_summary.txt

GCDA_FILES=()
while IFS= read -r gcda_path; do
  GCDA_FILES+=("${gcda_path}")
done < <(find "${OBJ_DIR}" -maxdepth 1 -name '*.gcda' | sort)

if [[ ${#GCDA_FILES[@]} -eq 0 ]]; then
  echo "No .gcda files found under ${OBJ_DIR}. Run tests first." | tee -a coverage_summary.txt
  exit 1
fi

for gcda_file in "${GCDA_FILES[@]}"; do
  "${GCOV_BIN}" -b -c "${gcda_file}" | tee -a coverage_summary.txt
done

echo "Coverage artifacts written to: ${BUILD_DIR}/coverage"
ls -1 *.gcov 2>/dev/null || true

popd >/dev/null
