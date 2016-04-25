import alt from '../alt';
import EventActions from '../actions/EventActions';

class EventStore {
	constructor() {
		this.events = [];

		this.bindListeners({
			handleEventsChanged: EventActions.EVENTS_CHANGED,
			handleEventChanged: EventActions.EVENT_CHANGED,
			handleEventCreated: EventActions.EVENT_CREATED,
			handleEventRemoved: EventActions.EVENT_REMOVED,
			handleFetchEvents: EventActions.FETCH_EVENTS,
			handleUpdateEvent: EventActions.UPDATE_EVENT
		});

		this.exportPublicMethods({
			findById: this.findById.bind(this)
		});
	}

	handleEventsChanged(events) {
		this.events = events;
	}

	handleEventChanged(updated_event) {
		this.events = this.events.map( (event) => {
			if (event._id === updated_event._id) {
				return updated_event;
			} else {
				return event;
			}
		});
	}

	handleFetchEvents() {
		this.events = [];
	}

	handleUpdateEvent(data) {
		this.events = this.events.map( (event) => {
			// merge updated params into the given event object
			if (event._id === data._id) {
				return Object.assign({}, event, data.updatedParams);
			} else {
				return event;
			}
		});
	}

	handleEventRemoved(data) {
		// Keep all the events which don't match the removed event's id
		this.events = this.events.filter( event => {
			return event._id !== data._id;
		});
		console.log(this.events);
	}

	handleEventCreated(data) {
		this.events.push(data);
	}

	findById(event_id) {
		var matchingEvents = this.events.filter( (event) => event._id === event_id);
		if (matchingEvents.length === 0) {
			return undefined;
		} else {
			return matchingEvents[0];
		}
	}
}

export default alt.createStore(EventStore, 'EventStore');
