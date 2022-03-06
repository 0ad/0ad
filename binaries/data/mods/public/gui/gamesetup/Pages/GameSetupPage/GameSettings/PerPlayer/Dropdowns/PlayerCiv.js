PlayerSettingControls.PlayerCiv = class PlayerCiv extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		g_GameSettings.playerCiv.watch(() => this.rebuild(), ["values", "locked"]);

		this.items = this.getItems(false);
		this.allItems = this.getItems(true);
		this.wasLocked = undefined;
		this.values = prepareForDropdown(this.items);

		this.rebuild();
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

	rebuild()
	{
		const isLocked = g_GameSettings.playerCiv.locked[this.playerIndex];
		if (this.wasLocked !== isLocked)
		{
			this.wasLocked = isLocked;
			this.values = prepareForDropdown(isLocked ? this.allItems : this.items);

			this.dropdown.list = this.values.name;
			this.dropdown.list_data = this.values.civ;

			// If not locked, reset selection, else we can end up with empty label.
			if (!isLocked && g_IsController && g_GameSettings.playerCiv.values[this.playerIndex])
				this.onSelectionChange(0);
		}
		this.render();
	}

	render()
	{
		this.setEnabled(!g_GameSettings.playerCiv.locked[this.playerIndex]);
		this.setSelectedValue(g_GameSettings.playerCiv.values[this.playerIndex]);
	}

	getItems(allItems)
	{
		let values = [];

		for (let civ in g_CivData)
			if (allItems || g_CivData[civ].SelectableInGameSetup)
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
