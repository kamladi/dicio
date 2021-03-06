var bodyParser       = require('body-parser');
var eventsCtrl       = require('./controllers/Events');
var express          = require('express');
var expressValidator = require('express-validator');
var Gateway					 = require('./Gateway');
var groupsCtrl       = require('./controllers/Groups');
var logger           = require('morgan');
var ObjectId         = require('mongoose').Types.ObjectId;
var outletsCtrl      = require('./controllers/Outlets');
var path             = require('path');
var eventsCtrl       = require('./controllers/Events');
var timeSeriesCtrl   = require('./controllers/TimeSeries');

var app = express();

// Config settings
app.set('json spaces', 4);
app.use(logger('dev'));
app.use((req, res, next)  =>{
  res.header("Access-Control-Allow-Origin", "*");
  res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  next();
});
app.use(express.static(path.join(__dirname, 'public')));
// use if sending json data
app.use(bodyParser.json());
// use if sending url-encoded form data (i.e. from web page)
app.use(bodyParser.urlencoded({ extended: false }));
app.use(expressValidator({
	customValidators: {
		isObjectId: (value) => {
			return ObjectId.isValid(value);
		}
	}
}));

// Application Routes
app.get('/outlets/', outletsCtrl.getOutlets);
app.get('/outlets/clear', outletsCtrl.clearOutlets);
app.get('/outlets/:id/:action(on|off)', outletsCtrl.sendOutletAction);
app.get('/outlets/:id', outletsCtrl.getOutletDetails);
app.post('/outlets/:id', outletsCtrl.updateOutlet);
app.get('/events/', eventsCtrl.getEvents);
app.post('/events/', eventsCtrl.createEvent);
app.get('/events/clear', eventsCtrl.clearEvents);
app.get('/events/:id', eventsCtrl.getEventDetails);
app.post('/events/:id', eventsCtrl.updateEvent);
app.delete('/events/:id', eventsCtrl.deleteEvent);
app.get('/groups/', groupsCtrl.getGroups);
app.post('/groups/', groupsCtrl.createGroup);
app.get('/groups/clear', groupsCtrl.clearGroups);
app.get('/groups/:id/:action(on|off)', groupsCtrl.sendGroupAction);
app.get('/groups/:id', groupsCtrl.getGroupDetails);
app.post('/groups/:id', groupsCtrl.updateGroup);
app.delete('/groups/:id', groupsCtrl.deleteGroup);
app.get('/handlepacket/:packet', (req, res, next) => {
	return Gateway.handleData(req.params.packet)
		.then(result => res.json(result))
		.catch(next);
});
app.get('/graphs/:id', timeSeriesCtrl.getSensorHistory);

// Undefined Route Handler
// (request url doesn't match any routes)
app.use( (req, res, next) => {
	var err = new Error('Not Found');
	err.status = 404;
	next(err);
});

// Error Handler (for any propagated errors)
app.use( (err, req, res) => {
	res.status(err.status || 500);
	res.render('error', {
		message: err.message,
		error: err
	});
});

module.exports = app;
