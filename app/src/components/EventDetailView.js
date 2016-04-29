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
import {GroupPicker} from './GroupPicker';
import {SensorPicker} from './SensorPicker';
import {SENSORS} from '../lib/Constants';

import EventStore from '../stores/EventStore';
import OutletStore from '../stores/OutletStore';
import GroupStore from '../stores/GroupStore';
import EventActions from '../actions/EventActions';

const REFRESH_INTERVAL = 3000;
const OUTPUT_TYPE_VALUES = ['outlet', 'group'];

class InputThresholdSelector extends Component {
  constructor(props) {
    super(props);
    this.getSelectedIndex = this.getSelectedIndex.bind(this);
    this.getValues = this.getValues.bind(this);
    this.onValueChange = this.onValueChange.bind(this);
  }

  getSelectedIndex() {
    return (this.props.input_threshold === 'before'
      || this.props.input_threshold === 'below') ? 0 : 1
  }

  getValues() {
    if(this.props.input === 'time')
      return ['Before','After']
    else
      return ['Below','Above']
  }

  onValueChange(newValue) {
    if (this.props.input === 'time')
      newValue = newValue.replace('After','above').replace('Before','below');
    else
      newValue = newValue.toLowerCase();
    this.props.onValueChange(newValue);
  }

  render() {
    return (
      <SegmentedControlIOS
        style={styles.segmentedControl}
        values={this.getValues()}
        selectedIndex={this.getSelectedIndex()}
        onValueChange={this.onValueChange}>
      </SegmentedControlIOS>
    )
  }
}

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
    this.onOutputTypeChange = this.onOutputTypeChange.bind(this);
    this.onNameChange = this.onNameChange.bind(this);
	}

	componentDidMount() {
    var event = EventStore.getState().events.filter( (event) => event._id === this.props.event_id)[0];
    this.setState({
      event: event,
      selectedOutputTypeIndex: (event.output_type === 'outlet') ? 0 : 1,
      loaded: true
    });
    EventStore.listen(this.onChange);
    EventActions.fetchEvent(this.props.event_id);
    // Regularly ping the server for the latest event data.
    this.updateIntervalId = setInterval(() => {
      EventActions.fetchEvent(this.props.event_id);
    }, REFRESH_INTERVAL);
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
  showGroupSelector(param) {
    var title = 'Output Group';

    this.props.navigator.push({
      title: title,
      component: GroupPicker,
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

  showInputValueSelector() {
    var onInputValueChange = (param_name, newValue) => {
      var input = this.state.event.input;
      if (input === 'time') {
        if (newValue.indexOf('pm') > 0) {
          newValue = parseInt(newValue.replace('pm','')) + 12;
        } else {
          newValue = parseInt(newValue.replace('am',''));
        }
      } else if (input === 'temperature') {
        newValue = parseInt(newValue.replace('deg',''));
      } else {
        newValue = parseInt(newValue);
      }
      this.onFormValueChanged('input_value', newValue);
    }
    this.props.navigator.push({
      title: 'Input Threshold',
      component: InputValuePicker,
      passProps: {
        paramName: 'input_value',
        selectedValue: this.formatThresholdValue(this.state.event.input_value),
        onValueChange: onInputValueChange,
        input: this.state.event.input
        //onValueChange: this.onFormValueChanged
      }
    });
  }

  onOutputTypeChange(e) {
    var newValue = OUTPUT_TYPE_VALUES[e.nativeEvent.selectedSegmentIndex];
    if (newValue === 'outlet' && this.state.event.output_type != 'outlet') {
      if (OutletStore.getState().outlets.length === 0) {
        AlertIOS.alert("Cannot choose an outlet, no outlets exist");
        this.setState({ selectedOutputTypeIndex: OUTPUT_TYPE_VALUES.indexOf('group') });
        return;
      }
      var updatedParams = {
        output_type: newValue,
        output_outlet_id: OutletStore.getState().outlets[0]._id
      };
      EventActions.updateEvent(this.props.event_id, updatedParams);
    } else if (newValue === 'group' && this.state.event.output_type != 'group') {
      if (GroupStore.getState().groups.length === 0) {
        AlertIOS.alert("Cannot choose a group, no groups exist");
        this.setState({ selectedOutputTypeIndex: OUTPUT_TYPE_VALUES.indexOf('outlet') });
        return;
      }
      var updatedParams = {
        output_type: newValue,
        output_group_id: GroupStore.getState().groups[0]._id
      };
      EventActions.updateEvent(this.props.event_id, updatedParams);
    }
  }

  // Convert the selected value to the display format.
  formatThresholdValue(value) {
    if (this.state.event.input === 'time') {
      var time = parseInt(value);
      if (time > 12) {
        return (time - 12) + 'pm';
      } else {
        return time + 'am';
      }
    } else if (this.state.event.input === 'temperature') {
      return value + 'deg';
    } else {
      return value;
    }
  }

  removeEvent(event_id) {
    this.props.navigator.pop();
    EventActions.removeEvent(event_id);
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
		if (!inputOutlet) {
      return (<View style={styles.container}><Text>Unknown Input Outlet Id: {event.input_outlet_id}</Text></View>);
    }


    var outputChooserView;
    if (event.output_type === 'outlet') {
      var outputOutlet = OutletStore.findById(event.output_outlet_id);
      if (!outputOutlet) {
        outputChooserView = (<View style={styles.container}><Text>Unknown Output Outlet Id: {event.output_outlet_id}</Text></View>);
      } else {
        outputChooserView = (
          <View style={styles.row}>
            <Text style={styles.label}>Output Outlet:</Text>
            <TouchableOpacity
              style={[styles.button, styles.buttonWhite]}
              onPress={() => this.showOutletSelector('output_outlet_id')}>
              <Text style={styles.buttonText}>{outputOutlet.name}</Text>
            </TouchableOpacity>
          </View>
        );
      }
    } else if (event.output_type === 'group') {
      var outputGroup = GroupStore.findById(event.output_group_id);
      if (!outputGroup) {
        console.log('lol');
        outputChooserView = (<View style={styles.container}><Text>Unknown Output Group Id: {event.output_group_id}</Text></View>);
      } else {
        outputChooserView = (
          <View style={styles.row}>
            <Text style={styles.label}>Output Group:</Text>
            <TouchableOpacity
              style={[styles.button, styles.buttonWhite]}
              onPress={() => this.showGroupSelector('output_group_id')}>
              <Text style={styles.buttonText}>{outputGroup.name}</Text>
            </TouchableOpacity>
          </View>
        );
      }

    } else {
      outputChooserView = (<View style={styles.container}/>);
    }

    return (
			<ScrollView
        contentContainerStyle={styles.container}
        keyboardShouldPersistTaps={false}>
        <View style={styles.row}>
          <Text style={styles.label}>Change Name:</Text>
          <EditableTextField value={event.name} onSubmit={this.onNameChange} />
        </View>
				<View style={styles.row}>
          <Text style={styles.label}>Input Outlet:</Text>
          <TouchableOpacity
            style={[styles.button, styles.buttonWhite]}
            onPress={() => this.showOutletSelector('input_outlet_id')}>
            <Text style={styles.buttonText}>{inputOutlet.name}</Text>
          </TouchableOpacity>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Input:</Text>
          <TouchableOpacity
            style={[styles.button, styles.buttonWhite]}
            onPress={() => this.showSensorSelector()}>
            <Text style={styles.buttonText}>{event.input}</Text>
          </TouchableOpacity>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Trigger:</Text>
          <InputThresholdSelector
            input={event.input}
            input_threshold={event.input_threshold}
            onValueChange={(newValue) => this.onFormValueChanged('input_threshold', newValue)}
            />
          <TouchableOpacity style={[styles.button, styles.buttonWhite,{marginLeft: 15}]}
            onPress={() => this.showInputValueSelector('input_value')}>
            <Text style={styles.buttonText}>
              {this.formatThresholdValue(event.input_value)}
            </Text>
          </TouchableOpacity>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Output Type:</Text>
          <SegmentedControlIOS
            style={styles.segmentedControl}
            values={OUTPUT_TYPE_VALUES}
            selectedIndex={this.state.selectedOutputTypeIndex}
            onChange={this.onOutputTypeChange}>
          </SegmentedControlIOS>
        </View>
				{outputChooserView}
        <View style={styles.row}>
          <Text style={styles.label}>Output Action:</Text>
          <SegmentedControlIOS
            style={styles.segmentedControl}
            values={['OFF','ON']}
            selectedIndex={(event.output_action === 'OFF') ? 0 : 1}
            onValueChange={(newValue) => this.onFormValueChanged('output_action', newValue)}>
          </SegmentedControlIOS>
        </View>
        <View style={styles.row}>
          <TouchableOpacity
            style={[styles.button, styles.buttonRed]}
            onPress={() => this.removeEvent(event._id)}>
            <Text style={styles.buttonText}>Delete Event</Text>
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
    marginBottom: 15
  },
  label: {
    fontSize: 20,
    marginRight: 15
  },
  buttonText: {
  	fontSize: 20,
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
  buttonRed: {
    backgroundColor: 'red',
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
