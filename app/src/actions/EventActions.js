import alt from '../alt';
import {API_EVENTS_URL} from '../lib/Constants';

class EventActions {
  eventsChanged(events) {
    return events;
  }

  eventChanged(event) {
    console.log(event);
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

  updateEvent(event_id, update_params) {
    console.log("updating: ", update_params);
    fetch(`${API_EVENTS_URL}/${event_id}`, {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(update_params)
    })
      .then((response) => response.json())
      .then((responseData) => {
        this.eventChanged(responseData);
      })
      .catch(console.error)
      .done();
    return {};
  }

}

export default alt.createActions(EventActions);
