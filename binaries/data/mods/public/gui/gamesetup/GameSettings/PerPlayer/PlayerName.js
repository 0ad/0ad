// TODO: There should be an indication which player is not ready yet
// The color does not indicate it's meaning and is insufficient to inform many players.
PlayerSettingControls.PlayerName = class extends GameSettingControl
{
	constructor(...args)
	{
		super(...args);

		this.playerName = Engine.GetGUIObjectByName("playerName[" + this.playerIndex + "]");

		this.displayedName = undefined;
		this.guid = undefined;
	}

	onMapChange(mapData)
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;

		let mapPData = this.gameSettingsControl.getPlayerData(mapData, this.playerIndex);
		pData.Name = mapPData && mapPData.Name || g_Settings.PlayerDefaults[this.playerIndex + 1].Name;
		this.gameSettingsControl.updateGameAttributes();
	}

	onGameAttributesChange()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;

		if (!pData.Name)
		{
			pData.Name = g_Settings.PlayerDefaults[this.playerIndex + 1].Name;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;

		this.displayedName = g_IsNetworked ? pData.Name : translate(pData.Name);
		this.rebuild();
	}

	onPlayerAssignmentsChange()
	{
		this.guid = undefined;

		for (let guid in g_PlayerAssignments)
			if (g_PlayerAssignments[guid].player == this.playerIndex + 1)
			{
				this.guid = guid;
				break;
			}

		this.rebuild();
	}

	rebuild()
	{
		let name = this.displayedName;
		if (!name)
			return;

		if (g_IsNetworked)
		{
			let status = this.guid ? g_PlayerAssignments[this.guid].status : this.ReadyTags.length - 1;
			name = setStringTags(this.displayedName, this.ReadyTags[status]);
		}

		this.playerName.caption = name;
	}

	onGameAttributesFinalize()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;

		if (g_GameAttributes.mapType != "scenario" && pData.AI)
		{
			// Pick one of the available botnames for the chosen civ
			// Determine botnames
			let chosenName = pickRandom(g_CivData[pData.Civ].AINames);

			if (!g_IsNetworked)
				chosenName = translate(chosenName);

			// Count how many players use the chosenName
			let usedName = g_GameAttributes.settings.PlayerData.filter(otherPData =>
				otherPData.Name && otherPData.Name.indexOf(chosenName) !== -1).length;

			pData.Name =
				usedName ?
					sprintf(this.RomanLabel, {
						"playerName": chosenName,
						"romanNumber": this.RomanNumbers[usedName + 1]
					}) :
					chosenName;
		}
		else
			// Copy client playernames so they appear in replays
			for (let guid in g_PlayerAssignments)
				if (g_PlayerAssignments[guid].player == this.playerIndex + 1)
					pData.Name = g_PlayerAssignments[guid].name;
	}
};

PlayerSettingControls.PlayerName.prototype.RomanLabel =
	translate("%(playerName)s %(romanNumber)s");

PlayerSettingControls.PlayerName.prototype.RomanNumbers =
	[undefined, "I", "II", "III", "IV", "V", "VI", "VII", "VIII"];

PlayerSettingControls.PlayerName.prototype.ReadyTags = [
	{
		"color": "white",
	},
	{
		"color": "green",
	},
	{
		"color": "150 150 250",
	}
];
