var TutorialAI = (function() {
var m = {};
			 
m.TutorialAI = function(settings) {
	API3.BaseAI.call(this, settings);

	this.turn = 0;
	
	this.firstTime = true;

	this.savedEvents = [];
	
	this.toUpdate = [];
}

m.TutorialAI.prototype = new API3.BaseAI();

//Some modules need the gameState to fully initialise
m.TutorialAI.prototype.runInit = function(gameState) {
	if (this.firstTime){
		this.firstTime = false;
		
		this.chooseTutorial(gameState);
		
		if (this.tutorial === undefined) return;
		
		this.currentPos = 0;
		this.currentState = this.tutorial[this.currentPos];
		
		this.lastChat = -1000000;
	}
};

m.TutorialAI.prototype.chooseTutorial = function(gameState) {
	var trees = gameState.updatingCollection("trees", API3.Filters.byClass("ForestPlant"), gameState.getEntities());
	
	var numTrees = trees.length;
	switch (numTrees) {
	case 945:
		this.tutorial = economic_walkthrough;
		return;
	case 221:
		this.tutorial = introductoryTutorial;
		return;
	}
	this.chat("Tutorial AI does not recognise this map, are you sure you have selected a tutorial map?  (" + numTrees + " trees found)");
};

m.TutorialAI.prototype.OnUpdate = function() {
	if (this.gameFinished){
		return;
	}
	
	if (this.events.length > 0){
		this.savedEvents = this.savedEvents.concat(this.events);
	}
	
	Engine.ProfileStart("tutorialBot");
	
	var gameState = this.gameState;
	this.runInit(gameState);
	
	if (this.tutorial === undefined) return;
	
	if (gameState.getTimeElapsed() - this.lastChat > 30000){
		this.chat(this.currentState.instructions);
		this.lastChat = gameState.getTimeElapsed();
	}
	
	// check to see if we need to change state
	var nextState = this.tutorial[this.currentPos + 1];
	var doNext = false;
	switch (nextState.trigger) {
	case "near_cc":
		var ents = gameState.getEnemyEntities();
		var CC = gameState.updatingCollection("cc", API3.Filters.and(API3.Filters.byClass("CivCentre"), API3.Filters.byOwner(2)), gameState.getEntities());
		
		ents.forEach(function(ent) {
			if (!ent.position()){
				return;
			}
			if (API3.VectorDistance(CC.toEntityArray()[0].position(), ent.position()) < 50){
				doNext = true;
			}
		});
	case "food_gathered":
		var food = gameState.getResourceSupplies("food");
		food.forEach(function (ent) {
			if (ent.resourceSupplyAmount() !== ent.resourceSupplyMax()) {
				doNext = true;
			}
		});
		break;
	case "wood_gathered":
		var food = gameState.getResourceSupplies("wood");
		food.forEach(function (ent) {
			if (ent.resourceSupplyAmount() != ent.resourceSupplyMax()) {
				doNext = true;
			}
		});
		break;
	case "training_start":
		var trainingStructures = gameState.getEnemyEntities();
		trainingStructures.forEach(function (ent) {
			if (ent.trainingQueue()) {
				var queue = ent.trainingQueue();
				for (var i in queue) {
					if (queue[i].unitTemplate === nextState.template &&
					    queue[i].count === nextState.count) {
						doNext = true;
					}
				}
			}
		});
		break;
	case "entity_count":
		var ents = gameState.updatingCollection(
			nextState.template, 
			API3.Filters.byType(nextState.template),
			gameState.getEnemyEntities());
		if (ents.length >= nextState.count) {
			doNext = true;
		}
		break;
	case "entity_counts":
		doNext = true;
		for (var i = 0; i < nextState.templates.length; i++) {
			var ents = gameState.updatingCollection(
				nextState.templates[i], 
				API3.Filters.byType(nextState.templates[i]),
				gameState.getEnemyEntities());
			if (ents.length < nextState.counts[i]) {
				doNext = false;
			}
		}
		break;
	case "dead_enemy_units":
		var ents = gameState.updatingCollection(nextState.collectionId);
		if (ents.length === 0) {
			doNext = true;
		}
		break;
	case"relative_time":
		if (this.relativeTimeStart === undefined) {
			this.relativeTimeStart = gameState.getTimeElapsed();
		}
		if (this.relativeTimeStart + nextState.time * 1000 < gameState.getTimeElapsed()) {
			delete this.relativeTimeStart;
			doNext = true;
		}
	case "time":
		if (nextState.time*1000 < gameState.getTimeElapsed()) {
			doNext = true;
		}
		break;
	}
	if (doNext){
		this.currentPos += 1;
		this.currentState = this.tutorial[this.currentPos];
		this.chat(this.currentState.instructions);
		this.lastChat = gameState.getTimeElapsed();
		if (this.currentState.action) {
			this.currentState.action(gameState);
		}
		
		if (this.currentPos >= this.tutorial.length - 1){
			gameState.getOwnEntities().destroy();
			this.gameFinished = true;
		}
	}
	
	delete this.savedEvents;
	this.savedEvents = [];

	Engine.ProfileStop();

	this.turn++;
};

// TODO: Remove override when the whole AI state is serialised
m.TutorialAI.prototype.Deserialize = function(data)
{
	BaseAI.prototype.Deserialize.call(this, data);
	this._entityMetadata = {};
};

// Override the default serializer
m.TutorialAI.prototype.Serialize = function()
{
	return {
		_rawEntities: this._rawEntities,
		_ownEntities: this._ownEntities,
		_entityMetadata: {} // We store fancy data structures in entity metadata so 
		                    //don't try and serialize it
	};
};

m.debug = function(output)
{
	API3.debug(output);
}


return m;
}());
