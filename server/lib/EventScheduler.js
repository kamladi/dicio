var Event      = require('../models/Event');
var ObjectId   = require('mongoose').Types.ObjectId;
var Outlet     = require('../models/Outlet');

/**
 * Given an event and an outlet (where the outlet is the event's input_outlet),
 * determine if an action should be triggered.
 * Compares the event's threshold value and direction with the outlet's
 * appropriate sensor value.
 * @return a non-null 'action' object if an action should be fired,
 *   null otherwise.
 */
function shouldTriggerAction(event, outlet) {
	var thresholdDirection = event.input_threshold;
	var threshold = null;
	var testValue = null;
	if (event.input === 'time') {
		// Compare the hours of the current time and the saved hour value.
		testValue = (new Date()).getHours();
		threshold = (new Date()).setHours(event.input_value);
	} else if (event.input === 'temperature') {
		testValue = outlet.cur_temperature;
		threshold = event.input_value;
	} else if (event.input === 'light') {
		testValue = outlet.cur_light;
		threshold = event.input_value;
	} else {
		// Shouldn't get here, but let's make temperature the default.
		testValue = outlet.cur_temperature;
		threshold = event.input_value;
	}
	// Create 'action' object if the test value is past the threshold
	// in the desired direction.
	if ( (thresholdDirection === 'above' && threshold <= testValue)
			|| (thresholdDirection === 'below' && testValue < threshold)) {
		return {
			eventId: event._id,
			outletId: event.output_outlet_id,
			action: event.output_action
		};
	} else {
		return null;
	}
}

function triggerCommandsFromEvents(outlet) {
	// Get all events that use this outlet as an input.
	return Event.find({input_outlet_id: new ObjectId(outlet._id)}).exec()
		.then( events => {
			// Map each event to a possible action (if one should be triggered)
			// TODO: how to handle duplicate/conflicting actions?
			return events.map( event => shouldTriggerAction(event, outlet))
				.filter( action => (action != null));
		}).then( actions => {
			// Map each action to a Promise<result>, where result is a
			// (macAddress,action) object if a command should be sent, null otherwise
			var actionPromises = actions.map( action => {
				// Get the output outlet defined by the event.
				// Convert it into an action if the outlet's state doesn't match the
				// 	Event's action.
				return Outlet.findById(new ObjectId(action.outletId)).exec()
					.then( outlet => {
						if (!outlet) throw new Error('Invalid output outlet id in event');

						if (outlet && outlet.status != action.action) {
							console.log(`Outlet ${outlet.mac_address} ${outlet.status}=>${action.action}`);

							// Add the output outlet's mac address to the action object
							return Object.assign(action, {
								destMacAddress: outlet.mac_address,
							});
						} else {
							// Outlet state already matches desired action, don't send command.
							return null;
						}
					});
			});
			// Wait for all outlet queries to complete, then filter out any null objects.
			return Promise.all(actionPromises)
				.then(actions => {
					return actions.filter( action => (action != null));
				});
		});
}

exports.shouldTriggerAction = shouldTriggerAction;
exports.triggerCommandsFromEvents = triggerCommandsFromEvents;
