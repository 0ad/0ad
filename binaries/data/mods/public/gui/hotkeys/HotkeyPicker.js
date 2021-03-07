/**
 * Handle the interface to pick a hotkey combination.
 * The player must keep a key combination for 2s in the input field for it to be registered.
 */
class HotkeyPicker
{
	constructor(metadata, onClose, name, combinations)
	{
		this.metadata = metadata;
		this.name = name;
		this.combinations = combinations;
		this.window = Engine.GetGUIObjectByName("hotkeyPicker");
		this.window.hidden = false;

		this.enteringInput = -1;

		if (this.metadata.hotkeys[name])
		{
			Engine.GetGUIObjectByName("hotkeyPickerTitle").caption = translate(this.metadata.hotkeys[name].name);
			Engine.GetGUIObjectByName("hotkeyPickerDescHotkey").caption = translate(this.metadata.hotkeys[name].desc);
		}
		else
		{
			Engine.GetGUIObjectByName("hotkeyPickerTitle").caption = this.name;
			Engine.GetGUIObjectByName("hotkeyPickerDescHotkey").hidden = true;
		}

		this.setupCombinations();
		this.render();

		Engine.GetGUIObjectByName("hotkeyPickerReset").onPress = () => {
			// This is a bit "using a bazooka to kill a fly"
			Engine.ConfigDB_RemoveValue("user", "hotkey." + this.name);
			Engine.ConfigDB_WriteFile("user", "config/user.cfg");
			Engine.ReloadHotkeys();
			let data = Engine.GetHotkeyMap();
			this.combinations = data[this.name];
			this.setupCombinations();
			this.render();
		};
		Engine.GetGUIObjectByName("hotkeyPickerCancel").onPress = () => {
			onClose(this, false);
		};
		Engine.GetGUIObjectByName("hotkeyPickerSave").onPress = () => {
			onClose(this, true);
		};
	}

	setupCombinations()
	{
		for (let i = 0; i < 4; ++i)
		{
			let s = Engine.GetGUIObjectByName("combination[" + i + "]").size;
			s.top = +i * 60 + 120;
			s.bottom = +i * 60 + 150;
			Engine.GetGUIObjectByName("combination[" + i + "]").size = s;
			Engine.GetGUIObjectByName("combNb[" + i + "]").caption = sprintf(translate("#%i"), i);

			if (i == this.combinations.length)
				this.combinations.push([]);

			let input = Engine.GetGUIObjectByName("combMapping[" + i + "]");

			let picker = Engine.GetGUIObjectByName("picker[" + i + "]");
			Engine.GetGUIObjectByName("combMappingBtn[" + i + "]").onPress = () => {
				this.enteringInput = i;
				picker.focus();
				this.render();
			};

			picker.onKeyChange = keys => {
				input.caption = (keys.length ?
					formatHotkeyCombination(keys) + translate(" (hold to register)") :
					translate("Enter new Hotkey, hold to register."));
			};

			Engine.GetGUIObjectByName("deleteComb[" + i + "]").onPress = (j => () => {
				this.combinations[j] = [];
				this.render();
			})(i);

			picker.onCombination = (j => keys => {
				this.combinations[j] = keys;
				this.enteringInput = -1;
				picker.blur();

				this.render();
			})(i);
		}
	}

	close()
	{
		this.window.hidden = true;
		for (let i = 0; i < 4; ++i)
			Engine.GetGUIObjectByName("picker[" + i + "]").blur();
	}

	render()
	{
		for (let i = 0; i < 4; ++i)
		{
			let input = Engine.GetGUIObjectByName("combMapping[" + i + "]");
			if (i == this.enteringInput)
				input.caption = translate("Enter new Hotkey, hold to register.");
			else
				input.caption = formatHotkeyCombination(this.combinations[i]) || "(unused)";
			Engine.GetGUIObjectByName("conflicts[" + i + "]").caption = "";

			Engine.GetGUIObjectByName("deleteComb[" + i + "]").hidden = !this.combinations[i].length;

			let conflicts = (Engine.GetConflicts(this.combinations[i]) || [])
				.filter(name => name != this.name).map(translate);
			if (conflicts.length)
				Engine.GetGUIObjectByName("conflicts[" + i + "]").caption =
					coloredText(translate("May conflict with: "), "255 153 0") + conflicts.join(", ");
		}
		// Gray out buttons when entering an input so it's obvious clicking on them will do nothing.
		Engine.GetGUIObjectByName("hotkeyPickerReset").enabled = this.enteringInput == -1;
		Engine.GetGUIObjectByName("hotkeyPickerCancel").enabled = this.enteringInput == -1;
		Engine.GetGUIObjectByName("hotkeyPickerSave").enabled = this.enteringInput == -1;
	}
}
