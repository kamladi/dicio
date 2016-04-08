'use strict';
import React, {
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
});
