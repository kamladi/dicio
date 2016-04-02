var SP         = require('serialport');
var SerialPort = SP.SerialPort;
var Outlet     = require('./models/Outlet');
var Event      = require('./models/Event');
var WS 			= require('./websockets');

// Constants
const DEFAULT_SERIAL_PORT = '/dev/ttty.usbserial-AM017Y3E';
const BAUD_RATE = 115200;
const OUTLET_SENSOR_MESSAGE         = 5;
const OUTLET_ACTION_MESSAGE         = 6;
const OUTLET_ACTION_ACK_MESSAGE     = 7;
const OUTLET_HANDSHAKE_MESSAGE      = 8;
const OUTLET_HANDSHAKE_ACK_MESSAGE  = 9;
const OUTLET_LOST_NODE_MESSAGE 			= 10;
const MAX_COMMAND_ID_NUM						= 65536;

// Globals
var serialPort = null;
var commandId = 2048;

/*
 * Returns True if we have made a successful connection to the gateway,
 * False otherwise.
 */
function isConnected() {
	return serialPort && serialPort.isOpen();
}


/*
 * Saves the given sensor data into the database for the outlet with the given
 * MAC address.
 * @returns Promise<Outlet> with updated outlet data
 */
function saveSensorData(macAddress, power, temperature, light) {
	return Outlet.find({mac_address: macAddress}).exec()
	  .then( outlets => {
	    var outlet = null;
	    if (outlets.length == 0) {
	      // Unrecognized MAC address, create a new one.
	      console.log(`New MAC Address ${macAddress}, creating new outlet.`);
	      var outlet = new Outlet();
	    } else {
	    	// Outlet found in database.
				var outlet = outlets[0];
	    }

	    // Update outlet object with new properties, and save.
	    outlet.cur_temperature = temperature;
	    outlet.cur_light = light;
	    outlet.cur_power = power;
	    console.log(`Outlet ${macAddress} updated.`);
	    return outlet.save();
	  }).catch(console.error);
}

/*
 * Handle a Sensor Data Message
 * @returns Promise<Outlet> updated outlet data
 */
function handleSensorDataMessage(macAddress, payload) {
	// Parse sensor data, convert to ints
  var sensorValues = payload.split(',').map(value => parseInt(value));

  // We're expecting five values: power, temp, light, (eventually status).
  if (sensorValues.length !== 3) {
    throw new Error(`Not enough sensor values in packet: ${sensorValues}`);
  }

  // Get data values from payload.
  var power = sensorValues[0];
      temperature = sensorValues[1],
      light = sensorValues[2];

  // TODO: Trigger events as necessary.
  return saveSensorData(macAddress, power, temperature, light);
}

/*
 * Handle a Action Ack Message. Simply toggles the 'status' of the outlet in the
 * database.
 * @returns Promise<Outlet> updated outlet data.
 */
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

/*
 * Handle a Handshake Ack Message. Create a new outlet object in database,
 * And send a websocket message to the app to notify the user.
 */
function handleHandshakeAckMessage(macAddress, payload) {
	return Outlet.find({mac_address: macAddress}).exec()
	  .then( outlets => {
	    if (outlets.length > 0) {
	    	throw new Error("Handshake ack message received for existing outlet MAC address: " + macAddress);
	    }
	    var outlet = new Outlet({mac_address: macAddress});
	    return outlet.save();
	  }).then( outlet => {
	    // Send socket mesage to app announcing new node
	    return WS.sendNewNodeMessage(outlet._id, outlet.name);
	  }).catch(console.error);
}

/**
 * Handle a Lost Node. Sends a Websocket message to the client with the outlet's
 * name and id.
 * (TODO: decide if we should immediately delete from the database, or set a
 * 	'disconnected' flag)
 */
function handleLostNodeMessage(macAddress, payload) {
	var payloadValues = payload.split(',');
	if (payloadValues.length < 1) {
		return Promise.reject(new Error('invalid payload: ', payload));
	}
	var lostMacAddress = payloadValues[0];
	return Outlet.find({mac_address: lostMacAddress}).exec()
	  .then( outlets => {
	    if (outlets.length === 0) {
	    	throw new Error("Received LOST NODE message for unknown outlet " + lostMacAddress);
	    }

	    // Mark outlet as inactive in database.
	    var outlet = outlets[0];
	    outlet.active = false;
	    return outlet.save()
	  }).then( outlet => {
	  	// Send socket mesage to app announcing new node.
	    return WS.sendLostNodeMessage(outlet._id, outlet.name);
	  }).catch(console.error);
}

// Parse and handle data packet.
// TODO:
// 1) Update time series sensor data
// 2) Iterate over events involving this outlet, and
// 			execute any actions is applicable
function handleData(data) {
  console.log("[Gateway] >>>>>>>>>>", data);

	/** Parse Packet **/
	/** Packet format: "mac_addr:seq_num:msg_id:payload" **/
	// "source_mac_addr:seq_num:msg_type:num_hops:payload"
	var components = data.split(':');
	if (components.length !== 5) {
		console.error("Invalid packet length");
		return Promise.reject(new Error("Invalid minimum packet length"));
	}
	var macAddress = parseInt(components[0]),
	    msgId = parseInt(components[2]),
	    payload = components[4];

	switch(msgId) {
		case OUTLET_SENSOR_MESSAGE:
			return handleSensorDataMessage(macAddress, payload);
		case OUTLET_ACTION_ACK_MESSAGE:
			return handleActionAckMessage(macAddress, payload);
		case OUTLET_HANDSHAKE_ACK_MESSAGE:
    	return handleHandshakeAckMessage(macAddress, payload);
    case OUTLET_LOST_NODE_MESSAGE:
    	return handleLostNodeMessage(macAddress, payload);
		default:
			console.error(`Unknown Message type: ${msgId}`);
			return Promise.reject(new Error(`Unknown Message type: ${msgId}`));
	}
}

/*
 * Given an outlet's mac address and an action ('ON'/'OFF'),
 * Send a message to the gateway to be propagated to that outlet.
 * Returns a promise which will be fulfilled when the packet is sent.
 * @returns Promise<message> the message sent to the gateway.
 */
function sendAction(outletMacAddress, action) {
  return new Promise( (resolve, reject) => {
    if (!isConnected()) {
      reject(new Error("Connection to gateway has not started yet"));
    } else if (action !== 'ON' && action !== 'OFF') {
      return reject(new Error(`Invalid action: ${action}. Must be "ON" or "OFF"`));
    } else {
      // Convert action string to enum value
      action = (action === 'ON') ? 0x1 : 0x0;

      // Packet format: "source_mac_addr:seq_num:msg_type:num_hops:payload"
 			//   where "payload" has structure "cmd_id,dest_outlet_id,action,"
      // Server sends message with source_id 0, seq_num 0, num_hops 0
      var packet = `0:0:${OUTLET_ACTION_MESSAGE}:0:0,${outletMacAddress},${action},`;
      var sourceMacAddr = 0x0,
      		seqNum = 0x0,
      		msgType = OUTLET_ACTION_MESSAGE,
      		numHops = 0x0,
      		destOutletAddr = parseInt(outletMacAddress) & 0xFF;

      // increment command ID
      commandId = (commandId + 1) % MAX_COMMAND_ID_NUM;

      // split command id into two bytes
      var cmdIdLower = commandId & 0xFF;
      var cmdIdUpper = (commandId >> 8) & 0xFF;

      // 0x0D is the integer value for '\r' (carriage return)
      var packet = new Buffer([
      	sourceMacAddr, seqNum, msgType, numHops,
      	cmdIdUpper, cmdIdLower, destOutletAddr, action, 0x0D
      ]);
      console.log("Packet to be sent: ", packet);

      serialPort.write(packet, (err) => {
        if (err) {
          return reject(err);
        } else {
	      	serialPort.drain((err) => {
	      		if(err){
	      			return reject(err);
	      		} else {
	      			console.log("Successfully sent packet to gateway!");
	      			resolve(packet);
	      		}
	     		});
        }
      });
    }
  }).catch(console.error);
};


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
	    console.log('Serial Port opened');

	    // Listen for "data" event from serial port
	    serialPort.on('data', handleData);
	});

	serialPort.on('error', (err) => {
		console.error('Serial Port Error: ', err);
	});

	serialPort.on('close', () => {
		console.log('Serial Port connection closed.');
	});
};

// export functions to make them public
exports.handleData = handleData;
exports.sendAction = sendAction;
exports.isConnected = isConnected;
exports.start = start;


