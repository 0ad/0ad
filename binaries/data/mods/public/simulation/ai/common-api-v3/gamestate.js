/**
 * Provides an API for the rest of the AI scripts to query the world state at a
 * higher level than the raw data.
 */
var GameState = function(SharedScript, state, player) {
	this.sharedScript = SharedScript;
	this.EntCollecNames = SharedScript._entityCollectionsName;
	this.EntCollec = SharedScript._entityCollections;
	this.timeElapsed = state.timeElapsed;
	this.templates = SharedScript._templates;
	this.techTemplates = SharedScript._techTemplates;
	this.entities = SharedScript.entities;
	this.player = player;
	this.playerData = state.players[player];
	this.techModifications = SharedScript.techModifications[player];
	this.buildingsBuilt = 0;
	
	this.ai = null; // must be updated by the AIs.
	
	this.cellSize = 4; // Size of each map tile
	
	this.turnCache = {};
};
GameState.prototype.update = function(SharedScript, state) {
	this.sharedScript = SharedScript;
	this.EntCollecNames = SharedScript._entityCollectionsName;
	this.EntCollec = SharedScript._entityCollections;
	this.timeElapsed = state.timeElapsed;
	this.templates = SharedScript._templates;
	this.techTemplates = SharedScript._techTemplates;
	this.entities = SharedScript.entities;
	this.playerData = state.players[this.player];
	this.techModifications = SharedScript.techModifications[this.player];

	this.buildingsBuilt = 0;
	this.turnCache = {};
};

GameState.prototype.updatingCollection = function(id, filter, collection){
	// automatically add the player ID
	id = this.player + "-" + id;
		
	if (!this.EntCollecNames[id]){
		if (collection !== undefined)
			this.EntCollecNames[id] = collection.filter(filter);
		else {
			this.EntCollecNames[id] = this.entities.filter(filter);
		}
		this.EntCollecNames[id].registerUpdates();
	}
	
	return this.EntCollecNames[id];
};
GameState.prototype.destroyCollection = function(id){
	// automatically add the player ID
	id = this.player + "-" + id;
	
	if (this.EntCollecNames[id] !== undefined){
		this.sharedScript.removeUpdatingEntityCollection(this.EntCollecNames[id]);
		delete this.EntCollecNames[id];
	}
};
GameState.prototype.getEC = function(id){
	// automatically add the player ID
	id = this.player + "-" + id;

	if (this.EntCollecNames[id] !== undefined)
		return this.EntCollecNames[id];
	return undefined;
};

GameState.prototype.updatingGlobalCollection = function(id, filter, collection) {
	if (!this.EntCollecNames[id]){
		if (collection !== undefined)
			this.EntCollecNames[id] = collection.filter(filter);
		else
			this.EntCollecNames[id] = this.entities.filter(filter);
		this.EntCollecNames[id].registerUpdates();
	}
	
	return this.EntCollecNames[id];
};
GameState.prototype.destroyGlobalCollection = function(id)
{
	if (this.EntCollecNames[id] !== undefined){
		this.sharedScript.removeUpdatingEntityCollection(this.EntCollecNames[id]);
		delete this.EntCollecNames[id];
	}
};
GameState.prototype.getGEC = function(id)
{
	if (this.EntCollecNames[id] !== undefined)
		return this.EntCollecNames[id];
	return undefined;
};

GameState.prototype.currentPhase = function()
{
	if (this.isResearched("phase_city"))
		return 3;
	if (this.isResearched("phase_town"))
		return 2;
	if (this.isResearched("phase_village"))
		return 1;
	return 0;
};

GameState.prototype.isResearched = function(template)
{
	return this.playerData.researchedTechs[template] !== undefined;
};

// this is an absolute check that doesn't check if we have a building to research from.
GameState.prototype.canResearch = function(techTemplateName, noRequirementCheck)
{
	var template = this.getTemplate(techTemplateName);
	if (!template)
		return false;

	// researching or already researched: NOO.
	if (this.playerData.researchQueued[techTemplateName] || this.playerData.researchStarted[techTemplateName] || this.playerData.researchedTechs[techTemplateName])
		return false;

	if (noRequirementCheck === false)
		return true;
	
	// not already researched, check if we can.
	// basically a copy of the function in technologyManager since we can't use it.
	// Checks the requirements for a technology to see if it can be researched at the current time
		
	// The technology which this technology supersedes is required
	if (template.supersedes() && !this.playerData.researchedTechs[template.supersedes()])
		return false;

	// if this is a pair, we must check that the paire tech is not being researched
	if (template.pair())
	{
		var other = template.pairedWith();
		if (this.playerData.researchQueued[other] || this.playerData.researchStarted[other] || this.playerData.researchedTechs[other])
			return false;
	}

	return this.checkTechRequirements(template.requirements());
}

// Private function for checking a set of requirements is met
// basically copies TechnologyManager's
GameState.prototype.checkTechRequirements = function (reqs)
{
	// If there are no requirements then all requirements are met
	if (!reqs)
		return true;
	
	if (reqs.tech)
	{
		return (this.playerData.researchedTechs[reqs.tech] !== undefined && this.playerData.researchedTechs[reqs.tech]);
	}
	else if (reqs.all)
	{
		for (var i = 0; i < reqs.all.length; i++)
		{
			if (!this.checkTechRequirements(reqs.all[i]))
				return false;
		}
		return true;
	}
	else if (reqs.any)
	{
		for (var i = 0; i < reqs.any.length; i++)
		{
			if (this.checkTechRequirements(reqs.any[i]))
				return true;
		}
		return false;
	}
	else if (reqs.class)
	{
		if (reqs.numberOfTypes)
		{
			if (this.playerData.typeCountsByClass[reqs.class])
				return (reqs.numberOfTypes <= Object.keys(this.playerData.typeCountsByClass[reqs.class]).length);
			else
				return false;
		}
		else if (reqs.number)
		{
			if (this.playerData.classCounts[reqs.class])
				return (reqs.number <= this.playerData.classCounts[reqs.class]);
			else
				return false;
		}
	}
	else if (reqs.civ)
	{
		if (this.playerData.civ == reqs.civ)
			return true;
		else
			return false;
	}
	
	// The technologies requirements are not a recognised format
	error("Bad requirements " + uneval(reqs));
	return false;
};


GameState.prototype.getTimeElapsed = function()
{
	return this.timeElapsed;
};

GameState.prototype.getTemplate = function(type)
{
	if (this.techTemplates[type] !== undefined)
		return new Technology(this.techTemplates, type);
	
	if (!this.templates[type])
		return null;
	
	return new EntityTemplate(this.templates[type], this.techModifications);
};

GameState.prototype.applyCiv = function(str) {
	return str.replace(/\{civ\}/g, this.playerData.civ);
};
GameState.prototype.civ = function() {
	return this.playerData.civ;
};

/**
 * @returns {Resources}
 */
GameState.prototype.getResources = function() {
	return new Resources(this.playerData.resourceCounts);
};

GameState.prototype.getMap = function() {
	return this.sharedScript.passabilityMap;
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
	if (!(name in this.sharedScript.passabilityClasses)){
		error("Tried to use invalid passability class name '" + name + "'");
	}
	return this.sharedScript.passabilityClasses[name];
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
	return this.updatingCollection("own-entities", Filters.byOwner(this.player));
};

GameState.prototype.getEnemyEntities = function() {
	var diplomacyChange = false;
	var enemies = this.getEnemies();
	if (this.enemies){
		if (this.enemies.length != enemies.length){
			diplomacyChange = true;
		}else{
			for (var i  = 0; i < enemies.length; i++){
				if (enemies[i] !== this.enemies[i]){
					diplomacyChange = true;
				}
			}
		}
	}
	if (diplomacyChange || !this.enemies){
		return this.updatingCollection("enemy-entities", Filters.byOwners(enemies));
		this.enemies = enemies;
	}
	return this.getEC("enemy-entities");
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
	return this.updatingCollection(key + "-" + value, Filters.byMetadata(this.player, key, value),this.getOwnEntities());
};

GameState.prototype.getOwnEntitiesByRole = function(role){
	return this.getOwnEntitiesByMetadata("role", role);
};

GameState.prototype.getOwnTrainingFacilities = function(){
	return this.updatingCollection("own-training-facilities", Filters.byTrainingQueue(), this.getOwnEntities());
};

GameState.prototype.getOwnResearchFacilities = function(){
	return this.updatingCollection("own-research-facilities", Filters.byResearchAvailable(), this.getOwnEntities());
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

GameState.prototype.countOwnQueuedEntitiesWithMetadata = function(data, value) {
	// Count entities in building production queues
	var count = 0;
	this.getOwnTrainingFacilities().forEach(function(ent) {
		ent.trainingQueue().forEach(function(item) {
			if (item.metadata && item.metadata[data] && item.metadata[data] == value)
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
	var maxQueueLength = 3; // avoid tying up resources in giant training queues
	
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

/**
 * Find buildings that are capable of researching the given tech, and aren't
 * already too busy.
 */
GameState.prototype.findResearchers = function(templateName) {
	// let's check we can research the tech.
	if (!this.canResearch(templateName))
		return [];
	
	var template = this.getTemplate(templateName);
	
	return this.getOwnResearchFacilities().filter(function(ent) { //}){
		var techs = ent.researchableTechs();
		if (!techs || (template.pair() && techs.indexOf(template.pair()) === -1) || (!template.pair() && techs.indexOf(templateName) === -1))
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
	return this.updatingGlobalCollection("resource-" + resource, Filters.byResource(resource), this.getEntities());
};

GameState.prototype.getEntityLimits = function() {
	return this.playerData.entityLimits;
};

GameState.prototype.getEntityCounts = function() {
	return this.playerData.entityCounts;
};

// Checks whether the maximum number of buildings have been cnstructed for a certain catergory
GameState.prototype.isEntityLimitReached = function(category) {
	if(this.playerData.entityLimits[category] === undefined || this.playerData.entityCounts[category] === undefined)
		return false;
	if(this.playerData.entityLimits[category].LimitsPerCivCentre != undefined)
		return (this.playerData.entityCounts[category] >= this.playerData.entityCounts["CivilCentre"]*this.playerData.entityLimits[category].LimitPerCivCentre);
	else
		return (this.playerData.entityCounts[category] >= this.playerData.entityLimits[category]);
};

GameState.prototype.findTrainableUnits = function(classes){
	var allTrainable = [];
	this.getOwnEntities().forEach(function(ent) {
		var trainable = ent.trainableEntities();
		if (ent.hasClass("Structure"))
			for (var i in trainable){
				if (allTrainable.indexOf(trainable[i]) === -1){
					allTrainable.push(trainable[i]);
				}
			}
	});
	var ret = [];
	for (var i in allTrainable) {
		var template = this.getTemplate(allTrainable[i]);
		var okay = true;
		
		for (o in classes)
			if (!template.hasClass(classes[o]))
				okay = false;
		
		if (template.hasClass("Hero"))	// disabling heroes for now
			okay = false;
		
		if (okay)
			ret.push( [allTrainable[i], template] );
	}
	return ret;
};

// Return all techs which can currently be researched
// Does not factor cost.
// If there are pairs, both techs are returned.
GameState.prototype.findAvailableTech = function() {
	
	var allResearchable = [];
	this.getOwnEntities().forEach(function(ent) {
		var searchable = ent.researchableTechs();
		for (var i in searchable) {
			if (allResearchable.indexOf(searchable[i]) === -1) {
				allResearchable.push(searchable[i]);
			}
		}
	});
	
	var ret = [];
	for (var i in allResearchable) {
		var template = this.getTemplate(allResearchable[i]);

		if (template.pairDef())
		{
			var techs = template.getPairedTechs();
			if (this.canResearch(techs[0]._templateName))
				ret.push([techs[0]._templateName, techs[0]] );
			if (this.canResearch(techs[1]._templateName))
				ret.push([techs[1]._templateName, techs[1]] );
		} else {
			if (this.canResearch(allResearchable[i]) && template._templateName != "phase_town" && template._templateName != "phase_city_generic")
				ret.push( [allResearchable[i], template] );
		}
	}
	return ret;
};

// defcon utilities
GameState.prototype.timeSinceDefconChange = function() {
	return this.getTimeElapsed()-this.ai.defconChangeTime;
};
GameState.prototype.setDefcon = function(level,force) {
	if (this.ai.defcon >= level || force) {
		this.ai.defcon = level;
		this.ai.defconChangeTime = this.getTimeElapsed();
	}
};
GameState.prototype.defcon = function() {
	return this.ai.defcon;
};

