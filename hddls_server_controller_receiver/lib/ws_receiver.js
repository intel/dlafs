'use strict';
const crypto = require('crypto');
const fileHelper = require('./file_helper');
exports.websocketParser = function websocketParser(message) {
    var result;
    if(message instanceof Buffer && message.length != 0) {
        result = bufParser(message);
    } else {
        result = fileHelper.safelyJSONParser(message);
    }
    return result;
}

function bufParser(buf) {
    let headerDelimiter = buf.indexOf(1);
    let headers = fileHelper.safelyJSONParser(buf.slice(0, headerDelimiter).toString());
    let delimiterChecksum = buf.lastIndexOf(1);
    var result =  {
        headers : headers,
        payload: buf.slice(headerDelimiter+1, delimiterChecksum),
        checkSum: buf.slice(delimiterChecksum + 1, buf.length).toString()
    }
    var hash = crypto.createHash('md5').setEncoding('hex');
    hash.update(result.payload);
    let localCheckSum = hash.digest('hex');
    result.checkSumPass = (localCheckSum == result.checkSum);

    return result;
}

function sendMessage(ws, message, code) {
    ws.send({headers: {method: 'text', code: code || 200}, payload: `${message}`});
}