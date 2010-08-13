function Player() {}

Player.prototype.Schema =
	"<a:component type='system'/><empty/>";

Player.prototype.Init = function()
{
	this.playerID = undefined;
	this.name = "Unknown";
	this.civ = "gaia";
	this.colour = { "r": 0.0, "g": 0.0, "b": 0.0, "a": 1.0 };
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

Player.prototype.SetName = function(name)
{
	this.name = name;
};

Player.prototype.GetName = function()
{
	return this.name;
};

Player.prototype.SetCiv = function(civcode)
{
	this.civ = civcode;
};

Player.prototype.GetCiv = function()
{
	return this.civ;
};

Player.prototype.SetColour = function(r, g, b)
{
	this.colour = { "r": r/255.0, "g": g/255.0, "b": b/255.0, "a": 1.0 };
};

Player.prototype.GetColour = function()
{
	return this.colour;
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
	// Check if we can afford it all
	var amountsNeeded = {};
	for (var type in amounts)
		if (amounts[type] > this.resourceCount[type])
			amountsNeeded[type] = amounts[type] - this.resourceCount[type];
	
	var formattedAmountsNeeded = [];
	for (var type in amountsNeeded)
		formattedAmountsNeeded.push(type + ": " + amountsNeeded[type]);
			
	// If we don't have enough resources, send a notification to the player
	if (formattedAmountsNeeded.length)
	{
		var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGUIInterface.PushNotification("Resources needed: " + formattedAmountsNeeded.join(", "));
		return false;
	}
	else
	{
		// Subtract the resources
		for (var type in amounts)
			this.resourceCount[type] -= amounts[type];	
	}

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
