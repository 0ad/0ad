function BaseAI(settings)
{
	if (!settings)
		return;

	// Make some properties non-enumerable, so they won't be serialised
	Object.defineProperty(this, "_player", {value: settings.player, enumerable: false});
	Object.defineProperty(this, "_templates", {value: settings.templates, enumerable: false});
	Object.defineProperty(this, "_derivedTemplates", {value: {}, enumerable: false});

	this._ownEntities = {};

	this._entityMetadata = {};
}

// Components that will be disabled in foundation entity templates.
// (This is a bit yucky and fragile since it's the inverse of
// CCmpTemplateManager::CopyFoundationSubset and only includes components
// that our EntityTemplate class currently uses.)
var g_FoundationForbiddenComponents = {
	"TrainingQueue": 1,
	"ResourceSupply": 1,
	"ResourceDropsite": 1,
	"GarrisonHolder": 1,
};

BaseAI.prototype.GetTemplate = function(name)
{
	if (this._templates[name])
		return this._templates[name];

	if (this._derivedTemplates[name])
		return this._derivedTemplates[name];

	// If this is a foundation template, construct it automatically
	if (name.substr(0, 11) === "foundation|")
	{
		var base = this.GetTemplate(name.substr(11));

		var foundation = {};
		for (var key in base)
			if (!g_FoundationForbiddenComponents[key])
				foundation[key] = base[key];

		this._derivedTemplates[name] = foundation;
		return foundation;
	}

	error("Tried to retrieve invalid template '"+name+"'");
	return null;
};

BaseAI.prototype.HandleMessage = function(state)
{
	if (!this._rawEntities)
	{
		// Do a (shallow) clone of all the initial entity properties (in order
		// to copy into our own script context and minimise cross-context
		// weirdness), and remember the entities owned by our player
		this._rawEntities = {};
		for (var id in state.entities)
		{
			var ent = state.entities[id];

			this._rawEntities[id] = {};
			for (var prop in ent)
				this._rawEntities[id][prop] = ent[prop];

			if (ent.owner === this._player)
				this._ownEntities[id] = this._rawEntities[id];
		}
	}
	else
	{
		this.ApplyEntitiesDelta(state);
	}

	Engine.ProfileStart("HandleMessage setup");

	this.entities = new EntityCollection(this, this._rawEntities);
	this.player = this._player;
	this.playerData = state.players[this._player];
	this.templates = this._templates;
	this.timeElapsed = state.timeElapsed;
	this.map = state.map;
	this.passabilityClasses = state.passabilityClasses;

	Engine.ProfileStop();

	this.OnUpdate();

	// Clean up temporary properties, so they don't disturb the serializer
	delete this.entities;
	delete this.player;
	delete this.playerData;
	delete this.templates;
	delete this.timeElapsed;
	delete this.map;
	delete this.passabilityClasses;
};

BaseAI.prototype.ApplyEntitiesDelta = function(state)
{
	Engine.ProfileStart("ApplyEntitiesDelta");

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
			delete this._ownEntities[evt.msg.entity];
		}
		else if (evt.type == "TrainingFinished")
		{
			// Apply metadata stored in training queues, but only if they
			// look like they were added by us
			if (evt.msg.owner === this._player)
				for each (var ent in evt.msg.entities)
					this._entityMetadata[ent] = ShallowClone(evt.msg.metadata);
		}
	}

	for (var id in state.entities)
	{
		var changes = state.entities[id];

		if ("owner" in changes)
		{
			var wasOurs = (this._rawEntities[id].owner !== undefined
				&& this._rawEntities[id].owner === this._player);

			var isOurs = (changes.owner === this._player);

			if (wasOurs && !isOurs)
				delete this._ownEntities[id];
			else if (!wasOurs && isOurs)
				this._ownEntities[id] = this._rawEntities[id];
		}

		for (var prop in changes)
			this._rawEntities[id][prop] = changes[prop];
	}

	Engine.ProfileStop();
};

BaseAI.prototype.OnUpdate = function(state)
{
};

BaseAI.prototype.chat = function(message)
{
	Engine.PostCommand({"type": "chat", "message": message});
};
