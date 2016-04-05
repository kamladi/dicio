var app     = require('./app');
var db      = require('./db');
var Gateway = require('./Gateway');
var http 		= require('http');
var WS 			= require('./websockets');

// Connect to database
db.connect();

// Setup initial data in database (for dev purposes)
require('./initial_data').init();

// Get serial port from command line args
var port = null;
if (process.argv.length >= 3) {
	port = process.argv[2];
}

// Start serial port handler
Gateway.start(port);

var server = http.createServer();

// Connect websocket handler to server
WS.init(server);

// Connect app to web server
server.on('request', app);

// Start web server
server.listen(3000, () => {
    console.log('App Listening on Port 3000');
});
