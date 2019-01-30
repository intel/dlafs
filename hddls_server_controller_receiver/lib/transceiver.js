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
'use strict'

class Transceiver {
    constructor(socket, handler) {
        this._socket = socket;
        this._packet = {};
        this._process = false;
        this._state = 'HEADER';
        this._payloadLength = 0;
        this._bufferedBytes = 0;
        this.queue = [];
        this.handler = handler;
    }

    init() {
        this._socket.on('data', (data) => {
            this._bufferedBytes += data.length;
            this.queue.push(data);
            this._process = true;
            this._onData();
          });
          this._socket.on('served', this.handler);
          this._socket.on('error', err=>console.log(err.message));
    }

    _hasEnough(size) {
        if (this._bufferedBytes >= size) {
          return true;
        }
        this._process = false;
        return false;
    }

    _readBytes(size) {
       if(this.queue.length === 0) {
         return null
       }
        let result;
        this._bufferedBytes -= size;
        if (size === this.queue[0].length) {
          return this.queue.shift();
        }
        if (size < this.queue[0].length) {
          result = this.queue[0].slice(0, size);
          this.queue[0] = this.queue[0].slice(size);
          return result;
        }
        result = Buffer.allocUnsafe(size);
        let offset = 0;
        let length;
        while (size > 0) {
          length = this.queue[0].length;
          if (size >= length) {
            this.queue[0].copy(result, offset);
            offset += length;
            this.queue.shift();
          } else {
            this.queue[0].copy(result, offset, 0, size);
            this.queue[0] = this.queue[0].slice(size);
          }
          size -= length;
        }
        return result;
      }

      _getHeader() {
        if (this._hasEnough(8)) {
          this._payloadLength = this._readBytes(4).readUInt32BE(0, true) - 4;
          this._state = 'PAYLOAD';
        }
      }

      _getPayload() {
        if (this._hasEnough(this._payloadLength)) {
          let type = this._readBytes(4).readUInt32BE(0,true);
          let received = this._readBytes(this._payloadLength - 4);
          !!received  && this._socket.emit('served', {type: type, payload: received});
          this._state = 'HEADER';
        }
      }

      _onData(data) {
        while (this._process) {
          switch (this._state) {
            case 'HEADER':
              this._getHeader();
              break;
            case 'PAYLOAD':
              this._getPayload();
              break;
          }
        }
      }

      send(message, type=0) {
        let buffer = Buffer.from(message);
        this._header(buffer.length + 8, type);
        this._packet.message = buffer;
        this._send();
      }

      _header(messageLength, type) {
        this._packet.header = { length: messageLength, type: type};
      };

      destroy(){
        this._socket.destroy();
      }

      _send() {
        let contentLength = Buffer.allocUnsafe(4);
        let contentType = Buffer.allocUnsafe(4);
        contentLength.writeUInt32BE(this._packet.header.length);
        contentType.writeUInt32BE(this._packet.header.type);
        this._socket.write(contentLength);
        this._socket.write(contentType);
        this._socket.write(this._packet.message);
        this._packet = {};
      }
};

module.exports = Transceiver;
