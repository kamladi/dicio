var db = require('./db');
var serialPort = require('./serialport');
var app = require('./app');

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
serialPort.start(port);

// Start web server
app.listen(3000, () => {
    console.log('App Listening on Port 3000');
});
