function Wonder() {}

Wonder.prototype.Schema = 
	"<element name='DurationMultiplier' a:help='A civ-specific time-bonus/handicap for the wonder-victory-condition.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

Wonder.prototype.Init = function()
{
};

Wonder.prototype.Serialize = null;

/**
 * Returns the number of minutes that a player has to keep the wonder in order to win.
 */
Wonder.prototype.GetVictoryDuration = function()
{
	var cmpEndGameManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager);
	return cmpEndGameManager.GetWonderDuration() * this.template.DurationMultiplier;
};

Engine.RegisterComponentType(IID_Wonder, "Wonder", Wonder);
