var mongoose = require('mongoose');
var ObjectId = mongoose.Schema.ObjectId;

var groupSchema = new mongoose.Schema({
	name: {type: String, default: "NEW GROUP"},
	outlets: [{type: ObjectId, ref: 'Outlet'}]
});

module.exports = mongoose.model('Group', groupSchema);
