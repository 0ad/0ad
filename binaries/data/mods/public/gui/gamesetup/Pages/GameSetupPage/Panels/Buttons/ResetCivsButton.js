class ResetCivsButton
{
	constructor(setupWindow)
	{
		this.gameSettingsControl = setupWindow.controls.gameSettingsControl;
		this.gameSettingsControl.registerGameAttributesBatchChangeHandler(this.onGameAttributesBatchChange.bind(this));

		this.civResetButton = Engine.GetGUIObjectByName("civResetButton");
		this.civResetButton.tooltip = this.Tooltip;
		this.civResetButton.onPress = this.onPress.bind(this);
	}

	onGameAttributesBatchChange()
	{
		if (g_GameAttributes.mapType)
			this.civResetButton.hidden = g_GameAttributes.mapType == "scenario" || !g_IsController;
	}

	onPress()
	{
		if (!g_GameAttributes.settings || !g_GameAttributes.settings.PlayerData)
			return;

		for (let pData of g_GameAttributes.settings.PlayerData)
			pData.Civ = this.RandomCivId;

		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
}

ResetCivsButton.prototype.Tooltip =
	translate("Reset any civilizations that have been selected to the default (random).");

ResetCivsButton.prototype.RandomCivId =
	"random";
