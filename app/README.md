## Setup
1. Install global react native command line tools with `npm install -g react-native-cli`.
If you get a "Permission Denied" error, run with sudo.
2. Install app dependencies with `npm install`. This will install all dependencies listed in `package.json`.

## Running the app
1. Run `open ios/dicio_ios.xcodeproj` to open the ios project in Xcode.
2. Make sure the backend server and database are running (see instructions in `../server/README.md`.
2. Click the Run button ('play button icon') to open the iOS simulator and run the app.

## Developing
React native supportshot code reloading as you develop. To edit the app,
open any of the .js files in your editor of choice. When you save the file,
the app should automatically update in the simulator. If it doesn't update, hitting `Cmd+R`
should force a refresh.

The App debugger runs on a Chrome window (this is where console.log statements and errors will go).
To enable debugging, hit `Cmd+Ctrl+Z` in the simulator (shortcut for "shaking" the device). This will bring up
a popup menu. Selecting 'Debug in Chrome' will open a Chrome window, where you can open the console by hitting
'Cmd+Option+J'.

## File Structure
- `index.ios.js` is the "main" file for the app. It instantiates the app by rendering the root-level tab bar
(the "Events" and "Outlets" tabs at the bottom of the screen), which everryting else is nested under.
- `src/components` holds the code for all the view components (listviews, detail views, etc.).
- `src/stores` hold the data stores for Outlets and Events. These classes manage the state for all components in the app.
Various components will listen for changes to these data stores and automatically update as necessary.
- `src/actions` hold the possible 'actions' on data in the app. Components will call these functions, which will usually
send HTTP requests the the server, and update the data stores with the data received from the server.
- `src/lib` holds misc. utility functions and global constants.
