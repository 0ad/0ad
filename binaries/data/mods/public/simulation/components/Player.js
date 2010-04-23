function Player() {}

Player.prototype.Schema =
	"<a:component type='system'/><empty/>";

Player.prototype.Init = function()
{
	this.playerID = undefined;
	this.playerName = "Unknown";
	this.civ = "celt";
	this.popCount = 0;
	this.popLimit = 50;
	this.resourceCount = {
		"food": 2000,	
		"wood": 1500,	
		"metal": 500,	
		"stone": 1000	
	};
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

Player.prototype.GetResourceCounts = function()
{
	return this.resourceCount;
};

Player.prototype.AddResource = function(type, amount)
{
	this.resourceCount[type] += (+amount);
};

Player.prototype.AddResources = function(amounts)
{
	for (var type in amounts)
		this.resourceCount[type] += (+amounts[type]);
};

Player.prototype.TrySubtractResources = function(amounts)
{
	// Check we can afford it all
	for (var type in amounts)
		if (amounts[type] > this.resourceCount[type])
			return false;

	// Subtract the resources
	for (var type in amounts)
		this.resourceCount[type] -= amounts[type];

	return true;
};

Player.prototype.OnGlobalOwnershipChanged = function(msg)
{
	if (msg.from == this.playerID)
	{
		var cost = Engine.QueryInterface(msg.entity, IID_Cost);
		if (cost)
		{
			this.popCount -= cost.GetPopCost();
			this.popLimit -= cost.GetPopBonus();
		}
	}
	
	if (msg.to == this.playerID)
	{
		var cost = Engine.QueryInterface(msg.entity, IID_Cost);
		if (cost)
		{
			this.popCount += cost.GetPopCost();
			this.popLimit += cost.GetPopBonus();
		}
	}
};

Engine.RegisterComponentType(IID_Player, "Player", Player);
