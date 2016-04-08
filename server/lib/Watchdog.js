"use strict";
const EventEmitter = require('events').EventEmitter;

// Default expiration time is one minute.
const DEFAULT_DURATION = 1000*60;
const TIMEOUT_EVENT_NAME = 'timeout';

/**
 * Simple watchdog timer. Construct with the desired timer duration.
 * After the given duration, if kick() has not been called, this object will
 * emit an event defined by TIMEOUT_EVENT_NAME.
 */
class Watchdog extends EventEmitter {
	constructor(duration) {
		super();
		this.duration = duration || DEFAULT_DURATION;
		this.timer = null;
		this.kick();
	}

	kick() {
		// Clear existing timer if necessary.
		this.dispose();

		// Trigger new timeout
		this.timer = setTimeout(
			this.emit.bind(this, TIMEOUT_EVENT_NAME), this.duration
		);
	}

	dispose() {
		if (this.timer) {
			clearTimeout(this.timer);
		}
	}
}

module.exports = Watchdog;
