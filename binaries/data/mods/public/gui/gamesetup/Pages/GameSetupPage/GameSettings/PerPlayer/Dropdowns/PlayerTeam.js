PlayerSettingControls.PlayerTeam = class PlayerTeam extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = prepareForDropdown([
			{
				"label": this.NoTeam,
				"id": this.NoTeamId
			},
			...Array.from(
				new Array(g_MaxTeams),
				(v, i) => ({
					"label": i + 1,
					"id": i
				}))
		]);

		this.dropdown.list = this.values.label;
		this.dropdown.list_data = this.values.id;

		g_GameSettings.playerTeam.watch(() => this.render(), ["values", "locked"]);
		this.render();
	}

	setControl()
	{
		this.label = Engine.GetGUIObjectByName("playerTeamText[" + this.playerIndex + "]");
		this.dropdown = Engine.GetGUIObjectByName("playerTeam[" + this.playerIndex + "]");
	}

	render()
	{
		this.setEnabled(g_GameSettings.map.type != "scenario");
		this.setSelectedValue(g_GameSettings.playerTeam.values[this.playerIndex]);
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.playerTeam.setValue(this.playerIndex, itemIdx - 1);
		this.gameSettingsControl.setNetworkInitAttributes();
	}
};

PlayerSettingControls.PlayerTeam.prototype.Tooltip =
	translate("Select player's team.");

PlayerSettingControls.PlayerTeam.prototype.NoTeam =
	translateWithContext("team", "None");

PlayerSettingControls.PlayerTeam.prototype.NoTeamId = -1;
