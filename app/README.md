## Setup
1. Install global react native command line tools with `npm install -g react-native-cli`.
If you get a "Permission Denied" error, run with sudo.
2. Install app dependencies with `npm install`. This will install all dependencies listed in `package.json`.

## Running the app
1. Run `open ios/dicio_ios.xcodeproj` to open the ios project in Xcode.
2. Click the Run button ('play button icon') to open the iOS simulator and run the app.

# Developing
React native supportshot code reloading as you develop. To edit the app,
open any of the .js files in your editor of choice. When you save the file,
the app should automatically update in the simulator. If it doesn't update, hitting `Cmd+R`
should force a refresh.

The App debugger runs on a Chrome window (this is where console.log statements and errors will go).
To enable debugging, hit `Cmd+Ctrl+Z` in the simulator (shortcut for "shaking" the device). This will bring up
a popup menu. Selecting 'Debug in Chrome' will open a Chrome window, where you can open the console by hitting
'Cmd+Option+J'.
