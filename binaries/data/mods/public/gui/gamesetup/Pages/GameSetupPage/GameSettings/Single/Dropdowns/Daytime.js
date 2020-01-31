GameSettingControls.Daytime = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = undefined;
	}

	onHoverChange()
	{
		this.dropdown.tooltip = this.values.Description[this.dropdown.hovered] || this.Tooltip;
	}

	onMapChange(mapData)
	{
		if (mapData && mapData.settings && mapData.settings.Daytime)
		{
			this.values = prepareForDropdown([
				{
					"Id": "random",
					"Name": setStringTags(this.RandomTitle, this.RandomItemTags),
					"Description": this.RandomDescription,
					"Preview": mapData.settings.Preview
				},
				...mapData.settings.Daytime.map(item => ({
					"Id": item.Id,
					"Name": translate(item.Name),
					"Description": translate(item.Description),
					"Preview": item.Preview
				}))
			]);

			this.dropdown.list = this.values.Name;
			this.dropdown.list_data = this.values.Id;
		}
		else
			this.values = undefined;

		this.setHidden(!this.values);
	}

	onGameAttributesChange()
	{
		if (!g_GameAttributes.map)
			return;

		if (this.values)
		{
			if (this.values.Id.indexOf(g_GameAttributes.settings.Daytime || undefined) == -1)
			{
				g_GameAttributes.settings.Daytime = "random";
				this.gameSettingsControl.updateGameAttributes();
			}

			let preview = this.values.Preview[this.values.Id.indexOf(g_GameAttributes.settings.Daytime)];
			if (!g_GameAttributes.settings.Preview || g_GameAttributes.settings.Preview != preview)
			{
				g_GameAttributes.settings.Preview = preview;
				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else if (g_GameAttributes.settings.Daytime !== undefined)
		{
			delete g_GameAttributes.settings.Daytime;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		if (g_GameAttributes.settings.Daytime)
			this.setSelectedValue(g_GameAttributes.settings.Daytime);
	}

	getAutocompleteEntries()
	{
		return this.values && this.values.Name.slice(1);
	}

	onSelectionChange(itemIdx)
	{
		g_GameAttributes.settings.Daytime = this.values.Id[itemIdx];

		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}

	onPickRandomItems()
	{
		if (this.values && g_GameAttributes.settings.Daytime == "random")
		{
			g_GameAttributes.settings.Daytime = pickRandom(this.values.Id.slice(1));
			this.gameSettingsControl.updateGameAttributes();
			this.gameSettingsControl.pickRandomItems();
		}
	}
};

GameSettingControls.Daytime.prototype.TitleCaption =
	translate("Daytime");

GameSettingControls.Daytime.prototype.Tooltip =
	translate("Select whether the match takes place at daylight or night.");

GameSettingControls.Daytime.prototype.RandomTitle =
	translateWithContext("daytime selection", "Random");

GameSettingControls.Daytime.prototype.RandomDescription =
	translateWithContext("daytime selection", "Randomly pick a time of the day.");

GameSettingControls.Daytime.prototype.AutocompleteOrder = 0;
