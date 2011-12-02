// Decides when to a new house needs to be built
var HousingManager = function() {
	
};

HousingManager.prototype.buildMoreHouses = function(gameState, queues) {
	// temporary 'remaining population space' based check, need to do
	// predictive in future
	if (gameState.getPopulationLimit() - gameState.getPopulation() < 20
			&& gameState.getPopulationLimit() < gameState.getPopulationMax()) {
		var numConstructing = gameState.countEntitiesWithType(gameState.applyCiv("foundation|structures/{civ}_house"));
		var numPlanned = queues.house.totalLength();

		var additional = Math.ceil((20 - (gameState.getPopulationLimit() - gameState.getPopulation())) / 10)
				- numConstructing - numPlanned;

		for ( var i = 0; i < additional; i++) {
			queues.house.addItem(new BuildingConstructionPlan(gameState, "structures/{civ}_house"));
		}
	}
};

HousingManager.prototype.update = function(gameState, queues) {
	Engine.ProfileStart("housing update");

	this.buildMoreHouses(gameState, queues);

	Engine.ProfileStop();
};
