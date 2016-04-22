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

import GroupStore from '../stores/GroupStore';
import GroupActions from '../actions/GroupActions';
import {GroupDetailView} from './GroupDetailView';

export class GroupListView extends Component {
  constructor(props) {
    super(props);
    this.onGroupPress = this.onGroupPress.bind(this);
    this.renderRow = this.renderRow.bind(this);
    this.onChange = this.onChange.bind(this);
    this.state = GroupStore.getState();
    this.state.dataSource = new ListView.DataSource({
        rowHasChanged: (row1, row2) => row1 !== row2
      });
    this.state.loaded = false;
  }

  componentDidMount() {
    GroupStore.listen(this.onChange);
    GroupActions.fetchGroups();
  }

  componentWillUnmount() {
    GroupStore.unlisten(this.onChange);
  }

  onChange(state) {
    this.setState({
      dataSource: this.state.dataSource.cloneWithRows(state.groups),
      groups: state.groups,
      loaded: true
    });
  }

  renderLoadingView() {
    return (
      <View style={styles.container}>
        <Text>
          Loading groups...
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

  onGroupPress(group) {
    var refreshGroup = () => { GroupActions.fetchGroup(group._id); }
    this.props.navigator.push({
      title: group.name,
      name: 'group',
      component: GroupDetailView,
      passProps: {
        group_id: group._id,
      },
      rightButtonTitle: 'Refresh',
      onRightButtonPress: () => {  refreshGroup(); }
    });
  }

  renderRow(group) {
    return (
      <View key={group._id}>
        <TouchableHighlight
          style={styles.listItem}
          onPress={ () => this.onGroupPress(group)}
          underlayColor='#EEEEEE'>
          <Text style={styles.title}>{group.name}</Text>
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