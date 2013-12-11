var PlayerID = -1;

function BaseAI(settings)
{
	if (!settings)
		return;

	this.player = settings.player;
	PlayerID = this.player;

	// played turn, in case you don't want the AI to play every turn.
	this.turn = 0;
}

//Return a simple object (using no classes etc) that will be serialized into saved games
BaseAI.prototype.Serialize = function()
{
	// TODO: ought to get the AI script subclass to serialize its own state
	// TODO: actually this is part of a larger reflection on wether AIs should or not.
	return {};
};

//Called after the constructor when loading a saved game, with 'data' being
//whatever Serialize() returned
BaseAI.prototype.Deserialize = function(data, sharedScript)
{
	// TODO: ought to get the AI script subclass to deserialize its own state
	// TODO: actually this is part of a larger reflection on wether AIs should or not.
	this.isDeserialized = true;
};

BaseAI.prototype.Init = function(state, sharedAI)
{
	// define some references
	this.entities = sharedAI.entities;
	this.templates = sharedAI.templates;
	this.passabilityClasses = sharedAI.passabilityClasses;
	this.passabilityMap = sharedAI.passabilityMap;
	this.territoryMap = sharedAI.territoryMap;
	this.accessibility = sharedAI.accessibility;
	this.terrainAnalyzer = sharedAI.terrainAnalyzer;
	
	this.techModifications = sharedAI._techModifications[this.player];
	this.playerData = sharedAI.playersData[this.player];
	
	this.gameState = sharedAI.gameState[PlayerID];
	this.gameState.ai = this;
	this.sharedScript = sharedAI;
	
	this.timeElapsed = sharedAI.timeElapsed;

	this.CustomInit(this.gameState, this.sharedScript);
}

BaseAI.prototype.CustomInit = function()
{	// AIs override this function
};

BaseAI.prototype.HandleMessage = function(state, sharedAI)
{
	this.events = sharedAI.events;
	
	if (this.isDeserialized && this.turn !== 0)
	{
		this.isDeserialized = false;
		this.Init(state, sharedAI);
		warn("AIs don't work completely with saved games yet. You may run into idle units and unused buildings.");
	} else if (this.isDeserialized)
		return;
	this.OnUpdate(sharedAI);
};

BaseAI.prototype.OnUpdate = function()
{	// AIs override this function
};

BaseAI.prototype.chat = function(message)
{
	Engine.PostCommand({"type": "chat", "message": message});
};
BaseAI.prototype.chatTeam = function(message)
{
	Engine.PostCommand({"type": "chat", "message": "/team " +message});
};
BaseAI.prototype.chatEnemies = function(message)
{
	Engine.PostCommand({"type": "chat", "message": "/enemy " +message});
};

