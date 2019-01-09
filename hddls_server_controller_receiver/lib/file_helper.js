//Copyright (C) 2018 Intel Corporation
// 
//SPDX-License-Identifier: MIT
//
//MIT License
//
//Copyright (c) 2018 Intel Corporation
//
//Permission is hereby granted, free of charge, to any person obtaining a copy of
//this software and associated documentation files (the "Software"),
//to deal in the Software without restriction, including without limitation the rights to
//use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
//and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
//AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
'use strict';

const fs = require('fs');
const path = require('./path_parser')
const crypto = require('crypto');

function isString(str) {
    return typeof(str) == 'string' || str instanceof String;
}

exports.isString = isString;

function safelyJSONParser (json) {

  var parsed
  try {
    parsed = JSON.parse(json)
  } catch (e) {
    return null
  }
  return parsed;
}

exports.safelyJSONParser = safelyJSONParser;

/**
* @param {Array} folderPath folder to upload
* @param {Object} ws the websocket
* @param {Object} rl the readline interface
* @param {String} type the operation type
*/
exports.uploadDir = function (folderPath, ws, type, cb) {
 fs.readdir(folderPath, (err, files) => {
   if(!files || files.length === 0) {
     if (cb) cb();
     return;
   }
   var targetFiles = [];
   for(let index in files) {
     if(path.extname(files[index]).toLowerCase() === '.bin' || path.extname(files[index]).toLowerCase() === '.xml') {
       targetFiles.push(path.join(folderPath, files[index]));
       console.log(targetFiles[targetFiles.length - 1]);
     }
   }
   uploadFile(targetFiles, ws, type, cb);
 });
}


/**
 * @param {Array} files files to upload
 * @param {Object} ws the websocket
 * @param {String} type the operation type
 * @param {Object} cb the readline interface
 */
exports.uploadFile = function uploadFile (files, ws, headers, cb) {
    if(files.length === 0) {
      if(cb) {
        cb();
      }
      return
    }
    const fileEntry = files.pop();
    var filePath, checkSum = null;
    if(isString(fileEntry)) {
      filePath = fileEntry;
    } else if(fileEntry.length !== 0){
      let fileObj = Object.entries(fileEntry);
      filePath = fileObj[0][0];
      checkSum = fileObj[0][1];
    }
    fs.lstat(filePath, (err, stat)=> {
      if(err || stat.isDirectory()) {
        uploadFile(files, ws, headers, cb);
        return;
      }
      headers.path = filePath;
      var stream = fs.createReadStream(filePath);
      if(checkSum == null) {
        var hash = crypto.createHash('md5').setEncoding('hex');
        stream.pipe(hash);
      }
      ws.send(Buffer.from(JSON.stringify(headers)+String.fromCharCode(1)), {fin: false});
      stream.on('data', (chunk)=> {
        ws.send(chunk, { fin: false });
      });
      stream.on('end', ()=> {
        checkSum = checkSum || hash.read();
        ws.send(String.fromCharCode(1) + checkSum, { fin: true });
        console.log(`${filePath} upload complete, MD5 ${checkSum}`);
        uploadFile(files, ws, headers, cb);
      });
    })
  }


exports.scanDir = function (folderPath, isRoot=false) {
    return new Promise((resolve, reject) => {
        fs.readdir(folderPath, (err, files) => {
            if(err || files.length === 0) {
              reject(err || `empty model folder ${folderPath}`);
            }
            var targetFiles = [];
            for(let index in files) {
              if (isRoot === true) {
                targetFiles.push(path.join(folderPath, files[index]));
              } else if(path.extname(files[index]).toLowerCase() === '.bin' || path.extname(files[index]).toLowerCase() === '.xml') {
                targetFiles.push(path.join(folderPath, files[index]));
              }
            }
            resolve(targetFiles);
          });
    });
}



exports.cmpFile = function (filePath, modelCheck) {
    return new Promise((resolve, reject) => {
        fs.lstat(filePath, (err, stat)=> {
            if(err || stat.isDirectory()) {
              reject(err || `empty model file ${filePath}`);
            }
            var stream = fs.createReadStream(filePath);
            var hash = crypto.createHash('md5').setEncoding('hex');
            var fileName = path.basename(filePath);
            var model = path.basename(path.dirname(filePath));
            stream.pipe(hash);
            stream.on('end', ()=> {
              hash.end();
              let check = hash.read();
              if(!modelCheck.hasOwnProperty(model) || modelCheck[model].has_model==='No' || check !== modelCheck[model]['model_file'][fileName]) {
                resolve({[filePath]: check});
              } else {
                resolve();
                console.log(`\x1b[31mServer has file ${filePath}\x1b[0m`);
              }
            });
        })
    });
  }

function safelyFileReadSync(filePath, encoding='utf8') {
  var file;
  try{
    file = fs.readFileSync(filePath, encoding);
  } catch(ex){
    file = null;
  }
  return file;
}

exports.safelyFileReadSync =safelyFileReadSync 


function updateCheckSum(filePath, checkSum) {
  var dir = path.dirname(path.dirname(filePath));
  var modelName = path.basename(path.dirname(filePath));
  var fileName = path.basename(filePath);
  var fileInfo = {};
  var modelInfo = {};
  var modelMeta = safelyJSONParser(safelyFileReadSync(path.join(dir, 'model_info.json'))) || {};
  if(modelMeta.hasOwnProperty(modelName) && modelMeta[modelName].hasOwnProperty('model_file')) {
    fileInfo = modelMeta[modelName]["model_file"];
  }
  if(modelMeta.hasOwnProperty(modelName)) {
    modelInfo = modelMeta[modelName]
  }
  fileInfo = Object.assign(fileInfo,
    {
        [fileName]: checkSum
    }
  );
  modelInfo = Object.assign(modelInfo, {"model_file" : fileInfo, has_model: "Yes"});
  modelMeta = Object.assign(modelMeta, {[modelName]: modelInfo});
  return modelMeta;
}
exports.updateCheckSum = updateCheckSum;

function saveBuffer(filePath, buffer) {
  var folderName = path.dirname(filePath);
  if(!fs.existsSync(folderName))
  {
    console.log('making dir %s', './' + folderName);
    fs.mkdirSync('./' + folderName, { recursive: true });
  }
  var stream = fs.createWriteStream(filePath);
  stream.on("open", ()=>{
    stream.write(buffer);
    stream.end();
  });
}

exports.saveBuffer = saveBuffer;


exports.uploadBuffer = function uploadBuffer (buf, ws, headers) {

  var buffStream = BufferStream(buf);
  if(checkSum == null) {
    var hash = crypto.createHash('md5').setEncoding('hex');
    buffStream.pipe(hash);
  }
  ws.send(Buffer.from(JSON.stringify(headers)+String.fromCharCode(1)), {fin: false});
  stream.on('data', (chunk)=> {
    ws.send(chunk, { fin: false });
  });
  stream.on('end', ()=> {
    checkSum = checkSum || hash.read();
    ws.send(String.fromCharCode(1) + checkSum, { fin: true });
    console.log(`Buffer upload complete, MD5 ${checkSum}`);
  });
}
