#!/bin/sh
# Cold Boot scoping pass for the Rainbow Array Machine.
# Reads COLD-BOOT.md, verifies the seal, the license, and the
# baseline, then re-verifies the tree by execution. Prints PROVEN
# (this run) separately from DECLARED (the sealed fields). Fails
# closed: any broken gate makes the whole pass fail.
set -u
cd "$(dirname "$0")"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
FAILED=0

field() { sed -n "s/^$1: //p" COLD-BOOT.md | head -n 1; }

gate() {
    name=$1; shift
    if "$@" >"$TMP/$name.log" 2>&1; then
        echo "GATE $name: PASS"
    else
        echo "GATE $name: FAIL"
        tail -n 5 "$TMP/$name.log" | sed 's/^/    /'
        FAILED=1
    fi
}

echo "COLD BOOT — rainbow-array-machine"

# 1. Seal: SHA-256 of the canonical field block.
awk '/COLD-BOOT:FIELDS:BEGIN/{f=1;next} /COLD-BOOT:FIELDS:END/{f=0} f' COLD-BOOT.md > "$TMP/fields.txt"
SEAL=$(sha256sum "$TMP/fields.txt" | cut -d' ' -f1)
gate seal test "$SEAL" = "$(field seal)"

# 2. License: bytes and SPDX identifier match the sealed fields.
LIC_SHA=$(sha256sum "$(field license_file)" | cut -d' ' -f1)
gate license_sha test "$LIC_SHA" = "$(field license_sha256)"
gate license_spdx grep -q "SPDX-License-Identifier: $(field license_spdx)" "$(field license_file)"

# 3. Baseline: the sealed commit exists and is an ancestor of HEAD.
BASE=$(field baseline_commit)
gate baseline_ancestor git merge-base --is-ancestor "$BASE" HEAD

# 4. Identity: the stack reports the sealed architecture.
make report > "$TMP/report.txt" 2>&1
gate identity grep -q "Architecture: $(field architecture)" "$TMP/report.txt"

# 5. Build.
gate build make

# 6. Suites: every suite named in the sealed list.
SUITES=$(field suites)
OLDIFS=$IFS; IFS=,
for t in $SUITES; do gate "suite_$t" make "$t"; done
IFS=$OLDIFS

# 7. Artifacts: traceback byte-equality + skull decode round trip.
gate artifact_traceback python3 src/ram_traceback.py \
    examples/runtime-array/symbolic.ram examples/runtime-array/symbolic.wasm \
    -o "$TMP/Symbolic.lean"
./skull decode examples/runtime-array/vector.skull "$TMP/vector.wasm" 2>"$TMP/skull.log"
gate artifact_skull cmp "$TMP/vector.wasm" examples/runtime-array/vector.wasm

echo "---"
echo "DECLARED: baseline $BASE; license $(field license_spdx); suites: $SUITES"
echo "Working tree changes (informational): $(git status --porcelain | wc -l)"
if [ "$FAILED" -eq 0 ]; then
    echo "COLD BOOT: PASS — every gate above was executed in this run."
else
    echo "COLD BOOT: FAIL — do not trust summaries of this tree until the failed gates pass."
fi
exit "$FAILED"
