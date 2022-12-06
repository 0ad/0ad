/**
 * Stores player color for all players.
 */
GameSettings.prototype.Attributes.PlayerColor = class PlayerColor extends GameSetting
{
	init()
	{
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
		// Provide the access to all the colors defined in simulation/data/settings/player_defaults.json,
		// regardless of current playercount.
		let values = [];
		let mapColors = false;
		for (let i = 0; i < this.DefaultColors.length; ++i)
		{
			let col = this._getMapData(i);
			if (col)
				mapColors = true;
			if (mapColors)
				values.push(col || this._findFarthestUnusedColor(values));
			else
				values.push(this.DefaultColors[i]);
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

		for (let defaultColor of this.DefaultColors)
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

GameSettings.prototype.Attributes.PlayerColor.prototype.DefaultColors = [
	{ "r": 10, "g": 10, "b": 190 },
	{ "r": 230, "g": 10, "b": 10 },
	{ "r": 125 , "g": 235, "b": 15 },
	{ "r": 255, "g": 255, "b": 55 },
	{ "r": 130, "g": 0, "b": 230 },
	{ "r": 255, "g": 130, "b": 0 },
	{ "r": 10, "g": 230, "b": 230 },
	{ "r": 20, "g": 80, "b": 60 },
	{ "r": 220, "g": 160, "b": 220 },
	{ "r": 80, "g": 255, "b": 190 },
	{ "r": 50, "g": 150, "b": 255 },
	{ "r": 100, "g": 150, "b": 30 },
	{ "r": 100, "g": 60, "b": 30 },
	{ "r": 128, "g": 0, "b": 64 },
	{ "r": 255, "g": 200, "b": 140 },
	{ "r": 80, "g": 80, "b": 80 }
];
