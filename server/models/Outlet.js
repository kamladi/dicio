var mongoose = require('mongoose');

// TODO: Add property for outlet's MAC address
var outletSchema = new mongoose.Schema({
	name: { type: String, default: "NEW OUTLET" },
	mac_address: { type: String, unique: true },
	last_sequence_number: Number,
	hardware_version: String,
	status: { type: String, enum: ['ON','OFF'], default: 'OFF'},
	cur_temperature: Number,
	cur_humidity: Number,
	cur_light: Number,
	cur_power: Number,
	active: {type: Boolean, default: true},
	created: { type: Date, default: new Date() },
	last_updated: { type: Date, default: new Date() }
});

// Update last_updated timestamp automatically before saving.
outletSchema.pre('save', function(next){
  this.last_updated = new Date();
  next();
});

module.exports = mongoose.model('Outlet', outletSchema);
