class HotkeysPage
{
	constructor(metadata)
	{
		this.metadata = metadata;

		Engine.GetGUIObjectByName("hotkeyList").onMouseLeftDoubleClickItem = () => {
			let idx = Engine.GetGUIObjectByName("hotkeyList").selected;
			let picker = new HotkeyPicker(
				this.metadata,
				this.onHotkeyPicked.bind(this),
				Engine.GetGUIObjectByName("hotkeyList").list_data[idx],
				clone(this.hotkeys[Engine.GetGUIObjectByName("hotkeyList").list_data[idx]])
			);
		};
		Engine.GetGUIObjectByName("hotkeyList").onHoverChange = () => this.onHotkeyHover();

		Engine.GetGUIObjectByName("hotkeyFilter").onSelectionChange = () => this.setupHotkeyList();
		Engine.GetGUIObjectByName("hotkeyFilter").onHoverChange = () => this.onFilterHover();

		Engine.GetGUIObjectByName("hotkeyTextFilter").onTextEdit = () => this.setupHotkeyList();

		this.saveButton = Engine.GetGUIObjectByName("hotkeySave");
		this.saveButton.enabled = false;

		Engine.GetGUIObjectByName("hotkeyClose").onPress = () => Engine.PopGuiPage();
		Engine.GetGUIObjectByName("hotkeyReset").onPress = () => this.resetUserHotkeys();
		this.saveButton.onPress = () => {
			this.saveUserHotkeys();
			this.saveButton.enabled = false;
		};

		this.setupHotkeyData();
		this.setupFilters();
		this.setupHotkeyList();
	}

	setupHotkeyData()
	{
		let hotkeydata = Engine.GetHotkeyMap();
		this.hotkeys = hotkeydata;
		let categories = clone(this.metadata.categories);
		for (let name in categories)
			categories[name].hotkeys = [];
		for (let hotkeyName in this.hotkeys)
		{
			if (this.metadata.hotkeys[hotkeyName])
				for (let cat of this.metadata.hotkeys[hotkeyName].categories)
					categories[cat].hotkeys.push(hotkeyName);
			else
				categories[this.metadata.DEFAULT_CATEGORY].hotkeys.push(hotkeyName);
		}
		for (let cat in categories)
			categories[cat].hotkeys.sort((a, b) => {
				if (!this.metadata.hotkeys[a] || !this.metadata.hotkeys[b])
					return !this.metadata.hotkeys[a] ? 1 : -1;
				return this.metadata.hotkeys[a].order - this.metadata.hotkeys[b].order;
			});
		for (let cat in categories)
			if (categories[cat].hotkeys.length === 0)
				delete categories[cat];
		this.categories = categories;
	}

	setupFilters()
	{
		let dropdown = Engine.GetGUIObjectByName("hotkeyFilter");
		let names = [];
		for (let cat in this.categories)
			names.push(translateWithContext("hotkey metadata", this.categories[cat].name));
		dropdown.list = [translate("All Hotkeys")].concat(names);
		dropdown.list_data = [-1].concat(Object.keys(this.categories));
		dropdown.selected = 0;
	}

	setupHotkeyList()
	{
		let hotkeyList = Engine.GetGUIObjectByName("hotkeyList");
		hotkeyList.selected = -1;
		let textFilter = Engine.GetGUIObjectByName("hotkeyTextFilter").caption.toLowerCase();

		let hotkeys;
		let dropdown = Engine.GetGUIObjectByName("hotkeyFilter");
		if (dropdown.selected && dropdown.selected !== 0)
			hotkeys = this.categories[dropdown.list_data[dropdown.selected]].hotkeys;
		else
			hotkeys = Object.values(this.categories).map(x => x.hotkeys).flat();
		hotkeys = hotkeys.filter(x => {
			return x.indexOf(textFilter) !== -1 ||
				translateWithContext("hotkey metadata", this.metadata.hotkeys[x]?.name || x).toLowerCase().indexOf(textFilter) !== -1;
		});

		hotkeyList.list_name = hotkeys.map(x => translateWithContext("hotkey metadata", this.metadata.hotkeys[x]?.name || x));
		hotkeyList.list_mapping = hotkeys.map(x => formatHotkeyCombinations(this.hotkeys[x]));
		hotkeyList.list = hotkeys.map(() => 0);
		hotkeyList.list_data = hotkeys.map(x => x);
	}

	onFilterHover()
	{
		let dropdown = Engine.GetGUIObjectByName("hotkeyFilter");
		if (dropdown.hovered === -1)
			dropdown.tooltip = "";
		else if (dropdown.hovered === 0)
			dropdown.tooltip = translate("All available hotkeys.");
		else
			dropdown.tooltip = translateWithContext("hotkey metadata", this.categories[dropdown.list_data[dropdown.hovered]].desc);
	}

	onHotkeyHover()
	{
		let hotkeyList = Engine.GetGUIObjectByName("hotkeyList");
		if (hotkeyList.hovered === -1)
			hotkeyList.tooltip = "";
		else
		{
			let hotkey = hotkeyList.list_data[hotkeyList.hovered];
			hotkeyList.tooltip = this.metadata.hotkeys[hotkey]?.desc ?
						translateWithContext("hotkey metadata", this.metadata.hotkeys[hotkey]?.desc) :
						translate(markForTranslation("No tooltip available"));
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

		this.saveButton.enabled = true;
		this.setupHotkeyList();
	}

	resetUserHotkeys()
	{
		messageBox(
			400, 200,
			translate("Reset all hotkeys to default values?\nWARNING: this cannot be reversed."),
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
					this.saveButton.enabled = false;
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


function init()
{
	let hotkeyPage = new HotkeysPage(new HotkeyMetadata());
}
