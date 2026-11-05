(module
  ;; BQN vector f64.add; result length 3, f64 little-endian at memory offset 0
  (memory (export "memory") 1)
  (func (export "run")
    i32.const 0
    f64.const 1.0
    f64.const 4.0
    f64.add
    f64.store
    i32.const 8
    f64.const 2.0
    f64.const 5.0
    f64.add
    f64.store
    i32.const 16
    f64.const 3.0
    f64.const 6.0
    f64.add
    f64.store)
)
