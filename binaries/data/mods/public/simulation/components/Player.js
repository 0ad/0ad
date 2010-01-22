function Player() {}

Player.prototype.Init = function()
{
	this.playerID = undefined;
	this.playerName = "Unknown";
	this.civ = "celt";
	this.popCount = 0;
	this.popLimit = 50;
};

Player.prototype.SetPlayerID = function(id)
{
	this.playerID = id;
};

Player.prototype.GetPopulationCount = function()
{
	return this.popCount;
};

Player.prototype.GetPopulationLimit = function()
{
	return this.popLimit;
};

Player.prototype.OnGlobalOwnershipChanged = function(msg)
{
	if (msg.from == this.playerID)
	{
		var cost = Engine.QueryInterface(msg.entity, IID_Cost);
		if (cost)
			this.popCount -= cost.GetPopCost();
	}
	
	if (msg.to == this.playerID)
	{
		var cost = Engine.QueryInterface(msg.entity, IID_Cost);
		if (cost)
			this.popCount += cost.GetPopCost();
	}
};

Engine.RegisterComponentType(IID_Player, "Player", Player);
