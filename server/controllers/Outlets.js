var BadRequestError = require('../lib/utils').BadRequestError;
var Gateway         = require('../Gateway');
var ObjectId        = require('mongoose').Types.ObjectId;
var Outlet          = require('../models/Outlet');

/*
 * Returns a list of all outlet names and id's
 */
exports.getOutlets = (req, res, next) => {
	return Outlet.find({}).exec()
		.then( outlets => {
			return res.json(outlets);
		}).catch(next);
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

/*
 * Removes all outlets from the database
 * (useful for development purposes)
 */
exports.clearOutlets = (req, res, next) => {
	return Outlet.remove({}).exec()
		.then( () => {
			return res.send('outlets reset');
		}).catch(next);
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
			if (Gateway.isConnected()) {
				// Forward command to gateway to propagate to the outlet. Eventually,
				// the outlet will respond with its new state, and the database will
				// be updated. Because of this, the outlet is not immediately updated.
				Gateway.sendAction(outlet.mac_address, action)
					.catch(console.error);
				return outlet;
			} else {
				// If we aren't connected to the gateway node, 'fake' an update by
				// simply updating the status of the node in our database.
				console.warn("Gateway not connected, faking the result of the action");
				outlet.status = action;
				return outlet.save();
			}
		})
		.then((outlet) => { return res.json(outlet); })
		.catch(next);
}

exports.updateOutlet = (req, res, next) => {
	req.checkParams('id', 'Invalid Outlet ID').notEmpty().isObjectId();
	// todo: add check for body param 'name'
	console.log(req.body);
	// if (!req.body.name) {
	// 	throw new Error('Invalid outlet name');
	// }
	var id = new ObjectId(req.params.id);
	var name = req.body.name;
	return Outlet.findById(id).exec()
		.then( outlet => {
			if (!outlet) {
				throw new BadRequestError(`Cannot find outlet with id ${id}`)
			}
			outlet.name = name;
			outlet.save()
				.then( outlet => res.json(outlet) )
				.catch(next);
		})
		.catch(next);
}
