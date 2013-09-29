// "local" global variables for stuffs that will need a unique ID
// Note that since order of loading is alphabetic, this means this file must go before any other file using them.
var uniqueIDBOPlans = 0;	// training/building/research plans
var uniqueIDBases = 1;	// base manager ID. Starts at one because "0" means "no base" on the map
var uniqueIDTPlans = 1;	// transport plans. starts at 1 because 0 might be used as none.

function AegisBot(settings) {
	BaseAI.call(this, settings);

	Config.updateDifficulty(settings.difficulty);
	
	this.turn = 0;

	this.playedTurn = 0;
		
	this.priorities = Config.priorities;

	// this.queues can only be modified by the queue manager or things will go awry.
	this.queues = {};
	for (i in this.priorities)
		this.queues[i] = new Queue();

	this.queueManager = new QueueManager(this.queues, this.priorities);

	this.HQ = new HQ();

	this.firstTime = true;

	this.savedEvents = [];
	
	this.defcon = 5;
	this.defconChangeTime = -10000000;
}

AegisBot.prototype = new BaseAI();

AegisBot.prototype.InitShared = function(gameState, sharedScript) {
	
	this.HQ.init(gameState,sharedScript.events,this.queues);
	debug ("Initialized with the difficulty " + Config.difficulty);

	var ents = gameState.getEntities().filter(Filters.byOwner(PlayerID));
	var myKeyEntities = ents.filter(function(ent) {
			return ent.hasClass("CivCentre");
	});

	if (myKeyEntities.length == 0){
		myKeyEntities = gameState.getEntities().filter(Filters.byOwner(PlayerID));
	}
	
	var filter = Filters.byClass("CivCentre");
	var enemyKeyEntities = gameState.getEntities().filter(Filters.not(Filters.byOwner(PlayerID))).filter(filter);
	
	if (enemyKeyEntities.length == 0){
		enemyKeyEntities = gameState.getEntities().filter(Filters.not(Filters.byOwner(PlayerID)));
	}

	this.myIndex = this.accessibility.getAccessValue(myKeyEntities.toEntityArray()[0].position());
	
	this.pathFinder = new aStarPath(gameState, false, true);
	this.pathsToMe = [];
	this.pathInfo = { "angle" : 0, "needboat" : true, "mkeyPos" : myKeyEntities.toEntityArray()[0].position(), "ekeyPos" : enemyKeyEntities.toEntityArray()[0].position() };
	
	// First path has a sampling of 3, which ensures we'll get at least one path even on Acropolis. The others are 6 so might fail.
	var pos = [this.pathInfo.mkeyPos[0] + 150*Math.cos(this.pathInfo.angle),this.pathInfo.mkeyPos[1] + 150*Math.sin(this.pathInfo.angle)];
	var path = this.pathFinder.getPath(this.pathInfo.ekeyPos, pos, 2, 2);// uncomment for debug:*/, 300000, gameState);

	//Engine.DumpImage("initialPath" + PlayerID + ".png", this.pathFinder.TotorMap.map, this.pathFinder.TotorMap.width,this.pathFinder.TotorMap.height,255);
	
	if (path !== undefined && path[1] !== undefined && path[1] == false) {
		// path is viable and doesn't require boating.
		// blackzone the last two waypoints.
		this.pathFinder.markImpassableArea(path[0][0][0],path[0][0][1],20);
		this.pathsToMe.push(path[0][0][0]);
		this.pathInfo.needboat = false;
	}

	this.pathInfo.angle += Math.PI/3.0;

	this.chooseRandomStrategy();
}

AegisBot.prototype.OnUpdate = function(sharedScript) {
	if (this.gameFinished){
		return;
	}
	
	if (this.events.length > 0 && this.turn !== 0){
		this.savedEvents = this.savedEvents.concat(this.events);
	}
	
	
	// Run the update every n turns, offset depending on player ID to balance the load
	if ((this.turn + this.player) % 8 == 5) {
		
		Engine.ProfileStart("Aegis bot (player " + this.player +")");
		
		this.playedTurn++;
		
		if (this.gameState.getOwnEntities().length === 0){
			Engine.ProfileStop();
			return; // With no entities to control the AI cannot do anything 
		}

		if (this.pathInfo !== undefined)
		{
			var pos = [this.pathInfo.mkeyPos[0] + 150*Math.cos(this.pathInfo.angle),this.pathInfo.mkeyPos[1] + 150*Math.sin(this.pathInfo.angle)];
			var path = this.pathFinder.getPath(this.pathInfo.ekeyPos, pos, 6, 5);// uncomment for debug:*/, 300000, this.gameState);
			if (path !== undefined && path[1] !== undefined && path[1] == false) {
				// path is viable and doesn't require boating.
				// blackzone the last two waypoints.
				this.pathFinder.markImpassableArea(path[0][0][0],path[0][0][1],20);
				this.pathsToMe.push(path[0][0][0]);
				this.pathInfo.needboat = false;
			}
			
			this.pathInfo.angle += Math.PI/3.0;
			
			if (this.pathInfo.angle > Math.PI*2.0)
			{
				if (this.pathInfo.needboat)
				{
					debug ("Assuming this is a water map");
					this.HQ.waterMap = true;
				}
				delete this.pathFinder;
				delete this.pathInfo;
			}
		}
		
		var townPhase = this.gameState.townPhase();
		var cityPhase = this.gameState.cityPhase();
		// try going up phases.
		// TODO: softcode this.
		if (this.gameState.canResearch(townPhase,true) && this.gameState.getTimeElapsed() > (Config.Economy.townPhase*1000) && this.gameState.getPopulation() > 40
			&& this.gameState.findResearchers(townPhase,true).length != 0 && this.queues.majorTech.length() === 0
			&& this.gameState.getOwnEntities().filter(Filters.byClass("Village")).length > 5)
		{
			this.queueManager.pauseQueue("villager", true);
			this.queueManager.pauseQueue("citizenSoldier", true);
			this.queueManager.pauseQueue("house", true);
			this.queues.majorTech.addItem(new ResearchPlan(this.gameState, townPhase,0,-1,true));	// we rush the town phase.
			debug ("Trying to reach town phase");
		}
		else if (this.gameState.canResearch(cityPhase,true) && this.gameState.getTimeElapsed() > (Config.Economy.cityPhase*1000)
				&& this.gameState.getOwnEntitiesByRole("worker").length > 85
				&& this.gameState.findResearchers(cityPhase, true).length != 0 && this.queues.majorTech.length() === 0) {
			debug ("Trying to reach city phase");
			this.queues.majorTech.addItem(new ResearchPlan(this.gameState, cityPhase));
		}
		// defcon cooldown
		if (this.defcon < 5 && this.gameState.timeSinceDefconChange() > 20000)
		{
			this.defcon++;
			debug ("updefconing to " +this.defcon);
			if (this.defcon >= 4 && this.HQ.hasGarrisonedFemales)
				this.HQ.ungarrisonAll(this.gameState);
		}
		
		this.HQ.update(this.gameState, this.queues, this.savedEvents);

		this.queueManager.update(this.gameState);

		/*
		 // Use this to debug informations about the metadata.
		if (this.playedTurn % 10 === 0)
		{
			// some debug informations about units.
			var units = this.gameState.getOwnEntities();
			for (var i in units._entities)
			{
				var ent = units._entities[i];
				if (!ent.isIdle())
					continue;
				warn ("Unit " + ent.id() + " is a " + ent._templateName);
				if (sharedScript._entityMetadata[PlayerID][ent.id()])
				{
					var metadata = sharedScript._entityMetadata[PlayerID][ent.id()];
					for (var j in metadata)
					{
						warn ("Metadata " + j);
						if (typeof(metadata[j]) == "object")
							warn ("Object");
						else if (typeof(metadata[j]) == undefined)
							warn ("Undefined");
						else
							warn(uneval(metadata[j]));
					}
				}
			}
		}*/

			
		//if (this.playedTurn % 5 === 0)
		//	this.queueManager.printQueues(this.gameState);
		
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

AegisBot.prototype.chooseRandomStrategy = function()
{
	// deactivated for now.
	this.strategy = "normal";
	// rarely and if we can assume it's not a water map.
	if (!this.pathInfo.needboat && 0)//Math.random() < 0.2 && Config.difficulty == 2)
	{
		this.strategy = "rush";
		// going to rush.
		this.HQ.targetNumWorkers = 0;
		Config.Economy.townPhase = 480;
		Config.Economy.cityPhase = 900;
		Config.Economy.farmsteadStartTime = 600;
		Config.Economy.femaleRatio = 0;	// raise it since we'll want to rush age 2.
	}
};

// TODO: Remove override when the whole AI state is serialised
// TODO: this currently is very much equivalent to "rungamestateinit" with a few hacks. Should deserialize/serialize properly someday.
AegisBot.prototype.Deserialize = function(data, sharedScript)
{
	BaseAI.prototype.Deserialize.call(this, data);
	
	var ents = sharedScript.entities.filter(Filters.byOwner(PlayerID));
	var myKeyEntities = ents.filter(function(ent) {
		return ent.hasClass("CivCentre");
	});
	
	if (myKeyEntities.length == 0){
		myKeyEntities = sharedScript.entities.filter(Filters.byOwner(PlayerID));
	}
	
	var filter = Filters.byClass("CivCentre");
	var enemyKeyEntities = sharedScript.entities.filter(Filters.not(Filters.byOwner(PlayerID))).filter(filter);
	
	if (enemyKeyEntities.length == 0){
		enemyKeyEntities = sharedScript.entities.filter(Filters.not(Filters.byOwner(PlayerID)));
	}
	
	this.terrainAnalyzer = sharedScript.terrainAnalyzer;
	this.passabilityMap = sharedScript.passabilityMap;

	var fakeState = { "ai" : this, "sharedScript" : sharedScript };
	this.pathFinder = new aStarPath(fakeState, false, true);
	this.pathsToMe = [];
	this.pathInfo = { "angle" : 0, "needboat" : true, "mkeyPos" : myKeyEntities.toEntityArray()[0].position(), "ekeyPos" : enemyKeyEntities.toEntityArray()[0].position() };
	
	// First path has a sampling of 3, which ensures we'll get at least one path even on Acropolis. The others are 6 so might fail.
	var pos = [this.pathInfo.mkeyPos[0] + 150*Math.cos(this.pathInfo.angle),this.pathInfo.mkeyPos[1] + 150*Math.sin(this.pathInfo.angle)];
	var path = this.pathFinder.getPath(this.pathInfo.ekeyPos, pos, 2, 2);
	
	if (path !== undefined && path[1] !== undefined && path[1] == false) {
		// path is viable and doesn't require boating.
		// blackzone the last two waypoints.
		this.pathFinder.markImpassableArea(path[0][0][0],path[0][0][1],20);
		this.pathsToMe.push(path[0][0][0]);
		this.pathInfo.needboat = false;
	}
	this.pathInfo.angle += Math.PI/3.0;
};

// Override the default serializer
AegisBot.prototype.Serialize = function()
{
	//var ret = BaseAI.prototype.Serialize.call(this);
	return {};
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
