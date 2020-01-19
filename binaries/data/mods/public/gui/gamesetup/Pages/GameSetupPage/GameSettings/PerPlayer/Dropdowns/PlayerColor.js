PlayerSettingControls.PlayerColor = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.defaultColors = g_Settings.PlayerDefaults.slice(1).map(pData => pData.Color);
	}

	setControl()
	{
		this.dropdown = Engine.GetGUIObjectByName("playerColor[" + this.playerIndex + "]");
		this.playerBackgroundColor = Engine.GetGUIObjectByName("playerBackgroundColor[" + this.playerIndex + "]");
		this.playerColorHeading = Engine.GetGUIObjectByName("playerColorHeading");
	}

	onMapChange(mapData)
	{
		Engine.ProfileStart("updatePlayerColorList");
		let hidden = !g_IsController || g_GameAttributes.mapType == "scenario";
		this.dropdown.hidden = hidden;
		this.playerColorHeading.hidden = hidden;

		// Step 1: Pick colors that the map specifies, add most unsimilar default colors
		// Provide the access to g_MaxPlayers different colors, regardless of current playercount.
		let values = [];
		for (let i = 0; i < g_MaxPlayers; ++i)
		{
			let pData = this.gameSettingsControl.getPlayerData(mapData, i);
			values.push(pData && pData.Color || this.findFarthestUnusedColor(values));
		}

		// Step 2: Sort these colors so that the order is most reminiscent of the default colors
		this.values = [];
		for (let i = 0; i < g_MaxPlayers; ++i)
		{
			let closestColor;
			let smallestDistance = Infinity;
			for (let color of values)
			{
				if (this.values.some(col => sameColor(col, color)))
					continue;

				let distance = colorDistance(color, this.defaultColors[i]);
				if (distance <= smallestDistance)
				{
					closestColor = color;
					smallestDistance = distance;
				}
			}
			this.values.push(closestColor);
		}

		this.dropdown.list = this.values.map(color => coloredText(this.ColorIcon, rgbToGuiColor(color)));
		this.dropdown.list_data = this.values.map((color, i) => i);

		// If the map specified a color for this slot, use that
		let mapPlayerData = this.gameSettingsControl.getPlayerData(mapData, this.playerIndex);
		let playerData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);

		if (playerData && mapPlayerData && mapPlayerData.Color &&
			(!playerData.Color || !sameColor(playerData.Color, mapPlayerData.Color)))
		{
			playerData.Color = mapPlayerData.Color;
			this.gameSettingsControl.updateGameAttributes();
		}

		Engine.ProfileStop();
	}

	onGameAttributesChange()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData || !this.values)
			return;

		let inUse =
			pData.Color &&
			g_GameAttributes.settings.PlayerData.some((otherPData, i) =>
				i < this.playerIndex &&
				otherPData.Color &&
				sameColor(pData.Color, otherPData.Color));

		if (!pData.Color || this.values.indexOf(pData.Color) == -1 || inUse)
		{
			pData.Color =
				(pData.Color && !inUse) ?
					this.findClosestColor(pData.Color, this.values) :
					this.getUnusedColor();

			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData || !this.values)
			return;

		this.setSelectedValue(this.values.indexOf(pData.Color));
		this.playerBackgroundColor.sprite = "color:" + rgbToGuiColor(pData.Color, 100);
	}

	onAssignPlayer(source, target)
	{
		if (g_GameAttributes.mapType != "scenario" && source && target)
			[source.Color, target.Color] = [target.Color, source.Color];
	}

	onSelectionChange(itemIdx)
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);

		// If someone else has that color, give that player the old color
		let otherPData = g_GameAttributes.settings.PlayerData.find(data =>
			sameColor(this.values[itemIdx], data.Color));

		if (otherPData)
			otherPData.Color = pData.Color;

		pData.Color = this.values[itemIdx];
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}

	findClosestColor(targetColor, colors)
	{
		let colorDistances = colors.map(color => colorDistance(color, targetColor));

		let smallestDistance = colorDistances.find(
			distance => colorDistances.every(distance2 => distance2 >= distance));

		return colors.find(color => colorDistance(color, targetColor) == smallestDistance);
	}

	findFarthestUnusedColor(values)
	{
		let farthestColor;
		let farthestDistance = 0;

		for (let defaultColor of this.defaultColors)
		{
			let smallestDistance = Infinity;
			for (let usedColor of values)
			{
				let distance = colorDistance(usedColor, defaultColor);
				if (distance < smallestDistance)
					smallestDistance = distance;
			}

			if (smallestDistance >= farthestDistance)
			{
				farthestColor = defaultColor;
				farthestDistance = smallestDistance;
			}
		}
		return farthestColor;
	}

	getUnusedColor()
	{
		return this.values.find(color =>
			g_GameAttributes &&
			g_GameAttributes.settings &&
			g_GameAttributes.settings.PlayerData &&
			g_GameAttributes.settings.PlayerData.every(pData => !pData.Color || !sameColor(color, pData.Color)));
	}
};

PlayerSettingControls.PlayerColor.prototype.Tooltip =
	translate("Pick a color.");

PlayerSettingControls.PlayerColor.prototype.ColorIcon =
	"■";
