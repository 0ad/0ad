GameSettingControls.MapSize = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.previousMapType = undefined;

		this.dropdown.list = g_MapSizes.Name;
		this.dropdown.list_data = g_MapSizes.Tiles;
	}

	onHoverChange()
	{
		this.dropdown.tooltip = g_MapSizes.Tooltip[this.dropdown.hovered] || this.Tooltip;
	}

	onMapChange(mapData)
	{
		let mapValue =
			mapData &&
			mapData.settings &&
			mapData.settings.Size || undefined;

		if (g_GameAttributes.mapType == "random" && mapValue !== undefined && mapValue != g_GameAttributes.settings.Size)
		{
			g_GameAttributes.settings.Size = mapValue;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesChange()
	{
		if (!g_GameAttributes.mapType ||
			!g_GameAttributes.settings ||
			this.previousMapType == g_GameAttributes.mapType)
			return;

		this.previousMapType = g_GameAttributes.mapType;

		let available = g_GameAttributes.mapType == "random";
		this.setHidden(!available);

		if (available)
		{
			if (g_GameAttributes.settings.Size === undefined)
			{
				g_GameAttributes.settings.Size = g_MapSizes.Tiles[g_MapSizes.Default];
				this.gameSettingsControl.updateGameAttributes();
			}
			this.setSelectedValue(g_GameAttributes.settings.Size);
		}
		else if (g_GameAttributes.settings.Size !== undefined)
		{
			delete g_GameAttributes.settings.Size;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	getAutocompleteEntries()
	{
		return g_MapSizes.Name;
	}

	onSelectionChange(itemIdx)
	{
		g_GameAttributes.settings.Size = g_MapSizes.Tiles[itemIdx];
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.MapSize.prototype.TitleCaption =
	translate("Map Size");

GameSettingControls.MapSize.prototype.Tooltip =
	translate("Select map size. (Larger sizes may reduce performance.)");

GameSettingControls.MapSize.prototype.AutocompleteOrder = 0;
