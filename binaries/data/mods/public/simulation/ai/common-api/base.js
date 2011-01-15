function BaseAI(settings)
{
	if (!settings)
		return;

	// Make some properties non-enumerable, so they won't be serialised
	Object.defineProperty(this, "_player", {value: settings.player, enumerable: false});
	Object.defineProperty(this, "_templates", {value: settings.templates, enumerable: false});
}

BaseAI.prototype.HandleMessage = function(state)
{
	if (!this._rawEntities)
		this._rawEntities = state.entities;
	else
		this.ApplyEntitiesDelta(state);

//print("### "+uneval(state)+"\n\n");
//print("@@@ "+uneval(this._rawEntities)+"\n\n");

	this.entities = new EntityCollection(this, this._rawEntities);

	this.OnUpdate();

	// Clean up temporary properties, so they don't disturb the serializer
	delete this.entities;
};

BaseAI.prototype.ApplyEntitiesDelta = function(state)
{
	for each (var evt in state.events)
	{
		if (evt.type == "Destroy")
		{
			delete this._rawEntities[evt.msg.entity];
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
