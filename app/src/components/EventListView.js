'use strict';
import React, {
  AlertIOS,
  Component,
  NavigatorIOS,
  TouchableHighlight,
  ListView,
  StyleSheet,
  ScrollView,
  Text,
  View
} from 'react-native';

import EventStore from '../stores/EventStore';
import OutletStore from '../stores/OutletStore';
import EventActions from '../actions/EventActions';
import {EventDetailView} from './EventDetailView';

export class EventListView extends Component {
  constructor(props) {
    super(props);
    this.onEventPress = this.onEventPress.bind(this);
    this.renderRow = this.renderRow.bind(this);
    this.onChange = this.onChange.bind(this);
    this.state = EventStore.getState();
    this.state.dataSource = new ListView.DataSource({
        rowHasChanged: (row1, row2) => row1 !== row2
      });
    this.state.loaded = false;
    this.createEvent = this.createEvent.bind(this);
  }

  componentDidMount() {
    EventStore.listen(this.onChange);
    EventActions.fetchEvents();
  }

  componentWillUnmount() {
    EventStore.unlisten(this.onChange);
  }

  onChange(state) {
    this.setState({
      dataSource: this.state.dataSource.cloneWithRows(state.events),
      events: state.events,
      loaded: true
    });
  }

  createEvent() {
    var outlets = OutletStore.getState().outlets;
    if (outlets.length == 0) {
      AlertIOS.alert("Cannot create event if there are no active outlets");
    } else {
      var onEventNameChosen = (name) => {
        EventActions.createEvent({
          name: name,
          input_outlet_id: outlets[0]._id,
          output_outlet_id: outlets[0]._id})
          .then(() => EventActions.fetchEvents())
          .catch(console.error);
      }

      AlertIOS.prompt(
        'Create New Event', // Title text
        'Choose a name:', // Label above text field
        [
          {text: 'Cancel', onPress: () => console.log('Cancel Pressed'), style: 'cancel'},
          {text: 'OK', onPress: onEventNameChosen, style: 'cancel'},
        ],
        'plain-text',
        "NEW EVENT" // Default value for text field
        );
      }
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
    return (
      <View style={styles.container}>
        <ListView
          dataSource={this.state.dataSource}
          renderRow={this.renderRow}
          style={styles.listview}
        />
        <TouchableHighlight style={[styles.button, styles.newEventButton]}
          onPress={() => this.createEvent()}>
          <Text style={styles.buttonText}>Create New Event</Text>
        </TouchableHighlight>
      </View>
    );
  }

  onEventPress(event) {
    var refreshEvent = () => { EventActions.fetchEvent(event._id); }
    this.props.navigator.push({
      title: event.name,
      name: 'event',
      component: EventDetailView,
      passProps: {
        event_id: event._id
      },
      rightButtonTitle: 'Refresh',
      onRightButtonPress: () => {  refreshEvent(); }
    });
  }

  renderRow(event) {
    return (
      <View key={event._id}>
        <TouchableHighlight
          style={styles.listItem}
          onPress={ () => this.onEventPress(event)}
          underlayColor='#EEEEEE'>
          <Text style={styles.title}>{event.name}</Text>
        </TouchableHighlight>
      </View>
    );
  }
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    padding: 30,
    paddingBottom: 60,
    backgroundColor: '#EEEEEE',
  },
  listItem: {
    flex: 1,
    flexDirection: 'row',
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: '#f7f8fc',
    borderBottomWidth: 0.5,
    borderTopWidth: 0.5,
    borderColor: '#D0DBE4',
    paddingTop: 10
  },
  title: {
    fontSize: 20,
    marginBottom: 8,
    textAlign: 'center',
  },
  listView: {
    flex: 1,
    marginTop: 65,
    backgroundColor: '#F5FCFF',
  },
  buttonText: {
    fontSize: 20,
    color: '#FFFFFF',
    textAlign: 'center'
  },
  button: {
    marginTop: 15,
    marginBottom: 15,
    marginLeft: 10,
    padding: 10,
    borderRadius: 10
  },
  newEventButton: {
    backgroundColor: 'deepskyblue',
  }
});
