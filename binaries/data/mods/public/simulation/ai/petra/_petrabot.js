Engine.IncludeModule("common-api");

var PETRA = (function() {
var m = {};

// "local" global variables for stuffs that will need a unique ID
// Note that since order of loading is alphabetic, this means this file must go before any other file using them.
m.playerGlobals = [];

m.PetraBot = function PetraBot(settings)
{
	API3.BaseAI.call(this, settings);

	this.turn = 0;
	this.playedTurn = 0;

	this.Config = new m.Config();
	this.Config.updateDifficulty(settings.difficulty);	
	//this.Config.personality = settings.personality;	

	this.savedEvents = {};
	
	this.defcon = 5;
	this.defconChangeTime = -10000000;
};

m.PetraBot.prototype = new API3.BaseAI();

m.PetraBot.prototype.CustomInit = function(gameState, sharedScript)
{
	this.initPersonality();

	this.priorities = this.Config.priorities;
	// this.queues can only be modified by the queue manager or things will go awry.
	this.queues = {};
	for (var i in this.priorities)
		this.queues[i] = new m.Queue();

	this.queueManager = new m.QueueManager(this.Config, this.queues, this.priorities);

	this.HQ = new m.HQ(this.Config);
	gameState.Config = this.Config;

	m.playerGlobals[PlayerID] = {};
	m.playerGlobals[PlayerID].uniqueIDBOPlans = 0;	// training/building/research plans
	m.playerGlobals[PlayerID].uniqueIDBases = 1;	// base manager ID. Starts at one because "0" means "no base" on the map
	m.playerGlobals[PlayerID].uniqueIDTPlans = 1;	// transport plans. starts at 1 because 0 might be used as none.	
	m.playerGlobals[PlayerID].uniqueIDArmy = 0;

	this.HQ.init(gameState,this.queues);

	var filter = API3.Filters.byClass("CivCentre");
	var myKeyEntities = gameState.getOwnEntities().filter(filter);
	if (myKeyEntities.length == 0)
		myKeyEntities = gameState.getOwnEntities();
	
	var enemyKeyEntities = gameState.getEnemyEntities().filter(filter);	
	if (enemyKeyEntities.length == 0)
		enemyKeyEntities = gameState.getEnemyEntities();

	this.myIndex = this.accessibility.getAccessValue(myKeyEntities.toEntityArray()[0].position());
	
	this.pathFinder = new API3.aStarPath(gameState, false, true);
	this.pathsToMe = [];
	this.pathInfo = { "angle" : 0, "needboat" : true, "mkeyPos" : myKeyEntities.toEntityArray()[0].position(), "ekeyPos" : enemyKeyEntities.toEntityArray()[0].position() };
	
	// First path has a sampling of 3, which ensures we'll get at least one path even on Acropolis. The others are 6 so might fail.
	var pos = [this.pathInfo.mkeyPos[0] + 150*Math.cos(this.pathInfo.angle),this.pathInfo.mkeyPos[1] + 150*Math.sin(this.pathInfo.angle)];
	var path = this.pathFinder.getPath(this.pathInfo.ekeyPos, pos, 2, 2);// uncomment for debug:*/, 300000, gameState);

	//Engine.DumpImage("initialPath" + this.player + ".png", this.pathFinder.TotorMap.map, this.pathFinder.TotorMap.width,this.pathFinder.TotorMap.height,255);
	
	if (path !== undefined && path[1] !== undefined && path[1] == false) {
		// path is viable and doesn't require boating.
		// blackzone the last two waypoints.
		this.pathFinder.markImpassableArea(path[0][0][0],path[0][0][1],20);
		this.pathsToMe.push(path[0][0][0]);
		this.pathInfo.needboat = false;
	}

	this.pathInfo.angle += Math.PI/3.0;
};

m.PetraBot.prototype.OnUpdate = function(sharedScript)
{
	if (this.gameFinished)
		return;

	for (var i in this.events)
	{
		if(this.savedEvents[i] !== undefined)
			this.savedEvents[i] = this.savedEvents[i].concat(this.events[i]);
		else
			this.savedEvents[i] = this.events[i];
	}

	// Run the update every n turns, offset depending on player ID to balance the load
	if ((this.turn + this.player) % 8 == 5)
	{		
		Engine.ProfileStart("PetraBot bot (player " + this.player +")");
		
		this.playedTurn++;
		
		if (this.gameState.getOwnEntities().length === 0)
		{
			Engine.ProfileStop();
			return; // With no entities to control the AI cannot do anything 
		}

		if (this.pathInfo !== undefined)
		{
			var pos = [this.pathInfo.mkeyPos[0] + 150*Math.cos(this.pathInfo.angle),this.pathInfo.mkeyPos[1] + 150*Math.sin(this.pathInfo.angle)];
			var path = this.pathFinder.getPath(this.pathInfo.ekeyPos, pos, 6, 5);// uncomment for debug:*/, 300000, this.gameState);
			if (path !== undefined && path[1] !== undefined && path[1] == false)
			{
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
					m.debug ("Assuming this is a water map");
					this.HQ.waterMap = true;
				}
				delete this.pathFinder;
				delete this.pathInfo;
			}
		}
		
		var townPhase = this.gameState.townPhase();
		var cityPhase = this.gameState.cityPhase();
		
		// try going up phases.
		// TODO: softcode this more
		if (this.gameState.canResearch(townPhase,true) && this.gameState.getPopulation() >= this.Config.Economy.popForTown - 10
			&& this.gameState.findResearchers(townPhase,true).length != 0 && this.queues.majorTech.length() === 0)
		{
			var plan = new m.ResearchPlan(this.gameState, townPhase, true);
			plan.lastIsGo = false;
			plan.onStart = function (gameState) { gameState.ai.HQ.econState = "growth"; gameState.ai.HQ.OnTownPhase(gameState) };
			plan.isGo = function (gameState) {
				var ret = gameState.getPopulation() >= gameState.Config.Economy.popForTown;
				if (ret && !this.lastIsGo)
					this.onGo(gameState);
				else if (!ret && this.lastIsGo)
					this.onNotGo(gameState);
				this.lastIsGo = ret;
				return ret;
			};
			plan.onGo = function (gameState) { gameState.ai.HQ.econState = "townPhasing"; m.debug ("Trying to reach TownPhase"); };
			plan.onNotGo = function (gameState) { gameState.ai.HQ.econState = "growth"; };

			this.queues.majorTech.addItem(plan);
		}
		else if (this.gameState.canResearch(cityPhase,true) && this.gameState.getTimeElapsed() > (this.Config.Economy.cityPhase*1000)
				&& this.gameState.getOwnEntitiesByRole("worker", true).length > 85
				&& this.gameState.findResearchers(cityPhase, true).length != 0 && this.queues.majorTech.length() === 0
				&& this.queues.civilCentre.length() === 0)
		{
			var plan = new m.ResearchPlan(this.gameState, cityPhase, true);
			plan.onStart = function (gameState) { gameState.ai.HQ.OnCityPhase(gameState) };
			this.queues.majorTech.addItem(plan);

		}
		// defcon cooldown
		if (this.defcon < 5 && this.gameState.timeSinceDefconChange() > 20000)
		{
			this.defcon++;
			m.debug ("updefconing to " +this.defcon);
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
		var n = this.savedEvents["Create"].length % 29;
		for (var i = 0; i < n; i++)
			Math.random();
		
		for (var i in this.savedEvents)
			this.savedEvents[i] = [];

		Engine.ProfileStop();
	}
	
	this.turn++;
};

// defines our core components strategy-wise.
// TODO: the sky's the limit here.
m.PetraBot.prototype.initPersonality = function()
{
	if (this.Config.difficulty >= 2)
	{
		this.Config.personality.aggressive = Math.random();
		this.Config.personality.cooperative = Math.random();
	}

	if (this.Config.personality.aggressive > 0.7)
	{
		this.Config.Military.popForBarracks1 = 0;
		this.Config.Economy.popForTown = 70;
		this.Config.Economy.cityPhase = 900;
		this.Config.Economy.popForMarket = 80;
		this.Config.Economy.targetNumBuilders = 2;
		this.Config.Economy.femaleRatio = 0.3;
		this.Config.Defense.prudence = 0.5;
		this.Config.priorities.defenseBuilding = 60;
	}

	if (this.Config.debug == 0)
		return;
	warn(" >>>  Petra bot: personality = " + uneval(this.Config.personality));
};

/*m.PetraBot.prototype.Deserialize = function(data, sharedScript)
{
};

// Override the default serializer
PetraBot.prototype.Serialize = function()
{
	return {};
};*/

// For the moment we just use the debugging flag and the debugging function from the API.
// Maybe it will make sense in the future to separate them.
m.DebugEnabled = function()
{
	return API3.DebugEnabled;
}

m.debug = function(output)
{
	API3.debug(output);
}


return m;
}());
