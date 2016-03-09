var Outlet = require('../models/Outlet');
var ObjectId = require('mongoose').Types.ObjectId;
var utils = require('../lib/utils');
var BadRequestError = utils.BadRequestError;
var Gateway = require('../gateway');

/*
 * Returns a list of all outlet names and id's
 */
exports.getOutlets = (req, res) => {
	return Outlet.find({}, 'name').exec()
		.then( outlets => {
			return res.json(outlets);
		});
};

/*
 * Returns and object of all the details for a particular outlet with
 * the given id.
 */
exports.getOutletDetails = (req, res, next) => {
	req.checkParams('id', 'Invalid Outlet ID').notEmpty().isObjectId();
	var errors = req.validationErrors();
	if (errors) {
		return res.send(errors, 400);
	}
	var id = new ObjectId(req.params.id);
	return Outlet.findById(id).exec()
		.then( outlet => {
			if (!outlet) {
				throw new BadRequestError(`Cannot find outlet with id ${id}`);
			}
			return res.json(outlet);
		})
		.catch(next);
};

exports.clearOutlets = (req, res) => {
	return Outlet.remove({}).exec()
		.then( () => {
			return res.send('outlets reset');
		});
};

exports.sendOutletAction = (req, res, next) => {
	req.checkParams('id', 'Invalid Outlet ID').notEmpty().isObjectId();
	var id = new ObjectId(req.params.id);
	return Outlet.findById(id).exec()
		.then( outlet => {
			if (!outlet) {
				throw new BadRequestError(`Cannot find outlet with id ${id}`);
			}
			var action = (req.params.action === 'on') ? 'ON' : 'OFF';
			Gateway.sendAction(outlet.mac_address, action);
			return res.json(outlet);
		})
		.catch(next);
}
