import alt from '../alt';
import {API_EVENTS_URL} from '../lib/Constants';

class EventActions {
  eventsChanged(events) {
    return events;
  }

  eventChanged(event) {
    return event;
  }

  eventRemoved(event) {
    return event;
  }

  eventCreated(event) {
    return event;
  }

  fetchEvents() {
  	fetch(API_EVENTS_URL)
      .then((response) => response.json())
      .then((responseData) => {
        this.eventsChanged(responseData);
      })
      .catch(console.error)
      .done();
    return [];
  }

  fetchEvent(event_id) {
    fetch(`${API_EVENTS_URL}/${event_id}`)
      .then((response) => response.json())
      .then((responseData) => {
        this.eventChanged(responseData);
      })
      .catch(console.error)
      .done();
    return {};
  }

  updateEvent(event_id, updatedParams) {
    console.log("updating: ", updatedParams);
    fetch(`${API_EVENTS_URL}/${event_id}`, {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(updatedParams)
    })
      .then((response) => response.json())
      .then((responseData) => {
        this.eventChanged(responseData);
      })
      .catch(console.error)
      .done();
    return {event_id, updatedParams};
  }

  removeEvent(event_id) {
    console.log('deleting event: ' + event_id);
    return fetch(`${API_EVENTS_URL}/${event_id}`, {
      method: 'DELETE',
      headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
      }
    })
      .then((response) => response.json())
      .then((responseData) => {
        this.eventRemoved(responseData);
      })
      .catch(console.error);
  }

  createEvent(params) {
    console.log('creating event');
    return fetch(`${API_EVENTS_URL}`, {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(params)
    })
      .then((response) => response.json())
      .then((responseData) => {
        this.eventCreated(responseData);
      })
      .catch(console.error);
  }

}

export default alt.createActions(EventActions);
