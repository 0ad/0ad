GameSettingControls.TriggerDifficulty = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = undefined;
	}

	onHoverChange()
	{
		this.dropdown.tooltip =
			this.values && this.values.Tooltip[this.dropdown.hovered] ||
			this.Tooltip;
	}

	onMapChange(mapData)
	{
		let available = mapData && mapData.settings && mapData.settings.SupportedTriggerDifficulties || undefined;
		this.setHidden(!available);

		if (available)
		{
			Engine.ProfileStart("updateTriggerDifficultyList");
			let values = g_Settings.TriggerDifficulties;
			if (Array.isArray(available.Values))
				values = values.filter(diff => available.Values.indexOf(diff.Name) != -1);

			this.values = prepareForDropdown(values);

			this.dropdown.list = this.values.Title;
			this.dropdown.list_data = this.values.Difficulty;

			if (mapData.settings.TriggerDifficulty &&
				this.values.Difficulty.indexOf(mapData.settings.TriggerDifficulty) != -1)
				g_GameAttributes.settings.TriggerDifficulty = mapData.settings.TriggerDifficulty;
			Engine.ProfileStop();
		}
		else
		{
			this.values = undefined;
			this.defaultValue = undefined;
		}
	}

	onGameAttributesChange()
	{
		if (!g_GameAttributes.mapType)
			return;

		if (this.values)
		{
			if (this.values.Difficulty.indexOf(g_GameAttributes.settings.TriggerDifficulty || undefined) == -1)
			{
				g_GameAttributes.settings.TriggerDifficulty = this.values.Difficulty[this.values.Default];
				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else if (g_GameAttributes.settings.TriggerDifficulty !== undefined)
		{
			delete g_GameAttributes.settings.TriggerDifficulty;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		if (this.values)
			this.setSelectedValue(g_GameAttributes.settings.TriggerDifficulty);
	}

	onSelectionChange(itemIdx)
	{
		g_GameAttributes.settings.TriggerDifficulty = this.values.Difficulty[itemIdx];
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.TriggerDifficulty.prototype.TitleCaption =
	translate("Difficulty");

GameSettingControls.TriggerDifficulty.prototype.Tooltip =
	translate("Select the difficulty of this scenario.");
