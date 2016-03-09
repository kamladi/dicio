var mongoose = require('mongoose');

const DB_URL =
    process.env.MONGOLAB_URI ||
    process.env.MONGOHQ_URL ||
    'mongodb://127.0.0.1:27017/diccio';

exports.DB_URL = DB_URL;

exports.connect = () => {
	mongoose.connect(DB_URL);
	mongoose.connection.on('error', function () {
	  console.error('MongoDB Connection Error. Please make sure that MongoDB is running.');
	  process.exit(1);
	});
};
