var mongoose = require('mongoose');

// TODO: Add property for outlet's MAC address
var outletSchema = new mongoose.Schema({
	name: String,
	status: { type: String, enum: ['ON','OFF'], default: 'ON'},
	cur_temperature: Number,
	cur_humidity: Number,
	cur_light: Number,
	cur_power: Number
});

module.exports = mongoose.model('Outlet', outletSchema);
