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
import * as Icon from 'react-native-vector-icons/Ionicons';
import OutletStore from '../stores/OutletStore';
import OutletActions from '../actions/OutletActions';
import {OutletDetailView} from './OutletDetailView';

const REFRESH_INTERVAL = 1000;

export class OutletListView extends Component {
  constructor(props) {
    super(props);

    this.onOutletPress = this.onOutletPress.bind(this);
    this.renderRow = this.renderRow.bind(this);
    this.onChange = this.onChange.bind(this);
    this.state = OutletStore.getState();
    this.state.dataSource = new ListView.DataSource({
      rowHasChanged: (r1, r2) => r1 !== r2
    });
    this.state.loaded = false;
  }

  componentDidMount() {
    OutletStore.listen(this.onChange);
    OutletActions.fetchOutlets();

    // Regularly ping the server for the latest outlet data.
    this.updateIntervalId = setInterval(() => {
      OutletActions.fetchOutlets();
    }, REFRESH_INTERVAL);
  }

  componentWillUnmount() {
    OutletStore.unlisten(this.onChange);
    clearInterval(this.updateIntervalId);
  }

  onChange(state) {
    this.setState({
      dataSource: this.state.dataSource.cloneWithRows(state.outlets),
      outlets: state.outlets,
      loaded: true
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

  onOutletPress(outlet) {
    var refreshOutlet = () => { OutletActions.fetchOutlet(outlet._id); }
    this.props.navigator.push({
      title: outlet.name,
      name: 'outlet',
      component: OutletDetailView,
      passProps: {
        outlet_id: outlet._id,
      },
      rightButtonTitle: 'Refresh',
      onRightButtonPress: () => {  refreshOutlet(); }
    });
  }

  renderRow(outlet) {
    var titleStyle = (outlet.active) ? styles.title
        : [styles.title,styles.inactive];
    return (
      <View key={outlet._id}>
        <TouchableHighlight
          style={styles.listItem}
          onPress={ () => this.onOutletPress(outlet)}
          underlayColor='#EEEEEE'>
          <Text style={titleStyle}>{outlet.name}</Text>
        </TouchableHighlight>
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
  inactive: {
    color: '#999999'
  },
  listView: {
    flex: 1,
    marginTop: 65,
    backgroundColor: '#F5FCFF',
  },
});
