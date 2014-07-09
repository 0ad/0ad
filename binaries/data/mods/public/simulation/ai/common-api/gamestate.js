var API3 = function(m)
{


/**
 * Provides an API for the rest of the AI scripts to query the world state at a
 * higher level than the raw data.
 */
m.GameState = function() {
	this.ai = null; // must be updated by the AIs.
	this.cellSize = 4.0; // Size of each map tile

	this.buildingsBuilt = 0;
	this.turnCache = {};
};

m.GameState.prototype.init = function(SharedScript, state, player) {
	this.sharedScript = SharedScript;
	this.EntCollecNames = SharedScript._entityCollectionsName;
	this.EntCollec = SharedScript._entityCollections;
	this.timeElapsed = SharedScript.timeElapsed;
	this.circularMap = SharedScript.circularMap;
	this.templates = SharedScript._templates;
	this.techTemplates = SharedScript._techTemplates;
	this.entities = SharedScript.entities;
	this.player = player;
	this.playerData = this.sharedScript.playersData[this.player];
	this.techModifications = SharedScript._techModifications[this.player];
	this.barterPrices = SharedScript.barterPrices;
	this.gameType = SharedScript.gameType;
};

m.GameState.prototype.update = function(SharedScript, state) {
	this.sharedScript = SharedScript;
	this.EntCollecNames = SharedScript._entityCollectionsName;
	this.EntCollec = SharedScript._entityCollections;
	this.timeElapsed = SharedScript.timeElapsed;
	this.templates = SharedScript._templates;
	this.techTemplates = SharedScript._techTemplates;
	this._entities = SharedScript._entities;
	this.entities = SharedScript.entities;
	this.playerData = SharedScript.playersData[this.player];
	this.techModifications = SharedScript._techModifications[this.player];
	this.barterPrices = SharedScript.barterPrices;

	this.buildingsBuilt = 0;
	this.turnCache = {};
};

m.GameState.prototype.updatingCollection = function(id, filter, collection, allowQuick){
	// automatically add the player ID in front.
	id = this.player + "-" + id;
	if (!this.EntCollecNames[id])  {
		if (collection !== undefined)
			this.EntCollecNames[id] = collection.filter(filter);
		else {
			this.EntCollecNames[id] = this.entities.filter(filter);
		}
		if (allowQuick)
			this.EntCollecNames[id].allowQuickIter();
		this.EntCollecNames[id].registerUpdates();
		//	warn ("New Collection named " +id);
	}
	
	return this.EntCollecNames[id];
};
m.GameState.prototype.destroyCollection = function(id){
	// automatically add the player ID
	id = this.player + "-" + id;
	
	if (this.EntCollecNames[id] !== undefined){
		this.sharedScript.removeUpdatingEntityCollection(this.EntCollecNames[id]);
		delete this.EntCollecNames[id];
	}
};
m.GameState.prototype.getEC = function(id){
	// automatically add the player ID
	id = this.player + "-" + id;

	if (this.EntCollecNames[id] !== undefined)
		return this.EntCollecNames[id];
	return undefined;
};

m.GameState.prototype.updatingGlobalCollection = function(id, filter, collection, allowQuick) {
	if (!this.EntCollecNames[id]){
		if (collection !== undefined)
			this.EntCollecNames[id] = collection.filter(filter);
		else
			this.EntCollecNames[id] = this.entities.filter(filter);
		if (allowQuick)
			this.EntCollecNames[id].allowQuickIter();
		this.EntCollecNames[id].registerUpdates();
		//warn ("New Global Collection named " +id);
	}
	
	return this.EntCollecNames[id];
};
m.GameState.prototype.destroyGlobalCollection = function(id)
{
	if (this.EntCollecNames[id] !== undefined){
		this.sharedScript.removeUpdatingEntityCollection(this.EntCollecNames[id]);
		delete this.EntCollecNames[id];
	}
};
m.GameState.prototype.getGEC = function(id)
{
	if (this.EntCollecNames[id] !== undefined)
		return this.EntCollecNames[id];
	return undefined;
};

m.GameState.prototype.getTimeElapsed = function()
{
	return this.timeElapsed;
};

m.GameState.prototype.getBarterPrices = function()
{
	return this.barterPrices;
};

m.GameState.prototype.getGameType = function()
{
	return this.gameType;
};

m.GameState.prototype.getTemplate = function(type)
{
	if (this.techTemplates[type] !== undefined)
		return new m.Technology(this.techTemplates, type);
	
	if (!this.templates[type])
		return null;
	
	return new m.Template(this.templates[type], this.techModifications);
};

m.GameState.prototype.applyCiv = function(str) {
	return str.replace(/\{civ\}/g, this.playerData.civ);
};

m.GameState.prototype.civ = function() {
	return this.playerData.civ;
};

m.GameState.prototype.currentPhase = function()
{
	if (this.isResearched("phase_city"))
		return 3;
	if (this.isResearched("phase_town"))
		return 2;
	if (this.isResearched("phase_village"))
		return 1;
	return 0;
};

m.GameState.prototype.townPhase = function()
{
	if (this.playerData.civ == "athen")
		return "phase_town_athen";
	return "phase_town_generic";
};

m.GameState.prototype.cityPhase = function()
{
	if (this.playerData.civ == "athen")
		return "phase_city_athen";
	else if (this.playerData.civ == "celt")
		return "phase_city_gauls";
	return "phase_city_generic";
};

m.GameState.prototype.isResearched = function(template)
{
	return this.playerData.researchedTechs[template] !== undefined;
};

// true if started or queued
m.GameState.prototype.isResearching = function(template)
{
	return (this.playerData.researchStarted[template] !== undefined || this.playerData.researchQueued[template] !== undefined);
};

// this is an "in-absolute" check that doesn't check if we have a building to research from.
m.GameState.prototype.canResearch = function(techTemplateName, noRequirementCheck)
{
	var template = this.getTemplate(techTemplateName);
	if (!template)
		return false;

	// researching or already researched: NOO.
	if (this.playerData.researchQueued[techTemplateName] || this.playerData.researchStarted[techTemplateName] || this.playerData.researchedTechs[techTemplateName])
		return false;

	if (noRequirementCheck === true)
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
m.GameState.prototype.checkTechRequirements = function (reqs)
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

m.GameState.prototype.getMap = function() {
	return this.sharedScript.passabilityMap;
};

m.GameState.prototype.getPassabilityClassMask = function(name) {
	if (!(name in this.sharedScript.passabilityClasses)){
		error("Tried to use invalid passability class name '" + name + "'");
	}
	return this.sharedScript.passabilityClasses[name];
};

m.GameState.prototype.getResources = function() {
	return new m.Resources(this.playerData.resourceCounts);
};

m.GameState.prototype.getPopulation = function() {
	return this.playerData.popCount;
};

m.GameState.prototype.getPopulationLimit = function() {
	return this.playerData.popLimit;
};

m.GameState.prototype.getPopulationMax = function() {
	return this.playerData.popMax;
};

m.GameState.prototype.getPlayerID = function() {
	return this.player;
};

m.GameState.prototype.isPlayerAlly = function(id) {
	return this.playerData.isAlly[id];
};

m.GameState.prototype.isPlayerEnemy = function(id) {
	return this.playerData.isEnemy[id];
};

m.GameState.prototype.getEnemies = function(){
	var ret = [];
	for (var i in this.playerData.isEnemy){
		if (this.playerData.isEnemy[i]){
			ret.push(i);
		}
	}
	return ret;
};

m.GameState.prototype.getAllies = function(){  // Player is not included
	var ret = [];
	for (var i in this.playerData.isAlly){
		if (this.playerData.isAlly[i] && +i !== this.player){
			ret.push(i);
		}
	}
	return ret;
};

m.GameState.prototype.isEntityAlly = function(ent) {
	if (ent && ent.owner && (typeof ent.owner) === "function"){
		return this.playerData.isAlly[ent.owner()];
	} else if (ent && ent.owner){
		return this.playerData.isAlly[ent.owner];
	}
	return false;
};

m.GameState.prototype.isEntityEnemy = function(ent) {
	if (ent && ent.owner && (typeof ent.owner) === "function"){
		return this.playerData.isEnemy[ent.owner()];
	} else if (ent && ent.owner){
		return this.playerData.isEnemy[ent.owner];
	}
	return false;
};
 
m.GameState.prototype.isEntityOwn = function(ent) {
	if (ent && ent.owner && (typeof ent.owner) === "function"){
		return ent.owner() == this.player;
	} else if (ent && ent.owner){
		return ent.owner == this.player;
	}
	return false;
};

m.GameState.prototype.getEntityById = function(id){
	if (this.entities._entities[id])
		return this.entities._entities[id];

	return undefined;
};

m.GameState.prototype.getEntities = function() {
	return this.entities;
};

m.GameState.prototype.getOwnEntities = function() {
	return this.updatingGlobalCollection("" + this.player + "-entities", m.Filters.byOwner(this.player));
};

m.GameState.prototype.getOwnStructures = function() {
	return this.updatingGlobalCollection("" + this.player + "-structures", m.Filters.byClass("Structure"), this.getOwnEntities());
};

m.GameState.prototype.getOwnUnits = function() {
	return this.updatingGlobalCollection("" + this.player + "-units", m.Filters.byClass("Unit"), this.getOwnEntities());
};

m.GameState.prototype.getAllyEntities = function() {
	return this.entities.filter(m.Filters.byOwners(this.getAllies()));
};

// Try to use a parameter for those three, it'll be a lot faster.

m.GameState.prototype.getEnemyEntities = function(enemyID) {
	if (enemyID === undefined)
		return this.entities.filter(m.Filters.byOwners(this.getEnemies()));

	return this.updatingGlobalCollection("" + enemyID + "-entities", m.Filters.byOwner(enemyID));
};

m.GameState.prototype.getEnemyStructures = function(enemyID) {
	if (enemyID === undefined)
		return this.getEnemyEntities().filter(m.Filters.byClass("Structure"));

	return this.updatingGlobalCollection("" + enemyID + "-structures", m.Filters.byClass("Structure"), this.getEnemyEntities(enemyID));
};

m.GameState.prototype.getEnemyUnits = function(enemyID) {
	if (enemyID === undefined)
		return this.getEnemyEntities().filter(m.Filters.byClass("Unit"));

	return this.updatingGlobalCollection("" + enemyID + "-units", m.Filters.byClass("Unit"), this.getEnemyEntities(enemyID));
};

// if maintain is true, this will be stored. Otherwise it's one-shot.
m.GameState.prototype.getOwnEntitiesByMetadata = function(key, value, maintain){
	if (maintain === true)
		return this.updatingCollection(key + "-" + value, m.Filters.byMetadata(this.player, key, value),this.getOwnEntities());
	return this.getOwnEntities().filter(m.Filters.byMetadata(this.player, key, value));
};

m.GameState.prototype.getOwnEntitiesByRole = function(role, maintain){
	return this.getOwnEntitiesByMetadata("role", role, maintain);
};

m.GameState.prototype.getOwnEntitiesByType = function(type, maintain){
	var filter = m.Filters.byType(type);
	if (maintain === true)
		return this.updatingCollection("type-" + type, filter, this.getOwnEntities());
	return this.getOwnEntities().filter(filter);

};

m.GameState.prototype.getOwnTrainingFacilities = function(){
	return this.updatingGlobalCollection("" + this.player + "-training-facilities", m.Filters.byTrainingQueue(), this.getOwnEntities(), true);
};

m.GameState.prototype.getOwnResearchFacilities = function(){
	return this.updatingGlobalCollection("" + this.player + "-research-facilities", m.Filters.byResearchAvailable(), this.getOwnEntities(), true);
};


m.GameState.prototype.countEntitiesByType = function(type, maintain) {
	return this.getOwnEntitiesByType(type, maintain).length;
};

m.GameState.prototype.countEntitiesAndQueuedByType = function(type, maintain) {
	var count = this.countEntitiesByType(type, maintain);
	
	// Count building foundations
	if (this.getTemplate(type).hasClass("Structure") === true)
		count += this.countFoundationsByType(type, true);
	else if (this.getTemplate(type).resourceSupplyType() !== undefined)	// animal resources
		count += this.countEntitiesByType("resource|" + type, true);
	else
	{
		// Count entities in building production queues
		// TODO: maybe this fails for corrals.
		this.getOwnTrainingFacilities().forEach(function(ent){
			ent.trainingQueue().forEach(function(item) {
				if (item.unitTemplate == type){
					count += item.count;
				}
			});
		});
	}
	
	return count;
};

m.GameState.prototype.countFoundationsByType = function(type, maintain) {
	var foundationType = "foundation|" + type;

	if (maintain === true)
		return this.updatingCollection("foundation-type-" + type, m.Filters.byType(foundationType), this.getOwnFoundations()).length;

	var count = 0;
	this.getOwnStructures().forEach(function(ent) {
		var t = ent.templateName();
		if (t == foundationType)
			++count;
	});
	return count;
};

m.GameState.prototype.countOwnEntitiesByRole = function(role) {
	return this.getOwnEntitiesByRole(role).length;
};

m.GameState.prototype.countOwnEntitiesAndQueuedWithRole = function(role) {
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

m.GameState.prototype.countOwnQueuedEntitiesWithMetadata = function(data, value) {
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

m.GameState.prototype.getOwnFoundations = function() {
	return this.updatingGlobalCollection("" + this.player + "-foundations", m.Filters.isFoundation(), this.getOwnStructures());
};

m.GameState.prototype.getOwnDropsites = function(resource){
	if (resource !== undefined)
		return this.updatingCollection("dropsite-" + resource, m.Filters.isDropsite(resource), this.getOwnEntities(), true);
	return this.updatingCollection("dropsite-all", m.Filters.isDropsite(), this.getOwnEntities(), true);
};

m.GameState.prototype.getResourceSupplies = function(resource){
	return this.updatingGlobalCollection("resource-" + resource, m.Filters.byResource(resource), this.getEntities(), true);
};

m.GameState.prototype.getHuntableSupplies = function(){
	return this.updatingGlobalCollection("resource-hunt", m.Filters.isHuntable(), this.getEntities(), true);
};

m.GameState.prototype.getFishableSupplies = function(){
	return this.updatingGlobalCollection("resource-fish", m.Filters.isFishable(), this.getEntities(), true);
};

// This returns only units from buildings.
m.GameState.prototype.findTrainableUnits = function(classes){
	var allTrainable = [];
	this.getOwnStructures().forEach(function(ent) {
		var trainable = ent.trainableEntities();
		for (var i in trainable){
			if (allTrainable.indexOf(trainable[i]) === -1) {
				allTrainable.push(trainable[i]);
			}
		}
	});
	var ret = [];
	for (var i in allTrainable) {
		var template = this.getTemplate(allTrainable[i]);

		if (template.hasClass("Hero"))	// disabling heroes for now
			continue;

		if (!template.available(this))
			continue;
		
		var okay = true;
		for (var o in classes)
			if (!template.hasClass(classes[o]))
				okay = false;

		if (okay)
			ret.push( [allTrainable[i], template] );
	}
	return ret;
};

// Return all techs which can currently be researched
// Does not factor cost.
// If there are pairs, both techs are returned.
m.GameState.prototype.findAvailableTech = function() {
	
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
			if (this.canResearch(allResearchable[i]) && template._templateName != this.townPhase() && template._templateName != this.cityPhase())
				ret.push( [allResearchable[i], template] );
		}
	}
	return ret;
};

/**
 * Find buildings that are capable of training said template.
 * Getting the best is up to the AI.
 */
m.GameState.prototype.findTrainers = function(template) {	
	return this.getOwnTrainingFacilities().filter(function(ent) {
		var trainable = ent.trainableEntities();
		if (!trainable || trainable.indexOf(template) == -1)
			return false;
		return true;
	});
};

/**
 * Find units that are capable of constructing the given building type.
 */
m.GameState.prototype.findBuilders = function(template) {
	return this.getOwnUnits().filter(function(ent) {
		var buildable = ent.buildableEntities();
		if (!buildable || buildable.indexOf(template) == -1)
			return false;

		return true;
	});
};

// Find buildings that are capable of researching the given tech
m.GameState.prototype.findResearchers = function(templateName, noRequirementCheck) {
	// let's check we can research the tech.
	if (!this.canResearch(templateName, noRequirementCheck))
		return [];

	var template = this.getTemplate(templateName);
	var self = this;
	
	return this.getOwnResearchFacilities().filter(function(ent) {
		var techs = ent.researchableTechs();
		for (var i in techs)
		{
			var thisTemp = self.getTemplate(techs[i]);
			if (thisTemp.pairDef())
			{
				var pairedTechs = thisTemp.getPairedTechs();
				if (pairedTechs[0]._templateName == templateName || pairedTechs[1]._templateName == templateName)
					return true;
			} else {
				if (techs[i] == templateName)
					return true;
			}
		}
		return false;
	});
};

m.GameState.prototype.getEntityLimits = function() {
	return this.playerData.entityLimits;
};

m.GameState.prototype.getEntityCounts = function() {
	return this.playerData.entityCounts;
};

// Checks whether the maximum number of buildings have been cnstructed for a certain catergory
m.GameState.prototype.isEntityLimitReached = function(category) {
	if(this.playerData.entityLimits[category] === undefined || this.playerData.entityCounts[category] === undefined)
		return false;
	return (this.playerData.entityCounts[category] >= this.playerData.entityLimits[category]);
};

// defcon utilities
m.GameState.prototype.timeSinceDefconChange = function() {
	return this.getTimeElapsed()-this.ai.defconChangeTime;
};
m.GameState.prototype.setDefcon = function(level,force) {
	if (this.ai.defcon >= level || force) {
		this.ai.defcon = level;
		this.ai.defconChangeTime = this.getTimeElapsed();
	}
};
m.GameState.prototype.defcon = function() {
	return this.ai.defcon;
};

return m;

}(API3);

