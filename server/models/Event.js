var mongoose = require('mongoose');
var ObjectId = mongoose.Schema.ObjectId;

var eventSchema = new mongoose.Schema({
	name: String,
	input_outlet_id: ObjectId,
	input: {
		type: String,
		enum: ['time', 'light', 'temperature'],
		default: 'time'
	},
	input_threshold: { type: String, enum: ['above','below'], default: 'above' },
	input_value: Number,
	// Events can either trigger an outlet or a group of outlets.
	output_type: {type: String, enum: ['outlet','group'], default: 'outlet'},
	output_outlet_id: ObjectId,
	output_group_id: ObjectId,
	output_action: { type: String, enum: ['ON','OFF'], default: 'ON' },
	created: { type: Date, default: new Date() },
	last_updated: { type: Date, default: new Date() }
});

// Update last_updated timestamp automatically before saving.
eventSchema.pre('save', function(next){
  this.last_updated = new Date();
  next();
});

module.exports = mongoose.model('Event', eventSchema);
