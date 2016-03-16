import React, {
  Component,
  TextInput,
  StyleSheet
} from 'react-native';

/**
 * Custom Wrapper around TextInput.
 * By default, the text input will always show the current value of the data
 * given to it, and any change we want to make will automatically update the
 * data store value, However, we don't want to update the value until we have
 * finished typing and hit <Enter>, but we want to display whatever we type in
 * the text input field. Therefore, we need to maintain our own state for the
 * value being displayed in the text input (separate from the data store value).
 * @requires value the original input value to be displayed/reset to.
 * @requires onSubmit handler function to be called when textfield is submitted.
 */
export class EditableTextField extends Component {
  constructor(props) {
    super(props);
    this.state = {value: props.value};
    this.onSubmit = this.onSubmit.bind(this);
    this.onChange = this.onChange.bind(this);
  }

  // Called when the input is 'submitted' aka the <Enter> key is pressed
  onSubmit() {
    var success = this.props.onSubmit(this.state.value);
    // If the onsubmit handler fails (possibly due to validation error),
    // restore the original value to the text field.
    if (!success) {
      this.setState({value: this.props.value});
    }
  }

  // Called when any character is typed in the text input
  onChange(value) {
    this.setState({value: value})
  }

  render() {
    return (
      <TextInput
          style={styles.textField}
          onChangeText={this.onChange}
          onSubmitEditing={this.onSubmit}
          value={this.state.value}
        />
    );
  }
}

const styles = StyleSheet.create({
  textField: {
    height: 50,
    width: 175,
    borderColor: '#CCCCCC',
    borderWidth: 1,
    padding: 10,
    backgroundColor: '#FFFFFF',
    borderRadius: 10,
    marginTop: 15,
    marginBottom: 15
  }
});
