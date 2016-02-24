var SP = require('serialport');
var SerialPort = SP.SerialPort;
var Outlet = require('./models/Outlet');
var Event = require('./models/Event');

// Constants
const SERIAL_PORT = '/dev/tty.usbserial-AE00BUMD';
const BAUD_RATE = 115200;
const OUTLET_SENSOR_MESSAGE = 0;
const OUTLET_ACTION_MESSAGE = 1;

// Globals
var serialPort = null;
var hasStarted = false;

// Parse and handle data packet.
// TODO:
// 2) Update time series sensor data
// 3) Iterate over events involving this outlet, and
// 			execute any actions is applicable
function handleData(data) {
  console.log("RX: ", data);
	
	/** Parse Packet **/
	/** Packet format: "mac_addr:seq_num:msg_id:payload" **/
	var components = data.split(':');
	if (components.length !== 4) {
		console.error("Invalid minimum packet length");
		return;
	}
	var macAddress = components[0],
	    msgId = components[2],
	    data = components[3];
	return Outlet.findOne({mac_address: macAddress}).exec()
	  .then( outlet => {
	    if (!outlet) {
	      throw new Error(`Outlet does not exist for MAC Address: ${macAddress}`);
	    }
	    if (msgId != OUTLET_SENSOR_MESSAGE) {
	      throw new Error(`Invalid Message ID: ${msgId}`);
	    }
	    
	    // Parse sensor data, convert to ints
	    var sensorValues = data.split(',').map(value => parseInt(value));
	    
	    // We're expecting five values: status, temp, humidity, light, and power.
	    if (sensorValues.length != 5) {
	      throw new Error(`Not enough sensor values in packet: ${sensorValues}`);
	    }
	    var status = sensorValues[0],
	        cur_temperature = sensorValues[1],
	        cur_humidity = sensorValues[2],
	        cur_light = sensorValues[3],
	        cur_power = sensorValues[4];
	    
	    // Update outlet object with new properties, and save.
	    outlet.status = status;
	    outlet.cur_temperature = cur_temperature;
	    outlet.cur_humidity = cur_humidity;
	    outlet.cur_light = cur_light;
	    outlet.cur_power = cur_power;
	    return outlet.save();
	  }).catch(console.error);
}

/*
 * Given an outlet's mac address and an action ('ON'/'OFF'),
 * Send a message to the gateway to be propagated to that outlet.
 * Returns a promise which will be fulfilled when the packet is sent.
 */
exports.sendAction = function sendAction(outletMacAddress, action) {
  return new Promise( (resolve, reject) => {
    if (!hasStarted) {
      reject(new Error("Connection to gateway has not started yet"));
    } else if (action !== 'ON' || action !== 'OFF') {
      reject(new Error(`Invalid action: ${action}. Must be "ON" or "OFF"`));
    } else {
      // Convert action string t enum value
      action = (action === 'ON') ? 1 : 0;
      var packet = `${outletMacAddress}:${action}`;
      serialPort.write(packet, (err) => {
        if (err) {
          reject(err);
        } else {
          resolve(outletMacAddress, action);
        }
      });
    }
  });
}

exports.start = function start(port) {
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
	    hasStarted = true;
	    console.log('Serial Port opened');

	    // Listen for "data" event from serial port
	    serialPort.on('data', handleData);
	});

	serialPort.on('error', (err) => {
		console.error('Serial Port Error: ', err);
	});
};

