(module
  (memory (export "memory") 1)
  (data (i32.const 0) "\00\00\00\00\00\00\00\00\00\00\00\00\01\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\01\00\00\00\00\00\00\00\01\00\00\00\01\00\00\00\00\00\00\00")
  (func (export "run") (result i32)
    (local i32 i32 i32 i32)
    i32.const 12
    i32.load offset=0
    i32.const 1
    i32.ne
    if
    i32.const -1
    return
    end
    i32.const 16
    i32.load offset=0
    i32.const 0
    i32.ne
    if
    i32.const -1
    return
    end
    i32.const 20
    i32.load offset=0
    i32.const 0
    i32.ne
    if
    i32.const -1
    return
    end
    i32.const 0
    local.set 0
    block
    loop
    local.get 0
    i32.const 4
    i32.ge_u
    br_if 1
    local.get 0
    i32.const 12
    i32.mul
    i32.const 0
    i32.add
    i32.load offset=0
    local.set 1
    local.get 0
    i32.const 12
    i32.mul
    i32.const 4
    i32.add
    i32.load offset=0
    local.set 2
    local.get 0
    i32.const 12
    i32.mul
    i32.const 8
    i32.add
    i32.load offset=0
    local.set 3
    local.get 2
    i32.const -1
    i32.mul
    local.set 3
    local.get 1
    local.set 2
    local.get 3
    local.set 1
    local.get 0
    i32.const 12
    i32.mul
    i32.const 8
    i32.add
    i32.load offset=0
    local.set 3
    local.get 0
    i32.const 12
    i32.mul
    i32.const 0
    i32.add
    local.get 1
    i32.store offset=0
    local.get 0
    i32.const 12
    i32.mul
    i32.const 4
    i32.add
    local.get 2
    i32.store offset=0
    local.get 0
    i32.const 12
    i32.mul
    i32.const 8
    i32.add
    local.get 3
    i32.store offset=0
    local.get 0
    i32.const 1
    i32.add
    local.set 0
    br 0
    end
    end
    i32.const 12
    i32.load offset=0
    i32.const 0
    i32.ne
    if
    i32.const -1
    return
    end
    i32.const 16
    i32.load offset=0
    i32.const 1
    i32.ne
    if
    i32.const -1
    return
    end
    i32.const 20
    i32.load offset=0
    i32.const 0
    i32.ne
    if
    i32.const -1
    return
    end
    i32.const 0
    local.set 0
    block
    loop
    local.get 0
    i32.const 4
    i32.ge_u
    br_if 1
    local.get 0
    i32.const 12
    i32.mul
    i32.const 0
    i32.add
    i32.load offset=0
    local.set 1
    local.get 0
    i32.const 12
    i32.mul
    i32.const 4
    i32.add
    i32.load offset=0
    local.set 2
    local.get 0
    i32.const 12
    i32.mul
    i32.const 8
    i32.add
    i32.load offset=0
    local.set 3
    local.get 2
    i32.const -1
    i32.mul
    local.set 3
    local.get 1
    local.set 2
    local.get 3
    local.set 1
    local.get 2
    i32.const -1
    i32.mul
    local.set 3
    local.get 1
    local.set 2
    local.get 3
    local.set 1
    local.get 2
    i32.const -1
    i32.mul
    local.set 3
    local.get 1
    local.set 2
    local.get 3
    local.set 1
    local.get 0
    i32.const 12
    i32.mul
    i32.const 8
    i32.add
    i32.load offset=0
    local.set 3
    local.get 0
    i32.const 12
    i32.mul
    i32.const 0
    i32.add
    local.get 1
    i32.store offset=0
    local.get 0
    i32.const 12
    i32.mul
    i32.const 4
    i32.add
    local.get 2
    i32.store offset=0
    local.get 0
    i32.const 12
    i32.mul
    i32.const 8
    i32.add
    local.get 3
    i32.store offset=0
    local.get 0
    i32.const 1
    i32.add
    local.set 0
    br 0
    end
    end
    i32.const 12
    i32.load offset=0
    i32.const 1
    i32.ne
    if
    i32.const -1
    return
    end
    i32.const 16
    i32.load offset=0
    i32.const 0
    i32.ne
    if
    i32.const -1
    return
    end
    i32.const 20
    i32.load offset=0
    i32.const 0
    i32.ne
    if
    i32.const -1
    return
    end
    i32.const 0
    local.set 0
    block
    loop
    local.get 0
    i32.const 4
    i32.ge_u
    br_if 1
    local.get 0
    i32.const 12
    i32.mul
    i32.const 0
    i32.add
    i32.load offset=0
    local.set 1
    local.get 0
    i32.const 12
    i32.mul
    i32.const 4
    i32.add
    i32.load offset=0
    local.set 2
    local.get 0
    i32.const 12
    i32.mul
    i32.const 8
    i32.add
    i32.load offset=0
    local.set 3
    local.get 1
    i32.const 3
    i32.add
    local.set 1
    local.get 2
    i32.const 4
    i32.add
    local.set 2
    local.get 3
    i32.const 5
    i32.add
    local.set 3
    local.get 0
    i32.const 12
    i32.mul
    i32.const 0
    i32.add
    local.get 1
    i32.store offset=0
    local.get 0
    i32.const 12
    i32.mul
    i32.const 4
    i32.add
    local.get 2
    i32.store offset=0
    local.get 0
    i32.const 12
    i32.mul
    i32.const 8
    i32.add
    local.get 3
    i32.store offset=0
    local.get 0
    i32.const 1
    i32.add
    local.set 0
    br 0
    end
    end
    i32.const 0
    i32.load offset=0
    i32.const 3
    i32.ne
    if
    i32.const -1
    return
    end
    i32.const 4
    i32.load offset=0
    i32.const 4
    i32.ne
    if
    i32.const -1
    return
    end
    i32.const 8
    i32.load offset=0
    i32.const 5
    i32.ne
    if
    i32.const -1
    return
    end
    i32.const 0
    local.set 0
    block
    loop
    local.get 0
    i32.const 4
    i32.ge_u
    br_if 1
    local.get 0
    i32.const 12
    i32.mul
    i32.const 0
    i32.add
    i32.load offset=0
    local.set 1
    local.get 0
    i32.const 12
    i32.mul
    i32.const 4
    i32.add
    i32.load offset=0
    local.set 2
    local.get 0
    i32.const 12
    i32.mul
    i32.const 8
    i32.add
    i32.load offset=0
    local.set 3
    local.get 1
    i32.const -3
    i32.add
    local.set 1
    local.get 2
    i32.const -4
    i32.add
    local.set 2
    local.get 3
    i32.const -5
    i32.add
    local.set 3
    local.get 0
    i32.const 12
    i32.mul
    i32.const 0
    i32.add
    local.get 1
    i32.store offset=0
    local.get 0
    i32.const 12
    i32.mul
    i32.const 4
    i32.add
    local.get 2
    i32.store offset=0
    local.get 0
    i32.const 12
    i32.mul
    i32.const 8
    i32.add
    local.get 3
    i32.store offset=0
    local.get 0
    i32.const 1
    i32.add
    local.set 0
    br 0
    end
    end
    i32.const 0
    i32.load offset=0
    i32.const 0
    i32.ne
    if
    i32.const -1
    return
    end
    i32.const 4
    i32.load offset=0
    i32.const 0
    i32.ne
    if
    i32.const -1
    return
    end
    i32.const 8
    i32.load offset=0
    i32.const 0
    i32.ne
    if
    i32.const -1
    return
    end
    i32.const 4
    end
  )
)
