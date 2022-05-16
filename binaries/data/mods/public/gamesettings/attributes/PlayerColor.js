/**
 * Stores player color for all players.
 */
GameSettings.prototype.Attributes.PlayerColor = class PlayerColor extends GameSetting
{
	init()
	{
		this.defaultColors = g_Settings.PlayerDefaults.slice(1).map(pData => pData.Color);

		this.watch(() => this.maybeUpdate(), ["available"]);
		this.settings.playerCount.watch(() => this.maybeUpdate(), ["nbPlayers"]);
		this.settings.map.watch(() => this.onMapChange(), ["map"]);

		// NB: watchers aren't auto-triggered when modifying array elements.
		this.values = [];
		this.locked = [];
		this._updateAvailable();
	}

	toInitAttributes(attribs)
	{
		if (!attribs.settings.PlayerData)
			attribs.settings.PlayerData = [];
		while (attribs.settings.PlayerData.length < this.values.length)
			attribs.settings.PlayerData.push({});
		for (let i in this.values)
			if (this.values[i])
				attribs.settings.PlayerData[i].Color = this.values[i];
	}

	fromInitAttributes(attribs)
	{
		if (!this.getLegacySetting(attribs, "PlayerData"))
			return;
		const pData = this.getLegacySetting(attribs, "PlayerData");
		for (let i = 0; i < this.values.length; ++i)
			if (pData[i] && pData[i].Color)
				this.setColor(i, pData[i].Color);
	}

	_resize(nb)
	{
		while (this.values.length > nb)
		{
			this.values.pop();
			this.locked.pop();
		}
		while (this.values.length < nb)
		{
			this.values.push(undefined);
			this.locked.push(false);
		}
	}

	onMapChange()
	{
		// Reset.
		this.locked = this.locked.map(x => this.settings.map.type == "scenario");
		this.trigger("locked");
		if (this.settings.map.type === "scenario")
			this._resize(0);
		this._updateAvailable();
		this.maybeUpdate();
	}

	maybeUpdate()
	{
		this._resize(this.settings.playerCount.nbPlayers);

		this.values.forEach((c, i) => this._set(i, c));
		this.trigger("values");
	}

	_set(playerIndex, color)
	{
		let inUse = this.values.findIndex((otherColor, i) =>
			color && otherColor &&
			sameColor(color, otherColor));
		if (inUse != -1 && inUse != playerIndex)
		{
			// Swap colors.
			let col = this.values[playerIndex];
			this.values[playerIndex] = undefined;
			this._set(inUse, col);
		}
		if (!color || this.available.indexOf(color) == -1)
		{
			this.values[playerIndex] = color ?
				this._findClosestColor(color, this.available) :
				this._getUnusedColor();
		}
		else
			this.values[playerIndex] = color;
	}

	get(playerIndex)
	{
		if (playerIndex >= this.values.length)
			return undefined;
		return this.values[playerIndex];
	}

	setColor(playerIndex, color)
	{
		this._set(playerIndex, color);
		this.trigger("values");
	}

	swap(sourceIndex, targetIndex)
	{
		[this.values[sourceIndex], this.values[targetIndex]] = [this.values[targetIndex], this.values[sourceIndex]];
		[this.locked[sourceIndex], this.locked[targetIndex]] = [this.locked[targetIndex], this.locked[sourceIndex]];
		this.trigger("values");
	}

	_getMapData(i)
	{
		let data = this.settings.map.data;
		if (!data || !data.settings || !data.settings.PlayerData)
			return undefined;
		if (data.settings.PlayerData.length <= i)
			return undefined;
		return data.settings.PlayerData[i].Color;
	}

	_updateAvailable()
	{
		// Pick colors that the map specifies, add most unsimilar default colors
		// Provide the access to g_MaxPlayers different colors, regardless of current playercount.
		let values = [];
		let mapColors = false;
		for (let i = 0; i < g_MaxPlayers; ++i)
		{
			let col = this._getMapData(i);
			if (col)
				mapColors = true;
			if (mapColors)
				values.push(col || this._findFarthestUnusedColor(values));
			else
				values.push(this.defaultColors[i]);
		}
		this.available = values;
	}

	_findClosestColor(targetColor, colors)
	{
		let closestColor;
		let closestColorDistance = 0;
		for (let color of colors)
		{
			let dist = colorDistance(targetColor, color);
			if (!closestColor || dist < closestColorDistance)
			{
				closestColor = color;
				closestColorDistance = dist;
			}
		}
		return closestColor;
	}

	_findFarthestUnusedColor(values)
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

	_getUnusedColor()
	{
		return this.available.find(color => {
			return this.values.every(otherColor => !otherColor || !sameColor(color, otherColor));
		});
	}
};
