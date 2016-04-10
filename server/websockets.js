var WebSocket = require('ws');
var WebSocketServer = WebSocket.Server;

// Global reference to websocket server.
var gSocketServer = null;
var gHasConnection = false;

function onConnect(socket) {
	console.log('New socket connection');
	gHasConnection = true;

	socket.on('message', (message) => {
		console.log(`Message from client: ${message}`);
	})
}

function init(server) {
	gSocketServer = new WebSocketServer({server: server});

	gSocketServer.on('connection', onConnect);

	gSocketServer.on('error', (err) => {
		console.error(err);
	});

	gSocketServer.on('close', (code, message) => {
		console.log('socket connection closed');
		gHasConnection = false;
	});
}

function isConnected() {
	return gSocketServer && gHasConnection;
}

function sendMessage(message) {
	return new Promise((resolve, reject) => {
		if (!isConnected()) {
			return reject(new Error('Cannot send websocket message, client not connected'));
		}
		gSocketServer.clients.forEach(client => client.send(JSON.stringify(message)));
		resolve(message);
	}).catch(console.error);
}

function sendNewNodeMessage(outlet_id, name) {
	return sendMessage({
		type: 'NEWNODE',
		outlet_id: outlet_id,
		outlet_name: name
	});
}

function sendLostNodeMessage(outlet_id, name) {
	return sendMessage({
		type: 'LOSTNODE',
		outlet_id: outlet_id,
		outlet_name: name
	});
}


function sendActiveNodeMessage(outlet_id, name) {
	return sendMessage({
		type: 'ACTIVENODE',
		outlet_id: outlet_id,
		outlet_name: name
	});
}

function sendDeadGatewayMessage() {
	return sendMessage({
		type:'DEADGATEWAY'
	});
}

exports.init = init;
exports.isConnected = isConnected;
exports.sendNewNodeMessage = sendNewNodeMessage;
exports.sendLostNodeMessage = sendLostNodeMessage;
exports.sendActiveNodeMessage = sendActiveNodeMessage;
exports.sendDeadGatewayMessage = sendDeadGatewayMessage;

