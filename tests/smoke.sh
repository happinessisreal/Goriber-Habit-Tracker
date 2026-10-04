#!/usr/bin/env bash
# End-to-end smoke test: drive the real binary through stdin and check behaviour.
set -euo pipefail
BIN=${1:-./habit-tracker}
export HABIT_DATA=$(mktemp) NO_COLOR=1
rm -f "$HABIT_DATA"
trap 'rm -f "$HABIT_DATA"' EXIT
fail() { echo "FAIL: $*"; echo "----- output -----"; echo "$out"; exit 1; }
# `timeout` guards against input loops; macOS has no coreutils timeout by default
run() { if command -v timeout >/dev/null; then timeout 10 "$BIN"; else "$BIN"; fi; }

# 1) junk input must not loop forever; create user, add to-dos, mark one done, a stopwatch session, exit
out=$(printf 'abc\n1\nrafi\n\n2\nrafi\n3\n1\nRead chapter 3\nSolve 5 problems\n\n2\n1\n3\n1\n1\nMath\n\n\n2\n\n4\n3\n' | run)
grep -q "Please enter a number" <<<"$out" || fail "junk input not rejected"
grep -q "User created" <<<"$out"         || fail "user not created"
grep -q "\[✓\] Read chapter 3" <<<"$out"  || fail "to-do not marked done"
grep -q "\[ \] Solve 5 problems" <<<"$out" || fail "second to-do missing"
grep -q "Logged 00:00:0[0-9] of Math" <<<"$out" || fail "stopwatch session not logged"
grep -q "Current streak   1 day" <<<"$out" || fail "streak not 1 day"

# 2) data persists across runs
out=$(printf '2\nrafi\n3\n3\n2\n\n4\n3\n' | run)
grep -q "\[✓\] Read chapter 3" <<<"$out" || fail "to-dos not persisted"
grep -q "Sessions         1" <<<"$out"   || fail "sessions not persisted"

# 3) duplicate users and unknown users are rejected; EOF exits cleanly
out=$(printf '1\nrafi\n\n2\nnobody\n\n' | run)
grep -q "already exists" <<<"$out" || fail "duplicate user accepted"
grep -q "User not found" <<<"$out" || fail "unknown user accepted"

echo "smoke tests passed"
