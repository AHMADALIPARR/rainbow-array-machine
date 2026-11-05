(module
  (func (export "run") (param i64) (result i64)
    local.get 0
    i64.const 5
    i64.mul
    i64.const 7
    i64.add)
)
