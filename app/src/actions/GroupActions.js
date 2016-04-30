import alt from '../alt';
import {API_GROUPS_URL, onNetworkRequestError} from '../lib/Constants';

class GroupActions {
  groupsChanged(groups) {
    return groups;
  }

  groupChanged(group) {
    return group;
  }

  groupRemoved(group) {
    return group;
  }

  groupCreated(group) {
    return group;
  }

  fetchGroups() {
  	fetch(API_GROUPS_URL)
      .then((response) => response.json())
      .then((responseData) => {
        this.groupsChanged(responseData);
      })
      .catch(this.onError)
      .done();
    return [];
  }

  fetchGroup(group_id) {
    fetch(`${API_GROUPS_URL}/${group_id}`)
      .then((response) => response.json())
      .then((responseData) => {
        this.groupChanged(responseData);
      })
      .catch(this.onError)
      .done();
    return {};
  }

  updateGroup(group_id, updatedParams) {
    console.log("updating: ", updatedParams);
    fetch(`${API_GROUPS_URL}/${group_id}`, {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(updatedParams)
    })
      .then((response) => response.json())
      .then((responseData) => {
        this.groupChanged(responseData);
      })
      .catch(console.error)
      .done();
    return {group_id, updatedParams};
  }

  removeGroup(group_id) {
    console.log('deleting group: ' + group_id);
    return fetch(`${API_GROUPS_URL}/${group_id}`, {
      method: 'DELETE',
      headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
      }
    })
      .then((response) => response.json())
      .then((responseData) => {
        this.groupRemoved(responseData);
      })
      .catch(console.error);
  }

  createGroup(params) {
    console.log('creating group');
    return fetch(`${API_GROUPS_URL}`, {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(params)
    })
      .then((response) => response.json())
      .then((responseData) => {
        this.groupCreated(responseData);
      })
      .catch(this.onError);
  }

  onError(err) {
    onNetworkRequestError(err);
  }

}

export default alt.createActions(GroupActions);
