function QBotAI(settings) {
	BaseAI.call(this, settings);

	this.turn = 0;

	this.playedTurn = 0;

	this.modules = {
			"economy": new EconomyManager(), 
			"military": new MilitaryAttackManager()
	};

	// this.queues can only be modified by the queue manager or things will go awry.
	this.queues = {
		house : new Queue(),
		citizenSoldier : new Queue(),
		villager : new Queue(),
		economicBuilding : new Queue(),
		dropsites : new Queue(),
		field : new Queue(),
		militaryBuilding : new Queue(),
		defenceBuilding : new Queue(),
		civilCentre: new Queue(),
		majorTech: new Queue(),
		minorTech: new Queue()
	};
	
	this.productionQueues = [];
	
	this.priorities = Config.priorities;
	
	this.queueManager = new QueueManager(this.queues, this.priorities);
	
	this.firstTime = true;

	this.savedEvents = [];
	
	this.waterMap = false;
	
	this.defcon = 5;
	this.defconChangeTime = -10000000;
}

QBotAI.prototype = new BaseAI();

// Bit of a hack: I run the pathfinder early, before the map apears, to avoid a sometimes substantial lag right at the start.
QBotAI.prototype.InitShared = function(gameState, sharedScript) {
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
}

//Some modules need the gameState to fully initialise
QBotAI.prototype.runInit = function(gameState, events){

	this.chooseRandomStrategy();

	for (var i in this.modules){
		if (this.modules[i].init){
			this.modules[i].init(gameState, events);
		}
	}
	debug ("inited");
	this.timer = new Timer();
	
	
	var ents = gameState.getOwnEntities();
	var myKeyEntities = gameState.getOwnEntities().filter(function(ent) {
														  return ent.hasClass("CivCentre");
														  });
	
	if (myKeyEntities.length == 0){
		myKeyEntities = gameState.getOwnEntities();
	}
	
	// disband the walls themselves
	if (gameState.playerData.civ == "iber") {
		gameState.getOwnEntities().filter(function(ent) { //}){
										  if (ent.hasClass("StoneWall") && !ent.hasClass("Tower"))
										  ent.destroy();
										  });
	}
	
	var filter = Filters.byClass("CivCentre");
	var enemyKeyEntities = gameState.getEnemyEntities().filter(filter);
	
	if (enemyKeyEntities.length == 0){
		enemyKeyEntities = gameState.getEnemyEntities();
	}
	
	//this.accessibility = new Accessibility(gameState, myKeyEntities.toEntityArray()[0].position());
	
	this.myIndex = this.accessibility.getAccessValue(myKeyEntities.toEntityArray()[0].position());
	
	if (enemyKeyEntities.length == 0)
		return;
		
	this.templateManager = new TemplateManager(gameState);
};

QBotAI.prototype.OnUpdate = function(sharedScript) {
	if (this.gameFinished){
		return;
	}
	
	if (this.events.length > 0){
		this.savedEvents = this.savedEvents.concat(this.events);
	}
	
	
	
	// Run the update every n turns, offset depending on player ID to balance the load
	// this also means that init at turn 0 always happen and is never run in parallel to the first played turn so I use an else if.
	if (this.turn == 0) {
		
		//Engine.DumpImage("terrain.png", this.accessibility.map, this.accessibility.width,this.accessibility.height,255)
		//Engine.DumpImage("Access.png", this.accessibility.passMap, this.accessibility.width,this.accessibility.height,this.accessibility.regionID+1)

		var gameState = sharedScript.gameState[PlayerID];
		gameState.ai = this;
		
		this.runInit(gameState, this.savedEvents);
		
		// Delete creation events
		delete this.savedEvents;
		this.savedEvents = [];
	} else if ((this.turn + this.player) % 10 == 0) {
		
		Engine.ProfileStart("Aegis bot");
		
		this.playedTurn++;
		
		var gameState = sharedScript.gameState[PlayerID];
		gameState.ai = this;
		
		if (gameState.getOwnEntities().length === 0){
			Engine.ProfileStop();
			return; // With no entities to control the AI cannot do anything 
		}

		if (this.pathInfo !== undefined)
		{
			var pos = [this.pathInfo.mkeyPos[0] + 150*Math.cos(this.pathInfo.angle),this.pathInfo.mkeyPos[1] + 150*Math.sin(this.pathInfo.angle)];
			var path = this.pathFinder.getPath(this.pathInfo.ekeyPos, pos, 6, 5);// uncomment for debug:*/, 300000, gameState);
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
					this.waterMap = true;
				}
				delete this.pathFinder;
				delete this.pathInfo;
			}
		}
		
		// try going up phases.
		if (gameState.canResearch("phase_town",true) && gameState.getTimeElapsed() > (Config.Economy.townPhase*1000)
			&& gameState.findResearchers("phase_town",true).length != 0 && this.queues.majorTech.totalLength() === 0) {
			this.queues.majorTech.addItem(new ResearchPlan(gameState, "phase_town",true));	// we rush the town phase.
			debug ("Trying to reach town phase");
		} else if (gameState.canResearch("phase_city_generic",true) && gameState.getTimeElapsed() > (Config.Economy.cityPhase*1000)
				&& gameState.getOwnEntitiesByRole("worker").length > 85
				&& gameState.findResearchers("phase_city_generic", true).length != 0 && this.queues.majorTech.totalLength() === 0) {
			debug ("Trying to reach city phase");
			this.queues.majorTech.addItem(new ResearchPlan(gameState, "phase_city_generic"));
		}

		
		// defcon cooldown
		if (this.defcon < 5 && gameState.timeSinceDefconChange() > 20000)
		{
			this.defcon++;
			debug ("updefconing to " +this.defcon);
			if (this.defcon >= 4 && this.modules.military.hasGarrisonedFemales)
				this.modules.military.ungarrisonAll(gameState);
		}
		
		for (var i in this.modules){
			this.modules[i].update(gameState, this.queues, this.savedEvents);
		}
		
		this.queueManager.update(gameState);
		
		/*
		if (this.playedTurn % 80 === 0)
		{
			// some debug informations about units.
			var units = gameState.getOwnEntities();
			for (var i in units._entities)
			{
				var ent = units._entities[i];
				debug ("Unit " + ent.id() + " is a " + ent._templateName);
				if (sharedScript._entityMetadata[PlayerID][ent.id()])
				{
					var metadata = sharedScript._entityMetadata[PlayerID][ent.id()];
					for (j in metadata)
					{
						debug ("Metadata " + j);
						if (typeof(metadata[j]) == "object")
							warn ("Object");
						else if (typeof(metadata[j]) == undefined)
							warn ("Undefined");
						else
							warn(metadata[j]);
					}
				}
			}
		}*/

			
		//if (this.playedTurn % 15 === 0)
		//	this.queueManager.printQueues(gameState);
		
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

QBotAI.prototype.chooseRandomStrategy = function()
{
	// deactivated for now.
	this.strategy = "normal";
	// rarely and if we can assume it's not a water map.
	if (!this.pathInfo.needboat && 0)//Math.random() < 0.2 && Config.difficulty == 2)
	{
		this.strategy = "rush";
		// going to rush.
		this.modules.economy.targetNumWorkers = 0;
		Config.Economy.townPhase = 480;
		Config.Economy.cityPhase = 900;
		Config.Economy.farmsteadStartTime = 600;
		Config.Economy.femaleRatio = 0;	// raise it since we'll want to rush age 2.
	}
};

// TODO: Remove override when the whole AI state is serialised
QBotAI.prototype.Deserialize = function(data)
{
	BaseAI.prototype.Deserialize.call(this, data);
};

// Override the default serializer
QBotAI.prototype.Serialize = function()
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
