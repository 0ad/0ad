/**
 * Provides an API for the rest of the AI scripts to query the world state at a
 * higher level than the raw data.
 */
var GameState = function(ai) {
	MemoizeInit(this);

	this.ai = ai;
	this.timeElapsed = ai.timeElapsed;
	this.templates = ai.templates;
	this.entities = ai.entities;
	this.player = ai.player;
	this.playerData = ai.playerData;
	this.buildingsBuilt = 0;
	
	if (!this.ai._gameStateStore){
		this.ai._gameStateStore = {};
	}
	this.store = this.ai._gameStateStore;
	
	this.cellSize = 4; // Size of each map tile
	
	this.turnCache = {};
};

GameState.prototype.updatingCollection = function(id, filter, collection){
	if (!this.store[id]){
		this.store[id] = collection.filter(filter);
		this.store[id].registerUpdates();
	}
	
	return this.store[id];
};

GameState.prototype.getTimeElapsed = function() {
	return this.timeElapsed;
};

GameState.prototype.getTemplate = function(type) {
	if (!this.templates[type]){
		return null;
	}
	
	return new EntityTemplate(this.templates[type]);
};

GameState.prototype.applyCiv = function(str) {
	return str.replace(/\{civ\}/g, this.playerData.civ);
};

/**
 * @returns {Resources}
 */
GameState.prototype.getResources = function() {
	return new Resources(this.playerData.resourceCounts);
};

GameState.prototype.getMap = function() {
	return this.ai.passabilityMap;
};

GameState.prototype.getTerritoryMap = function() {
	return Map.createTerritoryMap(this);
};

GameState.prototype.getPopulation = function() {
	return this.playerData.popCount;
};

GameState.prototype.getPopulationLimit = function() {
	return this.playerData.popLimit;
};

GameState.prototype.getPopulationMax = function() {
	return this.playerData.popMax;
};

GameState.prototype.getPassabilityClassMask = function(name) {
	if (!(name in this.ai.passabilityClasses)){
		error("Tried to use invalid passability class name '" + name + "'");
	}
	return this.ai.passabilityClasses[name];
};

GameState.prototype.getPlayerID = function() {
	return this.player;
};

GameState.prototype.isPlayerAlly = function(id) {
	return this.playerData.isAlly[id];
};

GameState.prototype.isPlayerEnemy = function(id) {
	return this.playerData.isEnemy[id];
};

GameState.prototype.getEnemies = function(){
	var ret = [];
	for (var i in this.playerData.isEnemy){
		if (this.playerData.isEnemy[i]){
			ret.push(i);
		}
	}
	return ret;
};

GameState.prototype.isEntityAlly = function(ent) {
	if (ent && ent.owner && (typeof ent.owner) === "function"){
		return this.playerData.isAlly[ent.owner()];
	} else if (ent && ent.owner){
		return this.playerData.isAlly[ent.owner];
	}
	return false;
};

GameState.prototype.isEntityEnemy = function(ent) {
	if (ent && ent.owner && (typeof ent.owner) === "function"){
		return this.playerData.isEnemy[ent.owner()];
	} else if (ent && ent.owner){
		return this.playerData.isEnemy[ent.owner];
	}
	return false;
};
 
GameState.prototype.isEntityOwn = function(ent) {
	if (ent && ent.owner && (typeof ent.owner) === "function"){
		return ent.owner() == this.player;
	} else if (ent && ent.owner){
		return ent.owner == this.player;
	}
	return false;
};

GameState.prototype.getOwnEntities = function() {
	if (!this.store.ownEntities){
		this.store.ownEntities = this.getEntities().filter(Filters.byOwner(this.player));
		this.store.ownEntities.registerUpdates();
	}
	
	return this.store.ownEntities;
};

GameState.prototype.getEnemyEntities = function() {
	var diplomacyChange = false;
	var enemies = this.getEnemies();
	if (this.store.enemies){
		if (this.store.enemies.length != enemies.length){
			diplomacyChange = true;
		}else{
			for (var i  = 0; i < enemies.length; i++){
				if (enemies[i] !== this.store.enemies[i]){
					diplomacyChange = true;
				}
			}
		}
	}
	if (diplomacyChange || !this.store.enemyEntities){
		var filter = Filters.byOwners(enemies);
		this.store.enemyEntities = this.getEntities().filter(filter);
		this.store.enemyEntities.registerUpdates();
		this.store.enemies = enemies;
	}
	
	return this.store.enemyEntities;
};

GameState.prototype.getEntities = function() {
	return this.entities;
};

GameState.prototype.getEntityById = function(id){
	if (this.entities._entities[id]) {
		return this.entities._entities[id];
	}else{
		//debug("Entity " + id + " requested does not exist");
	}
	return undefined;
};

GameState.prototype.getOwnEntitiesByMetadata = function(key, value){
	if (!this.store[key + "-" + value]){
		var filter = Filters.byMetadata(key, value);
		this.store[key + "-" + value] = this.getOwnEntities().filter(filter);
		this.store[key + "-" + value].registerUpdates();
	}
	
	return this.store[key + "-" + value];
};

GameState.prototype.getOwnEntitiesByRole = function(role){
	return this.getOwnEntitiesByMetadata("role", role);
};

// TODO: fix this so it picks up not in use training stuff
GameState.prototype.getOwnTrainingFacilities = function(){
	return this.updatingCollection("own-training-facilities", Filters.byTrainingQueue(), this.getOwnEntities());
};

GameState.prototype.getOwnEntitiesByType = function(type){
	var filter = Filters.byType(type);
	return this.updatingCollection("own-by-type-" + type, filter, this.getOwnEntities());
};

GameState.prototype.countEntitiesByType = function(type) {
	return this.getOwnEntitiesByType(type).length;
};

GameState.prototype.countEntitiesAndQueuedByType = function(type) {
	var count = this.countEntitiesByType(type);
	
	// Count building foundations
	count += this.countEntitiesByType("foundation|" + type);
	
	// Count animal resources
	count += this.countEntitiesByType("resource|" + type);

	// Count entities in building production queues
	this.getOwnTrainingFacilities().forEach(function(ent){
		ent.trainingQueue().forEach(function(item) {
			if (item.template == type){
				count += item.count;
			}
		});
	});
	
	return count;
};

GameState.prototype.countFoundationsWithType = function(type) {
	var foundationType = "foundation|" + type;
	var count = 0;
	this.getOwnEntities().forEach(function(ent) {
		var t = ent.templateName();
		if (t == foundationType)
			++count;
	});
	return count;
};

GameState.prototype.countOwnEntitiesByRole = function(role) {
	return this.getOwnEntitiesByRole(role).length;
};

GameState.prototype.countOwnEntitiesAndQueuedWithRole = function(role) {
	var count = this.countOwnEntitiesByRole(role);
	
	// Count entities in building production queues
	this.getOwnTrainingFacilities().forEach(function(ent) {
		ent.trainingQueue().forEach(function(item) {
			if (item.metadata && item.metadata.role == role)
				count += item.count;
		});
	});
	return count;
};

/**
 * Find buildings that are capable of training the given unit type, and aren't
 * already too busy.
 */
GameState.prototype.findTrainers = function(template) {
	var maxQueueLength = 2; // avoid tying up resources in giant training queues
	
	return this.getOwnTrainingFacilities().filter(function(ent) {

		var trainable = ent.trainableEntities();
		if (!trainable || trainable.indexOf(template) == -1)
			return false;

		var queue = ent.trainingQueue();
		if (queue) {
			if (queue.length >= maxQueueLength)
				return false;
		}

		return true;
	});
};

/**
 * Find units that are capable of constructing the given building type.
 */
GameState.prototype.findBuilders = function(template) {
	return this.getOwnEntities().filter(function(ent) {

		var buildable = ent.buildableEntities();
		if (!buildable || buildable.indexOf(template) == -1)
			return false;

		return true;
	});
};

GameState.prototype.getOwnFoundations = function() {
	return this.updatingCollection("ownFoundations", Filters.isFoundation(), this.getOwnEntities());
};

GameState.prototype.getOwnDropsites = function(resource){
	return this.updatingCollection("dropsite-own-" + resource, Filters.isDropsite(resource), this.getOwnEntities());
};

GameState.prototype.getResourceSupplies = function(resource){
	return this.updatingCollection("resource-" + resource, Filters.byResource(resource), this.getEntities());
};

GameState.prototype.getEntityLimits = function() {
	return this.playerData.entityLimits;
};

GameState.prototype.getEntityCounts = function() {
	return this.playerData.entityCounts;
};

// Checks whether the maximum number of buildings have been constructed for a certain catergory
GameState.prototype.isEntityLimitReached = function(category) {
	if(this.playerData.entityLimits[category] === undefined || this.playerData.entityCounts[category] === undefined)
		return false;
	return (this.playerData.entityCounts[category] >= this.playerData.entityLimits[category]);
};
