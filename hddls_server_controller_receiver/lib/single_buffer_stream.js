const {Readable} = require('stream');

class BufferStream extends Readable {
    constructor(buffer, options) {
        super(options);

        this._offset = 0;
        this._length = buffer.length;
        this._buf = buffer;

        this.on('end', function() {
              this._destroy();
        });
    }

    _read(size) {
        if ( this._offset < this._length ) {
            this.push( this._buf.slice( this._offset, ( this._offset + size ) ) );
            this._offset += size;
        }

        if ( this._offset >= this._length ) {
            this.push( null );
        }
    }

    _destroy() {
        this._buf = null;
        this._length = null;
        this._offset = null;
    }
}

module.exports = BufferStream;