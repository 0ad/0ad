GameSettingControls.PlayerCount = class PlayerCount extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = Array.from(
			new Array(g_MaxPlayers),
			(v, i) => i + 1);

		this.dropdown.list = this.values;
		this.dropdown.list_data = this.values;

		g_GameSettings.playerCount.watch(() => this.render(), ["nbPlayers"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	render()
	{
		this.setEnabled(g_GameSettings.map.type == "random");
		this.setSelectedValue(g_GameSettings.playerCount.nbPlayers);
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.playerCount.setNb(this.values[itemIdx]);
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.PlayerCount.prototype.TitleCaption =
	translate("Number of Players");

GameSettingControls.PlayerCount.prototype.Tooltip =
	translate("Select number of players.");
