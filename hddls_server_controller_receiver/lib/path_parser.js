'use strict';
function basename(path) {
return path.substring(path.lastIndexOf("/")+1);
}



function extname(path) {
    var base = this.basename(path);
    return base.substring(base.lastIndexOf('.'));
}


function dirname(path) {
    return path.substring(0, path.lastIndexOf("/"));
}

function join(a,b) {
    if(a === "") {
        return b;
    }
    return a+'/'+b;
}

exports.extname = extname
exports.dirname = dirname
exports.basename = basename
exports.join = join