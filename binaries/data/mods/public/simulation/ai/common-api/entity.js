var API3 = function(m)
{

// defines a template.
m.Template = m.Class({

	"_init": function(sharedAI, templateName, template)
	{
		this._templateName = templateName;
		this._template = template;
		// save a reference to the template tech modifications
		if (!sharedAI._templatesModifications[this._templateName])
			sharedAI._templatesModifications[this._templateName] = {};
		this._templateModif = sharedAI._templatesModifications[this._templateName];
		this._tpCache = new Map();
	},

	// Helper function to return a template value, adjusting for tech.
	"get": function(string)
	{
		if (this._entityModif && this._entityModif.has(string))
			return this._entityModif.get(string);
		else if (this._templateModif)
		{
			let owner = this._entity ? this._entity.owner : PlayerID;
			if (this._templateModif[owner] && this._templateModif[owner].has(string))
				return this._templateModif[owner].get(string);
		}

		if (!this._tpCache.has(string))
		{
			let value = this._template;
			let args = string.split("/");
			for (let arg of args)
			{
				value = value[arg];
				if (value == undefined)
					break;
			}
			this._tpCache.set(string, value);
		}
		return this._tpCache.get(string);
	},

	"templateName": function() { return this._templateName; },

	"genericName": function() { return this.get("Identity/GenericName"); },

	"civ": function() { return this.get("Identity/Civ"); },

	"matchLimit": function() {
		if (!this.get("TrainingRestrictions"))
			return undefined;
		return this.get("TrainingRestrictions/MatchLimit");
	},

	"classes": function() {
		let template = this.get("Identity");
		if (!template)
			return undefined;
		return GetIdentityClasses(template);
	},

	"hasClass": function(name) {
		if (!this._classes)
			this._classes = this.classes();
		return this._classes && this._classes.indexOf(name) != -1;
	},

	"hasClasses": function(array) {
		if (!this._classes)
			this._classes = this.classes();
		return this._classes && MatchesClassList(this._classes, array);
	},

	"requiredTech": function() { return this.get("Identity/RequiredTechnology"); },

	"available": function(gameState) {
		let techRequired = this.requiredTech();
		if (!techRequired)
			return true;
		return gameState.isResearched(techRequired);
	},

	// specifically
	"phase": function() {
		let techRequired = this.requiredTech();
		if (!techRequired)
			return 0;
		if (techRequired == "phase_village")
			return 1;
		if (techRequired == "phase_town")
			return 2;
		if (techRequired == "phase_city")
			return 3;
		if (techRequired.startsWith("phase_"))
			return 4;
		return 0;
	},

	"cost": function(productionQueue) {
		if (!this.get("Cost"))
			return {};

		let ret = {};
		for (let type in this.get("Cost/Resources"))
			ret[type] = +this.get("Cost/Resources/" + type);
		return ret;
	},

	"costSum": function(productionQueue) {
		let cost = this.cost(productionQueue);
		if (!cost)
			return 0;
		let ret = 0;
		for (let type in cost)
			ret += cost[type];
		return ret;
	},

	"techCostMultiplier": function(type) {
		return +(this.get("Researcher/TechCostMultiplier/"+type) || 1);
	},

	/**
	 * Returns { "max": max, "min": min } or undefined if no obstruction.
	 * max: radius of the outer circle surrounding this entity's obstruction shape
	 * min: radius of the inner circle
	 */
	"obstructionRadius": function() {
		if (!this.get("Obstruction"))
			return undefined;

		if (this.get("Obstruction/Static"))
		{
			let w = +this.get("Obstruction/Static/@width");
			let h = +this.get("Obstruction/Static/@depth");
			return { "max": Math.sqrt(w * w + h * h) / 2, "min": Math.min(h, w) / 2 };
		}

		if (this.get("Obstruction/Unit"))
		{
			let r = +this.get("Obstruction/Unit/@radius");
			return { "max": r, "min": r };
		}

		let right = this.get("Obstruction/Obstructions/Right");
		let left = this.get("Obstruction/Obstructions/Left");
		if (left && right)
		{
			let w = +right["@x"] + right["@width"] / 2 - left["@x"] + left["@width"] / 2;
			let h = Math.max(+right["@z"] + right["@depth"] / 2, +left["@z"] + left["@depth"] / 2) -
			        Math.min(+right["@z"] - right["@depth"] / 2, +left["@z"] - left["@depth"] / 2);
			return { "max": Math.sqrt(w * w + h * h) / 2, "min": Math.min(h, w) / 2 };
		}

		return { "max": 0, "min": 0 }; // Units have currently no obstructions
	},

	/**
	 * Returns the radius of a circle surrounding this entity's footprint.
	 */
	"footprintRadius": function() {
		if (!this.get("Footprint"))
			return undefined;

		if (this.get("Footprint/Square"))
		{
			let w = +this.get("Footprint/Square/@width");
			let h = +this.get("Footprint/Square/@depth");
			return Math.sqrt(w * w + h * h) / 2;
		}

		if (this.get("Footprint/Circle"))
			return +this.get("Footprint/Circle/@radius");

		return 0; // this should never happen
	},

	"maxHitpoints": function() { return +(this.get("Health/Max") || 0); },

	"isHealable": function()
	{
		if (this.get("Health") !== undefined)
			return this.get("Health/Unhealable") !== "true";
		return false;
	},

	"isRepairable": function() { return this.get("Repairable") !== undefined; },

	"getPopulationBonus": function() {
		if (!this.get("Population"))
			return 0;

		return +this.get("Population/Bonus");
	},

	"resistanceStrengths": function() {
		let resistanceTypes = this.get("Resistance");
		if (!resistanceTypes || !resistanceTypes.Entity)
			return undefined;

		let resistance = {};
		if (resistanceTypes.Entity.Capture)
			resistance.Capture = +this.get("Resistance/Entity/Capture");

		if (resistanceTypes.Entity.Damage)
		{
			resistance.Damage = {};
			for (let damageType in resistanceTypes.Entity.Damage)
				resistance.Damage[damageType] = +this.get("Resistance/Entity/Damage/" + damageType);
		}

		// ToDo: Resistance to StatusEffects.

		return resistance;
	},

	"attackTypes": function() {
		let attack = this.get("Attack");
		if (!attack)
			return undefined;

		let ret = [];
		for (let type in attack)
			ret.push(type);
		return ret;
	},

	"attackRange": function(type) {
		if (!this.get("Attack/" + type))
			return undefined;

		return {
			"max": +this.get("Attack/" + type +"/MaxRange"),
			"min": +(this.get("Attack/" + type +"/MinRange") || 0)
		};
	},

	"attackStrengths": function(type) {
		let attackDamageTypes = this.get("Attack/" + type + "/Damage");
		if (!attackDamageTypes)
			return undefined;

		let damage = {};
		for (let damageType in attackDamageTypes)
			damage[damageType] = +attackDamageTypes[damageType];

		return damage;
	},

	"captureStrength": function() {
		if (!this.get("Attack/Capture"))
			return undefined;

		return +this.get("Attack/Capture/Capture") || 0;
	},

	"attackTimes": function(type) {
		if (!this.get("Attack/" + type))
			return undefined;

		return {
			"prepare": +(this.get("Attack/" + type + "/PrepareTime") || 0),
			"repeat": +(this.get("Attack/" + type + "/RepeatTime") || 1000)
		};
	},

	// returns the classes this templates counters:
	// Return type is [ [-neededClasses- , multiplier], … ].
	"getCounteredClasses": function() {
		let attack = this.get("Attack");
		if (!attack)
			return undefined;

		let Classes = [];
		for (let type in attack)
		{
			let bonuses = this.get("Attack/" + type + "/Bonuses");
			if (!bonuses)
				continue;
			for (let b in bonuses)
			{
				let bonusClasses = this.get("Attack/" + type + "/Bonuses/" + b + "/Classes");
				if (bonusClasses)
					Classes.push([bonusClasses.split(" "), +this.get("Attack/" + type +"/Bonuses/" + b +"/Multiplier")]);
			}
		}
		return Classes;
	},

	// returns true if the entity counters the target entity.
	// TODO: refine using the multiplier
	"counters": function(target) {
		let attack = this.get("Attack");
		if (!attack)
			return false;
		let mcounter = [];
		for (let type in attack)
		{
			let bonuses = this.get("Attack/" + type + "/Bonuses");
			if (!bonuses)
				continue;
			for (let b in bonuses)
			{
				let bonusClasses = this.get("Attack/" + type + "/Bonuses/" + b + "/Classes");
				if (bonusClasses)
					mcounter.concat(bonusClasses.split(" "));
			}
		}
		return target.hasClasses(mcounter);
	},

	// returns, if it exists, the multiplier from each attack against a given class
	"getMultiplierAgainst": function(type, againstClass) {
		if (!this.get("Attack/" + type +""))
			return undefined;

		let bonuses = this.get("Attack/" + type + "/Bonuses");
		if (bonuses)
		{
			for (let b in bonuses)
			{
				let bonusClasses = this.get("Attack/" + type + "/Bonuses/" + b + "/Classes");
				if (!bonusClasses)
					continue;
				for (let bcl of bonusClasses.split(" "))
					if (bcl == againstClass)
						return +this.get("Attack/" + type + "/Bonuses/" + b + "/Multiplier");
			}
		}
		return 1;
	},

	"buildableEntities": function(civ) {
		let templates = this.get("Builder/Entities/_string");
		if (!templates)
			return [];
		return templates.replace(/\{native\}/g, this.civ()).replace(/\{civ\}/g, civ).split(/\s+/);
	},

	"trainableEntities": function(civ) {
		const templates = this.get("Trainer/Entities/_string");
		if (!templates)
			return undefined;
		return templates.replace(/\{native\}/g, this.civ()).replace(/\{civ\}/g, civ).split(/\s+/);
	},

	"researchableTechs": function(gameState, civ) {
		const templates = this.get("Researcher/Technologies/_string");
		if (!templates)
			return undefined;
		let techs = templates.split(/\s+/);
		for (let i = 0; i < techs.length; ++i)
		{
			let tech = techs[i];
			if (tech.indexOf("{civ}") == -1)
				continue;
			let civTech = tech.replace("{civ}", civ);
			techs[i] = TechnologyTemplates.Has(civTech) ?
			           civTech : tech.replace("{civ}", "generic");
		}
		return techs;
	},

	"resourceSupplyType": function() {
		if (!this.get("ResourceSupply"))
			return undefined;
		let [type, subtype] = this.get("ResourceSupply/Type").split('.');
		return { "generic": type, "specific": subtype };
	},

	"getResourceType": function() {
		if (!this.get("ResourceSupply"))
			return undefined;
		return this.get("ResourceSupply/Type").split('.')[0];
	},

	"getDiminishingReturns": function() { return +(this.get("ResourceSupply/DiminishingReturns") || 1); },

	"resourceSupplyMax": function() { return +this.get("ResourceSupply/Max"); },

	"maxGatherers": function() { return +(this.get("ResourceSupply/MaxGatherers") || 0); },

	"resourceGatherRates": function() {
		if (!this.get("ResourceGatherer"))
			return undefined;
		let ret = {};
		let baseSpeed = +this.get("ResourceGatherer/BaseSpeed");
		for (let r in this.get("ResourceGatherer/Rates"))
			ret[r] = +this.get("ResourceGatherer/Rates/" + r) * baseSpeed;
		return ret;
	},

	"resourceDropsiteTypes": function() {
		if (!this.get("ResourceDropsite"))
			return undefined;

		let types = this.get("ResourceDropsite/Types");
		return types ? types.split(/\s+/) : [];
	},

	"isResourceDropsite": function(resourceType) {
		const types = this.resourceDropsiteTypes();
		return types && (!resourceType || types.indexOf(resourceType) !== -1);
	},

	"isTreasure": function() { return this.get("Treasure") !== undefined; },

	"treasureResources": function() {
		if (!this.get("Treasure"))
			return undefined;
		let ret = {};
		for (let r in this.get("Treasure/Resources"))
			ret[r] = +this.get("Treasure/Resources/" + r);
		return ret;
	},


	"garrisonableClasses": function() { return this.get("GarrisonHolder/List/_string"); },

	"garrisonMax": function() { return this.get("GarrisonHolder/Max"); },

	"garrisonSize": function() { return this.get("Garrisonable/Size"); },

	"garrisonEjectHealth": function() { return +this.get("GarrisonHolder/EjectHealth"); },

	"getDefaultArrow": function() { return +this.get("BuildingAI/DefaultArrowCount"); },

	"getArrowMultiplier": function() { return +this.get("BuildingAI/GarrisonArrowMultiplier"); },

	"getGarrisonArrowClasses": function() {
		if (!this.get("BuildingAI"))
			return undefined;
		return this.get("BuildingAI/GarrisonArrowClasses").split(/\s+/);
	},

	"buffHeal": function() { return +this.get("GarrisonHolder/BuffHeal"); },

	"promotion": function() { return this.get("Promotion/Entity"); },

	"isPackable": function() { return this.get("Pack") != undefined; },

	"isHuntable": function() {
		// Do not hunt retaliating animals (dead animals can be used).
		// Assume entities which can attack, will attack.
		return this.get("ResourceSupply/KillBeforeGather") &&
			(!this.get("Health") || !this.get("Attack"));
	},

	"walkSpeed": function() { return +this.get("UnitMotion/WalkSpeed"); },

	"trainingCategory": function() { return this.get("TrainingRestrictions/Category"); },

	"buildTime": function(researcher) {
		let time = +this.get("Cost/BuildTime");
		if (researcher)
			time *= researcher.techCostMultiplier("time");
		return time;
	},

	"buildCategory": function() { return this.get("BuildRestrictions/Category"); },

	"buildDistance": function() {
		let distance = this.get("BuildRestrictions/Distance");
		if (!distance)
			return undefined;
		let ret = {};
		for (let key in distance)
			ret[key] = this.get("BuildRestrictions/Distance/" + key);
		return ret;
	},

	"buildPlacementType": function() { return this.get("BuildRestrictions/PlacementType"); },

	"buildTerritories": function() {
		if (!this.get("BuildRestrictions"))
			return undefined;
		let territory = this.get("BuildRestrictions/Territory");
		return !territory ? undefined : territory.split(/\s+/);
	},

	"hasBuildTerritory": function(territory) {
		let territories = this.buildTerritories();
		return territories && territories.indexOf(territory) != -1;
	},

	"hasTerritoryInfluence": function() {
		return this.get("TerritoryInfluence") !== undefined;
	},

	"hasDefensiveFire": function() {
		if (!this.get("Attack/Ranged"))
			return false;
		return this.getDefaultArrow() || this.getArrowMultiplier();
	},

	"territoryInfluenceRadius": function() {
		if (this.get("TerritoryInfluence") !== undefined)
			return +this.get("TerritoryInfluence/Radius");
		return -1;
	},

	"territoryInfluenceWeight": function() {
		if (this.get("TerritoryInfluence") !== undefined)
			return +this.get("TerritoryInfluence/Weight");
		return -1;
	},

	"territoryDecayRate": function() {
		return +(this.get("TerritoryDecay/DecayRate") || 0);
	},

	"defaultRegenRate": function() {
		return +(this.get("Capturable/RegenRate") || 0);
	},

	"garrisonRegenRate": function() {
		return +(this.get("Capturable/GarrisonRegenRate") || 0);
	},

	"visionRange": function() { return +this.get("Vision/Range"); },

	"gainMultiplier": function() { return +this.get("Trader/GainMultiplier"); },

	"isBuilder": function() { return this.get("Builder") !== undefined; },

	"isGatherer": function() { return this.get("ResourceGatherer") !== undefined; },

	"canGather": function(type) {
		let gatherRates = this.get("ResourceGatherer/Rates");
		if (!gatherRates)
			return false;
		for (let r in gatherRates)
			if (r.split('.')[0] === type)
				return true;
		return false;
	},

	"isGarrisonHolder": function() { return this.get("GarrisonHolder") !== undefined; },

	"isTurretHolder": function() { return this.get("TurretHolder") !== undefined; },

	/**
	 * returns true if the tempalte can capture the given target entity
	 * if no target is given, returns true if the template has the Capture attack
	 */
	"canCapture": function(target)
	{
		if (!this.get("Attack/Capture"))
			return false;
		if (!target)
			return true;
		if (!target.get("Capturable"))
			return false;
		let restrictedClasses = this.get("Attack/Capture/RestrictedClasses/_string");
		return !restrictedClasses || !target.hasClasses(restrictedClasses);
	},

	"isCapturable": function() { return this.get("Capturable") !== undefined; },

	"canGuard": function() { return this.get("UnitAI/CanGuard") === "true"; },

	"canGarrison": function() { return "Garrisonable" in this._template; },

	"canOccupyTurret": function() { return "Turretable" in this._template; },

	"isTreasureCollector": function() { return this.get("TreasureCollector") !== undefined; },
});


// defines an entity, with a super Template.
// also redefines several of the template functions where the only change is applying aura and tech modifications.
m.Entity = m.Class({
	"_super": m.Template,

	"_init": function(sharedAI, entity)
	{
		this._super.call(this, sharedAI, entity.template, sharedAI.GetTemplate(entity.template));

		this._entity = entity;
		this._ai = sharedAI;
		// save a reference to the template tech modifications
		if (!sharedAI._templatesModifications[this._templateName])
			sharedAI._templatesModifications[this._templateName] = {};
		this._templateModif = sharedAI._templatesModifications[this._templateName];
		// save a reference to the entity tech/aura modifications
		if (!sharedAI._entitiesModifications.has(entity.id))
			sharedAI._entitiesModifications.set(entity.id, new Map());
		this._entityModif = sharedAI._entitiesModifications.get(entity.id);
	},

	"queryInterface": function(iid) { return SimEngine.QueryInterface(this.id(), iid) },

	"toString": function() { return "[Entity " + this.id() + " " + this.templateName() + "]"; },

	"id": function() { return this._entity.id; },

	/**
	 * Returns extra data that the AI scripts have associated with this entity,
	 * for arbitrary local annotations.
	 * (This data should not be shared with any other AI scripts.)
	 */
	"getMetadata": function(player, key) { return this._ai.getMetadata(player, this, key); },

	/**
	 * Sets extra data to be associated with this entity.
	 */
	"setMetadata": function(player, key, value) { this._ai.setMetadata(player, this, key, value); },

	"deleteAllMetadata": function(player) { delete this._ai._entityMetadata[player][this.id()]; },

	"deleteMetadata": function(player, key) { this._ai.deleteMetadata(player, this, key); },

	"position": function() { return this._entity.position; },
	"angle": function() { return this._entity.angle; },

	"isIdle": function() { return this._entity.idle; },

	"getStance": function() { return this._entity.stance; },
	"unitAIState": function() { return this._entity.unitAIState; },
	"unitAIOrderData": function() { return this._entity.unitAIOrderData; },

	"hitpoints": function() { return this._entity.hitpoints; },
	"isHurt": function() { return this.hitpoints() < this.maxHitpoints(); },
	"healthLevel": function() { return this.hitpoints() / this.maxHitpoints(); },
	"needsHeal": function() { return this.isHurt() && this.isHealable(); },
	"needsRepair": function() { return this.isHurt() && this.isRepairable(); },
	"decaying": function() { return this._entity.decaying; },
	"capturePoints": function() {return this._entity.capturePoints; },
	"isInvulnerable": function() { return this._entity.invulnerability || false; },

	"isSharedDropsite": function() { return this._entity.sharedDropsite === true; },

	/**
	 * Returns the current training queue state, of the form
	 * [ { "id": 0, "template": "...", "count": 1, "progress": 0.5, "metadata": ... }, ... ]
	 */
	"trainingQueue": function() {
		return this._entity.trainingQueue;
	},

	"trainingQueueTime": function() {
		let queue = this._entity.trainingQueue;
		if (!queue)
			return undefined;
		let time = 0;
		for (let item of queue)
			time += item.timeRemaining;
		return time / 1000;
	},

	"foundationProgress": function() {
		return this._entity.foundationProgress;
	},

	"getBuilders": function() {
		if (this._entity.foundationProgress === undefined)
			return undefined;
		if (this._entity.foundationBuilders === undefined)
			return [];
		return this._entity.foundationBuilders;
	},

	"getBuildersNb": function() {
		if (this._entity.foundationProgress === undefined)
			return undefined;
		if (this._entity.foundationBuilders === undefined)
			return 0;
		return this._entity.foundationBuilders.length;
	},

	"owner": function() {
		return this._entity.owner;
	},

	"isOwn": function(player) {
		if (typeof this._entity.owner === "undefined")
			return false;
		return this._entity.owner === player;
	},

	"resourceSupplyAmount": function() {
		return this.queryInterface(Sim.IID_ResourceSupply)?.GetCurrentAmount();
	},

	"resourceSupplyNumGatherers": function()
	{
		return this.queryInterface(Sim.IID_ResourceSupply)?.GetNumGatherers();
	},

	"isFull": function()
	{
		let numGatherers = this.resourceSupplyNumGatherers();
		if (numGatherers)
			return this.maxGatherers() === numGatherers;

		return undefined;
	},

	"resourceCarrying": function() {
		return this.queryInterface(Sim.IID_ResourceGatherer)?.GetCarryingStatus();
	},

	"currentGatherRate": function() {
		// returns the gather rate for the current target if applicable.
		if (!this.get("ResourceGatherer"))
			return undefined;

		if (this.unitAIOrderData().length &&
			this.unitAIState().split(".")[1] == "GATHER")
		{
			let res;
			// this is an abuse of "_ai" but it works.
			if (this.unitAIState().split(".")[1] == "GATHER" && this.unitAIOrderData()[0].target !== undefined)
				res = this._ai._entities.get(this.unitAIOrderData()[0].target);
			else if (this.unitAIOrderData()[1] !== undefined && this.unitAIOrderData()[1].target !== undefined)
				res = this._ai._entities.get(this.unitAIOrderData()[1].target);
			if (!res)
				return 0;
			let type = res.resourceSupplyType();
			if (!type)
				return 0;

			let tstring = type.generic + "." + type.specific;
			let rate = +this.get("ResourceGatherer/BaseSpeed");
			rate *= +this.get("ResourceGatherer/Rates/" +tstring);
			if (rate)
				return rate;
			return 0;
		}
		return undefined;
	},

	"garrisonHolderID": function() {
		return this._entity.garrisonHolderID;
	},

	"garrisoned": function() { return this._entity.garrisoned; },

	"garrisonedSlots": function() {
		let count = 0;

		if (this._entity.garrisoned)
			for (let ent of this._entity.garrisoned)
				count += +this._ai._entities.get(ent).garrisonSize();

		return count;
	},

	"canGarrisonInside": function()
	{
		return this.garrisonedSlots() < this.garrisonMax();
	},

	/**
	 * returns true if the entity can attack (including capture) the given class.
	 */
	"canAttackClass": function(aClass)
	{
		let attack = this.get("Attack");
		if (!attack)
			return false;

		for (let type in attack)
		{
			if (type == "Slaughter")
				continue;
			let restrictedClasses = this.get("Attack/" + type + "/RestrictedClasses/_string");
			if (!restrictedClasses || !MatchesClassList([aClass], restrictedClasses))
				return true;
		}
		return false;
	},

	/**
	 * Derived from Attack.js' similary named function.
	 * @return {boolean} - Whether an entity can attack a given target.
	 */
	"canAttackTarget": function(target, allowCapture)
	{
		let attackTypes = this.get("Attack");
		if (!attackTypes)
			return false;

		let canCapture = allowCapture && this.canCapture(target);
		let health = target.get("Health");
		if (!health)
			return canCapture;

		for (let type in attackTypes)
		{
			if (type == "Capture" ? !canCapture : target.isInvulnerable())
				continue;

			let restrictedClasses = this.get("Attack/" + type + "/RestrictedClasses/_string");
			if (!restrictedClasses || !target.hasClasses(restrictedClasses))
				return true;
		}

		return false;
	},

	"move": function(x, z, queued = false, pushFront = false) {
		Engine.PostCommand(PlayerID, { "type": "walk", "entities": [this.id()], "x": x, "z": z, "queued": queued, "pushFront": pushFront });
		return this;
	},

	"moveToRange": function(x, z, min, max, queued = false, pushFront = false) {
		Engine.PostCommand(PlayerID, { "type": "walk-to-range", "entities": [this.id()], "x": x, "z": z, "min": min, "max": max, "queued": queued, "pushFront": pushFront });
		return this;
	},

	"attackMove": function(x, z, targetClasses, allowCapture = true, queued = false, pushFront = false) {
		Engine.PostCommand(PlayerID, { "type": "attack-walk", "entities": [this.id()], "x": x, "z": z, "targetClasses": targetClasses, "allowCapture": allowCapture, "queued": queued, "pushFront": pushFront });
		return this;
	},

	// violent, aggressive, defensive, passive, standground
	"setStance": function(stance) {
		if (this.getStance() === undefined)
			return undefined;
		Engine.PostCommand(PlayerID, { "type": "stance", "entities": [this.id()], "name": stance});
		return this;
	},

	"stopMoving": function() {
		Engine.PostCommand(PlayerID, { "type": "stop", "entities": [this.id()], "queued": false, "pushFront": false });
	},

	"unload": function(id) {
		if (!this.get("GarrisonHolder"))
			return undefined;
		Engine.PostCommand(PlayerID, { "type": "unload", "garrisonHolder": this.id(), "entities": [id] });
		return this;
	},

	// Unloads all owned units, don't unload allies
	"unloadAll": function() {
		if (!this.get("GarrisonHolder"))
			return undefined;
		Engine.PostCommand(PlayerID, { "type": "unload-all-by-owner", "garrisonHolders": [this.id()] });
		return this;
	},

	"garrison": function(target, queued = false, pushFront = false) {
		Engine.PostCommand(PlayerID, { "type": "garrison", "entities": [this.id()], "target": target.id(), "queued": queued, "pushFront": pushFront });
		return this;
	},

	"occupy-turret": function(target, queued = false, pushFront = false) {
		Engine.PostCommand(PlayerID, { "type": "occupy-turret", "entities": [this.id()], "target": target.id(), "queued": queued, "pushFront": pushFront });
		return this;
	},

	"attack": function(unitId, allowCapture = true, queued = false, pushFront = false) {
		Engine.PostCommand(PlayerID, { "type": "attack", "entities": [this.id()], "target": unitId, "allowCapture": allowCapture, "queued": queued, "pushFront": pushFront });
		return this;
	},

	"collectTreasure": function(target, queued = false, pushFront = false) {
		Engine.PostCommand(PlayerID, {
			"type": "collect-treasure",
			"entities": [this.id()],
			"target": target.id(),
			"queued": queued,
			"pushFront": pushFront
		});
		return this;
	},

	// moveApart from a point in the opposite direction with a distance dist
	"moveApart": function(point, dist) {
		if (this.position() !== undefined)
		{
			let direction = [this.position()[0] - point[0], this.position()[1] - point[1]];
			let norm = m.VectorDistance(point, this.position());
			if (norm === 0)
				direction = [1, 0];
			else
			{
				direction[0] /= norm;
				direction[1] /= norm;
			}
			Engine.PostCommand(PlayerID, { "type": "walk", "entities": [this.id()], "x": this.position()[0] + direction[0]*dist, "z": this.position()[1] + direction[1]*dist, "queued": false, "pushFront": false });
		}
		return this;
	},

	// Flees from a unit in the opposite direction.
	"flee": function(unitToFleeFrom) {
		if (this.position() !== undefined && unitToFleeFrom.position() !== undefined)
		{
			let FleeDirection = [this.position()[0] - unitToFleeFrom.position()[0],
			                     this.position()[1] - unitToFleeFrom.position()[1]];
			let dist = m.VectorDistance(unitToFleeFrom.position(), this.position());
			FleeDirection[0] = 40 * FleeDirection[0] / dist;
			FleeDirection[1] = 40 * FleeDirection[1] / dist;

			Engine.PostCommand(PlayerID, { "type": "walk", "entities": [this.id()], "x": this.position()[0] + FleeDirection[0], "z": this.position()[1] + FleeDirection[1], "queued": false, "pushFront": false });
		}
		return this;
	},

	"gather": function(target, queued = false, pushFront = false) {
		Engine.PostCommand(PlayerID, { "type": "gather", "entities": [this.id()], "target": target.id(), "queued": queued, "pushFront": pushFront });
		return this;
	},

	"repair": function(target, autocontinue = false, queued = false, pushFront = false) {
		Engine.PostCommand(PlayerID, { "type": "repair", "entities": [this.id()], "target": target.id(), "autocontinue": autocontinue, "queued": queued, "pushFront": pushFront });
		return this;
	},

	"returnResources": function(target, queued = false, pushFront = false) {
		Engine.PostCommand(PlayerID, { "type": "returnresource", "entities": [this.id()], "target": target.id(), "queued": queued, "pushFront": pushFront });
		return this;
	},

	"destroy": function() {
		Engine.PostCommand(PlayerID, { "type": "delete-entities", "entities": [this.id()] });
		return this;
	},

	"barter": function(buyType, sellType, amount) {
		Engine.PostCommand(PlayerID, { "type": "barter", "sell": sellType, "buy": buyType, "amount": amount });
		return this;
	},

	"tradeRoute": function(target, source) {
		Engine.PostCommand(PlayerID, { "type": "setup-trade-route", "entities": [this.id()], "target": target.id(), "source": source.id(), "route": undefined, "queued": false, "pushFront": false });
		return this;
	},

	"setRallyPoint": function(target, command) {
		let data = { "command": command, "target": target.id() };
		Engine.PostCommand(PlayerID, { "type": "set-rallypoint", "entities": [this.id()], "x": target.position()[0], "z": target.position()[1], "data": data });
		return this;
	},

	"unsetRallyPoint": function() {
		Engine.PostCommand(PlayerID, { "type": "unset-rallypoint", "entities": [this.id()] });
		return this;
	},

	"train": function(civ, type, count, metadata, pushFront = false)
	{
		let trainable = this.trainableEntities(civ);
		if (!trainable)
		{
			error("Called train("+type+", "+count+") on non-training entity "+this);
			return this;
		}
		if (trainable.indexOf(type) == -1)
		{
			error("Called train("+type+", "+count+") on entity "+this+" which can't train that");
			return this;
		}

		Engine.PostCommand(PlayerID, {
			"type": "train",
			"entities": [this.id()],
			"template": type,
			"count": count,
			"metadata": metadata,
			"pushFront": pushFront
		});
		return this;
	},

	"construct": function(template, x, z, angle, metadata) {
		// TODO: verify this unit can construct this, just for internal
		// sanity-checking and error reporting

		Engine.PostCommand(PlayerID, {
			"type": "construct",
			"entities": [this.id()],
			"template": template,
			"x": x,
			"z": z,
			"angle": angle,
			"autorepair": false,
			"autocontinue": false,
			"queued": false,
			"pushFront": false,
			"metadata": metadata	// can be undefined
		});
		return this;
	},

	"research": function(template, pushFront = false) {
		Engine.PostCommand(PlayerID, {
			"type": "research",
			"entity": this.id(),
			"template": template,
			"pushFront": pushFront
		});
		return this;
	},

	"stopProduction": function(id) {
		Engine.PostCommand(PlayerID, { "type": "stop-production", "entity": this.id(), "id": id });
		return this;
	},

	"stopAllProduction": function(percentToStopAt) {
		let queue = this._entity.trainingQueue;
		if (!queue)
			return true;	// no queue, so technically we stopped all production.
		for (let item of queue)
			if (item.progress < percentToStopAt)
				Engine.PostCommand(PlayerID, { "type": "stop-production", "entity": this.id(), "id": item.id });
		return this;
	},

	"guard": function(target, queued = false, pushFront = false) {
		Engine.PostCommand(PlayerID, { "type": "guard", "entities": [this.id()], "target": target.id(), "queued": queued, "pushFront": pushFront });
		return this;
	},

	"removeGuard": function() {
		Engine.PostCommand(PlayerID, { "type": "remove-guard", "entities": [this.id()] });
		return this;
	}
});

return m;

}(API3);
