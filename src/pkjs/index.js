var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);
clay.registerComponent(require('./ordered-list'));

Pebble.addEventListener('showConfiguration', function() {
	const claySettings = JSON.parse(localStorage.getItem('clay-settings') || '{}');
	claySettings.appv = localStorage.getItem("appv") || 0;
	localStorage.setItem('clay-settings', JSON.stringify(claySettings));

	clay.meta.userData = { "settings": claySettings };
	const url = clay.generateUrl();
	Pebble.openURL(url);
	localStorage.setItem('appv', 2);
});

Pebble.addEventListener("webviewclosed", function(e) {
	if (e && !e.response) { return; }
	const claySettings = JSON.parse(localStorage.getItem('clay-settings') || '{}');
	claySettings.appv = localStorage.getItem("appv") || 0;
	localStorage.setItem('clay-settings', JSON.stringify(claySettings));
});

