import React, {
	Component,
	PickerIOS,
	View,
	StyleSheet,
	Text
} from 'react-native';

import OutletStore from '../stores/OutletStore';
import OutletActions from '../actions/OutletActions';

export class OutletPicker extends Component {
	constructor(props) {
		super(props);
		this.state = {
			outlets: OutletStore.getState().outlets,
			selectedValue: this.props.selectedValue
		};
		this.onOutletListChange = this.onOutletListChange.bind(this);
		this.onValueChange = this.onValueChange.bind(this);
	}

	componentDidMount() {
		OutletStore.listen(this.onOutletListChange);
	}

	componentWillUnmount() {
		OutletStore.unlisten(this.onOutletListChange);
	}

	componentWillReceiveProps(newProps) {
		this.setState({
			selectedValue: newProps.selectedValue
		});
	}

	onOutletListChange(state) {
		this.setState({
			outlets: state.outlets
		});
	}

	onValueChange(newValue) {
		this.setState({
			selectedValue: newValue
		});
		this.props.onValueChange(this.props.paramName, newValue);
	}

	render() {
		// Render a list of Picker items
		var renderedOutletItems = this.state.outlets.map( (outlet) => {
			return <PickerIOS.Item key={outlet._id} label={outlet.name} value={outlet._id} />
		});

		return (
			<View style={styles.container}>
				<Text>Choose an Outlet:</Text>
				<PickerIOS
					style={styles.picker}
				  selectedValue={this.state.selectedValue}
				  onValueChange={this.onValueChange}>
				  {renderedOutletItems}
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
