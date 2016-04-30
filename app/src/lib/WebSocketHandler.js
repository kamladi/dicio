import EventEmitter from 'EventEmitter';
import {WEBSOCKET_URL, onNetworkRequestError} from './Constants';

export class WebSocketHandler extends EventEmitter {
	constructor() {
		super();
		this.socket = new WebSocket(WEBSOCKET_URL);

		this.socket.onopen = this.onConnection.bind(this);
		this.socket.onmessage = this.onMessage.bind(this);
		this.socket.onerror = this.onError.bind(this);
		this.socket.onclose = this.onClose.bind(this);
	}

	onConnection() {
		console.log('Websocket connection open.');
	}

	onMessage(event) {
		var {type, outlet_id, outlet_name} = JSON.parse(event.data);
		if (!type) {
			console.error(`unrecognized socket message: ${event.data}`);
		}
		if (type == 'DEADGATEWAY') {
			this.emit('deadGateway');
		} else if (type === 'NEWNODE') {
			this.emit('newNode', outlet_id, outlet_name);
		} else if (type === 'LOSTNODE') {
			this.emit('lostNode', outlet_id, outlet_name);
		} else if (type === 'ACTIVENODE') {
			this.emit('activeNode', outlet_id, outlet_name);
		} else {
			console.error(`unrecognized socket message: ${data}`);
		}
	}

	onError(err) {
		//console.error(err);
		onNetworkRequestError(err);
	}

	onClose(event) {
		console.log('Websocket connection closed: ', event.reason);
	}
}
