var WebSocket = require('ws');
var WebSocketServer = WebSocket.Server;

// Global reference to websocket server.
var wss = null;
var hasConnection = false;

function onConnect(socket) {
	console.log('New socket connection');
	hasConnection = true;

	socket.on('message', (message) => {
		console.log(`Message from client: ${message}`);
	})
}

function init(server) {
	wss = new WebSocketServer({server: server});

	wss.on('connection', onConnect);

	wss.on('error', (err) => {
		console.error(err);
	});

	wss.on('close', (code, message) => {
		console.log('socket connection closed');
		hasConnection = false;
	});
}

function isConnected() {
	return wss && hasConnection;
}

function sendMessage(message) {
	return new Promise((resolve, reject) => {
		if (!isConnected()) {
			return reject(new Error('Cannot send websocket message, client not connected'));
		}
		wss.clients.forEach(client => client.send(JSON.stringify(message)));
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

exports.init = init;
exports.isConnected = isConnected;
exports.sendNewNodeMessage = sendNewNodeMessage;
exports.sendLostNodeMessage = sendLostNodeMessage;

