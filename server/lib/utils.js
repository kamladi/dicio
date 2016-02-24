/** Utility Functions */
var util = require('util');

// Custom Error for bad/invalid requests
function BadRequestError (msg) {
	Error.call(this);
	Error.captureStackTrace(this, arguments.callee);
	this.message = msg;
	this.status = 400;
}

util.inherits(BadRequestError, Error);
exports.BadRequestError = BadRequestError;
