PlayerSettingControls.PlayerCiv = class PlayerCiv extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = prepareForDropdown(this.getItems());

		this.dropdown.list = this.values.name;
		this.dropdown.list_data = this.values.civ;

		g_GameSettings.playerCiv.watch(() => this.render(), ["values", "locked"]);
		this.render();
	}

	setControl()
	{
		this.label = Engine.GetGUIObjectByName("playerCivText[" + this.playerIndex + "]");
		this.dropdown = Engine.GetGUIObjectByName("playerCiv[" + this.playerIndex + "]");
	}

	onHoverChange()
	{
		this.dropdown.tooltip = this.values && this.values.tooltip[this.dropdown.hovered] || this.Tooltip;
	}

	render()
	{
		this.setEnabled(!g_GameSettings.playerCiv.locked[this.playerIndex]);
		this.setSelectedValue(g_GameSettings.playerCiv.values[this.playerIndex]);
	}

	getItems()
	{
		let values = [];

		for (let civ in g_CivData)
			if (g_CivData[civ].SelectableInGameSetup)
				values.push({
					"name": g_CivData[civ].Name,
					"autocomplete": g_CivData[civ].Name,
					"tooltip": g_CivData[civ].History,
					"civ": civ
				});

		values.sort(sortNameIgnoreCase);

		values.unshift({
			"name": setStringTags(this.RandomCivCaption, this.RandomItemTags),
			"autocomplete": this.RandomCivCaption,
			"tooltip": this.RandomCivTooltip,
			"civ": this.RandomCivId
		});

		return values;
	}

	getAutocompleteEntries()
	{
		return this.values.autocomplete;
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.playerCiv.setValue(this.playerIndex, this.values.civ[itemIdx]);
		this.gameSettingsController.setNetworkInitAttributes();
	}
};

PlayerSettingControls.PlayerCiv.prototype.Tooltip =
	translate("Choose the civilization for this player.");

PlayerSettingControls.PlayerCiv.prototype.RandomCivCaption =
	translateWithContext("civilization", "Random");

PlayerSettingControls.PlayerCiv.prototype.RandomCivId =
	"random";

PlayerSettingControls.PlayerCiv.prototype.RandomCivTooltip =
	translate("Picks one civilization at random when the game starts.");

PlayerSettingControls.PlayerCiv.prototype.AutocompleteOrder = 90;
