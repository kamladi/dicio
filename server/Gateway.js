var Event          = require('./models/Event');
var EventScheduler = require('./lib/EventScheduler');
var Outlet         = require('./models/Outlet');
var SensorRecord   = require('./models/SensorRecord');
var SP             = require('serialport');
var Watchdog       = require('./lib/Watchdog');
var WS 				     = require('./websockets');

// Constants
const DEFAULT_SERIAL_PORT   = '/dev/ttty.usbserial-AM017Y3E';
const BAUD_RATE             = 38400;
const MAX_COMMAND_ID    = 65536;
const SerialPort = SP.SerialPort;

// Message Types
const LOST_NODE_MESSAGE     = 1;
const RESET_MESSAGE 				= 2;
const SENSOR_MESSAGE        = 5;
const ACTION_MESSAGE        = 6;
const ACTION_ACK_MESSAGE    = 7;
const HANDSHAKE_MESSAGE     = 8;
const HANDSHAKE_ACK_MESSAGE = 9;
const HEARTBEAT_MESSAGE		= 10;

// Globals
var gSerialPort = null;
var gWatchdogTimer = null;

/*
 * Returns True if we have made a successful connection to the gateway,
 * False otherwise.
 */
function isConnected() {
	return gSerialPort && gSerialPort.isOpen();
}

/*
 * Saves the given sensor data into the database for the outlet with the given
 * MAC address.
 * @returns Promise<Outlet> with updated outlet data
 */
function saveSensorData(macAddress, power, temperature, light, status) {
	return Outlet.find({mac_address: macAddress}).exec()
	  .then( outlets => {
	    var outlet = null;
	    if (outlets.length == 0) {
	      // Unrecognized MAC address, create a new one.
	      console.log(`Received data message from new MAC address ${macAddress}, creating new outlet.`);
	      outlet = new Outlet({
	      	name: "NEW OUTLET " + macAddress,
	      	mac_address: macAddress
	      });
	    } else {
	    	// Outlet found in database.
				outlet = outlets[0];
	    }

	    // Update outlet object with new properties, and save.
	    outlet.cur_temperature = temperature;
	    outlet.cur_light = light;
	    outlet.cur_power = power;
	    outlet.status = status;

	    // If we've received a message from this outlet, it's obviously active.
	    if (!outlet.active) {
	    	outlet.active = true;
	    }

	    console.log(`Outlet ${macAddress} updated.`);
	    return outlet.save();
	  }).catch(console.error);
}

function saveTimeSeriesData(outlet) {
	var newRecord = new SensorRecord({
		timestamp: outlet.last_updated,
		mac_address: outlet.mac_address,
		cur_temperature: outlet.cur_temperature,
		cur_light: outlet.cur_light,
		cur_power: outlet.cur_power
	});
	return newRecord.save().then( () => outlet);
}

/*
 * Handle a Sensor Data Message
 * @returns Promise<Outlet> updated outlet data
 */
function handleSensorDataMessage(macAddress, payload) {
	// Parse sensor data, convert to ints
  var sensorValues = payload.split(',').map(value => parseInt(value));

  // We're expecting five values: power, temp, light, (eventually status).
  if (sensorValues.length !== 4) {
    throw new Error(`Invalid number of sensor values in packet: ${sensorValues}`);
  }

  // Get data values from payload.
  var power = sensorValues[0];
      temperature = sensorValues[1] / 10, // 245 => 24.5 deg
      light = sensorValues[2],
      status = (sensorValues[3] === 0) ? 'OFF' : 'ON';

  return saveSensorData(macAddress, power, temperature, light, status)
  	.then( outlet => saveTimeSeriesData(outlet))
  	.then(EventScheduler.triggerCommandsFromEvents)
  	.then( commands => {
  		console.log('Triggering commands: ', commands);
  		// Send and action to the gateway for each command object given
  		var commandPromises = commands.map(c => sendAction(c.destMacAddress, c.action));
  		// Wait until all actions are sent.
  		return Promise.all(commandPromises);
  	})
  	.catch(console.error);
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

	    // Parse sensor data, convert to ints
  		var payloadValues = payload.split(',').map(value => parseInt(value));
  		if (payloadValues.length < 2) {
  			throw new Error(`Not enough sensor values in packet: ${payloadValues}`);
  		}

  		var status = payloadValues[1];
  		console.log(`New outlet status: ${status}`);

	    // Toggle outlet status.
	    outlet.status = (status === 1) ? 'ON' : 'OFF';
	    return outlet.save();
		}).catch(console.error);
}


/*
 * Handle a Handshake Ack Message. Create a new outlet object in database,
 * And send a websocket message to the app to notify the user.
 */
function handleHandshakeAckMessage(macAddress, payload) {
	var payloadValues = payload.split(',');
	if (payloadValues.length < 3) {
		return Promise.reject(new Error('Invalid payload: ' + payload));
	}
	var newMacAddress = payloadValues[0];

  // Next two payload values are the upper and lower halves of the hardware version string
  // (converting each number to hexadecimal strings
  var hardwareVersion1 = parseInt(payloadValues[1]).toString(16);
	var hardwareVersion2 = parseInt(payloadValues[2]).toString(16);
	// Left-pad second value with zeros (result should be 4 characters long)
	hardwareVersion2 = ('0000' + hardwareVersion2).slice(-4);
	var hardwareVersion = hardwareVersion1 + hardwareVersion2;

	return Outlet.find({mac_address: newMacAddress}).exec()
	  .then( outlets => {
	    if (outlets.length > 0) {
	    	// if outlet already exists, update hardware version, and
	    	//  mark outlet as active again.
	    	var existingOutlet = outlets[0];
	    	if (existingOutlet.active) {
	    		// If the outlet is already active, ignore.
	    		console.log(`Received HAND-ACK message for already active outlet ${newMacAddress}`);
	    		return existingOutlet;
	    	} else {
	    		// Outlet exists, but is inactive: mark outlet as active again.
	    		existingOutlet.active = true;
		    	return existingOutlet.save()
		    		.then( outlet => {
					    // Send socket mesage to app announcing outlet has become
					    // active again.
					    return WS.sendActiveNodeMessage(outlet._id, outlet.name);
					  });
	    	}

	    }

      // Create new outlet object
	    var outlet = new Outlet({
	    	name: 'NEW OUTLET' + newMacAddress,
	    	mac_address: newMacAddress,
	    	hardware_version: hardwareVersion
	    });

	    return outlet.save()
	    	.then( outlet => {
			    // Send socket mesage to app announcing new outlet
			    return WS.sendNewNodeMessage(outlet._id, outlet.name);
			  });
	  }).catch(console.error);
}

function handleHeartbeatMessage(macAddress, payload) {
    var msg = 'Heartbeat from gateway received';
	console.log(msg);
    return Promise.resolve(msg);
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

	    var outlet = outlets[0];
	    if (!outlet.active) {
	    	// If the outlet is already inactive, ignore.
	    	console.log(`Received LOST NODE message for already inactive outlet ${lostMacAddress}`);
	    	return outlet;
	    } else {
	    	// Mark outlet as inactive in database.
	    	outlet.active = false;
	    	return outlet.save()
	    		.then( outlet => {
				  	// Send socket mesage to app announcing new node.
				  	console.log(`Received LOST NODE message for outlet ${lostMacAddress}`);
				    return WS.sendLostNodeMessage(outlet._id, outlet.name);
				  });
	    }
	  }).catch(console.error);
}

function deactivateOutlets() {
	console.log("Received RESET message, setting all outlets to inactive");
	return Outlet.update({}, { active: false }, {multi: true}).exec()
		.catch(console.error);
}

// Parse and handle data packet.
// TODO:
// 1) Update time series sensor data
// 2) Iterate over events involving this outlet, and
// 			execute any actions is applicable
function handleData(data) {
  console.log("[Gateway] >>>>>>>>>>", data);

  // Kick Watchdog timer.
	if (gWatchdogTimer) gWatchdogTimer.kick();

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
		case SENSOR_MESSAGE:
			return handleSensorDataMessage(macAddress, payload);
		case ACTION_ACK_MESSAGE:
			return handleActionAckMessage(macAddress, payload);
		case HANDSHAKE_ACK_MESSAGE:
    	    return handleHandshakeAckMessage(macAddress, payload);
    case HEARTBEAT_MESSAGE:
	    return handleHeartbeatMessage(macAddress, payload);
    case LOST_NODE_MESSAGE:
	    return handleLostNodeMessage(macAddress, payload);
	  case RESET_MESSAGE:
	  	return deactivateOutlets();
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
      var sourceMacAddr = 0x0,
      		seqNum = 0x0,
      		msgType = ACTION_MESSAGE,
      		numHops = 0x0,
      		destOutletAddr = parseInt(outletMacAddress) & 0xFF;

      // 0x0D is the integer value for '\r' (carriage return)
      var packet = new Buffer([
      	sourceMacAddr, 0, 0, msgType, numHops,
      	0, 0, destOutletAddr, action, 0x0D
      ]);
      console.log("Packet to be sent: ", packet);

      gSerialPort.write(packet, (err) => {
        if (err) {
          return reject(err);
        } else {
	      	gSerialPort.drain((err) => {
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

function reconnect(port) {
	gSerialPort.close( () => {
		start(port);
	});
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
	gSerialPort = new SerialPort(port, {
	    baudRate: BAUD_RATE,
	    parser: SP.parsers.readline("\r")
	});

	// Listen for "open" event form serial port
	gSerialPort.on('open', () => {
	    console.log('Serial Port opened');

	 		// Start watchdog timer.
	 		gWatchdogTimer = new Watchdog();

	 		// Notify app if watchdog timer expires.
	 		gWatchdogTimer.on('timeout', () => {
	 			console.error('Gateway watchdog timer expired!');
	 			reconnect(port);
	 			WS.sendDeadGatewayMessage();
	 		});

	    // Listen for "data" event from serial port
	    gSerialPort.on('data', handleData);
	});

	gSerialPort.on('error', (err) => {
		console.error('Serial Port Error: ', err);
	});

	gSerialPort.on('close', () => {
		console.log('Serial Port connection closed.');
		gWatchdogTimer.dispose();
	});
};

// export functions to make them public
exports.handleData = handleData;
exports.sendAction = sendAction;
exports.isConnected = isConnected;
exports.start = start;


