GameSettings.prototype.Attributes.MatchID = class MatchID extends GameSetting
{
	init()
	{
		this.matchID = Engine.GetMatchID();
	}

	toInitAttributes(attribs)
	{
		attribs.matchID = this.matchID;
	}

	fromInitAttributes(attribs)
	{
		if (attribs.matchID !== undefined)
			this.matchID = attribs.matchID;
	}
};
