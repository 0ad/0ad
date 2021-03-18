GameSettings.prototype.Attributes.GameSpeed = class GameSpeed extends GameSetting
{
	init()
	{
		this.gameSpeed = 1;
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		attribs.gameSpeed = this.gameSpeed;
	}

	fromInitAttributes(attribs)
	{
		if (!attribs.gameSpeed)
			return;
		this.gameSpeed = +attribs.gameSpeed;
	}

	onMapChange()
	{
		if (!this.getMapSetting("gameSpeed"))
			return;
		this.setSpeed(+this.getMapSetting("gameSpeed"));
	}

	setSpeed(speed)
	{
		this.gameSpeed = speed;
	}
};
