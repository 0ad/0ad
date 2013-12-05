// Shared script handling templates and basic terrain analysis
function SharedScript(settings)
{
	if (!settings)
		return;
	
	this._players = settings.players;
	this._templates = settings.templates;
	this._derivedTemplates = {};
	this._techTemplates = settings.techTemplates;
		
	this._entityMetadata = {};
	for (var i in this._players)
		this._entityMetadata[this._players[i]] = {};

	// always create for 8 + gaia players, since _players isn't aware of the human.
	this._techModifications = { 0 : {}, 1 : {}, 2 : {}, 3 : {}, 4 : {}, 5 : {}, 6 : {}, 7 : {}, 8 : {} };
	
	// array of entity collections
	this._entityCollections = [];
	// each name is a reference to the actual one.
	this._entityCollectionsName = {};
	this._entityCollectionsByDynProp = {};
	this._entityCollectionsUID = 0;
	
	// A few notes about these maps. They're updated by checking for "create" and "destroy" events for all resources
	// TODO: change the map when the resource amounts change for at least stone and metal mines.
	this.resourceMaps = {}; // Contains maps showing the density of wood, stone and metal
	this.CCResourceMaps = {}; // Contains maps showing the density of wood, stone and metal, optimized for CC placement.
	// Resource maps data.
	// By how much to divide the resource amount for plotting (ie a tree having 200 wood is "4").
	this.decreaseFactor = {'wood': 50.0, 'stone': 90.0, 'metal': 90.0, 'food': 40.0};

	this.turn = 0;
}

//Return a simple object (using no classes etc) that will be serialized
//into saved games
//TODO: that
// note: we'll need to serialize much more than that before this can work.
SharedScript.prototype.Serialize = function()
{
	// serializing entities without using the class.
	var entities = [];
	for (var id in this._entities)
	{
		var ent = this._entities[id];
		entities.push( [ent._template, ent._entity, ent._templateName]);
	}
	
	// serialiazing metadata will be done by each AI on a AI basis and they shall update the shared script with that info on deserialization (using DeserializeMetadata() ).
	// TODO: this may not be the most clever method.
	return { "entities" : entities, "techModifs" : this._techModifications, "passabClasses" : this.passabilityClasses, "passabMap" : this.passabilityMap,
		"timeElapsed" : this.timeElapsed, "techTemplates" : this._techTemplates, "players": this._players};
};

// Called after the constructor when loading a saved game, with 'data' being
// whatever Serialize() returned
// todo: very not-finished. Mostly hacky, and ugly too.
SharedScript.prototype.Deserialize = function(data)
{
	this._entities = {};

	this._players = data.players;
	this._entityMetadata = {};
	for (var i in this._players)
		this._entityMetadata[this._players[i]] = {};

	this._techModifications = data.techModifs;
	this.techModifications = data.techModifs;	// needed for entities
	
	this.passabilityClasses = data.passabClasses;
	this.passabilityMap = data.passabMap;
	var dataArray = [];
	for (var i in this.passabilityMap.data)
		dataArray.push(this.passabilityMap.data[i]);

	this.passabilityMap.data = dataArray;

	// TODO: this is needlessly slow (not to mention a hack)
	// Should probably call a "init" function rather to avoid this and serialize the terrainanalyzer state.
	var fakeState = { "passabilityClasses" : this.passabilityClasses, "passabilityMap":this.passabilityMap };
	this.terrainAnalyzer = new TerrainAnalysis (this, fakeState);
	this.accessibility = new Accessibility(fakeState, this.terrainAnalyzer);
	
	this._techTemplates = data.techTemplates;
	this.timeElapsed = data.timeElapsed;

	// deserializing entities;
	for (var i in data.entities)
	{
		var entData = data.entities[i];
		entData[1].template = entData[2];
		this._entities[entData[1].id] = new Entity(this, entData[1]);
	}
	// entity collection updated on create/destroy event.
	this.entities = new EntityCollection(this, this._entities);
	
	//deserializing game states.
	fakeState["timeElapsed"] = this.timeElapsed;
	fakeState["players"] = this._players;
	
	this.gameState = {};
	for (var i in this._players)
		this.gameState[this._players[i]] = new GameState(this, fakeState, this._players[i]);
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
	this.events = state.events;
	this.passabilityClasses = state.passabilityClasses;
	this.passabilityMap = state.passabilityMap;
	this.players = this._players;
	this.playersData = state.players;
	this.territoryMap = state.territoryMap;
	this.timeElapsed = state.timeElapsed;

	for (var o in state.players)
		this._techModifications[o] = state.players[o].techModifications;
	
	this._entities = {};

	for (var id in state.entities)
	{
		this._entities[id] = new Entity(this, state.entities[id]);
	}
	// entity collection updated on create/destroy event.
	this.entities = new EntityCollection(this, this._entities);
	
	// create the terrain analyzer
	this.terrainAnalyzer = new TerrainAnalysis();
	this.terrainAnalyzer.init(this, state);
	this.accessibility = new Accessibility();
	this.accessibility.init(state, this.terrainAnalyzer);
	
	// defined in TerrainAnalysis.js
	this.createResourceMaps(this);

	this.gameState = {};
	for (var i in this._players)
	{
		this.gameState[this._players[i]] = new GameState();
		this.gameState[this._players[i]].init(this,state,this._players[i]);
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
	
	for (var o in state.players)
		this._techModifications[o] = state.players[o].techModifications;

	for (var i in this.gameState)
		this.gameState[i].update(this,state);

	this.updateResourceMaps(this, this.events);
	this.terrainAnalyzer.updateMapWithEvents(this);
	
	//this.OnUpdate();

	this.turn++;
	
	Engine.ProfileStop();
};

SharedScript.prototype.ApplyEntitiesDelta = function(state)
{
	Engine.ProfileStart("Shared ApplyEntitiesDelta");

	var foundationFinished = {};
	
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
			for (var entCollection in this._entityCollections)
			{
				this._entityCollections[entCollection].updateEnt(this._entities[evt.msg.entity]);
			}
		}
		else if (evt.type == "Destroy")
		{
			// A small warning: javascript "delete" does not actually delete, it only removes the reference in this object.
			// the "deleted" object remains in memory, and any older reference to it will still reference it as if it were not "deleted".
			// Worse, they might prevent it from being garbage collected, thus making it stay alive and consuming ram needlessly.
			// So take care, and if you encounter a weird bug with deletion not appearing to work correctly, this is probably why.
			
			if (!this._entities[evt.msg.entity])
				continue;
			
			if (foundationFinished[evt.msg.entity])
				evt.msg["SuccessfulFoundation"] = true;
			
			// The entity was destroyed but its data may still be useful, so
			// remember the entity and this AI's metadata concerning it
			evt.msg.metadata = {};
			evt.msg.entityObj = (evt.msg.entityObj || this._entities[evt.msg.entity]);
			for (var i in this._players)
				evt.msg.metadata[this._players[i]] = this._entityMetadata[this._players[i]][evt.msg.entity];

			for each (var entCol in this._entityCollections)
			{
				entCol.removeEnt(this._entities[evt.msg.entity]);
			}
			this.entities.removeEnt(this._entities[evt.msg.entity]);

			delete this._entities[evt.msg.entity];
			for (var i in this._players)
				delete this._entityMetadata[this._players[i]][evt.msg.entity];
		}
		else if (evt.type == "EntityRenamed")
		{
			// Switch the metadata
			for (var i in this._players)
			{
				this._entityMetadata[this._players[i]][evt.msg.newentity] = this._entityMetadata[this._players[i]][evt.msg.entity];
				this._entityMetadata[this._players[i]][evt.msg.entity] = {};
			}
		}
		else if (evt.type == "TrainingFinished")
		{
			// Apply metadata stored in training queues
			for each (var ent in evt.msg.entities)
			{
				for (var key in evt.msg.metadata)
				{
					this.setMetadata(evt.msg.owner, this._entities[ent], key, evt.msg.metadata[key])
				}
			}
		}
		else if (evt.type == "ConstructionFinished")
		{
			// we can rely on this being before the "Destroy" command as this is the order defined by FOundation.js
			// we'll move metadata.
			if (!this._entities[evt.msg.entity])
				continue;
			var ent = this._entities[evt.msg.entity];
			var newEnt = this._entities[evt.msg.newentity];
			if (this._entityMetadata[ent.owner()] && this._entityMetadata[ent.owner()][evt.msg.entity] !== undefined)
				for (var key in this._entityMetadata[ent.owner()][evt.msg.entity])
				{
					this.setMetadata(ent.owner(), newEnt, key, this._entityMetadata[ent.owner()][evt.msg.entity][key])
				}
			foundationFinished[evt.msg.entity] = true;
		}
		else if (evt.type == "AIMetadata")
		{
			if (!this._entities[evt.msg.id])
				continue;	// might happen in some rare cases of foundations getting destroyed, perhaps.
			// Apply metadata (here for buildings for example)
			for (var key in evt.msg.metadata)
			{
				this.setMetadata(evt.msg.owner, this._entities[evt.msg.id], key, evt.msg.metadata[key])
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
		for (var entCollectionid in this._entityCollectionsByDynProp[property])
		{
			this._entityCollectionsByDynProp[property][entCollectionid].updateEnt(ent);
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
SharedScript.prototype.deleteMetadata = function(player, ent, key)
{
	var metadata = this._entityMetadata[player][ent.id()];
	
	if (!metadata || !(key in metadata))
		return true;
	metadata[key] = undefined;
	delete metadata[key];
	return true;
};

function copyPrototype(descendant, parent) {
    var sConstructor = parent.toString();
    var aMatch = sConstructor.match( /\s*function (.*)\(/ );
	if ( aMatch != null ) { descendant.prototype[aMatch[1]] = parent; }
	for (var m in parent.prototype) {
		descendant.prototype[m] = parent.prototype[m];
	}
};

