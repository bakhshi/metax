var WebSocket = require('ws');
var fs = require('fs');
var ws = new WebSocket('ws://localhost:'+ process.argv[3]);

ws.on('message', function(message) {
    console.log('Received: ' + message);
    fs.appendFileSync(process.argv[2], message + "\n");
});

ws.on('close', function(code) {
    console.log('Disconnected: ' + code);
});

ws.on('error', function(error) {
    console.log('Error: ' + error.code);
});
