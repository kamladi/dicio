var Event      = require('../models/Event');
var Group 		 = require('../models/Group');
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

	console.log(event.input, testValue, thresholdDirection, threshold);
	// Create 'action' object if the test value is past the threshold
	// in the desired direction.
	return (thresholdDirection === 'above' && threshold <= testValue)
			|| (thresholdDirection === 'below' && testValue < threshold);
}

/**
 * Given an event and input outlet, return a list of 'actions' to be sent to
 * the gateway. An 'action' is simply a raw JS object specifying which outlet to
 * actuate, and what command to send. If an event should trigger a group rather
 * than an individual outlet, it will create an action object for each outlet
 * in the specified group.
 * @param  {[Event]} events list of events that use the given outlet as an input
 * @param  {Outlet}  outlet input outlet object
 * @return {[action]}       list of actions
 */
function eventsToActions(events, outlet) {
	actions = [];
	// Map each event to a Promise, each promises will asyncronously add actions
	// to the actions list.
	var promises = events.map( event => {
		if (shouldTriggerAction(event, outlet)) {
			if (event.output_type === 'outlet') {
				// event should trigger an individual outlet.
				return Outlet.findById(event.output_outlet_id).exec()
					.then(outputOutlet => {
						if (!outputOutlet) throw new Error('Invalid output outlet id in event');

						console.log(`Outlet ${outputOutlet.mac_address} ${outputOutlet.status}=>${event.output_action}`);

						if (outputOutlet.status != event.output_action) {

							// Add the output outlet's mac address to the action object
							actions.push({
								eventId: event._id,
								outletId: outputOutlet._id,
								destMacAddress: outputOutlet.mac_address,
								action: event.output_action
							});
						}

						return actions;
					}).catch(e => {
						throw e;
					});

			} else {
				// Event should trigger a group
				return Group.findById(event.output_group_id).populate('outlets').exec()
					.then( group => {
						if (!group) throw new Error('Invalid output group id in event');

						// add an action for each outlet in the group to the list.
						group.outlets.forEach( groupOutlet => {
							if (groupOutlet && groupOutlet.status != event.output_action) {
								console.log(`Outlet ${groupOutlet.mac_address} ${groupOutlet.status}=>${event.output_action}`);

								// Add the output outlet's mac address to the action object
								actions.push({
									eventId: event._id,
									outletId: groupOutlet._id,
									destMacAddress: groupOutlet.mac_address,
									action: event.output_action
								});
							}
						});

						return actions;
					}).catch(e => {
						throw e;
					});
			}
		} else {
			console.log(event.name, event.input_outlet_id, "nah");
			return Promise.resolve([]);
		}

	});

	// Return the actions list whe all promises are complete.
	return Promise.all(promises)
		.then( () => {
			return actions;
		}).catch( e => {
			throw e;
		});
}

function triggerCommandsFromEvents(outlet) {
	// Get all events that use this outlet as an input.
	return Event.find({input_outlet_id: new ObjectId(outlet._id)}).exec()
		.then( events => {
			// build a list of 'action' objects specifiying destination outlets and
			// actions, if any outlets need to be actuated.
			// TODO: how to handle duplicate/conflicting actions?
			console.log(events);
			return eventsToActions(events, outlet);
		});
}

exports.shouldTriggerAction = shouldTriggerAction;
exports.triggerCommandsFromEvents = triggerCommandsFromEvents;
