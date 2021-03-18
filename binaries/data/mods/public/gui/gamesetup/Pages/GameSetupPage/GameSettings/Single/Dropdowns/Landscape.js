GameSettingControls.Landscape = class Landscape extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = undefined;
		g_GameSettings.landscape.watch(() => this.render(), ["value", "data"]);
		this.render();
	}

	onHoverChange()
	{
		this.dropdown.tooltip = this.values.Description[this.dropdown.hovered] || this.Tooltip;
	}

	render()
	{
		this.setHidden(!g_GameSettings.landscape.data);
		if (!g_GameSettings.landscape.data)
			return;

		let randomItems = [{
			"Id": "random",
			"Name": setStringTags(translateWithContext("landscape selection", "Random"), this.RandomItemTags),
			"Description": translateWithContext("landscape selection", "Select a random landscape.")
		}];
		let data = g_GameSettings.landscape.data;
		let items = [];
		for (let group of data)
		{
			let itemTag = this.translateItem(group);
			itemTag.Name = setStringTags(itemTag.Name, this.RandomItemTags);
			randomItems.push(itemTag);
			let sort = (item1, item2) => item1.Name > item2.Name;
			items = items.concat(group.Items.map(this.translateItem).sort(sort));
		}

		this.values = prepareForDropdown(randomItems.concat(items));

		this.dropdown.list = this.values.Name;
		this.dropdown.list_data = this.values.Id;

		this.setSelectedValue(g_GameSettings.landscape.value);
	}

	translateItem(item)
	{
		return {
			"Id": item.Id,
			"Name": translate(item.Name),
			"Description": translate(item.Description),
			"Preview": item.Preview
		};
	}

	getAutocompleteEntries()
	{
		if (!this.values)
			return undefined;

		let entries = [];
		for (let i = 0; i < this.values.Id.length; ++i)
			if (!this.values.Id[i].startsWith("random"))
				entries.push(this.values.Name[i]);

		return entries;
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.landscape.setValue(this.values.Id[itemIdx]);
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.Landscape.prototype.TitleCaption =
	translate("Landscape");

GameSettingControls.Landscape.prototype.Tooltip =
	translate("Select one of the landscapes of this map.");

GameSettingControls.Landscape.prototype.AutocompleteOrder = 0;
