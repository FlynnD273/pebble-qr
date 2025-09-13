
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
input {
	width: 100%;
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
	initialize: function(minified, clay) {
		try {
			let self = this;
			let $elem = self.$element;
			let $list = $elem[0].querySelector('#list');

			let strings = self.config.defaultValue || [];
			if (clay.meta.userData.settings && clay.meta.userData.settings[self.config.messageKey]) {
				strings = clay.meta.userData.settings[self.config.messageKey].split("\0");
			}

			function updateValue() {
				const values = Array.from($elem[0].querySelectorAll(".item-text")).map(el => el.value).filter(x => x !== "");
				const val = values.join("\0");
				self.set(val);
			}

			function createListItem(text = '') {
				const li = document.createElement('li');
				li.innerHTML = `
<input class="item-text" type="text" value="${text}" />
<div class="button-container">
	<div class="spacer"></div>
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
				li.querySelector('.item-text').addEventListener('input', () => {
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
				$list.appendChild(createListItem(""));
			}
			else {
				for (const str of strings) {
					$list.appendChild(createListItem(str));
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
