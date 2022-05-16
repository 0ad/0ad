/**
 * Map Preview.
 * Can optionally overwrite the default map preview.
 */
GameSettings.prototype.Attributes.MapPreview = class MapPreview extends GameSetting
{
	init()
	{
		this.value = undefined;
		this.settings.map.watch(() => this.updatePreview(), ["map"]);
		this.settings.biome.watch(() => this.updatePreview(), ["biome"]);
		this.settings.landscape.watch(() => this.updatePreview(), ["value"]);
		this.settings.daytime.watch(() => this.updatePreview(), ["value"]);
	}

	toInitAttributes(attribs)
	{
		if (this.value !== undefined)
			attribs.settings.mapPreview = this.value;
	}

	fromInitAttributes(attribs)
	{
		if (!!this.getLegacySetting(attribs, "mapPreview"))
			this.value = this.getLegacySetting(attribs, "mapPreview");
	}

	getPreviewForSubtype(basepath, subtype)
	{
		if (!subtype)
			return undefined;
		let substr = subtype.substr(subtype.lastIndexOf("/") + 1);
		let path = basepath + "_" + substr + ".png";
		if (this.settings.mapCache.previewExists(path))
			return this.settings.mapCache.getMapPreview(this.settings.map.type,
				this.settings.map.map, path);
		return undefined;
	}

	getLandscapePreview()
	{
		let filename = this.settings.landscape.getPreviewFilename();
		if (!filename)
			return undefined;
		return this.settings.mapCache.getMapPreview(this.settings.map.type,
			this.settings.map.map, filename);
	}

	updatePreview()
	{
		if (!this.settings.map.map)
		{
			this.value = undefined;
			return;
		}

		// This handles "random" map type (mostly for convenience).
		let mapPath = basename(this.settings.map.map);
		this.value = this.getPreviewForSubtype(mapPath, this.settings.biome.biome) ||
			this.getLandscapePreview() ||
			this.getPreviewForSubtype(mapPath, this.settings.daytime.value) ||
			this.settings.mapCache.getMapPreview(this.settings.map.type, this.settings.map.map);
	}
};
