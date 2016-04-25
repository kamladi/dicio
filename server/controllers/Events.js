var BadRequestError = require('../lib/utils').BadRequestError;
var Event           = require('../models/Event');
var ObjectId        = require('mongoose').Types.ObjectId;

/*
 * Returns a list of all event names and id's
 */
exports.getEvents = (req, res) => {
	return Event.find({})
		.then( events => {
			return res.json(events);
		});
};

/*
 * Returns and object of all the details for a particular event with
 * the given id.
 */
exports.getEventDetails = (req, res, next) => {
	req.checkParams('id', 'Invalid Event ID').notEmpty().isObjectId();
	var errors = req.validationErrors();
	if (errors) {
		return res.send(errors, 400);
	}
	var id = new ObjectId(req.params.id);
	return Event.findById(id).exec()
		.then( event => {
			if (!event) {
				throw new BadRequestError(`Cannot find event with id ${id}`);
			}
			return res.json(event);
		})
		.catch(next);
};

/*
 * Updates and event with the given ID with the parameters given in the request
 * body.
 */
exports.updateEvent = (req, res, next) => {
	req.checkParams('id', 'Invalid Event ID').notEmpty();
	var errors = req.validationErrors();
	if (errors) {
		return res.send(errors, 400);
	}
	var id = new ObjectId(req.params.id);
	const allowedParams = [
		'name', 'input_outlet_id', 'input', 'input_value', 'input_threshold',
		'output_type', 'output_group_id', 'output_outlet_id', 'output_action'
	];

	// Iterate over the list of allowed params, and if that param is in the
	// request body, add it to an object of updated params to be saved.
	var updatedParams = {};
	allowedParams.forEach( (param) => {
		if (req.body[param]) {
			// if the param is an id, cast to an ObjectId before saving.
			if (param.indexOf('_id') > -1) {
				updatedParams[param] = new ObjectId(req.body[param]);
			} else {
				updatedParams[param] = req.body[param];
			}
		}
	});

	return Event.findByIdAndUpdate(id, updatedParams, {new: true}).exec()
		.then( event => {
			// successful update, return update event
			return res.json(event);
		})
		.catch(next);
};


/**
 * Deletes an Event with the given id
 */
exports.deleteEvent = (req, res, next) => {
	req.checkParams('id', 'Invalid Event ID').isObjectId();
	var errors = req.validationErrors();
	if (errors) {
		return res.send(errors, 400);
	}
	var id = new ObjectId(req.params.id);
	return Event.remove({_id: id})
		.then( () => res.json('success'))
		.catch(next);
}

/*
 * Creates a new event with the given parameters in the request body.
 */
exports.createEvent = (req, res, next) => {
	req.checkBody('name', 'Invalid Event Name').notEmpty();
	req.checkBody('input_outlet_id', 'Undefined input outlet ID').notEmpty();
	// TODO: add more checkBody()'s for other params
	var errors = req.validationErrors();
	if (errors) {
		return res.send(errors, 400);
	}

	var params = {
		'name': req.body.name,
		'input_outlet_id': req.body.input_outlet_id,
		'input': req.body.input,
		'input_value': req.body.input_value,
		'input_threshold': req.body.input_threshold,
		'output_outlet_id': req.body.output_outlet_id,
		'output_action': req.output_action,
	};

	var newEvent = new Event(params);
	return newEvent.save()
		.then( event => {
			// Successful create, return created event
			return res.json(event);
		})
		.catch(next);
};

/*
 * Removes all events from the database
 * (useful for development purposes)
 */
exports.clearEvents = (req, res) => {
	return Event.remove({}).exec()
		.then( () => {
			return res.send('events reset');
		});
};
