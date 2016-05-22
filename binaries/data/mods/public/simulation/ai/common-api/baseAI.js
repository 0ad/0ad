var PlayerID = -1;

var API3 = (function() {

var m = {};

m.BaseAI = function(settings)
{
	if (!settings)
		return;

	this.player = settings.player;

	// played turn, in case you don't want the AI to play every turn.
	this.turn = 0;
};

//Return a simple object (using no classes etc) that will be serialized into saved games
m.BaseAI.prototype.Serialize = function()
{
	// TODO: ought to get the AI script subclass to serialize its own state
	// TODO: actually this is part of a larger reflection on wether AIs should or not.
	return {};
};

//Called after the constructor when loading a saved game, with 'data' being
//whatever Serialize() returned
m.BaseAI.prototype.Deserialize = function(data, sharedScript)
{
	// TODO: ought to get the AI script subclass to deserialize its own state
	// TODO: actually this is part of a larger reflection on wether AIs should or not.
	this.isDeserialized = true;
};

m.BaseAI.prototype.Init = function(state, playerID, sharedAI)
{
	PlayerID = playerID;
	// define some references
	this.entities = sharedAI.entities;
	this.templates = sharedAI.templates;
	this.passabilityClasses = sharedAI.passabilityClasses;
	this.passabilityMap = sharedAI.passabilityMap;
	this.territoryMap = sharedAI.territoryMap;
	this.accessibility = sharedAI.accessibility;
	this.terrainAnalyzer = sharedAI.terrainAnalyzer;
	
	this.gameState = sharedAI.gameState[this.player];
	this.gameState.ai = this;
	this.sharedScript = sharedAI;
	
	this.timeElapsed = sharedAI.timeElapsed;

	this.CustomInit(this.gameState, this.sharedScript);
};

m.BaseAI.prototype.CustomInit = function()
{	// AIs override this function
};

m.BaseAI.prototype.HandleMessage = function(state, playerID, sharedAI)
{
	PlayerID = playerID;
	this.events = sharedAI.events;
	this.passabilityMap = sharedAI.passabilityMap;
	this.territoryMap = sharedAI.territoryMap;

	if (this.isDeserialized)
	{
		this.Init(state, playerID, sharedAI);
		this.isDeserialized = false;
	}
	this.OnUpdate(sharedAI);
};

m.BaseAI.prototype.OnUpdate = function()
{	// AIs override this function
};

m.BaseAI.prototype.chat = function(message)
{
	Engine.PostCommand(PlayerID,{"type": "aichat", "message": message});
};

return m;

}());

