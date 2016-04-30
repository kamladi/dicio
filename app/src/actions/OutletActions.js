import alt from '../alt';
import {API_OUTLETS_URL, onNetworkRequestError} from '../lib/Constants';

class OutletActions {
  outletsChanged(outlets) {
    return outlets;
  }

  outletChanged(outlet) {
    return outlet;
  }

  fetchOutlets() {
  	return fetch(API_OUTLETS_URL)
      .then((response) => response.json())
      .then((responseData) => {
        this.outletsChanged(responseData);
      })
      .catch(this.onError)
  }

  fetchOutlet(outlet_id) {
    return fetch(`${API_OUTLETS_URL}/${outlet_id}`)
      .then((response) => response.json())
      .then((responseData) => {
        this.outletChanged(responseData);
      })
      .catch(this.onError)
  }

  updateOutletName(outlet_id, name) {
    return fetch(`${API_OUTLETS_URL}/${outlet_id}`, {
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
      .catch(this.onError)
  }

  onError(err) {
    onNetworkRequestError(err);
  }
}

export default alt.createActions(OutletActions);
