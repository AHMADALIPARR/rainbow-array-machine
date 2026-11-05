(module
  ;; 🐺 🔹 📜 : symbolic coefficient collection, integer mathematical semantics
  (func (export "run") (param $x i64) (result i64)
    local.get $x
    i64.const 5
    i64.mul
    i64.const 7
    i64.add)
)
