import alt from '../alt';
import GroupActions from '../actions/GroupActions';

class GroupStore {
	constructor() {
		this.groups = [];

		this.bindListeners({
			handleGroupsChanged: GroupActions.GROUPS_CHANGED,
			handleGroupChanged: GroupActions.GROUP_CHANGED,
			handleFetchGroups: GroupActions.FETCH_GROUPS,
			handleUpdateGroup: GroupActions.UPDATE_GROUP
		});

		this.exportPublicMethods({
			findById: this.findById.bind(this)
		});
	}

	handleGroupsChanged(groups) {
		this.groups = groups;
	}

	handleGroupChanged(updated_group) {
		this.groups = this.groups.map( (group) => {
			if (group._id === updated_group._id) {
				return updated_group;
			} else {
				return group;
			}
		});
	}

	handleFetchGroups() {
		this.groups = [];
	}

	handleUpdateGroup(data) {
		this.groups = this.groups.map( (group) => {
			// merge updated params into the given group object
			if (group._id === data._id) {
				return Object.assign({}, group, data.updatedParams);
			} else {
				return group;
			}
		});
	}

	findById(group_id) {
		var matchingGroups = this.groups.filter( (group) => group._id === group_id);
		if (matchingGroups.length === 0) {
			return undefined;
		} else {
			return matchingGroups[0];
		}
	}
}

export default alt.createStore(GroupStore, 'GroupStore');
