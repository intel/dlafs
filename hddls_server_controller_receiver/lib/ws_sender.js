exports.sendMessage = function (ws, message, code) {
    ws.send(JSON.stringify({headers: {method: 'text', code: code || 200}, payload: `${message}`}));
}

exports.sendProtocol = function (ws, headers, payload, code) {
    var protocol = Object.assign(
        {
            headers: headers,
            payload: payload,
            code: code || 200
        }
    );
    ws.send(JSON.stringify(protocol));
}