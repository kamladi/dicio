MongoClient = require('mongodb').MongoClient;

MongoClient.connect('mongodb://127.0.0.1:27017/dicio', (err,db) => {
	if (err) console.error(err);

	collection = db.collection('sensor_records');

	start = new Date();
	NUM_ITEMS = 500;

	function addSensorData(counter, cb) {
		start.setSeconds(start.getSeconds()+1);
		collection.insert({
			mac_address: Math.floor(Math.random() * 4).toString(10),
			timestamp: start,
			cur_temperature: Math.floor(Math.random() * 256),
			cur_humidity: Math.floor(Math.random() * 256),
			cur_light: Math.floor(Math.random() * 256),
			cur_power: Math.floor(Math.random() * 256),
		}, (err, docs) => {
			if (err) {
				console.error(err);
			}
			if (counter < NUM_ITEMS) {
				console.log("Added ", counter);
				addSensorData(counter + 1, cb);
			} else {
				cb(counter);
			}
		});
	}

	addSensorData(0, (counter) => {
		collection.count((err,count) => {
			if (err) console.error(err);
			console.log("Successfully added ", count, " items.");
			process.exit(0);
		});
	});

});
