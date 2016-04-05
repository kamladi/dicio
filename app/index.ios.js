/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 */
'use strict';
import React, {
	AlertIOS,
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
import {WebSocketHandler} from './src/lib/WebSocketHandler';

class dicio_ios extends Component {
	constructor(props) {
		super(props);
		this.state = {
			tab: 'outlets'
		};
		this.webSocketHandler = new WebSocketHandler();
		this.webSocketHandler.addListener('newNode', this.handleNewNode.bind(this));
		this.webSocketHandler.addListener('lostNode', this.handleLostNode.bind(this));
		this.webSocketHandler.addListener('activeNode', this.handleActiveNode.bind(this));
		this.webSocketHandler.addListener('deadGateway', this.handleDeadGateway.bind(this));
	}

	handleNewNode(outletId, outletName) {
		console.log(`NEW NODE: ${outletName}`);

		// Handler function for when user taps 'OK' on new outlet notification.
		function onNewOutletNameChosen (newName) {
			console.log('OK Pressed');
			OutletActions.updateOutletName(outletId, newName)
				.then(() => OutletActions.fetchOutlets())
				.catch(console.error);
		}

		// Prompt user for a new name for the new outlet.
		AlertIOS.prompt(
			'New outlet discovered!', // Title text
			'Choose a name:', // Label above text field
			[
				{text: 'Cancel', onPress: () => console.log('Cancel Pressed'), style: 'cancel'},
				{text: 'OK', onPress: onNewOutletNameChosen, style: 'cancel'},
			],
			'plain-text',
			outletName // Default value for text field
			);
	}

	handleLostNode(outletId, outletName) {
		console.log(`LOST NODE: ${outletName}`);
		AlertIOS.alert(`Lost Connection to outlet: ${outletName}`);
		OutletActions.fetchOutlets();
	}

	handleActiveNode(outletId, outletName) {
		console.log(`ACTIVE NODE: ${outletName}`);
		AlertIOS.alert(`Connection restored to outlet: ${outletName}`);
		OutletActions.fetchOutlets();
	}

	handleDeadGateway() {
		console.log('DEAD GATEWAY');
		AlertIOS.alert('Lost connection to gateway node');
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
