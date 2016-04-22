var SensorRecord 		= require('../models/SensorRecord');
var Outlet     			= require('../models/Outlet');
var ObjectId        = require('mongoose').Types.ObjectId;
var utils           = require('../lib/utils');
var BadRequestError = utils.BadRequestError;

const ALLOWED_GARNULARITIES = ['hour', 'minute', 'second'];
const ALLOWED_SENSORS = ['power', 'light', 'temperature', 'humidity'];
const MAX_HISTORY_LENGTH = 20; // will return last 10 hours/minutes/seconds of data

/*
 * Iteratively add levels of granularity depending on the required granularity.
 */
function granularityGroupKey(granularity) {
	var group = {
		day: '$day'
	};

	group['hour'] = '$hour';
	if (granularity == 'hour') {
		return group;
	}
	group['minute'] = '$minute';
	if (granularity == 'minute') {
		return group
	}
	group['second'] = '$second';
	if (granularity == 'second') {
		return group;
	}
	return group;
}
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
					mac_address: macAddress,
					// timestamp: {
					// 	$gte: earliest
					// }
			};

			// Project is a map command, equivalent to 'SELECT', in SQL
			var project = {
				mac_address: 1,
				timestamp: 1,
				day: {
					$dayOfYear: '$timestamp'
				},
				hour: {
					$hour: '$timestamp'
				},
				minute: {
					$minute: '$timestamp'
				},
				second: {
					$second: '$timestamp'
				}
			};
			project['cur_'+sensor] = 1

			// group
			var group = {
					// _id here is the key we want to group by.
				 _id: granularityGroupKey(granularity)
			};
			//group['_id'][granularity] = '$'+granularity;
			// Compute the average for each group for the sensor value we are querying
			group[sensor] = { $avg: '$cur_'+sensor };
			group['timestamp'] = { $min: '$timestamp'};

			var sort = {};
			sort['timestamp'] = -1;

			var projectResult = {};
			project[granularity]

			return SensorRecord.aggregate([
				{$match: match},{$project: project},{$group: group},{$sort: sort}
			]).exec();
		})
		.then( (aggregation) => {
			return res.json(aggregation);
		})
		.catch(next);
}
