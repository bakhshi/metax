var WebSocket = require('ws');
var fs = require('fs');
var ws = new WebSocket("ws://localhost:" + process.argv[2]);

ws.on('message', function(message) {
    console.log('Received: ' + message);
});

ws.on('close', function(code) {
    console.log('Disconnected: ' + code);
});

ws.on('error', function(error) {
    console.log('Error: ' + error.code);
});
