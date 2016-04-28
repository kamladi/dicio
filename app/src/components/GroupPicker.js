import React, {
	Component,
	PickerIOS,
	View,
	StyleSheet,
	Text
} from 'react-native';

import GroupStore from '../stores/GroupStore';
import GroupActions from '../actions/GroupActions';

export class GroupPicker extends Component {
	constructor(props) {
		super(props);
		this.state = {
			groups: GroupStore.getState().groups,
			selectedValue: this.props.selectedValue
		};
		this.onGroupListChange = this.onGroupListChange.bind(this);
		this.onValueChange = this.onValueChange.bind(this);
	}

	componentDidMount() {
		GroupStore.listen(this.onGroupListChange);
	}

	componentWillUnmount() {
		GroupStore.unlisten(this.onGroupListChange);
	}

	componentWillReceiveProps(newProps) {
		this.setState({
			selectedValue: newProps.selectedValue
		});
	}

	onGroupListChange(state) {
		this.setState({
			groups: state.groups
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
		var renderedGroupItems = this.state.groups.map( (group) => {
			return <PickerIOS.Item key={group._id} label={group.name} value={group._id} />
		});

		return (
			<View style={styles.container}>
				<Text>Choose an Group:</Text>
				<PickerIOS
					style={styles.picker}
				  selectedValue={this.state.selectedValue}
				  onValueChange={this.onValueChange}>
				  {renderedGroupItems}
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
