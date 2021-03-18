class ResetCivsButton
{
	constructor(setupWindow)
	{
		this.gameSettingsControl = setupWindow.controls.gameSettingsControl;

		this.civResetButton = Engine.GetGUIObjectByName("civResetButton");
		this.civResetButton.tooltip = this.Tooltip;
		this.civResetButton.onPress = this.onPress.bind(this);

		g_GameSettings.map.watch(() => this.render(), ["type"]);
	}

	render()
	{
		this.civResetButton.hidden = g_GameSettings.map.type == "scenario" || !g_IsController;
	}

	onPress()
	{
		for (let i = 0; i < g_GameSettings.playerCount.nbPlayers; ++i)
			g_GameSettings.playerCiv.setValue(i, "random");

		this.gameSettingsControl.setNetworkGameAttributes();
	}
}

ResetCivsButton.prototype.Tooltip =
	translate("Reset any civilizations that have been selected to the default (random).");
