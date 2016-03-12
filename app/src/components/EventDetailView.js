'use strict';
import React, {
  AppRegistry,
  Component,
  NavigatorIOS,
  ListView,
  StyleSheet,
  TouchableHighlight,
  Text,
  View,
  AlertIOS,
  SegmentedControlIOS,
} from 'react-native';

import {OutletPicker} from './OutletPicker';

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
	}

	componentDidMount() {
    this.setState({
      event: EventStore.getState().events.filter( (event) => event._id === this.props.event_id)[0],
      loaded: true
    });
    EventStore.listen(this.onChange);
    EventActions.fetchEvent(this.props.event_id);
  }

  componentWillUnmount() {
    EventStore.unlisten(this.onChange);
  }

  onChange(state) {
    this.setState({
      event: EventStore.findById(this.props.event_id),
      loaded: true
    });
  }

  // open a new view with a 'picker' to choose an outlet
  showOutletSelector(param) {
    var onValueChange = function (value) {
      var update_params = {};
      update_params[param] = value;
      EventActions.updateEvent(this.props.event_id, update_params);
    }.bind(this);

    var title = (param === 'input_outlet_id') ? 'Input Outlet' : 'Output Outlet';

    this.props.navigator.push({
      title: title,
      component: OutletPicker,
      passProps: {
        selectedValue: this.state.event[param],
        onValueChange: onValueChange
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
          <Text style={styles.label}>Input Outlet:</Text>
          <TouchableHighlight
            style={[styles.button, styles.buttonWhite]}
            onPress={() => this.showOutletSelector('input_outlet_id')}>
            <Text style={styles.buttonText}>{inputOutlet.name}</Text>
          </TouchableHighlight>
        </View>
				<Text>Input: {event.input}</Text>
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
            onValueChange={(value) => {
              EventActions.updateEvent(event._id, { output_action: value});
            }}>
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
  }
});
