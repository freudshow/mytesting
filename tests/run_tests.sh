#!/usr/bin/env bash
set -euo pipefail
WD=$(cd "$(dirname "$0")" && pwd)
SRC_DIR=$(dirname "$WD")/src/misc
BIN=$WD/eval_test
EXPR_FILE=$WD/expressions.txt
EXPECTED=$WD/expected_results.txt
TMP_OUT=$WD/out_results.txt

# compile
cc -Wall -Wextra -std=c11 "$SRC_DIR"/eval.c "$WD"/test_main.c -o "$BIN" -lm

# run evaluator with expressions
# evaluator stops on empty line, ensure expressions file ends with a blank line
cat "$EXPR_FILE" | "$BIN" > "$WD/full_output.txt" 2>&1

# extract Result: lines
grep '^Result:' "$WD/full_output.txt" | sed 's/^Result: //' > "$TMP_OUT"

# compare using python with tolerance
python3 - <<PY
exp = open('$EXPECTED').read().strip().splitlines()
out = open('$TMP_OUT').read().strip().splitlines()
if len(exp) != len(out):
    print(f'FAIL: expected {len(exp)} results, got {len(out)}')
    print('Full output:\n')
    print(open('$WD/full_output.txt').read())
    exit(2)

def isclose(a,b,rel_tol=1e-6,abs_tol=1e-6):
    return abs(a-b) <= max(rel_tol*max(abs(a),abs(b)), abs_tol)

for i,(e,o) in enumerate(zip(exp,out),1):
    try:
        ev = float(e)
        ov = float(o)
    except Exception as ex:
        if e.strip() != o.strip():
            print(f'FAIL on line {i}: expected "{e}", got "{o}"')
            exit(2)
        print(f'OK {i}: {o}')
        continue
    if isclose(ev, ov):
        print(f'OK {i}: {o}')
    else:
        print(f'FAIL {i}: expected {ev}, got {ov}')
        exit(2)
print('\nALL TESTS PASSED')
PY