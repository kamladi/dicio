import alt from '../alt';
import {API_OUTLETS_URL} from '../lib/Constants';

class OutletActions {
  outletsChanged(outlets) {
    return outlets;
  }

  outletChanged(outlet) {
    return outlet;
  }

  fetchOutlets() {
  	fetch(API_OUTLETS_URL)
      .then((response) => response.json())
      .then((responseData) => {
        this.outletsChanged(responseData);
      })
      .catch(console.error)
      .done();
    return [];
  }

  fetchOutlet(outlet_id) {
    fetch(`${API_OUTLETS_URL}/${outlet_id}`)
      .then((response) => response.json())
      .then((responseData) => {
        this.outletChanged(responseData);
      })
      .catch(console.error)
      .done();
    return {};
  }

  updateOutletName(outlet_id, name) {
    fetch(`${API_OUTLETS_URL}/${outlet_id}`, {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ name: name })
    })
      .then((response) => response.json())
      .then((responseData) => {
        this.outletChanged(responseData);
      })
      .catch(console.error)
      .done();
    return {};
  }
}

export default alt.createActions(OutletActions);
