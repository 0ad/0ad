var API3 = function(m)
{

/** Shared script handling templates and basic terrain analysis */
m.SharedScript = function(settings)
{
	if (!settings)
		return;

	this._players = settings.players;
	this._templates = settings.templates;
	this._derivedTemplates = {};
	this._techTemplates = settings.techTemplates;

	this._entityMetadata = {};
	for (let i in this._players)
		this._entityMetadata[this._players[i]] = {};

	// array of entity collections
	this._entityCollections = new Map();
	this._entitiesModifications = new Map();	// entities modifications
	this._templatesModifications = {};	// template modifications
	// each name is a reference to the actual one.
	this._entityCollectionsName = new Map();
	this._entityCollectionsByDynProp = {};
	this._entityCollectionsUID = 0;
};

/** Return a simple object (using no classes etc) that will be serialized into saved games */
m.SharedScript.prototype.Serialize = function()
{
	return {
		"players": this._players,
		"techTemplates": this._techTemplates,
		"templatesModifications": this._templatesModifications,
		"entitiesModifications": this._entitiesModifications,
		"metadata": this._entityMetadata
	};
};

/**
 * Called after the constructor when loading a saved game, with 'data' being
 * whatever Serialize() returned
 */
m.SharedScript.prototype.Deserialize = function(data)
{
	this._players = data.players;
	this._techTemplates = data.techTemplates;
	this._templatesModifications = data.templatesModifications;
	this._entitiesModifications = data.entitiesModifications;
	this._entityMetadata = data.metadata;
	this._derivedTemplates = {};

	this.isDeserialized = true;
};

m.SharedScript.prototype.GetTemplate = function(name)
{
	if (this._templates[name])
		return this._templates[name];

	if (this._derivedTemplates[name])
		return this._derivedTemplates[name];

	let template = Engine.GetTemplate(name);
	if (template)
	{
		this._derivedTemplates[name] = template;
		return template;
	}

	error("Tried to retrieve invalid template '"+name+"'");
	return null;
};

/**
 * Initialize the shared component.
 * We need to know the initial state of the game for this, as we will use it.
 * This is called right at the end of the map generation.
 */
m.SharedScript.prototype.init = function(state, deserialization)
{
	if (!deserialization)
		this._entitiesModifications = new Map();

	this.ApplyTemplatesDelta(state);

	this.passabilityClasses = state.passabilityClasses;
	this.players = this._players;
	this.playersData = state.players;
	this.timeElapsed = state.timeElapsed;
	this.circularMap = state.circularMap;
	this.mapSize = state.mapSize;
	this.gameType = state.gameType;
	this.alliedVictory = state.alliedVictory;
	this.ceasefireActive = state.ceasefireActive;

	this.passabilityMap = state.passabilityMap;
	if (this.mapSize % this.passabilityMap.width !== 0)
		 error("AI shared component inconsistent sizes: map=" + this.mapSize + " while passability=" + this.passabilityMap.width);
	this.passabilityMap.cellSize = this.mapSize / this.passabilityMap.width;
	this.territoryMap = state.territoryMap;
	if (this.mapSize % this.territoryMap.width !== 0)
		 error("AI shared component inconsistent sizes: map=" + this.mapSize + " while territory=" + this.territoryMap.width);
	this.territoryMap.cellSize = this.mapSize / this.territoryMap.width;

/*
	let landPassMap = new Uint8Array(this.passabilityMap.data.length);
	let waterPassMap = new Uint8Array(this.passabilityMap.data.length);
	let obstructionMaskLand = this.passabilityClasses["default-terrain-only"];
	let obstructionMaskWater = this.passabilityClasses["ship-terrain-only"];
	for (let i = 0; i < this.passabilityMap.data.length; ++i)
	{
		landPassMap[i] = (this.passabilityMap.data[i] & obstructionMaskLand) ? 0 : 255;
		waterPassMap[i] = (this.passabilityMap.data[i] & obstructionMaskWater) ? 0 : 255;
	}
	Engine.DumpImage("LandPassMap.png", landPassMap, this.passabilityMap.width, this.passabilityMap.height, 255);
	Engine.DumpImage("WaterPassMap.png", waterPassMap, this.passabilityMap.width, this.passabilityMap.height, 255);
*/

	this._entities = new Map();
	if (state.entities)
		for (let id in state.entities)
			this._entities.set(+id, new m.Entity(this, state.entities[id]));
	// entity collection updated on create/destroy event.
	this.entities = new m.EntityCollection(this, this._entities);

	// create the terrain analyzer
	this.terrainAnalyzer = new m.TerrainAnalysis();
	this.terrainAnalyzer.init(this, state);
	this.accessibility = new m.Accessibility();
	this.accessibility.init(state, this.terrainAnalyzer);

	// Setup resources
	this.resourceInfo = state.resources;
	m.Resources.prototype.types = state.resources.codes;
	// Resource types: ignore = not used for resource maps
	//                 abundant = abundant resource with small amount each
	//                 sparse = sparse resource, but huge amount each
	// The following maps are defined in TerrainAnalysis.js and are used for some building placement (cc, dropsites)
	// They are updated by checking for create and destroy events for all resources
	this.normalizationFactor = { "abundant": 50, "sparse": 90 };
	this.influenceRadius = { "abundant": 36, "sparse": 48 };
	this.ccInfluenceRadius = { "abundant": 60, "sparse": 120 };
	this.resourceMaps = {};   // Contains maps showing the density of resources
	this.ccResourceMaps = {}; // Contains maps showing the density of resources, optimized for CC placement.
	this.createResourceMaps();

	this.gameState = {};
	for (let i in this._players)
	{
		this.gameState[this._players[i]] = new m.GameState();
		this.gameState[this._players[i]].init(this,state, this._players[i]);
	}
};

/**
 * General update of the shared script, before each AI's update
 * applies entity deltas, and each gamestate.
 */
m.SharedScript.prototype.onUpdate = function(state)
{
	if (this.isDeserialized)
	{
		this.init(state, true);
		this.isDeserialized = false;
	}

	// deals with updating based on create and destroy messages.
	this.ApplyEntitiesDelta(state);
	this.ApplyTemplatesDelta(state);

	Engine.ProfileStart("onUpdate");

	// those are dynamic and need to be reset as the "state" object moves in memory.
	this.events = state.events;
	this.passabilityClasses = state.passabilityClasses;
	this.playersData = state.players;
	this.timeElapsed = state.timeElapsed;
	this.barterPrices = state.barterPrices;
	this.ceasefireActive = state.ceasefireActive;

	this.passabilityMap = state.passabilityMap;
	this.passabilityMap.cellSize = this.mapSize / this.passabilityMap.width;
	this.territoryMap = state.territoryMap;
	this.territoryMap.cellSize = this.mapSize / this.territoryMap.width;

	for (let i in this.gameState)
		this.gameState[i].update(this);

	// TODO: merge this with "ApplyEntitiesDelta" since after all they do the same.
	this.updateResourceMaps(this.events);

	Engine.ProfileStop();
};

m.SharedScript.prototype.ApplyEntitiesDelta = function(state)
{
	Engine.ProfileStart("Shared ApplyEntitiesDelta");

	let foundationFinished = {};

	// by order of updating:
	// we "Destroy" last because we want to be able to switch Metadata first.

	let CreateEvents = state.events.Create;
	for (let i = 0; i < CreateEvents.length; ++i)
	{
		let evt = CreateEvents[i];
		if (!state.entities[evt.entity])
			continue; // Sometimes there are things like foundations which get destroyed too fast

		let entity = new m.Entity(this, state.entities[evt.entity]);
		this._entities.set(evt.entity, entity);
		this.entities.addEnt(entity);

		// Update all the entity collections since the create operation affects static properties as well as dynamic
		for (let entCol of this._entityCollections.values())
			entCol.updateEnt(entity);
	}

	for (let evt of state.events.EntityRenamed)
	{	// Switch the metadata: TODO entityCollections are updated only because of the owner change. Should be done properly
		for (let p in this._players)
		{
			this._entityMetadata[this._players[p]][evt.newentity] = this._entityMetadata[this._players[p]][evt.entity];
			this._entityMetadata[this._players[p]][evt.entity] = {};
		}
	}

	for (let evt of state.events.TrainingFinished)
	{	// Apply metadata stored in training queues
		for (let entId of evt.entities)
			if (this._entities.has(entId))
				for (let key in evt.metadata)
					this.setMetadata(evt.owner, this._entities.get(entId), key, evt.metadata[key]);
	}

	for (let evt of state.events.ConstructionFinished)
	{	// we'll move metadata.
		if (!this._entities.has(evt.entity))
			continue;
		let ent = this._entities.get(evt.entity);
		let newEnt = this._entities.get(evt.newentity);
		if (this._entityMetadata[ent.owner()] && this._entityMetadata[ent.owner()][evt.entity] !== undefined)
			for (let key in this._entityMetadata[ent.owner()][evt.entity])
				this.setMetadata(ent.owner(), newEnt, key, this._entityMetadata[ent.owner()][evt.entity][key]);
		foundationFinished[evt.entity] = true;
	}

	for (let evt of state.events.AIMetadata)
	{
		if (!this._entities.has(evt.id))
			continue;	// might happen in some rare cases of foundations getting destroyed, perhaps.
						// Apply metadata (here for buildings for example)
		for (let key in evt.metadata)
			this.setMetadata(evt.owner, this._entities.get(evt.id), key, evt.metadata[key]);
	}

	let DestroyEvents = state.events.Destroy;
	for (let i = 0; i < DestroyEvents.length; ++i)
	{
		let evt = DestroyEvents[i];
		// A small warning: javascript "delete" does not actually delete, it only removes the reference in this object.
		// the "deleted" object remains in memory, and any older reference to it will still reference it as if it were not "deleted".
		// Worse, they might prevent it from being garbage collected, thus making it stay alive and consuming ram needlessly.
		// So take care, and if you encounter a weird bug with deletion not appearing to work correctly, this is probably why.
		if (!this._entities.has(evt.entity))
			continue;// probably should remove the event.

		if (foundationFinished[evt.entity])
			evt.SuccessfulFoundation = true;

		// The entity was destroyed but its data may still be useful, so
		// remember the entity and this AI's metadata concerning it
		evt.metadata = {};
		evt.entityObj = this._entities.get(evt.entity);
		for (let j in this._players)
			evt.metadata[this._players[j]] = this._entityMetadata[this._players[j]][evt.entity];

		let entity = this._entities.get(evt.entity);
		for (let entCol of this._entityCollections.values())
			entCol.removeEnt(entity);
		this.entities.removeEnt(entity);

		this._entities.delete(evt.entity);
		this._entitiesModifications.delete(evt.entity);
		for (let j in this._players)
			delete this._entityMetadata[this._players[j]][evt.entity];
	}

	for (let id in state.entities)
	{
		let changes = state.entities[id];
		let entity = this._entities.get(+id);
		for (let prop in changes)
		{
			entity._entity[prop] = changes[prop];
			this.updateEntityCollections(prop, entity);
		}
	}

	// apply per-entity aura-related changes.
	// this supersedes tech-related changes.
	for (let id in state.changedEntityTemplateInfo)
	{
		if (!this._entities.has(+id))
			continue;	// dead, presumably.
		let changes = state.changedEntityTemplateInfo[id];
		if (!this._entitiesModifications.has(+id))
			this._entitiesModifications.set(+id, new Map());
		let modif = this._entitiesModifications.get(+id);
		for (let change of changes)
			modif.set(change.variable, change.value);
	}
	Engine.ProfileStop();
};

m.SharedScript.prototype.ApplyTemplatesDelta = function(state)
{
	Engine.ProfileStart("Shared ApplyTemplatesDelta");

	for (let player in state.changedTemplateInfo)
	{
		let playerDiff = state.changedTemplateInfo[player];
		for (let template in playerDiff)
		{
			let changes = playerDiff[template];
			if (!this._templatesModifications[template])
				this._templatesModifications[template] = {};
			if (!this._templatesModifications[template][player])
				this._templatesModifications[template][player] = new Map();
			let modif = this._templatesModifications[template][player];
			for (let change of changes)
				modif.set(change.variable, change.value);
		}
	}
	Engine.ProfileStop();
};

m.SharedScript.prototype.registerUpdatingEntityCollection = function(entCollection)
{
	entCollection.setUID(this._entityCollectionsUID);
	this._entityCollections.set(this._entityCollectionsUID, entCollection);
	for (let prop of entCollection.dynamicProperties())
	{
		if (!this._entityCollectionsByDynProp[prop])
			this._entityCollectionsByDynProp[prop] = new Map();
		this._entityCollectionsByDynProp[prop].set(this._entityCollectionsUID, entCollection);
	}
	this._entityCollectionsUID++;
};

m.SharedScript.prototype.removeUpdatingEntityCollection = function(entCollection)
{
	let uid = entCollection.getUID();

	if (this._entityCollections.has(uid))
		this._entityCollections.delete(uid);

	for (let prop of entCollection.dynamicProperties())
		if (this._entityCollectionsByDynProp[prop].has(uid))
			this._entityCollectionsByDynProp[prop].delete(uid);
};

m.SharedScript.prototype.updateEntityCollections = function(property, ent)
{
	if (this._entityCollectionsByDynProp[property] === undefined)
		return;

	for (let entCol of this._entityCollectionsByDynProp[property].values())
		entCol.updateEnt(ent);
};

m.SharedScript.prototype.setMetadata = function(player, ent, key, value)
{
	let metadata = this._entityMetadata[player][ent.id()];
	if (!metadata)
		metadata = this._entityMetadata[player][ent.id()] = {};
	metadata[key] = value;

	this.updateEntityCollections('metadata', ent);
	this.updateEntityCollections('metadata.' + key, ent);
};

m.SharedScript.prototype.getMetadata = function(player, ent, key)
{
	let metadata = this._entityMetadata[player][ent.id()];

	if (!metadata || !(key in metadata))
		return undefined;
	return metadata[key];
};

m.SharedScript.prototype.deleteMetadata = function(player, ent, key)
{
	let metadata = this._entityMetadata[player][ent.id()];

	if (!metadata || !(key in metadata))
		return true;
	metadata[key] = undefined;
	delete metadata[key];
	this.updateEntityCollections('metadata', ent);
	this.updateEntityCollections('metadata.' + key, ent);
	return true;
};

m.copyPrototype = function(descendant, parent)
{
	let sConstructor = parent.toString();
	let aMatch = sConstructor.match( /\s*function (.*)\(/ );

	if ( aMatch != null )
		descendant.prototype[aMatch[1]] = parent;

	for (let p in parent.prototype)
		descendant.prototype[p] = parent.prototype[p];
};

return m;

}(API3);

