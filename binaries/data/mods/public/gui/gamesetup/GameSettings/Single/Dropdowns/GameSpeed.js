GameSettingControls.GameSpeed = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.previousAllowFastForward = undefined;
	}

	onMapChange(mapData)
	{
		let mapValue = mapData && mapData.gameSpeed || undefined;
		if (mapValue !== undefined && mapValue != g_GameAttributes.gameSpeed)
		{
			g_GameAttributes.gameSpeed = mapValue;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onPlayerAssignmentsChange()
	{
		this.update();
	}

	onGameAttributesChange()
	{
		this.update();
	}

	update()
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
			Engine.ProfileStart("updateGameSpeedList");
			this.previousAllowFastForward = allowFastForward;
			this.values = prepareForDropdown(
				g_Settings.GameSpeeds.filter(speed => !speed.FastForward || allowFastForward));

			this.dropdown.list = this.values.Title;
			this.dropdown.list_data = this.values.Speed;
			Engine.ProfileStop();
		}

		if (this.values.Speed.indexOf(g_GameAttributes.gameSpeed || undefined) == -1)
		{
			g_GameAttributes.gameSpeed = this.values.Speed[this.values.Default];
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		this.setSelectedValue(g_GameAttributes.gameSpeed);
	}

	onSelectionChange(itemIdx)
	{
		g_GameAttributes.gameSpeed = this.values.Speed[itemIdx];
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.GameSpeed.prototype.TitleCaption =
	translate("Game Speed");

GameSettingControls.GameSpeed.prototype.Tooltip =
	translate("Select game speed.");
