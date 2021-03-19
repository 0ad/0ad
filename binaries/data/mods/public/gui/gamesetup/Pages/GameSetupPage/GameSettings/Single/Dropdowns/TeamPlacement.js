GameSettingControls.TeamPlacement = class TeamPlacement extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);
		this.values = undefined;

		g_GameSettings.teamPlacement.watch(() => this.render(), ["value", "available"]);
		this.render();
	}

	onHoverChange()
	{
		this.dropdown.tooltip = this.values.Description[this.dropdown.hovered] || this.Tooltip;
	}

	render()
	{
		this.setHidden(!g_GameSettings.teamPlacement.value);
		if (!g_GameSettings.teamPlacement.value)
			return;

		let randomItem = clone(this.RandomItem);
		randomItem.Name = setStringTags(randomItem.Name, this.RandomItemTags);

		let patterns = [randomItem];

		for (let pattern of g_GameSettings.teamPlacement.available)
			patterns.push(g_GameSettings.teamPlacement.StartingPositions
				.find(pObj => pObj.Id == pattern));

		this.values = prepareForDropdown(patterns);

		this.dropdown.list = this.values.Name;
		this.dropdown.list_data = this.values.Id;

		this.setSelectedValue(g_GameSettings.teamPlacement.value);
	}

	getAutocompleteEntries()
	{
		return this.values && this.values.Name.slice(1);
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.teamPlacement.setValue(this.values.Id[itemIdx]);
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.TeamPlacement.prototype.TitleCaption =
	translate("Team Placement");

GameSettingControls.TeamPlacement.prototype.Tooltip =
	translate("Select one of the starting position patterns of this map.");

GameSettingControls.TeamPlacement.prototype.RandomItem = {
	"Id": "random",
	"Name": translateWithContext("team placement", "Random"),
	"Description": translateWithContext("team placement", "Select a random team placement pattern when starting the game.")
};

GameSettingControls.TeamPlacement.prototype.AutocompleteOrder = 0;
