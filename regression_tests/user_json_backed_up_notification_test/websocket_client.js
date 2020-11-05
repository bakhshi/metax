var WebSocket = require('ws');
var ws = new WebSocket('ws://localhost:'+ process.argv[2]);
var expected_message = '{"event":"user json backed up"}'

ws.on('message', function (message) {
        if (message === expected_message) {
                process.exit(0);
        }
});

ws.on('close', function() {
        process.exit(1);
});

ws.on('error', function(error) {
        console.log('Error: ' + error.code);
        process.exit(1);
});

setTimeout(function () {
        process.exit(1);
} , 200000);

