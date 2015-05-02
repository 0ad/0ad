var PETRA = function(m)
{

// Some functions that could be part of the gamestate.

// The next three are to register that we assigned a gatherer to a resource this turn.
// expects an entity
m.IsSupplyFull = function(gamestate, supply)
{
	if (supply.isFull() === true)
		return true;
	var count = supply.resourceSupplyNumGatherers();
	if (gamestate.turnCache["ressourceGatherer"] && gamestate.turnCache["ressourceGatherer"][supply.id()])
		count += gamestate.turnCache["ressourceGatherer"][supply.id()];
	if (count >= supply.maxGatherers())
		return true;
	return false;
};

// add a gatherer to the turn cache for this supply.
m.AddTCGatherer = function(gamestate, supplyID)
{
	if (gamestate.turnCache["ressourceGatherer"] && gamestate.turnCache["ressourceGatherer"][supplyID] !== undefined)
		++gamestate.turnCache["ressourceGatherer"][supplyID];
	else
	{ 
		if (!gamestate.turnCache["ressourceGatherer"])
			gamestate.turnCache["ressourceGatherer"] = {};
		gamestate.turnCache["ressourceGatherer"][supplyID] = 1;
	}
};

// remove a gatherer to the turn cache for this supply.
m.RemoveTCGatherer = function(gamestate, supplyID)
{
	if (gamestate.turnCache["ressourceGatherer"] && gamestate.turnCache["ressourceGatherer"][supplyID])
		--gamestate.turnCache["ressourceGatherer"][supplyID];
	else
		if (!gamestate.turnCache["ressourceGatherer"])
			gamestate.turnCache["ressourceGatherer"] = {};
		gamestate.turnCache["ressourceGatherer"][supplyID] = -1;
};

m.GetTCGatherer = function(gamestate, supplyID)
{
	if (gamestate.turnCache["ressourceGatherer"] && gamestate.turnCache["ressourceGatherer"][supplyID])
		return gamestate.turnCache["ressourceGatherer"][supplyID];
	else
		return 0;
};

// The next three are to register that we assigned a gatherer to a resource this turn.
m.AddTCRessGatherer = function(gamestate, resource)
{
	if (gamestate.turnCache["ressourceGatherer-" + resource])
		++gamestate.turnCache["ressourceGatherer-" + resource];
	else
		gamestate.turnCache["ressourceGatherer-" + resource] = 1;
};

m.GetTCRessGatherer = function(gamestate, resource)
{
	if (gamestate.turnCache["ressourceGatherer-" + resource])
		return gamestate.turnCache["ressourceGatherer-" + resource];
	else
		return 0;
};

return m;
}(PETRA);
