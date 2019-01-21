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