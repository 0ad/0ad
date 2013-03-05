// Shared script handling templates and basic terrain analysis
function SharedScript(settings)
{
	if (!settings)
		return;
	
	// Make some properties non-enumerable, so they won't be serialised
	Object.defineProperty(this, "_players", {value: settings.players, enumerable: false});
	Object.defineProperty(this, "_templates", {value: settings.templates, enumerable: false});
	Object.defineProperty(this, "_derivedTemplates", {value: {}, enumerable: false});
	Object.defineProperty(this, "_techTemplates", {value: settings.techTemplates, enumerable: false});
		
	this._entityMetadata = {};
	for (i in this._players)
		this._entityMetadata[this._players[i]] = {};

	// always create for 8 + gaia players, since _players isn't aware of the human.
	this._techModifications = { 0 : {}, 1 : {}, 2 : {}, 3 : {}, 4 : {}, 5 : {}, 6 : {}, 7 : {}, 8 : {} };
	
	// array of entity collections
	this._entityCollections = [];
	// each name is a reference to the actual one.
	this._entityCollectionsName = {};
	this._entityCollectionsByDynProp = {};
	this._entityCollectionsUID = 0;
		
	this.turn = 0;
}

//Return a simple object (using no classes etc) that will be serialized
//into saved games
// TODO: that
SharedScript.prototype.Serialize = function()
{
};

//Called after the constructor when loading a saved game, with 'data' being
//whatever Serialize() returned
// TODO: that
SharedScript.prototype.Deserialize = function(data)
{
};

// Components that will be disabled in foundation entity templates.
// (This is a bit yucky and fragile since it's the inverse of
// CCmpTemplateManager::CopyFoundationSubset and only includes components
// that our EntityTemplate class currently uses.)
var g_FoundationForbiddenComponents = {
	"ProductionQueue": 1,
	"ResourceSupply": 1,
	"ResourceDropsite": 1,
	"GarrisonHolder": 1,
};

// Components that will be disabled in resource entity templates.
// Roughly the inverse of CCmpTemplateManager::CopyResourceSubset.
var g_ResourceForbiddenComponents = {
	"Cost": 1,
	"Decay": 1,
	"Health": 1,
	"UnitAI": 1,
	"UnitMotion": 1,
	"Vision": 1
};

SharedScript.prototype.GetTemplate = function(name)
{	
	if (this._templates[name])
		return this._templates[name];
	
	if (this._derivedTemplates[name])
		return this._derivedTemplates[name];
	
	// If this is a foundation template, construct it automatically
	if (name.indexOf("foundation|") !== -1)
	{
		var base = this.GetTemplate(name.substr(11));
		
		var foundation = {};
		for (var key in base)
			if (!g_FoundationForbiddenComponents[key])
				foundation[key] = base[key];
		
		this._derivedTemplates[name] = foundation;
		return foundation;
	}
	else if (name.indexOf("resource|") !== -1)
	{
		var base = this.GetTemplate(name.substr(9));
		
		var resource = {};
		for (var key in base)
			if (!g_ResourceForbiddenComponents[key])
				resource[key] = base[key];
		
		this._derivedTemplates[name] = resource;
		return resource;
	}
	
	error("Tried to retrieve invalid template '"+name+"'");
	return null;
};

// initialize the shared component using a given gamestate (the initial gamestate after map creation, usually)
// this is called right at the end of map generation, before you actually reach the map.
SharedScript.prototype.initWithState = function(state) {	
	this.passabilityClasses = state.passabilityClasses;
	this.passabilityMap = state.passabilityMap;

	for (o in state.players)
		this._techModifications[o] = state.players[o].techModifications;
	
	this.techModifications = this._techModifications;
	
	this._entities = {};
	for (var id in state.entities)
	{
		this._entities[id] = new Entity(this, state.entities[id]);
	}
	// entity collection updated on create/destroy event.
	this.entities = new EntityCollection(this, this._entities);
	
	// create the terrain analyzer
	this.terrainAnalyzer = new TerrainAnalysis(this, state);
	this.accessibility = new Accessibility(state, this.terrainAnalyzer);
	
	this.gameState = {};
	for (i in this._players)
	{
		this.gameState[this._players[i]] = new GameState(this,state,this._players[i]);
	}

};

// General update of the shared script, before each AI's update
// applies entity deltas, and each gamestate.
SharedScript.prototype.onUpdate = function(state)
{
	this.ApplyEntitiesDelta(state);
	
	Engine.ProfileStart("onUpdate");
	
	this.events = state.events;
	this.passabilityClasses = state.passabilityClasses;
	this.passabilityMap = state.passabilityMap;
	this.players = this._players;
	this.playersData = state.players;
	this.territoryMap = state.territoryMap;
	this.timeElapsed = state.timeElapsed;
	
	for (o in state.players)
		this._techModifications[o] = state.players[o].techModifications;

	for (i in this.gameState)
		this.gameState[i].update(this,state);

	this.terrainAnalyzer.updateMapWithEvents(this);
	
	//this.OnUpdate();

	this.turn++;
	
	Engine.ProfileStop();
};

SharedScript.prototype.ApplyEntitiesDelta = function(state)
{
	Engine.ProfileStart("Shared ApplyEntitiesDelta");
	
	for each (var evt in state.events)
	{
		if (evt.type == "Create")
		{
			if (!state.entities[evt.msg.entity])
			{
				continue; // Sometimes there are things like foundations which get destroyed too fast
			}
			this._entities[evt.msg.entity] = new Entity(this, state.entities[evt.msg.entity]);
			this.entities.addEnt(this._entities[evt.msg.entity]);

			// Update all the entity collections since the create operation affects static properties as well as dynamic
			for each (var entCollection in this._entityCollections)
			{
				entCollection.updateEnt(this._entities[evt.msg.entity]);
			}

		}
		else if (evt.type == "Destroy")
		{
			// A small warning: javascript "delete" does not actually delete, it only removes the reference in this object/
			// the "deleted" object remains in memory, and any older reference to will still reference it as if it were not "deleted".
			// Worse, they might prevent it from being garbage collected, thus making it stay alive and consuming ram needlessly.
			// So take care, and if you encounter a weird bug with deletion not appearing to work correctly, this is probably why.
			if (!this._entities[evt.msg.entity])
			{
				continue;
			}
			// The entity was destroyed but its data may still be useful, so
			// remember the entity and this AI's metadata concerning it
			evt.msg.metadata = (evt.msg.metadata || []);
			evt.msg.entityObj = (evt.msg.entityObj || this._entities[evt.msg.entity]);
			//evt.msg.metadata[this._player] = this._entityMetadata[evt.msg.entity];

			for each (var entCol in this._entityCollections)
			{
				entCol.removeEnt(this._entities[evt.msg.entity]);
			}
			this.entities.removeEnt(this._entities[evt.msg.entity]);

			delete this._entities[evt.msg.entity];
			for (i in this._players)
				delete this._entityMetadata[this._players[i]][evt.msg.entity];
		}
		else if (evt.type == "TrainingFinished")
		{
			// Apply metadata stored in training queues
			for each (var ent in evt.msg.entities)
			{
				for (key in evt.msg.metadata)
				{
					this.setMetadata(evt.msg.owner, this._entities[ent], key, evt.msg.metadata[key])
				}
			}
		}
	}
	
	for (var id in state.entities)
	{
		var changes = state.entities[id];
		
		for (var prop in changes)
		{
			this._entities[id]._entity[prop] = changes[prop];
			this.updateEntityCollections(prop, this._entities[id]);
		}
	}
	Engine.ProfileStop();
};

SharedScript.prototype.registerUpdatingEntityCollection = function(entCollection, noPush)
{
	if (!noPush) {
		this._entityCollections.push(entCollection);
	}
	entCollection.setUID(this._entityCollectionsUID);
	for each (var prop in entCollection.dynamicProperties())
	{
		this._entityCollectionsByDynProp[prop] = this._entityCollectionsByDynProp[prop] || [];
		this._entityCollectionsByDynProp[prop].push(entCollection);
	}
	this._entityCollectionsUID++;
};

SharedScript.prototype.removeUpdatingEntityCollection = function(entCollection)
{
	for (var i in this._entityCollections)
	{
		if (this._entityCollections[i].getUID() === entCollection.getUID())
		{
			this._entityCollections.splice(i, 1);
		}
	}
	
	for each (var prop in entCollection.dynamicProperties())
	{
		for (var i in this._entityCollectionsByDynProp[prop])
		{
			if (this._entityCollectionsByDynProp[prop][i].getUID() === entCollection.getUID())
			{
				this._entityCollectionsByDynProp[prop].splice(i, 1);
			}
		}
	}
};

SharedScript.prototype.updateEntityCollections = function(property, ent)
{
	if (this._entityCollectionsByDynProp[property] !== undefined)
	{
		for each (var entCollection in this._entityCollectionsByDynProp[property])
		{
			entCollection.updateEnt(ent);
		}
	}
}

SharedScript.prototype.setMetadata = function(player, ent, key, value)
{
	var metadata = this._entityMetadata[player][ent.id()];
	if (!metadata)
		metadata = this._entityMetadata[player][ent.id()] = {};
	metadata[key] = value;
	
	this.updateEntityCollections('metadata', ent);
	this.updateEntityCollections('metadata.' + key, ent);
};
SharedScript.prototype.getMetadata = function(player, ent, key)
{
	var metadata = this._entityMetadata[player][ent.id()];
	
	if (!metadata || !(key in metadata))
		return undefined;
	return metadata[key];
};

function copyPrototype(descendant, parent) {
    var sConstructor = parent.toString();
    var aMatch = sConstructor.match( /\s*function (.*)\(/ );
	if ( aMatch != null ) { descendant.prototype[aMatch[1]] = parent; }
	for (var m in parent.prototype) {
		descendant.prototype[m] = parent.prototype[m];
	}
};

