GameSettingControls.WorldPopulation = class WorldPopulation extends GameSettingControlCheckbox
{
	constructor(...args)
	{
		super(...args);
		g_GameSettings.population.watch(() => this.render(), ["useWorldPop"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	render()
	{
		this.setEnabled(g_GameSettings.map.type != "scenario");
		this.setChecked(g_GameSettings.population.useWorldPop);
	}

	onPress(checked)
	{
		g_GameSettings.population.setPopCap(checked);
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.WorldPopulation.prototype.TitleCaption =
	translate("World population");

GameSettingControls.WorldPopulation.prototype.Tooltip =
	translate("When checked the Population Cap will be evenly distributed over all living players.");
