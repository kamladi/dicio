var mongoose = require('mongoose');
var ObjectId = mongoose.Schema.ObjectId;

// TODO: Add property for outlet's MAC address
var sensorRecordSchema = new mongoose.Schema({
	timestamp: {type: Date, default: Date.now},
	mac_address: {type: String, required: true},
	cur_temperature: Number,
	cur_humidity: Number,
	cur_light: Number,
	cur_power: Number
}, {collection: 'sensor_records'});

module.exports = mongoose.model('SensorRecord', sensorRecordSchema);
