## Installing Dependencies
1. `brew install node mongodb` to install node.js and mongodb
2. `npm install -d` to install node module dependencies (make sure you're in the server/ directory)

## Running the Server
1. In one terminal tab, run `mongod`. This will start the local MongoDB database. Ensure the database is listening on port `27017`
2. In another terminal tab, run `node index.js $SERIAL_PORT`, where `$SERIAL_PORT` is the device port for the connected gateway node (i.e. `/dev/tty.USB0`).
3. If you want the server to restart when you save changes, run `npm install -g nodemon` and run `nodemon index.js $SERIAL_PORT`.

## Starting the server with sample data
The server is set up to initialize the database with some sample outlets and events so you have something to look at when developing. This sample data is defined in `initial_data.js` and loaded into the database in `index.js`

## File Structure
- `index.js` - 'main file' for the application. Loads the database, the web server, and the serial port connection, and starts everything off.
- `app.js`- configures the web server, defines URL routes
- `serialport.js` - defines API to interface with the gateway over the serial port
- `db.js` - defines API to connect to the mongodb database
- `models/` - defines data models and properties for the database. These are instances of Mongoose classes (Mongoose is a mongodb database library, look up it's API online / read the code so far to see how to use it)
- `controllers/` - defines route handler functions to be called when users send requests to the server.
- `lib/` - a place to save global utility functions
