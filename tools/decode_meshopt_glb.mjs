// Decode a Tripo browser GLB to portable uncompressed glTF buffers, offline.
import fs from 'node:fs';
import { MeshoptDecoder } from '../.tools/meshoptimizer/node_modules/meshoptimizer/index.js';
await MeshoptDecoder.ready;
const raw=fs.readFileSync(process.argv[2]);
if(raw.readUInt32LE(0)!==0x46546c67||raw.readUInt32LE(4)!==2||raw.readUInt32LE(8)!==raw.length)throw Error('Invalid GLB');
let doc,bin;
for(let at=12;at<raw.length;){const size=raw.readUInt32LE(at),kind=raw.readUInt32LE(at+4),chunk=raw.subarray(at+8,at+8+size);at+=8+size;if(kind===0x4e4f534a)doc=JSON.parse(chunk.toString());if(kind===0x004e4942)bin=chunk;}
const chunks=[];let length=0;
for(const view of doc.bufferViews){
  const ext=view.extensions?.EXT_meshopt_compression;let data;
  if(ext){
    if(ext.buffer!==0)throw Error('Unsupported source buffer');
    data=new Uint8Array(ext.count*ext.byteStride);
    MeshoptDecoder.decodeGltfBuffer(data,ext.count,ext.byteStride,bin.subarray(ext.byteOffset||0,(ext.byteOffset||0)+ext.byteLength),ext.mode,ext.filter);
    if(data.length!==view.byteLength)throw Error('Invalid decoded size');
    delete view.extensions.EXT_meshopt_compression;if(!Object.keys(view.extensions).length)delete view.extensions;
  }else{
    if(view.buffer!==0)throw Error('Missing compressed buffer');
    data=bin.subarray(view.byteOffset||0,(view.byteOffset||0)+view.byteLength);
  }
  view.buffer=0;view.byteOffset=length;chunks.push(Buffer.from(data));length+=data.length;
  const padding=(4-length%4)%4;if(padding){chunks.push(Buffer.alloc(padding));length+=padding;}
}
doc.buffers=[{byteLength:length}];
for(const key of ['extensionsUsed','extensionsRequired'])if(doc[key]){doc[key]=doc[key].filter(x=>x!=='EXT_meshopt_compression');if(!doc[key].length)delete doc[key];}
const json=Buffer.from(JSON.stringify(doc)),jsonPadding=Buffer.alloc((4-json.length%4)%4,32),jsonLength=json.length+jsonPadding.length;
const head=Buffer.alloc(20);head.writeUInt32LE(0x46546c67,0);head.writeUInt32LE(2,4);head.writeUInt32LE(28+jsonLength+length,8);head.writeUInt32LE(jsonLength,12);head.writeUInt32LE(0x4e4f534a,16);
const binaryHead=Buffer.alloc(8);binaryHead.writeUInt32LE(length);binaryHead.writeUInt32LE(0x004e4942,4);
fs.writeFileSync(process.argv[3],Buffer.concat([head,json,jsonPadding,binaryHead,...chunks]));
console.log(`Decoded ${doc.bufferViews.length} buffer views to ${process.argv[3]}`);
