const fs=require('fs');
const assert=require('assert');
(async()=>{
  for(const [name,expected] of [['vector',[5,7,9]],['apl',[4,10,18]],['k',[-3,-3,-3]]]){
    const dir='examples/runtime-array/';
    const bytes=fs.readFileSync(dir+name+'.wasm');
    const m=JSON.parse(fs.readFileSync(dir+name+'.flat.json'));
    const instance=(await WebAssembly.instantiate(bytes)).instance;
    instance.exports.run();
    const result=Array.from(new Float64Array(instance.exports.memory.buffer,m.output_offset,m.length));
    assert.deepStrictEqual(result,expected);
    const decoded=require('child_process').execFileSync('python3',['src/ram_runtime.py','--decode',dir+name+'.skull','-o',dir+name+'.roundtrip.wasm']);
    assert(fs.readFileSync(dir+name+'.roundtrip.wasm').equals(bytes));
    console.log('PASS runtime array + skull roundtrip',name,result);
  }
  const instance=(await WebAssembly.instantiate(fs.readFileSync('examples/runtime-array/symbolic.wasm'))).instance;
  for(const x of [-4n,0n,7n,100n]) assert.strictEqual(instance.exports.run(x),5n*x+7n);
  console.log('PASS runtime Wolfram-compatible affine i64');
})().catch(e=>{console.error(e);process.exit(1)});
