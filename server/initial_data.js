var Event  = require('./models/Event');
var Outlet = require('./models/Outlet');

var outlets = [
	{
		name: 'Outlet 1',
		mac_address: '1',
		status: 'ON',
		cur_temperature: 1024,
		cur_humidity: 1024,
		cur_light: 1024,
		cur_power: 1024
	},
	{
		name: 'Outlet 2',
		mac_address: '2',
		status: 'ON',
		cur_temperature: 256,
		cur_humidity: 256,
		cur_light: 256,
		cur_power: 256
	},
	{
		name: 'Outlet 3',
		mac_address: '3',
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

function init() {
	// query all outlets
	return Outlet.find({}).exec()
		.then( result => {
			if (result.length === 0) {
				// add sample data if there aren't any outlets in the database
				outlets.forEach( (outletData, i) => {
					var o = new Outlet(outletData);
					o.save()
						.then((savedOutlet) => {
							// add a sample event which links to the newly created outlet
							var newEvent = new Event(events[i]);
							newEvent.input_outlet_id = savedOutlet._id;
							newEvent.output_outlet_id = savedOutlet._id;
							return newEvent.save();
						}).catch(console.trace);
				});
			}
		})
		.catch(console.trace);
};

exports.init = init;

