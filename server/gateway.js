var SP         = require('serialport');
var SerialPort = SP.SerialPort;
var Outlet     = require('./models/Outlet');
var Event      = require('./models/Event');

// Constants
const DEFAULT_SERIAL_PORT = '/dev/tty.usbserial-AE00BUMD';
const BAUD_RATE = 115200;
const OUTLET_SENSOR_MESSAGE         = 5;
const OUTLET_ACTION_MESSAGE         = 6;
const OUTLET_ACTION_ACK_MESSAGE     = 7;
const OUTLET_HANDSHAKE_MESSAGE      = 8;
const OUTLET_HANDSHAKE_ACK_MESSAGE  = 9;

// Globals
var serialPort = null;
var hasStarted = false;
var seqNum = 1;


function handleSensorDataMessage(macAddress, payload) {
	return Outlet.find({mac_address: macAddress}).exec()
	  .then( outlets => {
	    if (outlets.length == 0) {
	      throw new Error(`Outlet does not exist for MAC Address: ${macAddress}`);
	    }

	    var outlet = outlets[0];

	    // Parse sensor data, convert to ints
	    var sensorValues = payload.split(',').map(value => parseInt(value));

	    // We're expecting five values: power, temp, light, (eventually status).
	    if (sensorValues.length !== 3) {
	      throw new Error(`Not enough sensor values in packet: ${sensorValues}`);
	    }

	    // TODO: when status has been added to message,
	    // get status value from packet.
	    var cur_power = sensorValues[0];
	        cur_temperature = sensorValues[1],
	        cur_light = sensorValues[2],
	        status = 'OFF';

	    // Update outlet object with new properties, and save.
	    outlet.status = status;
	    outlet.cur_temperature = cur_temperature;
	    outlet.cur_light = cur_light;
	    outlet.cur_power = cur_power;
	    console.log(`Outlet ${macAddress} updated.`);
	    return outlet.save();
	  }).catch(console.error);
}

function handleActionAckMessage(macAddress, payload) {
	return Outlet.find({mac_address: macAddress}).exec()
		.then( outlets => {
			if (outlets.length == 0) {
	      throw new Error(`Outlet does not exist for MAC Address: ${macAddress}`);
	    }
	    var outlet = outlets[0];

	    // Toggle outlet status.
	    outlet.status = (outlet.status == 'ON') ? 'OFF' : 'ON';
	    return outlet.save();
		}).catch(console.error);
}

// Parse and handle data packet.
// TODO:
// 1) Update time series sensor data
// 2) Iterate over events involving this outlet, and
// 			execute any actions is applicable
function handleData(data) {
  console.log("RX: ", data);

	/** Parse Packet **/
	/** Packet format: "mac_addr:seq_num:msg_id:payload" **/
	// "source_mac_addr:seq_num:msg_type:num_hops:payload"
	var components = data.split(':');
	if (components.length !== 5) {
		console.error("Invalid minimum packet length");
		return;
	}
	var macAddress = parseInt(components[0]),
	    msgId = parseInt(components[2]),
	    payload = components[4];

	if (msgId === OUTLET_SENSOR_MESSAGE) {
		return handleSensorDataMessage(macAddress, payload);
  } else if (msgId == OUTLET_ACTION_ACK_MESSAGE) {
  	return handleActionAckMessage(macAddress, payload);
  } else {
  	console.error(`Unknown Message type: ${msgId}`);
  }

}

/*
 * Given an outlet's mac address and an action ('ON'/'OFF'),
 * Send a message to the gateway to be propagated to that outlet.
 * Returns a promise which will be fulfilled when the packet is sent.
 * Packet format: "source_mac_addr:seq_num:msg_type:num_hops:payload"
 * Where "payload" has structure "dest_outlet_id,action,"
 */
function sendAction(outletMacAddress, action) {
  return new Promise( (resolve, reject) => {
    if (!hasStarted) {
      reject(new Error("Connection to gateway has not started yet"));
    } else if (action !== 'ON' && action !== 'OFF') {
      reject(new Error(`Invalid action: ${action}. Must be "ON" or "OFF"`));
    } else {
      // Convert action string to enum value
      action = (action === 'ON') ? 1 : 0;
      // server sends message with source_id 0, seq_num 0, num_hops 0
      var packet = `0:${seqNum}:${OUTLET_ACTION_MESSAGE}:0:0,${outletMacAddress},${action},`;
      packet += "\r";
      // increment sequence number
      seqNum = (seqNum + 1) % 255;
      console.log("packet sent is..." + packet);
      serialPort.write(packet, (err) => {
        if (err) {
          reject(err);
        } else {
	      	serialPort.drain((err) => {
	      		if(err){
	      			reject(err);
	      		} else {
	      			resolve(packet);
	      		}
	     		});
        }
      });
    }
  });
};

/*
 * Returns True if we have made a successful connection to the gateway,
 * False otherwise.
 */
function isConnected() {
	return serialPort && serialPort.isOpen();
}

/*
 * Starts the connection to the gateway node. If a port was not given as an
 * argument, it assumed a constant defined above
 */
function start(port) {
	if (!port) {
		port = DEFAULT_SERIAL_PORT;
	}
	// Init serial port connection
	serialPort = new SerialPort(port, {
	    baudRate: BAUD_RATE,
	    parser: SP.parsers.readline("\r")
	});

	// Listen for "open" event form serial port
	serialPort.on('open', () => {
	    hasStarted = true;
	    console.log('Serial Port opened');

	    // Listen for "data" event from serial port
	    serialPort.on('data', handleData);
	});

	serialPort.on('error', (err) => {
		hasStarted = false;
		console.error('Serial Port Error: ', err);
	});
};

// export functions to make them public
exports.sendAction = sendAction;
exports.isConnected = isConnected;
exports.start = start;


