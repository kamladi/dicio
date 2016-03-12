/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 */
'use strict';
import React, {
  AppRegistry,
  Component,
  NavigatorIOS,
  TabBarIOS,
  ListView,
  StyleSheet,
  Text,
  View
} from 'react-native';
import * as Icon from 'react-native-vector-icons/Ionicons';
import {OutletListView} from './src/components/OutletListView';
import {OutletDetailView} from './src/components/OutletDetailView';
import {EventListView} from './src/components/EventListView';
import {EventDetailView} from './src/components/EventDetailView';
import OutletActions from './src/actions/OutletActions';
import EventActions from './src/actions/EventActions';

class dicio_ios extends Component {
	constructor(props) {
		super(props);
		this.state = {
			tab: 'outlets'
		};
	}

	render() {
		return (
			<TabBarIOS>
				<Icon.TabBarItemIOS
					iconName='outlet'
					onPress={() => {this.setState({ tab: 'outlets' }); }}
				  selected={this.state.tab === 'outlets'}
				  title='Outlets'>
				  <NavigatorIOS
		        ref="outletNav"
		        style={styles.nav}
		        initialRoute={{
		          component: OutletListView,
		          title: 'Outlets',
		          name: 'outlets',
		          rightButtonTitle: 'Refresh',
		          onRightButtonPress: () => { OutletActions.fetchOutlets(); }
		        }}
		      />
				</Icon.TabBarItemIOS>
				<Icon.TabBarItemIOS
					iconName='levels'
					onPress={() => {this.setState({ tab: 'events' }); }}
				  selected={this.state.tab === 'events'}
				  title='Events'>
				  <NavigatorIOS
		        ref="eventNav"
		        style={styles.nav}
		        initialRoute={{
		          component: EventListView,
		          title: 'Events',
		          name: 'events',
		          rightButtonTitle: 'Refresh',
		          onRightButtonPress: () => { EventActions.fetchEvents(); }
		        }}
		      />
				</Icon.TabBarItemIOS>
			</TabBarIOS>
		);
	}
}

const styles = StyleSheet.create({
  container: {
    flex: 1
  },
  welcome: {
    fontSize: 20,
    textAlign: 'center',
    margin: 10,
  },
  instructions: {
    textAlign: 'center',
    color: '#333333',
    marginBottom: 5,
  },
  title: {
    fontSize: 20,
    marginBottom: 8,
    textAlign: 'center',
  },
  nav: {
    flex: 1
  }
});


AppRegistry.registerComponent('dicio_ios', () => dicio_ios);
