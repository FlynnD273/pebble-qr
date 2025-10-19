var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);
clay.registerComponent(require('./ordered-list'));

Pebble.addEventListener('showConfiguration', function() {
	const claySettings = JSON.parse(localStorage.getItem('clay-settings') || '{}');
	claySettings.appv = localStorage.getItem("appv") || 0;

	clay.meta.userData = { "settings": claySettings };
	const url = clay.generateUrl();
	Pebble.openURL(url);
	localStorage.setItem('appv', 2);
});

