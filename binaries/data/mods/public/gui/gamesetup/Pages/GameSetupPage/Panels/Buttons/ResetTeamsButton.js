class ResetTeamsButton
{
	constructor(setupWindow)
	{
		this.gameSettingsControl = setupWindow.controls.gameSettingsControl;
		this.gameSettingsControl.registerGameAttributesBatchChangeHandler(this.onGameAttributesBatchChange.bind(this));

		this.teamResetButton = Engine.GetGUIObjectByName("teamResetButton");
		this.teamResetButton.tooltip = this.Tooltip;
		this.teamResetButton.onPress = this.onPress.bind(this);
	}

	onGameAttributesBatchChange()
	{
		if (!g_GameAttributes.mapType)
			return;

		this.teamResetButton.hidden = g_GameAttributes.mapType == "scenario" || !g_IsController;
	}

	onPress()
	{
		if (!g_GameAttributes.settings || !g_GameAttributes.settings.PlayerData)
			return;

		for (let pData of g_GameAttributes.settings.PlayerData)
			pData.Team = -1;

		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
}

ResetTeamsButton.prototype.Tooltip =
	translate("Reset all teams to the default.");
