var SP = require('serialport');
var SerialPort = SP.SerialPort;

// Constants
const SERIAL_PORT = '/dev/tty.usbserial-AE00BUMD';
const BAUD_RATE = 115200;


// Parse and handle data packet.
function handleData(data) {
  console.log("RX: ", data);
 	// If the packet is a valid outlet message,
	// 1) Update outlet data in database
	// 2) Update time series sensor data
	// 3) Iterate over events involving this outlet, and
	// 			execute any actions is applicable
}

exports.start = function (port) {
	if (!port) {
		port = SERIAL_PORT;
	}
	// Init serial port connection
	var serialPort = new SerialPort(port, {
	    baudRate: BAUD_RATE,
	    parser: SP.parsers.readline("\n")
	});

	// Listen for "open" event form serial port
	serialPort.on('open', () => {
	    console.log('Serial Port opened');

	    // Listen for "data" event from serial port
	    serialPort.on('data', handleData);
	});

	serialPort.on('error', (err) => {
		console.error('Serial Port Error: ', err);
	});
};

