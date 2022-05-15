/**
 * Map choice. This handles:
 *  - the map itself
 *  - map type (which is mostly a GUI thing and should probably be refactored out)
 *  - map script (for random maps).
 * When a non-"random" map is selected, the map 'script settings' are available at this.data.
 * TODO: the map description is currently tied to the map itself.
 */
GameSettings.prototype.Attributes.Map = class Map extends GameSetting
{
	init()
	{
		this.watch(() => this.updateMapMetadata(), ["map"]);
		this.randomOptions = [];
	}

	toInitAttributes(attribs)
	{
		attribs.map = this.map;
		attribs.mapType = this.type;
		if (this.script)
			attribs.script = this.script;
	}

	fromInitAttributes(attribs)
	{
		if (attribs.mapType)
			this.setType(attribs.mapType);

		if (!attribs.map)
			return;

		this.selectMap(attribs.map);
	}

	setType(mapType)
	{
		this.type = mapType;
	}

	selectMap(map)
	{
		this.data = this.settings.mapCache.getMapData(this.type, map);
		this.map = map;
	}

	updateMapMetadata()
	{
		if (this.type == "random" && this.data)
			this.script = this.data.settings.Script;
		else
			this.script = undefined;
	}

	pickRandomItems()
	{
		if (this.map !== "random")
			return false;
		this.selectMap(pickRandom(this.randomOptions));
		return true;
	}

	setRandomOptions(options)
	{
		this.randomOptions = clone(options);
		if (this.randomOptions.indexOf("random") !== -1)
			this.randomOptions.splice(this.randomOptions.indexOf("random"), 1);
	}
};
