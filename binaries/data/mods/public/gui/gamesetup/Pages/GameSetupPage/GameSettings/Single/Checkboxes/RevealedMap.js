GameSettingControls.RevealedMap = class RevealedMap extends GameSettingControlCheckbox
{
	constructor(...args)
	{
		super(...args);
		g_GameSettings.mapExploration.watch(() => this.render(), ["revealed"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	render()
	{
		this.setEnabled(g_GameSettings.map.type != "scenario");
		this.setChecked(g_GameSettings.mapExploration.revealed);
	}

	onPress(checked)
	{
		g_GameSettings.mapExploration.setRevealed(checked);
		this.gameSettingsController.setNetworkInitAttributes();
	}
};

GameSettingControls.RevealedMap.prototype.TitleCaption =
	// Translation: Make sure to differentiate between the revealed map and explored map settings!
	translate("Revealed Map");

GameSettingControls.RevealedMap.prototype.Tooltip =
	// Translation: Make sure to differentiate between the revealed map and explored map settings!
	translate("Toggle revealed map (see everything).");
