const fs=require('fs');
(async()=>{
 const path=process.argv[2];
 const manifest=JSON.parse(fs.readFileSync(path.replace(/\.wasm$/,'.flat.json'),'utf8'));
 const binary=fs.readFileSync(path);
 const {instance}=await WebAssembly.instantiate(binary);
 const result=instance.exports.run();
 if(result!==manifest.point_count)throw Error('run failed: '+result);
 const view=new DataView(instance.exports.memory.buffer);
 const actual=[];
 for(let i=0;i<result;i++)actual.push([0,1,2].map(a=>view.getInt32(i*12+a*4,true)));
 if(JSON.stringify(actual)!==JSON.stringify(manifest.expected_points))throw Error('coordinate mismatch');
 console.log('PASS runtime WASM: '+result+' points, '+manifest.operations.length+' operations');
})().catch(e=>{console.error(e);process.exit(1)});
