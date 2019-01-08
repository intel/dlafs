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
const {Readable} = require('stream');

class BufferStream extends Readable {
    constructor(buffers, options) {
        super(options);
        if(Array.isArray(buffers)) {
            this.buffers = buffers;
        }
        else  {
            this.buffers = [];
            this.buffers.push(buffers);
        }
        this.offset = 0;
        this.buf = this.buffers.shift();
    }

    _read(size) {
        var chunk = this.buf.slice(this.offset, this.offset + size);
        if(chunk) {
            this.push(chunk);
            this.offset += size;
        } else {
            var tmp = this.buffers.shift();
            if(!!tmp) {
                this.buf = tmp;
                this.offset = 0;
                this._read(size);
            } else {
                this.push(null);
            }
        }
    }
}

module.exports = BufferStream;