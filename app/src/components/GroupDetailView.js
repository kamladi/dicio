'use strict';
import React, {
  AppRegistry,
  Component,
  NavigatorIOS,
  ListView,
  StyleSheet,
  TouchableOpacity,
  Text,
  PickerIOS,
  View,
  AlertIOS,
  ScrollView,
  SegmentedControlIOS,
} from 'react-native';

import {EditableTextField} from './EditableTextField';
import {InputValuePicker} from './InputValuePicker';
import {OutletPicker} from './OutletPicker';
import {SensorPicker} from './SensorPicker';
import {SENSORS, API_GROUPS_URL} from '../lib/Constants';

import GroupStore from '../stores/GroupStore';
import OutletStore from '../stores/OutletStore';
import GroupActions from '../actions/GroupActions';

const REFRESH_INTERVAL = 3000;

class GroupActionButton extends Component {
  constructor(props) {
    super(props);
    this.onButtonPressed = this.onButtonPressed.bind(this);
  }

  onButtonPressed(action) {
    fetch(`${API_GROUPS_URL}/${this.props.group_id}/${action}`)
      .then((response) => response.json())
      .then((responseData) => {
        this.setState({
          group: responseData,
          loaded: true,
        });
      })
      .then( () => AlertIOS.alert(`"${action.toUpperCase()}" command sent to group.`))
      .done();
  }

  render() {
    return (
      <View>
        <TouchableOpacity
          style={[styles.button, styles.buttonOff]}
          underlayColor='#CCCCCC'
          onPress={() => this.onButtonPressed('off')}>
          <Text style={styles.buttonText}>Turn Off</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.button, styles.buttonOn]}
          underlayColor='#CCCCCC'
          onPress={() => this.onButtonPressed('on')}>
          <Text style={styles.buttonText}>Turn On</Text>
        </TouchableOpacity>
      </View>
    );

  }
}

export class GroupDetailView extends Component {
	constructor(props) {
		super(props);
		this.state = {
      group: null,
      loaded: false
    };
    this.onChange = this.onChange.bind(this);
    this.showOutletSelector = this.showOutletSelector.bind(this);
    this.onNameChange = this.onNameChange.bind(this);
    this.removeOutlet = this.removeOutlet.bind(this);
    this.removeGroup = this.removeGroup.bind(this);
	}

	componentDidMount() {
    this.setState({
      group: GroupStore.getState().groups.filter( (group) => group._id === this.props.group_id)[0],
      loaded: true
    });
    GroupStore.listen(this.onChange);
    GroupActions.fetchGroup(this.props.group_id);
    // Regularly ping the server for the latest group data.
    this.updateIntervalId = setInterval(() => {
      GroupActions.fetchGroup(this.props.group_id);
    }, REFRESH_INTERVAL);
  }

  componentWillUnmount() {
    GroupStore.unlisten(this.onChange);
    clearInterval(this.updateIntervalId);
  }

  onChange(state) {
    this.setState({
      group: GroupStore.findById(this.props.group_id),
      loaded: true
    });
  }

  onNameChange(newName) {
    var name = newName.trim();
    if (name.length === 0) {
      AlertIOS.alert("Invalid group name");
      return false;
    }
    GroupActions.updateGroup(this.props.group_id, {name: name});
    return true;
  }

  // open a new view with a 'picker' to choose an input/output outlet
  showOutletSelector(param) {
    var title = 'Select Outlet to Add to Group';

    var initialSelectedValue = null;
    var allOutlets = OutletStore.getState().outlets;
    if (allOutlets.length > 0) {
      initialSelectedValue = allOutlets[0];
    }

    var onValueChange = function (paramName, selectedOutletId) {
      this.addOutlet(selectedOutletId);
    }.bind(this);

    this.props.navigator.push({
      title: title,
      component: OutletPicker,
      passProps: {
        paramName: param,
        selectedValue: initialSelectedValue,
        onValueChange: (paramName, selectedOutletId) => this.addOutlet(selectedOutletId)
      },
      rightButtonTitle: 'Done',
      onRightButtonPress: () => { this.props.navigator.pop(); }
    });
  }

  removeOutlet(outlet_id) {
    // remove this outlet by filtering all the outlets that don't match this id
    var outletIds = this.state.group.outlets
      .map(outlet => outlet._id).filter( id => id !== outlet_id);
    GroupActions.updateGroup(this.props.group_id, {'outlets': JSON.stringify(outletIds)});
  }

  addOutlet(outlet_id) {
    var outlets = this.state.group.outlets;
    var existingOutlet = outlets.filter(outlet => outlet._id === outlet_id);
    // only add the outlet if it doesn't already exist in the outlets list
    if (existingOutlet.length === 0) {
      var newOutlets = outlets.map(outlet => outlet._id).concat([outlet_id]);
      GroupActions.updateGroup(this.props.group_id, {'outlets': JSON.stringify(newOutlets)});
    } else {
      AlertIOS.alert("Selected Outlet already exists in the group");
    }
  }

  removeGroup(group_id) {
    this.props.navigator.pop();
    GroupActions.removeGroup(group_id);
  }

  renderLoadingView() {
    return (
      <View style={styles.container}>
        <Text>
          Loading groups...
        </Text>
      </View>
    );
  }
	render() {
		if (!this.state.loaded) {
			return this.renderLoadingView();
		}

		var group = this.state.group;
    if (!group) {
      return (
        <View style={styles.container}>
          <Text>
            Group not defined, or group no longer exists :(
          </Text>
        </View>
      );
    }

    return (
			<ScrollView
        contentContainerStyle={styles.container}
        keyboardShouldPersistTaps={false}>
        <View style={styles.row}>
          <Text style={styles.label}>Change Name:</Text>
          <EditableTextField value={group.name} onSubmit={this.onNameChange} />
        </View>
				{group.outlets.map( (outlet, index) => {
          return (
            <View key={index} style={styles.row}>
              <Text style={styles.label}>{outlet.name}</Text>
              <TouchableOpacity style={[styles.button, styles.buttonRed]}
                onPress={() => this.removeOutlet(outlet._id)}>
                <Text style={styles.buttonText}>
                  Remove Outlet
                </Text>
              </TouchableOpacity>
            </View>
          )
        })}
        <View style={styles.row}>
          <TouchableOpacity
            style={[styles.button, styles.buttonLightBlue]}
            onPress={() => this.showOutletSelector('new_outlet')}>
            <Text style={styles.buttonText}>Add Outlet to Group</Text>
          </TouchableOpacity>
        </View>
        <View style={styles.row}>
          <GroupActionButton group_id={group._id} />
        </View>
        <View style={styles.row}>
          <TouchableOpacity
            style={[styles.button, styles.buttonRed]}
            onPress={() => this.removeGroup(group._id)}>
            <Text style={styles.buttonText}>Delete Group</Text>
          </TouchableOpacity>
        </View>
			</ScrollView>
		);
	}
}

const styles = StyleSheet.create({
	container: {
    flex: 1,
    padding: 30,
    backgroundColor: '#EEEEEE',
    alignItems: 'center'
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    marginTop: 15
  },
  label: {
    fontSize: 20,
    marginRight: 15
  },
  buttonText: {
  	fontSize: 20,
    color: 'white'
  },
  button: {
  	padding: 10,
  	borderRadius: 10,
    borderColor: '#cccccc',
    borderWidth: 1
  },
  buttonWhite: {
    backgroundColor: 'white',
  },
  buttonLightBlue: {
    backgroundColor: 'deepskyblue',
  },
  buttonRed: {
    backgroundColor: 'red',
  },
  buttonOn: {
  	backgroundColor: 'green',
  },
  buttonOff: {
  	backgroundColor: 'red',
  },
  picker: {
    width: 300
  }
});
