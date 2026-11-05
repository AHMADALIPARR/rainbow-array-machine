#!/bin/sh
# Executes the compiled example modules in Node's WebAssembly runtime
# and compares the exported results against the declared outputs.
# The runner script is written to a temp file: no JavaScript is
# stored in this repository; Node serves only as the WASM runtime.
set -eu
T=$(mktemp /tmp/ram-run-modules-XXXXXX.js)
trap 'rm -f "$T"' EXIT
cat > "$T" <<'EOF'
const fs = require('fs');
const expected = {
  'build/array.wasm':    [5,7,9,4,10,18,-3,-3,-3,58,64,139,154,1,4,2,5,3,6],
  'build/geometry.wasm': [0,1,0,-1,0,0,4,4,5,3,5,5,1,0,0,1,0,0,1,0,0],
  'build/symbolic.wasm': [27],
  'build/showcase.wasm': [5,7,9,58,64,139,154,0,1,0,-1,0,0,1,0,0,27],
  'build/polynomial.wasm': [11],
};
(async () => {
  for (const [file, want] of Object.entries(expected)) {
    const { instance } = await WebAssembly.instantiate(fs.readFileSync(file));
    const rc = instance.exports.run();
    if (rc !== 0) throw new Error(file + ': run() returned ' + rc);
    const mem = new Float64Array(instance.exports.memory.buffer,
      instance.exports.result_ptr(), instance.exports.result_len());
    const got = Array.from(mem);
    if (JSON.stringify(got) !== JSON.stringify(want))
      throw new Error(file + ': got ' + JSON.stringify(got));
    console.log('PASS standalone WASM: ' + got.length + ' outputs [' + got.join(',') + ']');
  }
})().catch(e => { console.error('FAIL: ' + e.message); process.exit(1); });
EOF
node "$T"
