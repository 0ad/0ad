function BaseAI(settings)
{
	if (!settings)
		return;

	// Make some properties non-enumerable, so they won't be serialised
	Object.defineProperty(this, "_player", {value: settings.player, enumerable: false});
	Object.defineProperty(this, "_templates", {value: settings.templates, enumerable: false});

	this._entityMetadata = {};
}

BaseAI.prototype.HandleMessage = function(state)
{
	if (!this._rawEntities)
		this._rawEntities = state.entities;
	else
		this.ApplyEntitiesDelta(state);

	this.entities = new EntityCollection(this, this._rawEntities);
	this.player = this._player;
	this.playerData = state.players[this._player];
	this.templates = this._templates;
	this.timeElapsed = state.timeElapsed;

	this.OnUpdate();

	// Clean up temporary properties, so they don't disturb the serializer
	delete this.entities;
	delete this.player;
	delete this.playerData;
	delete this.templates;
	delete this.timeElapsed;
};

BaseAI.prototype.ApplyEntitiesDelta = function(state)
{
	for each (var evt in state.events)
	{
		if (evt.type == "Create")
		{
			this._rawEntities[evt.msg.entity] = {};
		}
		else if (evt.type == "Destroy")
		{
			delete this._rawEntities[evt.msg.entity];
			delete this._entityMetadata[evt.msg.entity];
		}
		else if (evt.type == "TrainingFinished")
		{
			for each (var ent in evt.msg.entities)
			{
				this._entityMetadata[ent] = evt.msg.metadata;
			}
		}
	}

	for (var id in state.entities)
	{
		var changes = state.entities[id];
		for (var prop in changes)
			this._rawEntities[id][prop] = changes[prop];
	}
};

BaseAI.prototype.OnUpdate = function(state)
{
};

BaseAI.prototype.chat = function(message)
{
	Engine.PostCommand({"type": "chat", "message": message});
};
