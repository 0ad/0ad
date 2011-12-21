
function QBotAI(settings) {
	BaseAI.call(this, settings);

	this.turn = 0;

	this.modules = [ new EconomyManager(), new MilitaryAttackManager(), new HousingManager() ];

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
	
	this.priorities = {
		house : 500,
		citizenSoldier : 100,
		villager : 100,
		economicBuilding : 30,
		field: 4,
		advancedSoldier : 30,
		siege : 10,
		militaryBuilding : 50,
		defenceBuilding: 17,
		civilCentre: 1000
	};
	this.queueManager = new QueueManager(this.queues, this.priorities);
	
	this.firstTime = true;

	this.savedEvents = [];
	
	this.toUpdate = [];
}

QBotAI.prototype = new BaseAI();

//Some modules need the gameState to fully initialise
QBotAI.prototype.runInit = function(gameState){
	if (this.firstTime){
		for (var i = 0; i < this.modules.length; i++){
			if (this.modules[i].init){
				this.modules[i].init(gameState);
			}
		}

		var myCivCentres = gameState.getOwnEntities().filter(function(ent) {
			return ent.hasClass("CivCentre");
		});
		
		var filter = Filters.and(Filters.isEnemy(), Filters.byClass("CivCentre"));
		var enemyCivCentres = gameState.getEntities().filter(function(ent) {
			return ent.hasClass("CivCentre") && gameState.isEntityEnemy(ent);
		});
		
		this.accessibility = new Accessibility(gameState, myCivCentres.toEntityArray()[0].position());
		
		var pathFinder = new PathFinder(gameState);
		this.pathsToMe = pathFinder.getPaths(enemyCivCentres.toEntityArray()[0].position(), myCivCentres.toEntityArray()[0].position(), 'entryPoints');
		
		this.timer = new Timer();
		
		this.firstTime = false;
	}
};

QBotAI.prototype.registerUpdate = function(obj){
	this.toUpdate.push(obj);
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
		
		// Run these updates before the init so they don't get hammered by the initial creation
		// events at the start of the game.
		for (var i = 0; i < this.toUpdate.length; i++){
			this.toUpdate[i].update(gameState, this.savedEvents);
		}
		
		this.runInit(gameState);
		
		for (var i = 0; i < this.modules.length; i++){
			this.modules[i].update(gameState, this.queues, this.savedEvents);
		}
		
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

var debugOn = false;

function debug(output){
	if (debugOn){
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
