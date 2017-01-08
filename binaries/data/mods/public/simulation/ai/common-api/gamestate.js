var API3 = function(m)
{

/**
 * Provides an API for the rest of the AI scripts to query the world state at a
 * higher level than the raw data.
 */
m.GameState = function() {
	this.ai = null; // must be updated by the AIs.
};

m.GameState.prototype.init = function(SharedScript, state, player) {
	this.sharedScript = SharedScript;
	this.EntCollecNames = SharedScript._entityCollectionsName;
	this.timeElapsed = SharedScript.timeElapsed;
	this.circularMap = SharedScript.circularMap;
	this.templates = SharedScript._templates;
	this.techTemplates = SharedScript._techTemplates;
	this.entities = SharedScript.entities;
	this.player = player;
	this.playerData = SharedScript.playersData[this.player];
	this.barterPrices = SharedScript.barterPrices;
	this.gameType = SharedScript.gameType;
	this.alliedVictory = SharedScript.alliedVictory;
	this.ceasefireActive = SharedScript.ceasefireActive;

	// get the list of possible phases for this civ:
	// we assume all of them are researchable from the civil centre
	this.phases = [ { name: "phase_village" }, { name: "phase_town" }, { name: "phase_city" } ];
	let cctemplate = this.getTemplate(this.applyCiv("structures/{civ}_civil_centre"));
	if (!cctemplate)
		return;
	let techs = cctemplate.researchableTechs(this.getPlayerCiv());
	for (let i = 0; i < this.phases.length; ++i)
	{
		let k = techs.indexOf(this.phases[i].name);
		if (k !== -1)
		{
			this.phases[i].requirements = DeriveTechnologyRequirements(this.getTemplate(techs[k])._template, this.getPlayerCiv());
			continue;
		}
		for (let tech of techs)
		{
			let template = (this.getTemplate(tech))._template;
			if (template.replaces && template.replaces.indexOf(this.phases[i].name) != -1)
			{
				this.phases[i].name = tech;
				this.phases[i].requirements = DeriveTechnologyRequirements(template, this.getPlayerCiv());
				break;
			}
		}
	}
};

m.GameState.prototype.update = function(SharedScript)
{
	this.timeElapsed = SharedScript.timeElapsed;
	this.playerData = SharedScript.playersData[this.player];
	this.barterPrices = SharedScript.barterPrices;
	this.ceasefireActive = SharedScript.ceasefireActive;
};

m.GameState.prototype.updatingCollection = function(id, filter, collection)
{
	let gid = this.player + "-" + id;	// automatically add the player ID
	return this.updatingGlobalCollection(gid, filter, collection);
};

m.GameState.prototype.destroyCollection = function(id)
{
	let gid = this.player + "-" + id;	// automatically add the player ID
	this.destroyGlobalCollection(gid);
};

m.GameState.prototype.getEC = function(id)
{
	let gid = this.player + "-" + id;	// automatically add the player ID
	return this.getGEC(gid);
};

m.GameState.prototype.updatingGlobalCollection = function(id, filter, collection)
{
	if (this.EntCollecNames.has(id))
		return this.EntCollecNames.get(id);

	let newCollection = collection !== undefined ? collection.filter(filter) : this.entities.filter(filter);
	newCollection.registerUpdates();
	this.EntCollecNames.set(id, newCollection);
	return newCollection;
};

m.GameState.prototype.destroyGlobalCollection = function(id)
{
	if (!this.EntCollecNames.has(id))
		return;

	this.sharedScript.removeUpdatingEntityCollection(this.EntCollecNames.get(id));
	this.EntCollecNames.delete(id);
};

m.GameState.prototype.getGEC = function(id)
{
	if (!this.EntCollecNames.has(id))
		return undefined;
	return this.EntCollecNames.get(id);
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

m.GameState.prototype.getAlliedVictory = function()
{
	return this.alliedVictory;
};

m.GameState.prototype.isCeasefireActive = function()
{
	return this.ceasefireActive;
};

m.GameState.prototype.getTemplate = function(type)
{
	if (this.techTemplates[type] !== undefined)
		return new m.Technology(this.techTemplates, type);

	if (!this.templates[type])
		return null;

	return new m.Template(this.templates[type]);
};

m.GameState.prototype.applyCiv = function(str)
{
	return str.replace(/\{civ\}/g, this.playerData.civ);
};

m.GameState.prototype.getPlayerCiv = function(player)
{
	return player !== undefined ? this.sharedScript.playersData[player].civ : this.playerData.civ;
};

m.GameState.prototype.currentPhase = function()
{
	for (let i = this.phases.length; i > 0; --i)
		if (this.isResearched(this.phases[i-1].name))
			return i;
	return 0;
};

m.GameState.prototype.townPhase = function()
{
	return this.phases[1].name;
};

m.GameState.prototype.cityPhase = function()
{
	return this.phases[2].name;
};

m.GameState.prototype.getPhaseEntityRequirements = function(i)
{
	let entityReqs = [];

	for (let requirement of this.phases[i-1].requirements)
	{
		if (!requirement.entities)
			continue;
		for (let entity of requirement.entities)
			if (entity.check == "count")
				entityReqs.push({
					"class": entity.class,
					"count": entity.number
				});
	}

	return entityReqs;
}

m.GameState.prototype.isResearched = function(template)
{
	return this.playerData.researchedTechs[template] !== undefined;
};

/** true if started or queued */
m.GameState.prototype.isResearching = function(template)
{
	return this.playerData.researchStarted[template] !== undefined ||
	       this.playerData.researchQueued[template] !== undefined;
};

/** this is an "in-absolute" check that doesn't check if we have a building to research from. */
m.GameState.prototype.canResearch = function(techTemplateName, noRequirementCheck)
{
	let template = this.getTemplate(techTemplateName);
	if (!template)
		return false;

	// researching or already researched: NOO.
	if (this.playerData.researchQueued[techTemplateName] ||
	    this.playerData.researchStarted[techTemplateName] ||
	    this.playerData.researchedTechs[techTemplateName])
		return false;

	if (noRequirementCheck === true)
		return true;

	// if this is a pair, we must check that the pair tech is not being researched
	if (template.pair())
	{
		let other = template.pairedWith();
		if (this.playerData.researchQueued[other] ||
		    this.playerData.researchStarted[other] ||
		    this.playerData.researchedTechs[other])
			return false;
	}

	return this.checkTechRequirements(template.requirements(this.playerData.civ));
};

/**
 * Private function for checking a set of requirements is met.
 * Basically copies TechnologyManager, but compares against
 * variables only available within the AI
 */
m.GameState.prototype.checkTechRequirements = function(reqs)
{
	if (!reqs)
		return false;

	if (!reqs.length)
		return true;

	function doesEntitySpecPass(entity)
	{
		switch (entity.check)
		{
		case "count":
			if (!this.playerData.classCounts[entity.class] || this.playerData.classCounts[entity.class] < entity.number)
				return false;
			break;

		case "variants":
			if (!this.playerData.typeCountsByClass[entity.class] || Object.keys(this.playerData.typeCountsByClass[entity.class]).length < entity.number)
				return false;
			break;
		}
		return true;
	};

	return reqs.some(req => {
		return Object.keys(req).every(type => {
			switch (type)
			{
			case "techs":
				return req[type].every(tech => !!this.playerData.researchedTechs[tech]);

			case "entities":
				return req[type].every(doesEntitySpecPass, this);
			}
			return false;
		});
	});
};

m.GameState.prototype.getMap = function()
{
	return this.sharedScript.passabilityMap;
};

m.GameState.prototype.getPassabilityClassMask = function(name)
{
	if (!this.sharedScript.passabilityClasses[name])
		error("Tried to use invalid passability class name '" + name + "'");
	return this.sharedScript.passabilityClasses[name];
};

m.GameState.prototype.getResources = function()
{
	return new m.Resources(this.playerData.resourceCounts);
};

m.GameState.prototype.getPopulation = function()
{
	return this.playerData.popCount;
};

m.GameState.prototype.getPopulationLimit = function() {
	return this.playerData.popLimit;
};

m.GameState.prototype.getPopulationMax = function() {
	return this.playerData.popMax;
};

m.GameState.prototype.getPlayerID = function()
{
	return this.player;
};

m.GameState.prototype.hasAllies = function()
{
	for (let i in this.playerData.isAlly)
		if (this.playerData.isAlly[i] && +i !== this.player &&
		    this.sharedScript.playersData[i].state !== "defeated")
			return true;
	return false;
};

m.GameState.prototype.hasEnemies = function()
{
	for (let i in this.playerData.isEnemy)
		if (this.playerData.isEnemy[i] && +i !== 0 &&
		    this.sharedScript.playersData[i].state !== "defeated")
			return true;
	return false;
};

m.GameState.prototype.hasNeutrals = function()
{
	for (let i in this.playerData.isNeutral)
		if (this.playerData.isNeutral[i] &&
		    this.sharedScript.playersData[i].state !== "defeated")
			return true;
	return false;
};

m.GameState.prototype.isPlayerNeutral = function(id)
{
	return this.playerData.isNeutral[id];
};

m.GameState.prototype.isPlayerAlly = function(id)
{
	return this.playerData.isAlly[id];
};

m.GameState.prototype.isPlayerMutualAlly = function(id)
{
	return this.playerData.isMutualAlly[id];
};

m.GameState.prototype.isPlayerEnemy = function(id)
{
	return this.playerData.isEnemy[id];
};

m.GameState.prototype.getEnemies = function()
{
	let ret = [];
	for (let i in this.playerData.isEnemy)
		if (this.playerData.isEnemy[i])
			ret.push(+i);
	return ret;
};

m.GameState.prototype.getNeutrals = function()
{
	let ret = [];
	for (let i in this.playerData.isNeutral)
		if (this.playerData.isNeutral[i])
			ret.push(+i);
	return ret;
};

m.GameState.prototype.getAllies = function()
{
	let ret = [];
	for (let i in this.playerData.isAlly)
		if (this.playerData.isAlly[i])
			ret.push(+i);
	return ret;
};

m.GameState.prototype.getExclusiveAllies = function()
{	// Player is not included
	let ret = [];
	for (let i in this.playerData.isAlly)
		if (this.playerData.isAlly[i] && +i !== this.player)
			ret.push(+i);
	return ret;
};

m.GameState.prototype.getMutualAllies = function()
{
	let ret = [];
	for (let i in this.playerData.isMutualAlly)
		if (this.playerData.isMutualAlly[i] &&
		    this.sharedScript.playersData[i].isMutualAlly[this.player])
			ret.push(+i);
	return ret;
};

m.GameState.prototype.isEntityAlly = function(ent)
{
	if (!ent)
		return false;
	return this.playerData.isAlly[ent.owner()];
};

m.GameState.prototype.isEntityExclusiveAlly = function(ent)
{
	if (!ent)
		return false;
	return this.playerData.isAlly[ent.owner()] && ent.owner() !== this.player;
};

m.GameState.prototype.isEntityEnemy = function(ent)
{
	if (!ent)
		return false;
	return this.playerData.isEnemy[ent.owner()];
};

m.GameState.prototype.isEntityOwn = function(ent)
{
	if (!ent)
		return false;
	return ent.owner() === this.player;
};

m.GameState.prototype.getEntityById = function(id)
{
	if (this.entities._entities.has(+id))
		return this.entities._entities.get(+id);

	return undefined;
};

m.GameState.prototype.getEntities = function(id)
{
	if (id === undefined)
		return this.entities;

	return this.updatingGlobalCollection("" + id + "-entities", m.Filters.byOwner(id));
};

m.GameState.prototype.getStructures = function()
{
	return this.updatingGlobalCollection("structures", m.Filters.byClass("Structure"), this.entities);
};

m.GameState.prototype.getOwnEntities = function()
{
	return this.updatingGlobalCollection("" + this.player + "-entities", m.Filters.byOwner(this.player));
};

m.GameState.prototype.getOwnStructures = function()
{
	return this.updatingGlobalCollection("" + this.player + "-structures", m.Filters.byClass("Structure"), this.getOwnEntities());
};

m.GameState.prototype.getOwnUnits = function()
{
	return this.updatingGlobalCollection("" + this.player + "-units", m.Filters.byClass("Unit"), this.getOwnEntities());
};

m.GameState.prototype.getAllyEntities = function()
{
	return this.entities.filter(m.Filters.byOwners(this.getAllies()));
};

m.GameState.prototype.getExclusiveAllyEntities = function()
{
	return this.entities.filter(m.Filters.byOwners(this.getExclusiveAllies()));
};

m.GameState.prototype.getAllyStructures = function()
{
	return this.updatingCollection("ally-structures", m.Filters.byClass("Structure"), this.getAllyEntities());
};

m.GameState.prototype.resetAllyStructures = function()
{
	this.destroyCollection("ally-structures");
};

m.GameState.prototype.getNeutralStructures = function()
{
	return this.getStructures().filter(m.Filters.byOwners(this.getNeutrals()));
};

m.GameState.prototype.getEnemyEntities = function()
{
	return this.entities.filter(m.Filters.byOwners(this.getEnemies()));
};

m.GameState.prototype.getEnemyStructures = function(enemyID)
{
	if (enemyID === undefined)
		return this.updatingCollection("enemy-structures", m.Filters.byClass("Structure"), this.getEnemyEntities());

	return this.updatingGlobalCollection("" + enemyID + "-structures", m.Filters.byClass("Structure"), this.getEntities(enemyID));
};

m.GameState.prototype.resetEnemyStructures = function()
{
	this.destroyCollection("enemy-structures");
};

m.GameState.prototype.getEnemyUnits = function(enemyID)
{
	if (enemyID === undefined)
		return this.getEnemyEntities().filter(m.Filters.byClass("Unit"));

	return this.updatingGlobalCollection("" + enemyID + "-units", m.Filters.byClass("Unit"), this.getEntities(enemyID));
};

/** if maintain is true, this will be stored. Otherwise it's one-shot. */
m.GameState.prototype.getOwnEntitiesByMetadata = function(key, value, maintain)
{
	if (maintain === true)
		return this.updatingCollection(key + "-" + value, m.Filters.byMetadata(this.player, key, value),this.getOwnEntities());
	return this.getOwnEntities().filter(m.Filters.byMetadata(this.player, key, value));
};

m.GameState.prototype.getOwnEntitiesByRole = function(role, maintain)
{
	return this.getOwnEntitiesByMetadata("role", role, maintain);
};

m.GameState.prototype.getOwnEntitiesByType = function(type, maintain)
{
	let filter = m.Filters.byType(type);
	if (maintain === true)
		return this.updatingCollection("type-" + type, filter, this.getOwnEntities());
	return this.getOwnEntities().filter(filter);
};

m.GameState.prototype.getOwnEntitiesByClass = function(cls, maintain)
{
	let filter = m.Filters.byClass(cls);
	if (maintain)
		return this.updatingCollection("class-" + cls, filter, this.getOwnEntities());
	return this.getOwnEntities().filter(filter);
};

m.GameState.prototype.getOwnFoundationsByClass = function(cls, maintain)
{
	let filter = m.Filters.byClass(cls);
	if (maintain)
		return this.updatingCollection("foundations-class-" + cls, filter, this.getOwnFoundations());
	return this.getOwnFoundations().filter(filter);
};

m.GameState.prototype.getOwnTrainingFacilities = function()
{
	return this.updatingGlobalCollection("" + this.player + "-training-facilities", m.Filters.byTrainingQueue(), this.getOwnEntities());
};

m.GameState.prototype.getOwnResearchFacilities = function()
{
	return this.updatingGlobalCollection("" + this.player + "-research-facilities", m.Filters.byResearchAvailable(this.playerData.civ), this.getOwnEntities());
};


m.GameState.prototype.countEntitiesByType = function(type, maintain)
{
	return this.getOwnEntitiesByType(type, maintain).length;
};

m.GameState.prototype.countEntitiesAndQueuedByType = function(type, maintain)
{
	let template = this.getTemplate(type);
	if (!template)
		return 0;

	let count = this.countEntitiesByType(type, maintain);

	// Count building foundations
	if (template.hasClass("Structure") === true)
		count += this.countFoundationsByType(type, true);
	else if (template.resourceSupplyType() !== undefined)	// animal resources
		count += this.countEntitiesByType("resource|" + type, true);
	else
	{
		// Count entities in building production queues
		// TODO: maybe this fails for corrals.
		this.getOwnTrainingFacilities().forEach(function(ent) {
			for (let item of ent.trainingQueue())
				if (item.unitTemplate == type)
					count += item.count;
		});
	}

	return count;
};

m.GameState.prototype.countFoundationsByType = function(type, maintain)
{
	let foundationType = "foundation|" + type;

	if (maintain === true)
		return this.updatingCollection("foundation-type-" + type, m.Filters.byType(foundationType), this.getOwnFoundations()).length;

	let count = 0;
	this.getOwnStructures().forEach(function(ent) {
		if (ent.templateName() == foundationType)
			++count;
	});
	return count;
};

m.GameState.prototype.countOwnEntitiesByRole = function(role)
{
	return this.getOwnEntitiesByRole(role, "true").length;
};

m.GameState.prototype.countOwnEntitiesAndQueuedWithRole = function(role)
{
	let count = this.countOwnEntitiesByRole(role);

	// Count entities in building production queues
	this.getOwnTrainingFacilities().forEach(function(ent) {
		for (let item of ent.trainingQueue())
			if (item.metadata && item.metadata.role && item.metadata.role == role)
				count += item.count;
	});
	return count;
};

m.GameState.prototype.countOwnQueuedEntitiesWithMetadata = function(data, value)
{
	// Count entities in building production queues
	let count = 0;
	this.getOwnTrainingFacilities().forEach(function(ent) {
		for (let item of ent.trainingQueue())
			if (item.metadata && item.metadata[data] && item.metadata[data] == value)
				count += item.count;
	});
	return count;
};

m.GameState.prototype.getOwnFoundations = function()
{
	return this.updatingGlobalCollection("" + this.player + "-foundations", m.Filters.isFoundation(), this.getOwnStructures());
};

m.GameState.prototype.getOwnDropsites = function(resource)
{
	if (resource !== undefined)
		return this.updatingCollection("ownDropsite-" + resource, m.Filters.isDropsite(resource), this.getOwnEntities());
	return this.updatingCollection("ownDropsite-all", m.Filters.isDropsite(), this.getOwnEntities());
};

m.GameState.prototype.getAnyDropsites = function(resource)
{
	if (resource !== undefined)
		return this.updatingGlobalCollection("anyDropsite-" + resource, m.Filters.isDropsite(resource), this.getEntities());
	return this.updatingGlobalCollection("anyDropsite-all", m.Filters.isDropsite(), this.getEntities());
};

m.GameState.prototype.getResourceSupplies = function(resource)
{
	return this.updatingGlobalCollection("resource-" + resource, m.Filters.byResource(resource), this.getEntities());
};

m.GameState.prototype.getHuntableSupplies = function()
{
	return this.updatingGlobalCollection("resource-hunt", m.Filters.isHuntable(), this.getEntities());
};

m.GameState.prototype.getFishableSupplies = function()
{
	return this.updatingGlobalCollection("resource-fish", m.Filters.isFishable(), this.getEntities());
};

/** This returns only units from buildings. */
m.GameState.prototype.findTrainableUnits = function(classes, anticlasses)
{
	let allTrainable = [];
	let civ = this.playerData.civ;
	this.getOwnStructures().forEach(function(ent) {
		let trainable = ent.trainableEntities(civ);
		if (!trainable)
			return;
		for (let unit of trainable)
			if (allTrainable.indexOf(unit) === -1)
				allTrainable.push(unit);
	});
	let ret = [];
	let limits = this.getEntityLimits();
	let current = this.getEntityCounts();
	for (let trainable of allTrainable)
	{
		if (this.isTemplateDisabled(trainable))
			continue;
		let template = this.getTemplate(trainable);
		if (!template || !template.available(this))
			continue;

		let okay = true;
		for (let clas of classes)
		{
			if (template.hasClass(clas))
				continue;
			okay = false;
			break;
		}
		if (!okay)
			continue;

		for (let clas of anticlasses)
		{
			if (!template.hasClass(clas))
				continue;
			okay = false;
			break;
		}
		if (!okay)
			continue;

		let category = template.trainingCategory();
		if (category && limits[category] && current[category] >= limits[category])
			continue;

		ret.push( [trainable, template] );
	}
	return ret;
};

/**
 * Return all techs which can currently be researched
 * Does not factor cost.
 * If there are pairs, both techs are returned.
 */
m.GameState.prototype.findAvailableTech = function()
{
	let allResearchable = [];
	let civ = this.playerData.civ;
	this.getOwnEntities().forEach(function(ent) {
		let searchable = ent.researchableTechs(civ);
		if (!searchable)
			return;
		for (let tech of searchable)
			if (allResearchable.indexOf(tech) === -1)
				allResearchable.push(tech);
	});

	let ret = [];
	for (let tech of allResearchable)
	{
		let template = this.getTemplate(tech);
		if (template.pairDef())
		{
			let techs = template.getPairedTechs();
			if (this.canResearch(techs[0]._templateName))
				ret.push([techs[0]._templateName, techs[0]] );
			if (this.canResearch(techs[1]._templateName))
				ret.push([techs[1]._templateName, techs[1]] );
		}
		else
			if (this.canResearch(tech) && template._templateName != this.townPhase() &&
			                              template._templateName != this.cityPhase())
				ret.push( [tech, template] );
	}
	return ret;
};

/**
 * Return true if we have a building able to train that template
 */
m.GameState.prototype.hasTrainer = function(template)
{
	let civ = this.playerData.civ;
	for (let ent of this.getOwnTrainingFacilities().values())
	{
		let trainable = ent.trainableEntities(civ);
		if (trainable && trainable.indexOf(template) !== -1)
			return true;
	}
	return false;
};

/**
 * Find buildings able to train that template.
 */
m.GameState.prototype.findTrainers = function(template)
{
	let civ = this.playerData.civ;
	return this.getOwnTrainingFacilities().filter(function(ent) {
		let trainable = ent.trainableEntities(civ);
		return trainable && trainable.indexOf(template) !== -1;
	});
};

/**
 * Get any unit that is capable of constructing the given building type.
 */
m.GameState.prototype.findBuilder = function(template)
{
	for (let ent of this.getOwnUnits().values())
	{
		let buildable = ent.buildableEntities();
		if (buildable && buildable.indexOf(template) !== -1)
			return ent;
	}
	return undefined;
};

/** Return true if one of our buildings is capable of researching the given tech */
m.GameState.prototype.hasResearchers = function(templateName, noRequirementCheck)
{
	// let's check we can research the tech.
	if (!this.canResearch(templateName, noRequirementCheck))
		return false;

	let civ = this.playerData.civ;

	for (let ent of this.getOwnResearchFacilities().values())
	{
		let techs = ent.researchableTechs(civ);
		for (let tech of techs)
		{
			let temp = this.getTemplate(tech);
			if (temp.pairDef())
			{
				let pairedTechs = temp.getPairedTechs();
				if (pairedTechs[0]._templateName == templateName ||
				    pairedTechs[1]._templateName == templateName)
					return true;
			}
			else if (tech == templateName)
				return true;
		}
	}
	return false;
};

/** Find buildings that are capable of researching the given tech */
m.GameState.prototype.findResearchers = function(templateName, noRequirementCheck)
{
	// let's check we can research the tech.
	if (!this.canResearch(templateName, noRequirementCheck))
		return undefined;

	let self = this;
	let civ = this.playerData.civ;

	return this.getOwnResearchFacilities().filter(function(ent) {
		let techs = ent.researchableTechs(civ);
		for (let tech of techs)
		{
			let thisTemp = self.getTemplate(tech);
			if (thisTemp.pairDef())
			{
				let pairedTechs = thisTemp.getPairedTechs();
				if (pairedTechs[0]._templateName == templateName ||
				    pairedTechs[1]._templateName == templateName)
					return true;
			}
			else if (tech == templateName)
				return true;
		}
		return false;
	});
};

m.GameState.prototype.getEntityLimits = function()
{
	return this.playerData.entityLimits;
};

m.GameState.prototype.getEntityCounts = function()
{
	return this.playerData.entityCounts;
};

m.GameState.prototype.isTemplateDisabled = function(template)
{
	if (!this.playerData.disabledTemplates[template])
		return false;
	return this.playerData.disabledTemplates[template];
};

/** Checks whether the maximum number of buildings have been constructed for a certain catergory */
m.GameState.prototype.isEntityLimitReached = function(category)
{
	if (this.playerData.entityLimits[category] === undefined ||
	    this.playerData.entityCounts[category] === undefined)
		return false;
	return this.playerData.entityCounts[category] >= this.playerData.entityLimits[category];
};

m.GameState.prototype.getTraderTemplatesGains = function()
{
	let shipMechantTemplateName = this.applyCiv("units/{civ}_ship_merchant");
	let supportTraderTemplateName = this.applyCiv("units/{civ}_support_trader");
	let shipMerchantTemplate = !this.isTemplateDisabled(shipMechantTemplateName) && this.getTemplate(shipMechantTemplateName);
	let supportTraderTemplate = !this.isTemplateDisabled(supportTraderTemplateName) && this.getTemplate(supportTraderTemplateName);
	return {
		"navalGainMultiplier": shipMerchantTemplate && shipMerchantTemplate.gainMultiplier(),
		"landGainMultiplier": supportTraderTemplate && supportTraderTemplate.gainMultiplier()
	};
};

return m;

}(API3);

