class HotkeysPage
{
	constructor()
	{
		Engine.GetGUIObjectByName("hotkeyList").onMouseLeftDoubleClickItem = () => {
			let idx = Engine.GetGUIObjectByName("hotkeyList").selected;
			let picker = new HotkeyPicker(
				this.onHotkeyPicked.bind(this),
				Engine.GetGUIObjectByName("hotkeyList").list_data[idx],
				this.hotkeys[Engine.GetGUIObjectByName("hotkeyList").list_data[idx]]
			);
		};
		Engine.GetGUIObjectByName("hotkeyFilter").onSelectionChange = () => this.setupHotkeyList();

		Engine.GetGUIObjectByName("hotkeyTextFilter").onTextEdit = () => this.setupHotkeyList();

		Engine.GetGUIObjectByName("hotkeyClose").onPress = () => Engine.PopGuiPage();
		Engine.GetGUIObjectByName("hotkeyReset").onPress = () => this.resetUserHotkeys();
		Engine.GetGUIObjectByName("hotkeySave").onPress = () => {
			this.saveUserHotkeys();
		};

		this.setupHotkeyData();
		this.setupFilters();
		this.setupHotkeyList();
	}

	setupFilters()
	{
		let dropdown = Engine.GetGUIObjectByName("hotkeyFilter");
		let names = [];
		for (let cat in this.categories)
			names.push(this.categories[cat].label);
		dropdown.list = [translate("All Hotkeys")].concat(names);
		dropdown.list_data = [-1].concat(Object.keys(this.categories));
		dropdown.selected = 0;
	}

	setupHotkeyList()
	{
		let hotkeyList = Engine.GetGUIObjectByName("hotkeyList");
		hotkeyList.selected = -1;
		let textFilter = Engine.GetGUIObjectByName("hotkeyTextFilter").caption;
		let dropdown = Engine.GetGUIObjectByName("hotkeyFilter");
		if (dropdown.selected && dropdown.selected !== 0)
		{
			let category = this.categories[dropdown.list_data[dropdown.selected]];
			// This is inefficient but it seems fast enough.
			let hotkeys = category.hotkeys.filter(x => translate(x[0]).indexOf(textFilter) !== -1);
			hotkeyList.list_name = hotkeys.map(x => translate(x[0]));
			hotkeyList.list_mapping = hotkeys.map(x => formatHotkeyCombinations(x[1]));
			hotkeyList.list = hotkeys.map(() => 0);
			hotkeyList.list_data = hotkeys.map(x => x[0]);
		}
		else
		{
			// TODO SM62+ : refactor using flat()
			let flattened = [];
			for (let cat in this.categories)
				flattened = flattened.concat(this.categories[cat].hotkeys);
			flattened = flattened.filter(x => translate(x[0]).indexOf(textFilter) !== -1);
			hotkeyList.list_name = flattened.map(x => translate(x[0]));
			hotkeyList.list_mapping = flattened.map(x => formatHotkeyCombinations(x[1]));
			hotkeyList.list = flattened.map(() => 0);
			hotkeyList.list_data = flattened.map(x => x[0]);
		}
	}

	onHotkeyPicked(picker, success)
	{
		picker.close();
		if (!success)
			return;

		// Remove empty combinations which the picker added.
		picker.combinations = picker.combinations.filter(x => x.length);

		this.hotkeys[picker.name] = picker.combinations;
		// Have to find the correct line.
		let panel = Engine.GetGUIObjectByName("hotkeyList");
		for (let cat in this.categories)
		{
			let idx = this.categories[cat].hotkeys.findIndex(([name, _]) => name == picker.name);
			if (idx === -1)
				continue;
			this.categories[cat].hotkeys[idx][1] = picker.combinations;
		}

		this.setupHotkeyList();
	}

	setupHotkeyData()
	{
		let hotkeydata = Engine.GetHotkeyMap();
		this.hotkeys = hotkeydata;

		let categories = {
			"other": {
				"label": translate("Other hotkeys"),
				"hotkeys": []
			}
		};
		let n_categories = 1;
		for (let hotkeyName in this.hotkeys)
		{
			let category = "other";
			let firstdot = hotkeyName.indexOf('.');
			if (firstdot !== -1)
				category = hotkeyName.substr(0, firstdot);
			if (!(category in categories))
			{
				if (n_categories > 18)
					category = "other";
				categories[category] = {
					"label": category,
					"hotkeys": []
				};
			}
			categories[category].hotkeys.push([hotkeyName, this.hotkeys[hotkeyName]]);
		}
		// Remove categories that are too small to deserve a tab.
		for (let cat of Object.keys(categories))
			if (categories[cat].hotkeys.length < 3)
			{
				categories.other.hotkeys = categories.other.hotkeys.concat(categories[cat].hotkeys);
				delete categories[cat];
			}
		for (let cat in categories)
			categories[cat].hotkeys = categories[cat].hotkeys.sort();

		this.categories = categories;
	}

	resetUserHotkeys()
	{
		messageBox(
			400, 200,
			translate("Reset all hotkeys to default values?"),
			translate("Confirmation"),
			[translate("No"), translate("Yes")],
			[
				() => {},
				() => {
					for (let cat in this.categories)
						this.categories[cat].hotkeys.forEach(([name, _]) => {
							Engine.ConfigDB_RemoveValue("user", "hotkey." + name);
						});
					Engine.ConfigDB_WriteFile("user", "config/user.cfg");
					Engine.ReloadHotkeys();
					this.setupHotkeyData();
					this.setupHotkeyList();
				}
			]);
	}

	saveUserHotkeys()
	{
		for (let hotkey in this.hotkeys)
			Engine.ConfigDB_RemoveValue("user", "hotkey." + hotkey);
		Engine.ReloadHotkeys();
		let defaultData = Engine.GetHotkeyMap();
		for (let hotkey in this.hotkeys)
		{
			let keymap = formatHotkeyCombinations(this.hotkeys[hotkey], false);
			if (keymap.join("") !== formatHotkeyCombinations(defaultData[hotkey], false).join(""))
				Engine.ConfigDB_CreateValues("user", "hotkey." + hotkey, keymap);
		}
		Engine.ConfigDB_WriteFile("user", "config/user.cfg");
		Engine.ReloadHotkeys();
	}
}


function init(data)
{
	let hotkeyPage = new HotkeysPage(data);
}
