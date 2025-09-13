var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);
clay.registerComponent(require('./ordered-list'));

Pebble.addEventListener('showConfiguration', function() {
	var claySettings = JSON.parse(localStorage.getItem('clay-settings') || '{}');

	clay.meta.userData = { "settings": claySettings };
	var url = clay.generateUrl();
	Pebble.openURL(url);
});

