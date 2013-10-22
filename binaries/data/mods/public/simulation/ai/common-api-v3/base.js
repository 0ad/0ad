var PlayerID = -1;

function BaseAI(settings)
{
	
	if (!settings)
		return;

	// Make some properties non-enumerable, so they won't be serialised
	// Note: currently serialization isn't really handled that way.
	Object.defineProperty(this, "_player", {value: settings.player, enumerable: false});
	PlayerID = this._player;

	this.turn = 0;
	this.timeElapsed = 0;
}

//Return a simple object (using no classes etc) that will be serialized
//into saved games
BaseAI.prototype.Serialize = function()
{
	return {};
	// TODO: ought to get the AI script subclass to serialize its own state
};

//Called after the constructor when loading a saved game, with 'data' being
//whatever Serialize() returned
BaseAI.prototype.Deserialize = function(data, sharedScript)
{
	// TODO: ought to get the AI script subclass to deserialize its own state
};

BaseAI.prototype.InitWithSharedScript = function(state, sharedAI)
{
	this.accessibility = sharedAI.accessibility;
	this.terrainAnalyzer = sharedAI.terrainAnalyzer;
	this.passabilityClasses = sharedAI.passabilityClasses;
	this.passabilityMap = sharedAI.passabilityMap;
	this.territoryMap = sharedAI.territoryMap;
	this.timeElapsed = state.timeElapsed;

	this.gameState = sharedAI.gameState[PlayerID];
	this.gameState.ai = this;
	this.sharedScript = sharedAI;
		
	this.InitShared(this.gameState, this.sharedScript);
}

BaseAI.prototype.HandleMessage = function(state, sharedAI)
{
	Engine.ProfileStart("HandleMessage setup");

	this.entities = sharedAI.entities;
	this.events = sharedAI.events;
	this.passabilityClasses = sharedAI.passabilityClasses;
	this.passabilityMap = sharedAI.passabilityMap;
	this.player = this._player;
	this.playerData = sharedAI.playersData[this._player];
	this.templates = sharedAI.templates;
	this.territoryMap = sharedAI.territoryMap;
	this.timeElapsed = sharedAI.timeElapsed;
	this.accessibility = sharedAI.accessibility;
	this.terrainAnalyzer = sharedAI.terrainAnalyzer;
	this.techModifications = sharedAI._techModifications[this._player];

	Engine.ProfileStop();
	
	this.OnUpdate(sharedAI);

	// Clean up temporary properties, so they don't disturb the serializer
	delete this.entities;
	delete this.events;
	delete this.passabilityClasses;
	delete this.passabilityMap;
	delete this.player;
	delete this.playerData;
	delete this.templates;
	delete this.territoryMap;
	delete this.timeElapsed;
};


BaseAI.prototype.OnUpdate = function()
{	// AIs override this function
	// They should do at least this.turn++;
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

BaseAI.prototype.setMetadata = function(ent, key, value)
{
	var metadata = this._entityMetadata[ent.id()];
	if (!metadata)
		metadata = this._entityMetadata[ent.id()] = {};
	metadata[key] = value;

	this.updateEntityCollections('metadata', ent);
	this.updateEntityCollections('metadata.' + key, ent);
}

BaseAI.prototype.getMetadata = function(ent, key)
{
	var metadata = this._entityMetadata[ent.id()];
	
	if (!metadata || !(key in metadata))
		return undefined;
	return metadata[key];
}
