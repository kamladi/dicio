'use strict';
import React, {
  AppRegistry,
  Component,
  NavigatorIOS,
  ListView,
  StyleSheet,
  TouchableHighlight,
  Text,
  PickerIOS,
  View,
  AlertIOS,
  SegmentedControlIOS,
} from 'react-native';

import {EditableTextField} from './EditableTextField';
import {OutletPicker} from './OutletPicker';
import {SensorPicker} from './SensorPicker';
import {SENSORS} from '../lib/Constants';

import EventStore from '../stores/EventStore';
import OutletStore from '../stores/OutletStore';
import EventActions from '../actions/EventActions';

export class EventDetailView extends Component {
	constructor(props) {
		super(props);
		this.state = {
      event: null,
      loaded: false
    };
    this.onChange = this.onChange.bind(this);
    this.showOutletSelector = this.showOutletSelector.bind(this);
    this.onFormValueChanged = this.onFormValueChanged.bind(this);
    this.onNameChange = this.onNameChange.bind(this);
	}

	componentDidMount() {
    this.setState({
      event: EventStore.getState().events.filter( (event) => event._id === this.props.event_id)[0],
      loaded: true
    });
    EventStore.listen(this.onChange);
    EventActions.fetchEvent(this.props.event_id);
    // Regularly ping the server for the latest event data.
    this.updateIntervalId = setInterval(() => {
      EventActions.fetchEvent(this.props.event_id);
    }, 500);
  }

  componentWillUnmount() {
    EventStore.unlisten(this.onChange);
    clearInterval(this.updateIntervalId);
  }

  onChange(state) {
    this.setState({
      event: EventStore.findById(this.props.event_id),
      loaded: true
    });
  }

  onNameChange(newName) {
    var name = newName.trim();
    if (name.length === 0) {
      AlertIOS.alert("Invalid event name");
      return false;
    }
    EventActions.updateEvent(this.props.event_id, {name: name});
    return true;
  }

  onFormValueChanged(paramName, value) {
    var updatedParams = {};
    updatedParams[paramName] = value;
    EventActions.updateEvent(this.props.event_id, updatedParams);
  }

  // open a new view with a 'picker' to choose an input/output outlet
  showOutletSelector(param) {
    var title = (param === 'input_outlet_id') ? 'Input Outlet' : 'Output Outlet';

    this.props.navigator.push({
      title: title,
      component: OutletPicker,
      passProps: {
        paramName: param,
        selectedValue: this.state.event[param],
        onValueChange: this.onFormValueChanged
      }
    });
  }

  // open a new view with a 'picker' to choose an input/output outlet
  showSensorSelector() {
    this.props.navigator.push({
      title: 'Trigger',
      component: SensorPicker,
      passProps: {
        paramName: 'input',
        selectedValue: this.state.event.input,
        onValueChange: this.onFormValueChanged
      }
    });
  }

  renderLoadingView() {
    return (
      <View style={styles.container}>
        <Text>
          Loading events...
        </Text>
      </View>
    );
  }
	render() {
		if (!this.state.loaded) {
			return this.renderLoadingView();
		}

		var event = this.state.event;
    var inputOutlet = OutletStore.findById(event.input_outlet_id);
    var outputOutlet = OutletStore.findById(event.output_outlet_id);
		if (!inputOutlet) {
      return (<Text>Unknown Input Outlet Id: {event.input_outlet_id}</Text>);
    }
    if (!outputOutlet) {
      return (<Text>Unknown Output Outlet Id: {event.output_outlet_id}</Text>);
    }
    return (
			<View style={styles.container}>
        <View style={styles.row}>
          <Text style={styles.label}>Change Name:</Text>
          <EditableTextField value={event.name} onSubmit={this.onNameChange} />
        </View>
				<View style={styles.row}>
          <Text style={styles.label}>Input Outlet:</Text>
          <TouchableHighlight
            style={[styles.button, styles.buttonWhite]}
            onPress={() => this.showOutletSelector('input_outlet_id')}>
            <Text style={styles.buttonText}>{inputOutlet.name}</Text>
          </TouchableHighlight>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Input:</Text>
          <TouchableHighlight
            style={[styles.button, styles.buttonWhite]}
            onPress={() => this.showSensorSelector()}>
            <Text style={styles.buttonText}>{event.input}</Text>
          </TouchableHighlight>
        </View>

				<Text>Input Trigger: {event.input_threshold} {event.input_value}</Text>
				<View style={styles.row}>
          <Text style={styles.label}>Output Outlet:</Text>
          <TouchableHighlight
            style={[styles.button, styles.buttonWhite]}
            onPress={() => this.showOutletSelector('output_outlet_id')}>
            <Text style={styles.buttonText}>{outputOutlet.name}</Text>
          </TouchableHighlight>
				</View>
        <View style={styles.row}>
          <Text style={styles.label}>Output Action:</Text>
          <SegmentedControlIOS
            style={styles.segmentedControl}
            values={['OFF','ON']}
            selectedIndex={(event.output_action === 'OFF') ? 0 : 1}
            onValueChange={(newValue) => this.onFormValueChanged('output_action', newValue)}>
          </SegmentedControlIOS>
        </View>
			</View>
		);
	}
}

const styles = StyleSheet.create({
	container: {
    flex: 1,
    padding: 30,
    marginTop: 65,
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
  	fontSize: 20
  },
  button: {
  	padding: 10,
  	borderRadius: 10
  },
  buttonWhite: {
    backgroundColor: 'white',
    borderColor: '#cccccc',
    borderWidth: 1
  },
  buttonOn: {
  	backgroundColor: 'green',
  },
  buttonOff: {
  	backgroundColor: 'red',
  },
  segmentedControl: {
    width: 125,
    height: 50,
  },
  picker: {
    width: 300
  }
});
