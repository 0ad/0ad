GameSettingControls.GameSpeed = class GameSpeed extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.previousAllowFastForward = undefined;

		g_GameSettings.gameSpeed.watch(() => this.render(), ["gameSpeed"]);
		this.render();
	}

	onPlayerAssignmentsChange()
	{
		this.render();
	}

	render()
	{
		let allowFastForward = true;
		for (let guid in g_PlayerAssignments)
			if (g_PlayerAssignments[guid].player != -1)
			{
				allowFastForward = false;
				break;
			}

		if (this.previousAllowFastForward !== allowFastForward)
		{
			this.previousAllowFastForward = allowFastForward;
			let values = prepareForDropdown(
				g_Settings.GameSpeeds.filter(speed => !speed.FastForward || allowFastForward));

			this.dropdown.list = values.Title;
			this.dropdown.list_data = values.Speed;
		}

		this.setSelectedValue(g_GameSettings.gameSpeed.gameSpeed);
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.gameSpeed.setSpeed(this.dropdown.list_data[itemIdx]);
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.GameSpeed.prototype.TitleCaption =
	translate("Game Speed");

GameSettingControls.GameSpeed.prototype.Tooltip =
	translate("Select game speed.");
