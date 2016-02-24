var Outlet = require('./models/Outlet');
var Event = require('./models/Event');
var outlets = [
	{
		name: 'Outlet 1',
		status: 'ON',
		cur_temperature: 1024,
		cur_humidity: 1024,
		cur_light: 1024,
		cur_power: 1024
	},
	{
		name: 'Outlet 2',
		status: 'ON',
		cur_temperature: 256,
		cur_humidity: 256,
		cur_light: 256,
		cur_power: 256
	},
	{
		name: 'Outlet 3',
		status: 'OFF',
		cur_temperature: 10,
		cur_humidity: 10,
		cur_light: 10,
		cur_power: 10
	},
];

var events = [
	{ name: 'Event 1', input_threshold: 'above', input_value: 50 },
	{ name: 'Event 2', input_threshold: 'above', input_value: 100 },
	{ name: 'Event 3', input_threshold: 'below', input_value: 150 },
];

exports.init = () => {
	return Outlet.insertMany(outlets)
		.then( outlets => {
			outlets.forEach( (outlet, i) => {
				var newEvent = new Event(events[i]);
				newEvent.input_outlet_id = outlet._id;
				newEvent.output_outlet_id = outlet._id;
				newEvent.save();
			});
		});
};

