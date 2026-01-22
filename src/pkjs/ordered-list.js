module.exports = {
	name: 'orderedList',
	template: `
<div class="component component-ol">
	<label class="tap-highlight">
		<input data-manipulator-target type="hidden" />
		<span class="label">{{{label}}}</span>
		<span class="value"></span>
	</label>
	{{if description}}
	<div class="description">{{{description}}}</div>
	{{/if}}
	<ol id="list"></ol>
</div>`,
	style: `
.item-text {
	width: 100%;
  white-space: pre;
  overflow-wrap: normal;
  overflow-x: scroll;
}

.button-container {
	margin-top: 2px;
	margin-bottom: 10px;
	display: flex;
	gap: 4px;
}

.square {
	background: gray;
	border-radius: 5px;
	width: 40px;
	height: 40px;
	display: flex;
  align-items: center;
  justify-content: center;
}

.square > div {
	font-size: 24px;
}

.spacer {
	flex: max-content;
}
	`,
	manipulator: 'val',
	defaults: {
		label: '',
		description: ''
	},
	initialize: function(_, clay) {
		function migrateSettings(self, clay) {
			let strings = self.config.defaultValue || [];
			if (clay.meta.userData.settings === undefined) {
				clay.meta.userData.settings = claySettings;
				if (!clay.meta.userData.settings) {
					return strings;
				}
			}
			switch (parseInt(clay.meta.userData.settings["appv"])) {
				case 0:
					if (clay.meta.userData.settings && clay.meta.userData.settings[self.config.messageKey]) {
						strings = clay.meta.userData.settings[self.config.messageKey].split("\0").flatMap(x => [x, ""]);
					}
					break;
				case 2:
					if (clay.meta.userData.settings && clay.meta.userData.settings[self.config.messageKey]) {
						strings = clay.meta.userData.settings[self.config.messageKey].split("\0");
					}
					break;
			}
			return strings;
		}

		try {
			const self = this;
			let $elem = self.$element;
			let $list = $elem[0].querySelector('#list');

			let strings = migrateSettings(self, clay);

			function updateValue() {
				const values = Array.from($elem[0].querySelectorAll(".item-text")).map(el => el.value);
				const labels = Array.from($elem[0].querySelectorAll(".button-container")).map(el => el.querySelector(".label").value);
				const strs = [];
				for (let i = 0; i < values.length; i++) {
					if (values[i] !== "") {
						strs.push(values[i]);
						if (labels[i] !== "") {
							strs.push(labels[i]);
						}
						else {
							strs.push(" ");
						}
					}
				}
				const val = strs.join("\0");
				self.set(val);
			}

			function createListItem(text = '', label = '') {
				const li = document.createElement('li');
				li.innerHTML = `
<textarea class="item-text">${text}</textarea>
<div class="button-container">
	<input type="text" class="label" value="${label}" placeholder="QR Label"></input>
	<div class="square move-down">
		<div>▼</div>
	</div>
	<div class="square move-up">
		<div>▲</div>
	</div>
	<div class="square remove">
		<div>✖</div>
	</div>
</div>
`;
				const getHeight = t => {
					return `${Math.max(1.25, t.split("\n").length) * 20}px`;
				}
				const itm = li.querySelector('.item-text');
				itm.style.height = getHeight(text);
				itm.style.minHeight = getHeight(text);
				li.querySelector('.item-text').addEventListener('input', evt => {
					evt.target.style.height = getHeight(evt.target.value);
					evt.target.style.minHeight = getHeight(evt.target.value);
					updateValue();
				});

				li.querySelector('.label').addEventListener('input', evt => {
					evt.target.value = evt.target.value.substring(0, 18);
					updateValue();
				});

				li.querySelector('.remove').addEventListener('click', () => {
					li.remove();
					updateValue();
				});

				li.querySelector('.move-up').addEventListener('click', () => {
					const prev = li.previousElementSibling;
					if (prev) li.parentNode.insertBefore(li, prev);
					updateValue();
				});

				li.querySelector('.move-down').addEventListener('click', () => {
					const next = li.nextElementSibling;
					if (next) li.parentNode.insertBefore(next, li);
					updateValue();
				});

				if (text !== "") {
					updateValue();
				}

				return li;
			}

			if (strings.length === 0) {
				$list.appendChild(createListItem());
			}
			else {
				for (let i = 0; i < strings.length; i += 2) {
					$list.appendChild(createListItem(strings[i], strings[i + 1]));
				}
			}

			const add = document.createElement("button");
			add.type = "button";
			add.innerHTML = "✚";
			add.addEventListener('click', () => {
				$list.appendChild(createListItem(""));
			});

			$elem[0].appendChild(add);
		}
		catch (err) {
			alert("ERROR: " + err);
		}
	}
};
