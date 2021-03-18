GameSettingControls.ExploredMap = class ExploredMap extends GameSettingControlCheckbox
{
	constructor(...args)
	{
		super(...args);
		g_GameSettings.mapExploration.watch(() => this.render(), ["explored"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	render()
	{
		this.setEnabled(g_GameSettings.map.type != "scenario");
		this.setChecked(g_GameSettings.mapExploration.explored);
	}

	onPress(checked)
	{
		g_GameSettings.mapExploration.setExplored(checked);
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.ExploredMap.prototype.TitleCaption =
	// Translation: Make sure to differentiate between the revealed map and explored map settings!
	translate("Explored Map");

GameSettingControls.ExploredMap.prototype.Tooltip =
	// Translation: Make sure to differentiate between the revealed map and explored map settings!
	translate("Toggle explored map (see initial map).");
