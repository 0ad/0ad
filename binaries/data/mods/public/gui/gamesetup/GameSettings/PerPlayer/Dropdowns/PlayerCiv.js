PlayerSettingControls.PlayerCiv = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.fixedCiv = undefined;
		this.values = prepareForDropdown(this.getItems());

		this.dropdown.list = this.values.name;
		this.dropdown.list_data = this.values.civ;
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

	onMapChange(mapData)
	{
		let mapPData = this.gameSettingsControl.getPlayerData(mapData, this.playerIndex);
		this.fixedCiv = mapPData && mapPData.Civ || undefined;
	}

	onAssignPlayer(source, target)
	{
		if (g_GameAttributes.mapType != "scenario" && source && target)
			[source.Civ, target.Civ] = [target.Civ, source.Civ];
	}

	onGameAttributesChange()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData || !g_GameAttributes.mapType)
			return;

		if (this.fixedCiv)
		{
			if (!pData.Civ || this.fixedCiv != pData.Civ)
			{
				pData.Civ = this.fixedCiv;
				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else if (this.values.civ.indexOf(pData.Civ || undefined) == -1)
		{
			pData.Civ =
				g_GameAttributes.mapType == "scenario" ?
					g_Settings.PlayerDefaults[this.playerIndex + 1].Civ :
					this.RandomCivId;

			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData || !g_GameAttributes.mapType)
			return;

		this.setEnabled(!this.fixedCiv);
		this.setSelectedValue(pData.Civ);
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
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		pData.Civ = this.values.civ[itemIdx];
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}

	onPickRandomItems()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData || pData.Civ != this.RandomCivId)
			return;

		// Get a unique array of selectable cultures
		let cultures = Object.keys(g_CivData).filter(civ => g_CivData[civ].SelectableInGameSetup).map(civ => g_CivData[civ].Culture);
		cultures = cultures.filter((culture, index) => cultures.indexOf(culture) === index);

		// Pick a random civ of a random culture
		let culture = pickRandom(cultures);
		pData.Civ = pickRandom(Object.keys(g_CivData).filter(civ =>
			g_CivData[civ].Culture == culture && g_CivData[civ].SelectableInGameSetup));

		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.pickRandomItems();
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
