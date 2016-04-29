var BadRequestError = require('../lib/utils').BadRequestError;
var Gateway         = require('../Gateway');
var Group           = require('../models/Group');
var ObjectId        = require('mongoose').Types.ObjectId;

/*
 * Returns of list of all groups
 */
exports.getGroups = (req,res) => {
	return Group.find({}).populate('outlets')
		.then( groups => res.json(groups));
}

/*
 * Returns an object of all the details for a particular group with the
 * given id.
 */
exports.getGroupDetails = (req, res, next) => {
	req.checkParams('id', 'Invalid Group ID').notEmpty().isObjectId();
	var errors = req.validationErrors();
	if (errors) {
		return res.send(errors, 400);
	}
	var id = new ObjectId(req.params.id);
	return Group.findById(id)
		.populate('outlets').exec()
		.then( group => {
			if (!group) {
				throw new BadRequestError(`Cannot find group with id ${id}`);
			}
			return res.json(group);
		})
		.catch(next);
}

/*
 * Updates and group with the given ID with the parameters given in the request
 * body.
 */
exports.updateGroup = (req, res, next) => {
	req.checkParams('id', 'Invalid Group ID').notEmpty();
	var errors = req.validationErrors();
	if (errors) {
		return res.send(errors, 400);
	}
	var id = new ObjectId(req.params.id);

	var updatedParams = {};

	if (req.body.name) {
		updatedParams['name'] = req.body.name;
	}

	// Convert each outlet id string to an ObjectId
	if (req.body.outlets) {
		updatedParams['$set'] = {
			'outlets': JSON.parse(req.body.outlets).map( id => new ObjectId(id))
		}
	}

	return Group.findByIdAndUpdate(id, updatedParams, {new: true})
		.populate('outlets').exec()
		.then( group => {
			// successful update, return update group
			return res.json(group);
		})
		.catch(next);
};

/*
 * Creates a new group with the given parameters in the request body.
 */
exports.createGroup = (req, res, next) => {
	req.checkBody('name', 'Invalid Group Name').notEmpty();
	var errors = req.validationErrors();
	if (errors) {
		return res.send(errors, 400);
	}

	var newGroup = new Group({name: req.body.name, outlets: []});
	return newGroup.save()
		.then( group => res.json(group))
		.catch(next);
}

/**
 * Deletes a Group with the given id
 */
exports.deleteGroup = (req, res, next) => {
	req.checkParams('id', 'Invalid Group ID').isObjectId();
	var errors = req.validationErrors();
	if (errors) {
		return res.send(errors, 400);
	}
	var id = new ObjectId(req.params.id);
	return Group.findOneAndRemove({_id: id})
		.then( removedGroup => res.json(removedGroup))
		.catch(next);
}

/*
 * Removes all groups from the database
 * (useful for development purposes)
 */
exports.clearGroups = (req, res) => {
	return Group.remove({}).exec()
		.then( () => {
			return res.send('groups reset');
		});
};

exports.sendGroupAction = (req, res, next) => {
	req.checkParams('id', 'Invalid Group ID').notEmpty().isObjectId();
	var id = new ObjectId(req.params.id);
	return Group.findById(id).populate('outlets').exec()
		.then( group => {
			if (!group) {
				throw new BadRequestError(`Cannot find group with id ${id}`);
			}
			var action = (req.params.action === 'on') ? 'ON' : 'OFF';
			if (Gateway.isConnected()) {
				// Forward command to gateway to propagate to the network. Group commands
				// are treated as fire-and-forget (doesn't wait for a CMD-ACK message)
				group.outlets.forEach( outlet => {
					console.log('herp derp ', outlet.mac_address, action);
					return Gateway.sendAction(outlet.mac_address, action)
						.catch(console.error);
				});
				return group;
			} else {
				console.warn("Gateway not connected, not sending commands for group");
				return group;
			}
		})
		.then((group) => { return res.json(group); })
		.catch(next);
}
