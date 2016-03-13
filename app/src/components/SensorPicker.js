import React, {
	Component,
	PickerIOS,
	View,
	StyleSheet,
	Text
} from 'react-native';

import {SENSORS} from '../lib/Constants';

export class SensorPicker extends Component {
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
				<Text>Choose a Trigger:</Text>
				<PickerIOS
					style={styles.picker}
				  selectedValue={this.state.selectedValue}
				  onValueChange={this.onValueChange}>
				  {SENSORS.map( (sensor,idx) => {
						return <PickerIOS.Item key={idx} label={sensor} value={sensor} />
					})}
				</PickerIOS>
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
  picker: {
  	width: 300
  }
});
