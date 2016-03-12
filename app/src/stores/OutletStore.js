import alt from '../alt';
import OutletActions from '../actions/OutletActions';

class OutletStore {
	constructor() {
		this.outlets = [];

		this.bindListeners({
			handleOutletsChanged: OutletActions.OUTLETS_CHANGED,
			handleOutletChanged: OutletActions.OUTLET_CHANGED,
			handleFetchOutlets: OutletActions.FETCH_OUTLETS,
		});

		this.exportPublicMethods({
			findById: this.findById.bind(this)
		});
	}

	handleOutletsChanged(outlets) {
		this.outlets = outlets;
	}

	handleOutletChanged(updated_outlet) {
		this.outlets = this.outlets.map( (outlet) => {
			if (outlet._id === updated_outlet._id) {
				return updated_outlet;
			} else {
				return outlet;
			}
		});
	}

	handleFetchOutlets() {
		this.outlets = [];
	}

	findById(outlet_id) {
		var matchingOutlets = this.outlets.filter( (outlet) => outlet._id === outlet_id);
		if (matchingOutlets.length === 0) {
			return undefined;
		} else {
			return matchingOutlets[0];
		}
	}
}

export default alt.createStore(OutletStore, 'OutletStore');
