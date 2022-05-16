/**
 * TODO: There should be a dialog allowing to specify starting resources per player
 */
GameSettings.prototype.Attributes.StartingResources = class StartingResources extends GameSetting
{
	init()
	{
		this.defaultValue = this.getDefaultValue("StartingResources", "Resources") || 300;
		this.perPlayer = undefined;
		this.setResources(this.defaultValue);
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (this.perPlayer)
		{
			if (!attribs.settings.PlayerData)
				attribs.settings.PlayerData = [];
			while (attribs.settings.PlayerData.length < this.perPlayer.length)
				attribs.settings.PlayerData.push({});
			for (let i in this.perPlayer)
				if (this.perPlayer[i])
					attribs.settings.PlayerData[i].Resources = this.perPlayer[i];
		}
		attribs.settings.StartingResources = this.resources;
	}

	fromInitAttributes(attribs)
	{
		if (this.getLegacySetting(attribs, "StartingResources") !== undefined)
			this.setResources(this.getLegacySetting(attribs, "StartingResources"));
	}

	onMapChange()
	{
		this.perPlayer = undefined;
		if (this.settings.map.type != "scenario")
			return;
		if (!!this.getMapSetting("PlayerData") &&
		     this.getMapSetting("PlayerData").some(data => data.Resources))
			this.perPlayer = this.getMapSetting("PlayerData").map(data => data.Resources || undefined);
		else if (!this.getMapSetting("StartingResources"))
			this.setResources(this.defaultValue);
		else
			this.setResources(this.getMapSetting("StartingResources"));
	}

	setResources(res)
	{
		this.resources = res;
	}
};
