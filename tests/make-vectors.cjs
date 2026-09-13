const c=require('node:crypto'),z=require('node:zlib'),fs=require('node:fs');
const aes=(data,key,iv=null)=>{const cipher=c.createCipheriv(`aes-${key.length*8}-${iv?'cbc':'ecb'}`,Buffer.from(key),iv?Buffer.from(iv):null);return Buffer.concat([cipher.update(data),cipher.final()])};
const json='{"ids":"[347230]","level":"standard"}',path='/api/song/enhance/player/url/v1';
const digest=c.createHash('md5').update(`nobody${path}use${json}md5forencrypt`).digest('hex');
const input=`${path}-36cd479b6b5-${json}-36cd479b6b5-${digest}`;
const response=Buffer.from('{"code":200,"data":[{"url":null,"freeTrialInfo":{"start":0,"end":30}}]}');
const v={json,path,eapi:aes(input,'e82ckenh8dichen8').toString('hex').toUpperCase(),response:response.toString(),encrypted:aes(response,'e82ckenh8dichen8').toString('base64'),compressed:aes(z.gzipSync(response),'e82ckenh8dichen8').toString('base64'),form:new URLSearchParams({a:'中文 & +',b:'a~!*()'}).toString()};
fs.writeFileSync(new URL('./transport-vectors.json',`file://${__filename}`),JSON.stringify(v,null,2)+'\n');
