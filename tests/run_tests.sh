#!/usr/bin/env bash
set -euo pipefail

# First argument: path to ezlang executable (we'll pass it from CMake/CI)
EZLANG_EXE="${1:-./ezlang}"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_DIR="$ROOT_DIR/tests"

echo "Running EzLang tests..."
echo "Using interpreter: $EZLANG_EXE"
echo

FAILED=0

for ez in "$TEST_DIR"/*.ez; do
    name="$(basename "$ez" .ez)"
    expected="$TEST_DIR/$name.out"

    if [[ ! -f "$expected" ]]; then
        echo "[WARN] No expected output file for $name (missing $name.out), skipping"
        continue
    fi

    echo "==> $name"

    actual="$(mktemp)"
    if ! "$EZLANG_EXE" "$ez" >"$actual" 2>&1; then
        echo "  [FAIL] Interpreter exited with non-zero status"
        echo "  [stderr/stdout]:"
        sed 's/^/    /' "$actual"
        FAILED=1
    else
        if ! diff -u "$expected" "$actual" >/dev/null 2>&1; then
            echo "  [FAIL] Output mismatch:"
            diff -u "$expected" "$actual" | sed 's/^/    /'
            FAILED=1
        else
            echo "  [OK]"
        fi
    fi

    rm -f "$actual"
    echo
done

if [[ "$FAILED" -ne 0 ]]; then
    echo "Some tests FAILED."
    exit 1
fi

echo "All tests passed."
