
function QBotAI(settings) {
	BaseAI.call(this, settings);

	this.turn = 0;

	this.modules = {
			"economy": new EconomyManager(), 
			"military": new MilitaryAttackManager(), 
			"housing": new HousingManager()
	};

	// this.queues cannot be modified past initialisation or queue-manager will break
	this.queues = {
		house : new Queue(),
		citizenSoldier : new Queue(),
		villager : new Queue(),
		economicBuilding : new Queue(),
		field : new Queue(),
		advancedSoldier : new Queue(),
		siege : new Queue(),
		militaryBuilding : new Queue(),
		defenceBuilding : new Queue(),
		civilCentre: new Queue()
	};

	this.productionQueues = [];
	
	this.priorities = Config.priorities;
	
	this.queueManager = new QueueManager(this.queues, this.priorities);
	
	this.firstTime = true;

	this.savedEvents = [];
}

QBotAI.prototype = new BaseAI();

//Some modules need the gameState to fully initialise
QBotAI.prototype.runInit = function(gameState){
	if (this.firstTime){
		for (var i in this.modules){
			if (this.modules[i].init){
				this.modules[i].init(gameState);
			}
		}
		
		this.timer = new Timer();
		
		this.firstTime = false;
		
		var myKeyEntities = gameState.getOwnEntities().filter(function(ent) {
			return ent.hasClass("CivCentre");
		});
		
		if (myKeyEntities.length == 0){
			myKeyEntities = gameState.getOwnEntities();
		}
		
		
		var filter = Filters.byClass("CivCentre");
		var enemyKeyEntities = gameState.getEnemyEntities().filter(filter);
		
		if (enemyKeyEntities.length == 0){
			enemyKeyEntities = gameState.getEnemyEntities();
		}
		
		this.accessibility = new Accessibility(gameState, myKeyEntities.toEntityArray()[0].position());
		
		if (enemyKeyEntities.length == 0)
			return;
		
		var pathFinder = new PathFinder(gameState);
		this.pathsToMe = pathFinder.getPaths(enemyKeyEntities.toEntityArray()[0].position(), myKeyEntities.toEntityArray()[0].position(), 'entryPoints');
	}
};

QBotAI.prototype.OnUpdate = function() {
	if (this.gameFinished){
		return;
	}
	
	if (this.events.length > 0){
		this.savedEvents = this.savedEvents.concat(this.events);
	}
	
	// Run the update every n turns, offset depending on player ID to balance
	// the load
	if ((this.turn + this.player) % 10 == 0) {
		Engine.ProfileStart("qBot");
		
		var gameState = new GameState(this);
		
		if (gameState.getOwnEntities().length === 0){
			Engine.ProfileStop();
			return; // With no entities to control the AI cannot do anything 
		}
		
		this.runInit(gameState);
		
		for (var i in this.modules){
			this.modules[i].update(gameState, this.queues, this.savedEvents);
		}
		
		this.updateDynamicPriorities(gameState, this.queues);
		
		this.queueManager.update(gameState);
		
		// Generate some entropy in the random numbers (against humans) until the engine gets random initialised numbers
		// TODO: remove this when the engine gives a random seed
		var n = this.savedEvents.length % 29;
		for (var i = 0; i < n; i++){
			Math.random();
		}
		
		delete this.savedEvents;
		this.savedEvents = [];

		Engine.ProfileStop();
	}

	this.turn++;
};

QBotAI.prototype.updateDynamicPriorities = function(gameState, queues){
	// Dynamically change priorities
	Engine.ProfileStart("Change Priorities");
	var females = gameState.countEntitiesByType(gameState.applyCiv("units/{civ}_support_female_citizen"));
	var femalesTarget = this.modules["economy"].targetNumWorkers;
	var enemyStrength = this.modules["military"].measureEnemyStrength(gameState);
	var availableStrength = this.modules["military"].measureAvailableStrength();
	
	var additionalPriority = (enemyStrength - availableStrength) * 5;
	additionalPriority = Math.min(Math.max(additionalPriority, -50), 220);
	
	var advancedProportion = (availableStrength / 40) * (females/femalesTarget);
	advancedProportion = Math.min(advancedProportion, 0.7);
	
	this.priorities.citizenSoldier = (1-advancedProportion) * (150 + additionalPriority) + 1;
	this.priorities.advancedSoldier = advancedProportion * (150 + additionalPriority) + 1;
	
	if (females/femalesTarget > 0.7){
		this.priorities.defenceBuilding = 70;
	}
	Engine.ProfileStop();
};

// TODO: Remove override when the whole AI state is serialised
QBotAI.prototype.Deserialize = function(data)
{
	BaseAI.prototype.Deserialize.call(this, data);
};

// Override the default serializer
QBotAI.prototype.Serialize = function()
{
	var ret = BaseAI.prototype.Serialize.call(this);
	ret._entityMetadata = {};
	return ret;
};

function debug(output){
	if (Config.debug){
		if (typeof output === "string"){
			warn(output);
		}else{
			warn(uneval(output));
		}
	}
}

function copyPrototype(descendant, parent) {
    var sConstructor = parent.toString();
    var aMatch = sConstructor.match( /\s*function (.*)\(/ );
    if ( aMatch != null ) { descendant.prototype[aMatch[1]] = parent; }
    for (var m in parent.prototype) {
        descendant.prototype[m] = parent.prototype[m];
    }
}
