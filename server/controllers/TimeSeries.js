var SensorRecord 		= require('../models/SensorRecord');
var Outlet     			= require('../models/Outlet');
var ObjectId        = require('mongoose').Types.ObjectId;
var utils           = require('../lib/utils');
var BadRequestError = utils.BadRequestError;

const ALLOWED_GARNULARITIES = ['hour', 'minute', 'second'];
const ALLOWED_SENSORS = ['power', 'light', 'temperature', 'humidity'];
const MAX_HISTORY_LENGTH = 20; // will return last 10 hours/minutes/seconds of data

/*
 * Accepts two parameters as query params:
 * 'sensor': must be one of the ALLOWED_SENSORS
 * 'granularity': must be one of the ALLOWED_GRANULARITIES
 */
exports.getSensorHistory = (req, res, next) => {
	req.checkParams('id', 'Invalid Outlet ID').notEmpty().isObjectId();
	var errors = req.validationErrors();
	if (errors) {
		return res.send(errors, 400);
	}
	var id = new ObjectId(req.params.id);
	var sensor = req.query.sensor || 'power';
	var granularity = req.query.granularity || 'second';

	// Check if granularity param is allowed
	if (ALLOWED_SENSORS.indexOf(sensor) < 0) {
		throw new BadRequestError(
			"Invalid sensor, must be one of: " + ALLOWED_SENSORS.join(',')
		);
	}

	// Check if granularity param is allowed
	if (ALLOWED_GARNULARITIES.indexOf(granularity) < 0) {
		throw new BadRequestError(
			"Invalid granularity, must be one of: " + ALLOWED_GARNULARITIES.join(',')
		);
	}

	return Outlet.findById(id, 'mac_address').exec()
		.then( (outlet) => {
			if (!outlet) {
				throw new BadRequestError(`Cannot find outlet with id ${id}`);
			}

			/*** setup aggregation params ****/
			// Match is a filter command, equivalent to 'WHERE' in SQL
			var now =  new Date();
			var earliest = new Date(now);
			earliest.setHours(earliest.getHours() - MAX_HISTORY_LENGTH);
			var macAddress = outlet.mac_address;
			console.log(macAddress);
			var match = {
				$match: {
					mac_address: macAddress,
					// timestamp: {
					// 	gte: earliest,
					// 	lte: new Date()
					// }
				}
			};

			// Project is a map command, equivalent to 'SELECT', in SQL
			var project = {
				$project: {
					hour: {
						$hour: '$timestamp'
					},
					minute: {
						$minute: '$timestamp'
					},
					second: {
						$second: '$timestamp'
					},
					timestamp: 1,
					mac_address: 1
				}
			};
			project['$project']['cur_sensor'] = 1

			// group
			var group = {
				$group: {
				 _id: '$'+granularity, // _id here is the key we want to group by.
				}
			};
			// Compute the average for each group for the sensor value we are querying
			group['$group'][sensor] = { $avg: '$cur_'+sensor };

			var sort = {
				$sort: {
					timestamp: 1
				}
			};

			return SensorRecord.aggregate([match, project, group]).exec();
		})
		.then( (aggregation) => {
			return res.json(aggregation);
		})
		.catch(next);
}
