var mongoose = require('mongoose');
var ObjectId = mongoose.Schema.ObjectId;

var eventSchema = new mongoose.Schema({
	name: String,
	input_outlet_id: ObjectId,
	input: {
		type: String,
		enum: ['time', 'light', 'humidity', 'temperature'],
		default: 'time'
	},
	input_threshold: { type: String, enum: ['above','below'], default: 'above' },
	input_value: Number,
	output_outlet_id: ObjectId,
	output_action: { type: String, enum: ['ON','OFF'], default: 'ON' }
});

module.exports = mongoose.model('Event', eventSchema);
