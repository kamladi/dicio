import alt from '../alt';
import EventActions from '../actions/EventActions';

class EventStore {
	constructor() {
		this.events = [];

		this.bindListeners({
			handleEventsChanged: EventActions.EVENTS_CHANGED,
			handleEventChanged: EventActions.EVENT_CHANGED,
			handleFetchEvents: EventActions.FETCH_EVENTS,
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
