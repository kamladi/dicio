import React, {
	Component,
	PickerIOS,
	View,
	StyleSheet,
	Text
} from 'react-native';

const TIME_VALUES = ['12am','1am','2am','3am','4am','5am','6am','7am','8am',
		'9am','10am','11am','12pm','1pm','2pm','3pm','4pm','5pm','6pm','7pm','8pm',
		'9pm','10pm','11pm'];
const TEMP_VALUES = ['20deg', '30deg','40deg','50deg','60deg',
		'70deg','80deg','90deg'];
const LIGHT_VALUES = ['25', '50','75','100','125','150','175','200','225'];

const SENSOR_TO_OPTIONS = {
	'time': TIME_VALUES,
	'temperature': TEMP_VALUES,
	'light': LIGHT_VALUES
};

export class InputValuePicker extends Component {
	constructor(props) {
		super(props);
		this.state = {
			selectedValue: this.props.selectedValue
		};
		this.onValueChange = this.onValueChange.bind(this);
	}

	componentWillReceiveProps(newProps) {
		this.setState({
			selectedValue: newProps.selectedValue
		});
	}

	onValueChange(newValue) {
		this.setState({
			selectedValue: newValue
		});
		this.props.onValueChange(this.props.paramName, newValue);
	}

	render() {
		return (
			<View style={styles.container}>
				<Text>Choose a Threshold Value:</Text>
				<PickerIOS
					style={styles.picker}
				  selectedValue={this.state.selectedValue}
				  onValueChange={this.onValueChange}>
				  {this.renderItems()}
				</PickerIOS>
			</View>
		);
	}

	// Returns a rendered option list based on the given input sensor.
	renderItems() {
		var options = SENSOR_TO_OPTIONS[this.props.input];
		return options.map( (option,idx) => {
			return <PickerIOS.Item key={idx} label={option} value={option} />
		});
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
  picker: {
  	width: 300
  }
});
