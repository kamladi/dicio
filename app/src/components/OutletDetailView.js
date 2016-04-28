'use strict';
import React, {
  AppRegistry,
  Component,
  NavigatorIOS,
  ListView,
  StyleSheet,
  Text,
  View,
  TextInput,
  TouchableHighlight,
  AlertIOS,
  ScrollView,
  SegmentedControlIOS
} from 'react-native';

import { LineChart } from 'react-native-ios-charts';
import moment from 'moment';

import {EditableTextField} from './EditableTextField';
import {API_OUTLETS_URL,API_GRAPHS_URL, hasPowerSensor, hasLightSensor} from '../lib/Constants';
import OutletStore from '../stores/OutletStore';
import OutletActions from '../actions/OutletActions';

const LIGHT_VALUE_THRESHOLD = 500;

class OutletChart extends Component {
  constructor(props) {
    super(props);
    this.state = {
      selectedGranularityIndex: 2, // show 'seconds' by default
      labels: [],
      values: []
    };
    this.fetchData.bind(this);
  }

  componentDidMount() {
    var self = this;
    this.fetchData();
  }

  fetchData(granularity) {
    var granularity = granularity || 'second';
    var url = API_GRAPHS_URL + '/' + this.props.outlet_id + '?granularity=' + granularity;
    fetch(url)
      .then((response) => response.json())
      .then( graphData => {
        console.log(graphData);
        console.log(graphData.slice(0,10).map(record => this.formatTimestamp(record.timestamp)));
        this.setState({
          labels: graphData.slice(0,10).map(record => this.formatTimestamp(record.timestamp)),
          values: graphData.slice(0,10).map(record => record.power)
        });
        console.log(this.state);
      }).catch(console.errror);
  }

  formatTimestamp(timestamp) {
    return moment(timestamp).format("M/D/YY h:m:s");
  }

  render() {
    const defaultConfig = {
      dataSets: [{
        values: [-1, 1, -1, 1, -1, 1],
        drawValues: false,
        colors: ['rgb(199, 255, 140)'],
        label: 'Sine function',
        drawCubic: true,
        drawCircles: false,
        lineWidth: 2
      }, {
        values: [1, -1, 1, -1, 1, -1],
        drawValues: false,
        colors: ['rgb(255, 247, 141)'],
        label: 'Cosine function',
        drawCubic: true,
        drawCircles: false,
        lineWidth: 2
      }],
      backgroundColor: 'transparent',
      labels: ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun'],
      minOffset: 20,
      scaleYEnabled: false,
      legend: {
        textSize: 12
      },
      xAxis: {
        axisLineWidth: 0,
        drawLabels: false,
        position: 'bottom',
        drawGridLines: false
      },
      leftAxis: {
        customAxisMax: 1,
        customAxisMin: -1,
        labelCount: 11,
        startAtZero: false,
      },
      rightAxis: {
        enabled: false,
        drawGridLines: false
      },
      valueFormatter: {
        minimumSignificantDigits: 1,
        type: 'regular',
        maximumDecimalPlaces: 1
      }
    };

    var config = {};
    Object.assign(config, defaultConfig, {
      dataSets: [{
        values: this.state.values,
        colors: ['rgb(199, 255, 140)'],
        label: 'Power Usage',
        drawCubic: true,
        drawCircles: false,
        lineWidth: 2,
        valueTextFontSize: 12
      }],
      labels: this.state.labels,
    });
    return (
      <View style={styles.chartContainer}>
        <SegmentedControlIOS
          values={['Hour', 'Minute', 'Second']}
          selectedIndex={this.state.selectedGranularityIndex}
          onChange={(event) => {
            this.setState({selectedIndex: event.nativeEvent.selectedSegmentIndex});
          }}
          onValueChange={(value) => {
            this.fetchData(value.toLowerCase());
          }}
        />
        <LineChart config={config} style={styles.chart}/>
      </View>
    );
  }
}

class OutletActionButton extends Component {
	constructor(props) {
		super(props);
		this.onButtonPressed = this.onButtonPressed.bind(this);
	}

	onButtonPressed() {
    // toggle the current state to get the action we want to send.
    var action = (this.props.status === 'ON') ? 'off' : 'on';
		fetch(`${API_OUTLETS_URL}/${this.props.outlet_id}/${action}`)
      .then((response) => response.json())
      .then((responseData) => {
        this.setState({
          outlet: responseData,
          loaded: true,
        });
      })
      .then( () => AlertIOS.alert(`"${action.toUpperCase()}" command sent to outlet.`))
      .done();
	}

	render() {
		if (this.props.status === 'ON') {
			return (
				<TouchableHighlight
					style={[styles.button, styles.buttonOff]}
          underlayColor='#CCCCCC'
					onPress={() => this.onButtonPressed()}>
					<Text style={styles.buttonText}>Turn Off</Text>
				</TouchableHighlight>
			)
		} else {
			return (
				<TouchableHighlight
					style={[styles.button, styles.buttonOn]}
          underlayColor='#CCCCCC'
					onPress={() => this.onButtonPressed()}>
					<Text style={styles.buttonText}>Turn On</Text>
				</TouchableHighlight>
			)
		}

	}
}

export class OutletDetailView extends Component {
	constructor(props) {
		super(props);
		this.state = {
      outlet: null,
      loaded: false
    };
    this.onChange = this.onChange.bind(this);
    this.onNameChange = this.onNameChange.bind(this);
	}

	componentDidMount() {
    this.setState({
      outlet: OutletStore.getState().outlets.filter( (outlet) => outlet._id === this.props.outlet_id)[0],
      loaded: true
    });
    OutletStore.listen(this.onChange);
    OutletActions.fetchOutlet(this.props.outlet_id);

    // Regularly ping the server for the latest outlet data.
    this.updateIntervalId = setInterval(() => {
      OutletActions.fetchOutlet(this.props.outlet_id);
    }, 500);
  }

  componentWillUnmount() {
    OutletStore.unlisten(this.onChange);
    clearInterval(this.updateIntervalId);
  }

  onChange(state) {
    this.setState({
      outlet: state.outlets.filter( (outlet) => outlet._id === this.props.outlet_id)[0],
      loaded: true
    });
  }

  onNameChange(newName) {
    var name = newName.trim();
    if (name.length === 0) {
      AlertIOS.alert("Invalid outlet name");
      return false;
    }
    OutletActions.updateOutletName(this.props.outlet_id, name);
    return true;
  }

  getGraphData(granularity) {
    return fetch()
  }

  showChart(outlet_id) {
    this.props.navigator.push({
      title: 'chart',
      name: 'outlet',
      component: OutletChart,
      passProps: {
        outlet_id: outlet_id,
      }
    });
  }


  renderLoadingView() {
    return (
      <View style={styles.container}>
        <Text>
          Loading outlets...
        </Text>
      </View>
    );
  }
	render() {
		if (!this.state.loaded) {
			return this.renderLoadingView();
		}

		var outlet = this.state.outlet;

		var lightView;
		if (hasLightSensor(outlet)) {
			lightView = (
				<View style={styles.row}>
				  <Text style={styles.label}>Light:</Text>
	        <Text style={styles.sensorValue}>
	        	{(outlet.cur_light < LIGHT_VALUE_THRESHOLD) ? 'Dark' : 'Bright'}
	        </Text>
	      </View>
			);
		} else {
			lightView = (<View/>);
		}

		var powerView;
		if (hasPowerSensor(outlet)) {
			powerView = (
				<View style={styles.row}>
				  <Text style={styles.label}>Power:</Text>
	        <Text style={styles.sensorValue}>{outlet.cur_power}</Text>
	      </View>
	    );
		} else {
			powerView = (<View/>);
		}

		return (
			<ScrollView
        contentContainerStyle={styles.container}
        keyboardShouldPersistTaps={false}>
        <View style={styles.row}>
				  <Text style={styles.label}>Change name:</Text>
          <EditableTextField value={outlet.name} onSubmit={this.onNameChange} />
				</View>
        <View style={styles.row}>
          <Text style={styles.label}>Temp:</Text>
          <Text style={styles.sensorValue}>{outlet.cur_temperature}&deg;C</Text>
        </View>
        {lightView}
        {powerView}
        <View style={styles.row}>
				  <Text style={styles.label}>Status:</Text>
          <Text style={styles.sensorValue}>{outlet.status}</Text>
          <OutletActionButton status={outlet.status} outlet_id={outlet._id} />
        </View>
        <TouchableHighlight style={[styles.button, styles.showChartButton]}
          onPress={() => this.showChart(outlet._id)}>
          <Text style={styles.buttonText}>Show Power Usage</Text>
        </TouchableHighlight>
			</ScrollView>
		);
	}
}

const styles = StyleSheet.create({
	container: {
    flex: 1,
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
    marginRight: 15,
    flex: 1
  },
  sensorValue: {
    fontSize: 30,
    fontWeight: 'bold',
    flex: 1
  },
  buttonText: {
  	fontSize: 20,
    color: '#FFFFFF'
  },
  button: {
    marginTop: 15,
    marginBottom: 15,
    marginLeft: 10,
  	padding: 10,
  	borderRadius: 10
  },
  buttonOn: {
  	backgroundColor: 'green',
  },
  buttonOff: {
  	backgroundColor: 'red',
  },
  refreshButton: {
    backgroundColor: '#51C6ED'
  },
  showChartButton: {
    backgroundColor: 'deepskyblue'
  },
  chartContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'stretch',
    backgroundColor: 'white',
    paddingTop: 80,
    paddingBottom: 60,
    paddingLeft: 10,
    paddingRight: 20
  },
  chart: {
    flex: 1
  }
});
