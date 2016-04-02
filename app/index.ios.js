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
import {WEBSOCKET_URL} from './src/lib/Constants';

class dicio_ios extends Component {
	constructor(props) {
		super(props);
		this.state = {
			tab: 'outlets'
		};
		this.initWebSocket();
	}

	initWebSocket() {
		// TODO: refactor this to another file/class
		this.ws = new WebSocket(WEBSOCKET_URL);

		this.ws.onopen = () => {
			console.log('Websocket connection open.');
		}

		this.ws.onmessage = (e) => {
			var data = JSON.parse(e.data);
			if (!data.type) {
				console.error(`unrecognized socket message: ${e.data}`);
			}
			if (data.type === 'NEWNODE' && data.outlet_id && data.outlet_name) {
				console.log(`NEW NODE: ${data.outlet_name}`);
				// Handler function for when user taps 'OK' on new outlet notification.
				var onNewOutletNameChosen = (newName) => {
					console.log('OK Pressed');
					OutletActions.updateOutletName(data.outlet_id, newName)
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
					data.outlet_name // Default value for text field
					);
			} else if (data.type === 'LOSTNODE' && data.outlet_id && data.outlet_name) {
				console.log(`LOST NODE: ${data.outlet_name}`);
				AlertIOS.alert(`Lost Connection to outlet: ${data.outlet_name}`);
			} else {
				console.error(`unrecognized socket message: ${data}`);
			}
		}

		this.ws.onerror = (err) => {
			console.error(err);
		}

		this.ws.onclose = (data) => {
			console.log('Websocket connection closed.');
		}
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
