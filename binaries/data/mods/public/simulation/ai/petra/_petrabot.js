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
	this.elapsedTime = 0;

	this.uniqueIDs = {
		"armies": 0,
		"bases": 1,	// base manager ID starts at one because "0" means "no base" on the map
		"plans": 0,	// training/building/research plans
		"transports": 1	// transport plans start at 1 because 0 might be used as none
	}

	this.Config = new m.Config(settings.difficulty);

	this.savedEvents = {};
};

m.PetraBot.prototype = new API3.BaseAI();

m.PetraBot.prototype.CustomInit = function(gameState, sharedScript)
{
	this.Config.setConfig(gameState);

	// this.queues can only be modified by the queue manager or things will go awry.
	this.queues = {};
	for (var i in this.Config.priorities)
		this.queues[i] = new m.Queue();

	this.queueManager = new m.QueueManager(this.Config, this.queues);

	this.HQ = new m.HQ(this.Config);

	var myKeyEntities = gameState.getOwnEntities().filter(API3.Filters.byClass("CivCentre"));
	if (myKeyEntities.length == 0)
		myKeyEntities = gameState.getOwnEntities();

	this.myIndex = this.accessibility.getAccessValue(myKeyEntities.toEntityArray()[0].position());
	
	this.HQ.init(gameState, this.queues);
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
	this.elapsedTime = this.gameState.getTimeElapsed() / 1000;
	if ((this.turn + this.player) % 8 == 5)
	{		
		Engine.ProfileStart("PetraBot bot (player " + this.player +")");

		this.playedTurn++;

		if (this.gameState.getOwnEntities().length === 0)
		{
			Engine.ProfileStop();
			return; // With no entities to control the AI cannot do anything 
		}

		this.HQ.update(this.gameState, this.queues, this.savedEvents);

		this.queueManager.update(this.gameState);
		
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

/*m.PetraBot.prototype.Deserialize = function(data, sharedScript)
{
};

// Override the default serializer
PetraBot.prototype.Serialize = function()
{
	return {};
};*/

return m;
}());
