GameSettings.prototype.Attributes.MatchID = class MatchID extends GameSetting
{
	onFinalizeAttributes(attribs)
	{
		attribs.matchID = Engine.GetMatchID();
	}
};
