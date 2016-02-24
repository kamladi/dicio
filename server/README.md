## Installing Dependencies
1. `brew install node mongodb` to install node.js and mongodb
2. `npm install -d` to install node module dependencies (make sure you're in the server/ directory)

## Running the Server
1. In one terminal tab, run `mongod`. This will start the local MongoDB database. Ensure the database is listening on port `27017`
2. In another terminal tab, run `node index.js $SERIAL_PORT`, where `$SERIAL_PORT` is the device port for the connected gateway node (i.e. `/dev/tty.USB0`).
3. If you want the server to restart when you save changes, run `npm install -g nodemon` and run `nodemon index.js $SERIAL_PORT`.
