(module
  (memory (export "memory") 1)
  (func (export "run") (local $i i32)
    i32.const 0
    local.set $i
    block
      loop
        local.get $i
        i32.const 3
        i32.ge_u
        br_if 1
        local.get $i
        i32.const 8
        i32.mul
        i32.const 48
        i32.add
        local.get $i
        i32.const 8
        i32.mul
        f64.load
        local.get $i
        i32.const 8
        i32.mul
        i32.const 24
        i32.add
        f64.load
        f64.sub
        f64.store
        local.get $i
        i32.const 1
        i32.add
        local.set $i
        br 0
      end
    end)
  (data (i32.const 0) "\00\00\00\00\00\00\f0\3f\00\00\00\00\00\00\00\40\00\00\00\00\00\00\08\40\00\00\00\00\00\00\10\40\00\00\00\00\00\00\14\40\00\00\00\00\00\00\18\40")
)
