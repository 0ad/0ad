var API3 = function(m)
{

// defines a template.
// It's completely raw data, except it's slightly cleverer now and then.
m.Template = m.Class({

	_init: function(template)
	{
		this._template = template;
		this._tpCache = new Map();
	},

	// helper function to return a template value, optionally adjusting for tech.
	// TODO: there's no support for "_string" values here.
	get: function(string)
	{
		var value = this._template;
		if (this._auraTemplateModif && this._auraTemplateModif.has(string))
			return this._auraTemplateModif.get(string);
		else if (this._techModif && this._techModif.has(string))
			return this._techModif.get(string);
		else
		{
			if (!this._tpCache.has(string))
			{
				var args = string.split("/");
				for (let arg of args)
				{
					if (value[arg])
						value = value[arg];
					else
					{
						value = undefined;
						break;
					}
				}
				this._tpCache.set(string, value);
			}
			return this._tpCache.get(string);
		}
 
	},

	genericName: function() {
		if (!this.get("Identity") || !this.get("Identity/GenericName"))
			return undefined;
		return this.get("Identity/GenericName");
	},

	rank: function() {
		if (!this.get("Identity"))
			return undefined;
		return this.get("Identity/Rank");
	},

	classes: function() {
		var template = this.get("Identity");
		if (!template)
			return undefined;
		return GetIdentityClasses(template);
	},
	
	requiredTech: function() {
		return this.get("Identity/RequiredTechnology");
	},

	available: function(gameState) {
		if (this.requiredTech() === undefined)
			return true;
		return gameState.isResearched(this.get("Identity/RequiredTechnology"));
	},

	// specifically
	phase: function() {
		if (!this.get("Identity/RequiredTechnology"))
			return 0;
		if (this.get("Identity/RequiredTechnology") == "phase_village")
			return 1;
		if (this.get("Identity/RequiredTechnology") == "phase_town")
			return 2;
		if (this.get("Identity/RequiredTechnology") == "phase_city")
			return 3;
		return 0;
	},

	hasClass: function(name) {
		if (!this._classes)
			this._classes = this.classes();
		var classes = this._classes;
		return (classes && classes.indexOf(name) != -1);
	},

	hasClasses: function(array) {
		if (!this._classes)
			this._classes = this.classes();
		let classes = this._classes;
		if (!classes)
			return false;

		for (let cls of array)
			if (classes.indexOf(cls) === -1)
				return false;
		return true;
	},

	civ: function() {
		return this.get("Identity/Civ");
	},

	cost: function() {
		if (!this.get("Cost"))
			return undefined;

		var ret = {};
		for (var type in this.get("Cost/Resources"))
			ret[type] = +this.get("Cost/Resources/" + type);
		return ret;
	},

	costSum: function() {
		if (!this.get("Cost"))
			return undefined;

		var ret = 0;
		for (var type in this.get("Cost/Resources"))
			ret += +this.get("Cost/Resources/" + type);
		return ret;
	},

	/**
	 * Returns the radius of a circle surrounding this entity's
	 * obstruction shape, or undefined if no obstruction.
	 */
	obstructionRadius: function() {
		if (!this.get("Obstruction"))
			return undefined;

		if (this.get("Obstruction/Static"))
		{
			var w = +this.get("Obstruction/Static/@width");
			var h = +this.get("Obstruction/Static/@depth");
			return Math.sqrt(w*w + h*h) / 2;
		}

		if (this.get("Obstruction/Unit"))
			return +this.get("Obstruction/Unit/@radius");

		return 0; // this should never happen
	},

	/**
	 * Returns the radius of a circle surrounding this entity's
	 * footprint.
	 */
	footprintRadius: function() {
		if (!this.get("Footprint"))
			return undefined;

		if (this.get("Footprint/Square"))
		{
			var w = +this.get("Footprint/Square/@width");
			var h = +this.get("Footprint/Square/@depth");
			return Math.sqrt(w*w + h*h) / 2;
		}

		if (this.get("Footprint/Circle"))
			return +this.get("Footprint/Circle/@radius");

		return 0; // this should never happen
	},

	maxHitpoints: function()
	{
		if (this.get("Health") !== undefined)
			return +this.get("Health/Max");
		return 0;
	},

	isHealable: function()
	{
		if (this.get("Health") !== undefined)
			return this.get("Health/Unhealable") !== "true";
		return false;
	},

	isRepairable: function()
	{
		return this.get("Repairable") !== undefined;
	},

	getPopulationBonus: function() {
		return +this.get("Cost/PopulationBonus");
	},

	armourStrengths: function() {
		if (!this.get("Armour"))
			return undefined;

		return {
			hack: +this.get("Armour/Hack"),
			pierce: +this.get("Armour/Pierce"),
			crush: +this.get("Armour/Crush")
		};
	},

	attackTypes: function() {
		if (!this.get("Attack"))
			return undefined;

		var ret = [];
		for (var type in this.get("Attack"))
			ret.push(type);

		return ret;
	},

	attackRange: function(type) {
		if (!this.get("Attack/" + type +""))
			return undefined;

		return {
				max: +this.get("Attack/" + type +"/MaxRange"),
				min: +(this.get("Attack/" + type +"/MinRange") || 0)
		};
	},

	attackStrengths: function(type) {
		if (!this.get("Attack/" + type +""))
			return undefined;

		return {
			hack: +(this.get("Attack/" + type + "/Hack") || 0),
			pierce: +(this.get("Attack/" + type + "/Pierce") || 0),
			crush: +(this.get("Attack/" + type + "/Crush") || 0)
		};
	},

	captureStrength: function() {
		if (!this.get("Attack/Capture"))
			return undefined;

		return +this.get("Attack/Capture/Value") || 0;
	},

	attackTimes: function(type) {
		if (!this.get("Attack/" + type +""))
			return undefined;

		return {
			prepare: +(this.get("Attack/" + type + "/PrepareTime") || 0),
			repeat: +(this.get("Attack/" + type + "/RepeatTime") || 1000)
		};
	},

	// returns the classes this templates counters:
	// Return type is [ [-neededClasses- , multiplier], … ].
	getCounteredClasses: function() {
		if (!this.get("Attack"))
			return undefined;

		var Classes = [];
		for (var i in this.get("Attack"))
		{
			if (!this.get("Attack/" + i + "/Bonuses"))
				continue;
			for (var o in this.get("Attack/" + i + "/Bonuses"))
				if (this.get("Attack/" + i + "/Bonuses/" + o + "/Classes"))
					Classes.push([this.get("Attack/" + i +"/Bonuses/" + o +"/Classes").split(" "), +this.get("Attack/" + i +"/Bonuses" +o +"/Multiplier")]);
		}
		return Classes;
	},

	// returns true if the entity counters those classes.
	// TODO: refine using the multiplier
	countersClasses: function(classes) {
		if (!this.get("Attack"))
			return false;
		var mcounter = [];
		for (let i in this.get("Attack"))
		{
			if (!this.get("Attack/" + i + "/Bonuses"))
				continue;
			for (let o in this.get("Attack/" + i + "/Bonuses"))
				if (this.get("Attack/" + i + "/Bonuses/" + o + "/Classes"))
					mcounter.concat(this.get("Attack/" + i + "/Bonuses/" + o + "/Classes").split(" "));
		}
		for (let i in classes)
		{
			if (mcounter.indexOf(classes[i]) !== -1)
				return true;
		}
		return false;
	},

	// returns, if it exists, the multiplier from each attack against a given class
	getMultiplierAgainst: function(type, againstClass) {
		if (!this.get("Attack/" + type +""))
			return undefined;

		if (this.get("Attack/" + type + "/Bonuses"))
			for (var o in this.get("Attack/" + type + "/Bonuses"))
			{
				if (!this.get("Attack/" + type + "/Bonuses/" + o + "/Classes"))
					continue;
				var total = this.get("Attack/" + type + "/Bonuses/" + o + "/Classes").split(" ");
				for (var j in total)
					if (total[j] === againstClass)
						return +this.get("Attack/" + type + "/Bonuses/" + o + "/Multiplier");
			}
		return 1;
	},

	// returns true if the entity can attack the given class
	canAttackClass: function(saidClass) {
		if (!this.get("Attack"))
			return false;

		for (var i in this.get("Attack")) {
			if (!this.get("Attack/" + i + "/RestrictedClasses") || !this.get("Attack/" + i + "/RestrictedClasses/_string"))
				continue;
			var cannotAttack = this.get("Attack/" + i + "/RestrictedClasses/_string").split(" ");
			if (cannotAttack.indexOf(saidClass) !== -1)
				return false;
		}
		return true;
	},

	buildableEntities: function() {
		if (!this.get("Builder/Entities/_string"))
			return [];
		var civ = this.civ();
		var templates = this.get("Builder/Entities/_string").replace(/\{civ\}/g, civ).split(/\s+/);
		return templates; // TODO: map to Entity?
	},

	trainableEntities: function(civ) {
		if (!this.get("ProductionQueue/Entities/_string"))
			return undefined;
		var templates = this.get("ProductionQueue/Entities/_string").replace(/\{civ\}/g, civ).split(/\s+/);
		return templates;
	},

	researchableTechs: function(civ) {
		if (this.civ() !== civ)     // techs can only be researched in structure from the player civ
			return undefined;
		if (!this.get("ProductionQueue/Technologies/_string"))
			return undefined;
		var templates = this.get("ProductionQueue/Technologies/_string").split(/\s+/);
		return templates;
	},

	resourceSupplyType: function() {
		if (!this.get("ResourceSupply"))
			return undefined;
		var [type, subtype] = this.get("ResourceSupply/Type").split('.');
		return { "generic": type, "specific": subtype };
	},
	// will return either "food", "wood", "stone", "metal" and not treasure.
	getResourceType: function() {
		if (!this.get("ResourceSupply"))
			return undefined;
		var [type, subtype] = this.get("ResourceSupply/Type").split('.');
		if (type == "treasure")
			return subtype;
		return type;
	},

	resourceSupplyMax: function() {
		if (!this.get("ResourceSupply"))
			return undefined;
		return +this.get("ResourceSupply/Amount");
	},

	maxGatherers: function()
	{
		if (this.get("ResourceSupply") !== undefined)
			return +this.get("ResourceSupply/MaxGatherers");
		return 0;
	},
	
	resourceGatherRates: function() {
		if (!this.get("ResourceGatherer"))
			return undefined;
		var ret = {};
		var baseSpeed = +this.get("ResourceGatherer/BaseSpeed");
		for (var r in this.get("ResourceGatherer/Rates"))
			ret[r] = +this.get("ResourceGatherer/Rates/" + r) * baseSpeed;
		return ret;
	},

	resourceDropsiteTypes: function() {
		if (!this.get("ResourceDropsite"))
			return undefined;

		let types = this.get("ResourceDropsite/Types");
		return types ? types.split(/\s+/) : [];
	},


	garrisonableClasses: function() {
		if (!this.get("GarrisonHolder"))
			return undefined;
		return this.get("GarrisonHolder/List/_string");
	},

	garrisonMax: function() {
		if (!this.get("GarrisonHolder"))
			return undefined;
		return this.get("GarrisonHolder/Max");
	},

	garrisonEjectHealth: function() {
		if (!this.get("GarrisonHolder"))
			return undefined;
		return +this.get("GarrisonHolder/EjectHealth");
	},

	getDefaultArrow: function() {
		if (!this.get("BuildingAI"))
			return undefined;
		return +this.get("BuildingAI/DefaultArrowCount");
	},

	getArrowMultiplier: function() {
		if (!this.get("BuildingAI"))
			return undefined;
		return +this.get("BuildingAI/GarrisonArrowMultiplier");
	},

	getGarrisonArrowClasses: function() {
		if (!this.get("BuildingAI"))
			return undefined;
		return this.get("BuildingAI/GarrisonArrowClasses").split(/\s+/);
	},

	buffHeal: function() {
		if (!this.get("GarrisonHolder"))
			return undefined;
		return +this.get("GarrisonHolder/BuffHeal");
	},

	promotion: function() {
		if (!this.get("Promotion"))
			return undefined;
		return this.get("Promotion/Entity");
	},

	/**
	 * Returns whether this is an animal that is too difficult to hunt.
	 * (Any non domestic currently.)
	 */
	isUnhuntable: function() {   // used by Aegis
		if (!this.get("UnitAI") || !this.get("UnitAI/NaturalBehaviour"))
			return false;

		// only attack domestic animals since they won't flee nor retaliate.
		return this.get("UnitAI/NaturalBehaviour") !== "domestic";
	},

	isHuntable: function() {     // used by Petra
		if(!this.get("ResourceSupply") || !this.get("ResourceSupply/KillBeforeGather"))
			return false;

		// special case: rabbits too difficult to hunt for such a small food amount
		if (this.get("Identity") && this.get("Identity/SpecificName") && this.get("Identity/SpecificName") === "Rabbit")
			return false;

		// do not hunt retaliating animals (animals without UnitAI are dead animals)
		return !this.get("UnitAI") || !(this.get("UnitAI/NaturalBehaviour") === "violent" ||
			this.get("UnitAI/NaturalBehaviour") === "aggressive" ||
			this.get("UnitAI/NaturalBehaviour") === "defensive");
	},

	walkSpeed: function() {
		if (!this.get("UnitMotion") || !this.get("UnitMotion/WalkSpeed"))
			 return undefined;
		return +this.get("UnitMotion/WalkSpeed");
	},

	trainingCategory: function() {
		if (!this.get("TrainingRestrictions") || !this.get("TrainingRestrictions/Category"))
			return undefined;
		return this.get("TrainingRestrictions/Category");
	},

	buildCategory: function() {
		if (!this.get("BuildRestrictions") || !this.get("BuildRestrictions/Category"))
			return undefined;
		return this.get("BuildRestrictions/Category");
	},
	
	buildTime: function() {
		if (!this.get("Cost") || !this.get("Cost/BuildTime"))
			return undefined;
		return +this.get("Cost/BuildTime");
	},

	buildDistance: function() {
		if (!this.get("BuildRestrictions") || !this.get("BuildRestrictions/Distance"))
			return undefined;
		return this.get("BuildRestrictions/Distance");
	},

	buildPlacementType: function() {
		if (!this.get("BuildRestrictions") || !this.get("BuildRestrictions/PlacementType"))
			return undefined;
		return this.get("BuildRestrictions/PlacementType");
	},

	buildTerritories: function() {
		if (!this.get("BuildRestrictions") || !this.get("BuildRestrictions/Territory"))
			return undefined;
		return this.get("BuildRestrictions/Territory").split(/\s+/);
	},

	hasBuildTerritory: function(territory) {
		var territories = this.buildTerritories();
		return (territories && territories.indexOf(territory) != -1);
	},

	hasTerritoryInfluence: function() {
		return (this.get("TerritoryInfluence") !== undefined);
	},

	hasDefensiveFire: function() {
		if (!this.get("Attack") || !this.get("Attack/Ranged"))
			return false;
		return (this.getDefaultArrow() || this.getArrowMultiplier());
	},

	territoryInfluenceRadius: function() {
		if (this.get("TerritoryInfluence") !== undefined)
			return +this.get("TerritoryInfluence/Radius");
		else
			return -1;
	},

	territoryInfluenceWeight: function() {
		if (this.get("TerritoryInfluence") !== undefined)
			return +this.get("TerritoryInfluence/Weight");
		else
			return -1;
	},

	territoryDecayRate: function() {
		return (this.get("TerritoryDecay") ? +this.get("TerritoryDecay/DecayRate") : 0);
	},

	defaultRegenRate: function() {
		return (this.get("Capturable") ? +this.get("Capturable/RegenRate") : 0);
	},

	garrisonRegenRate: function() {
		return (this.get("Capturable") ? +this.get("Capturable/GarrisonRegenRate") : 0);
	},

	visionRange: function() {
		return +this.get("Vision/Range");
	}
});


// defines an entity, with a super Template.
// also redefines several of the template functions where the only change is applying aura and tech modifications.
m.Entity = m.Class({
	_super: m.Template,

	_init: function(sharedAI, entity)
	{
		this._super.call(this, sharedAI.GetTemplate(entity.template));

		this._templateName = entity.template;
		this._entity = entity;
		this._auraTemplateModif = new Map();	// template modification from auras. this is only for this entity.
		this._ai = sharedAI;
		if (!sharedAI._techModifications[entity.owner][this._templateName])
			sharedAI._techModifications[entity.owner][this._templateName] = new Map();
		this._techModif = sharedAI._techModifications[entity.owner][this._templateName]; // save a reference to the template tech modifications
	},

	toString: function() {
		return "[Entity " + this.id() + " " + this.templateName() + "]";
	},

	id: function() {
		return this._entity.id;
	},

	templateName: function() {
		return this._templateName;
	},

	/**
	 * Returns extra data that the AI scripts have associated with this entity,
	 * for arbitrary local annotations.
	 * (This data should not be shared with any other AI scripts.)
	 */
	getMetadata: function(player, key) {
		return this._ai.getMetadata(player, this, key);
	},

	/**
	 * Sets extra data to be associated with this entity.
	 */
	setMetadata: function(player, key, value) {
		this._ai.setMetadata(player, this, key, value);
	},

	deleteAllMetadata: function(player) {
		delete this._ai._entityMetadata[player][this.id()];
	},

	deleteMetadata: function(player, key) {
		this._ai.deleteMetadata(player, this, key);
	},

	position: function() { return this._entity.position; },

	isIdle: function() {
		if (typeof this._entity.idle === "undefined")
			return undefined;
		return this._entity.idle;
	},

	unitAIState: function() { return ((this._entity.unitAIState !== undefined) ? this._entity.unitAIState : undefined); },
	unitAIOrderData: function() { return ((this._entity.unitAIOrderData !== undefined) ? this._entity.unitAIOrderData : undefined); },

	hitpoints: function() { return ((this._entity.hitpoints !== undefined) ? this._entity.hitpoints : undefined); },
	isHurt: function() { return (this.hitpoints() < this.maxHitpoints()); },
	healthLevel: function() { return (this.hitpoints() / this.maxHitpoints()); },
	needsHeal: function() { return this.isHurt() && this.isHealable(); },
	needsRepair: function() { return this.isHurt() && this.isRepairable(); },
	decaying: function() { return ((this._entity.decaying !== undefined) ? this._entity.decaying : undefined); },
	capturePoints: function() {return ((this._entity.capturePoints !== undefined) ? this._entity.capturePoints : undefined); },

	/**
	 * Returns the current training queue state, of the form
	 * [ { "id": 0, "template": "...", "count": 1, "progress": 0.5, "metadata": ... }, ... ]
	 */
	trainingQueue: function() {
		var queue = this._entity.trainingQueue;
		return queue;
	},

	trainingQueueTime: function() {
		var queue = this._entity.trainingQueue;
		if (!queue)
			return undefined;
		var time = 0;
		for (var item of queue)
			time += item.timeRemaining;
		return time/1000;
	},

	foundationProgress: function() {
		if (this._entity.foundationProgress === undefined)
			return undefined;
		return this._entity.foundationProgress;
	},

	getBuilders: function() {
		if (this._entity.foundationProgress === undefined)
			return undefined;
		if (this._entity.foundationBuilders === undefined)
			return [];
		return this._entity.foundationBuilders;
	},

	getBuildersNb: function() {
		if (this._entity.foundationProgress === undefined)
			return undefined;
		if (this._entity.foundationBuilders === undefined)
			return 0;
		return this._entity.foundationBuilders.length;
	},

	owner: function() {
		return this._entity.owner;
	},

	isOwn: function(player) {
		if (typeof(this._entity.owner) === "undefined")
			return false;
		return this._entity.owner === player;
	},

	resourceSupplyAmount: function() {
		if(this._entity.resourceSupplyAmount === undefined)
			return undefined;
		return this._entity.resourceSupplyAmount;
	},

	resourceSupplyNumGatherers: function()
	{
		if (this._entity.resourceSupplyNumGatherers !== undefined)
			return this._entity.resourceSupplyNumGatherers;
		return undefined;
	},

	isFull: function()
	{
		if (this._entity.resourceSupplyNumGatherers !== undefined)
			return (this.maxGatherers() === this._entity.resourceSupplyNumGatherers);

		return undefined;
	},

	resourceCarrying: function() {
		if(this._entity.resourceCarrying === undefined)
			return undefined;
		return this._entity.resourceCarrying; 
	},

	currentGatherRate: function() {
		// returns the gather rate for the current target if applicable.
		if (!this.get("ResourceGatherer"))
			return undefined;

		if (this.unitAIOrderData().length &&
			(this.unitAIState().split(".")[1] === "GATHER" || this.unitAIState().split(".")[1] === "RETURNRESOURCE"))
		{
			let res;
			// this is an abuse of "_ai" but it works.
			if (this.unitAIState().split(".")[1] === "GATHER" && this.unitAIOrderData()[0].target !== undefined)
				res = this._ai._entities.get(this.unitAIOrderData()[0].target);
			else if (this.unitAIOrderData()[1] !== undefined && this.unitAIOrderData()[1].target !== undefined)
				res = this._ai._entities.get(this.unitAIOrderData()[1].target);
			if (!res)
				return 0;
			let type = res.resourceSupplyType();
			if (!type)
				return 0;

			if (type.generic === "treasure")
				return 1000;

			let tstring = type.generic + "." + type.specific;
			let rate = +this.get("ResourceGatherer/BaseSpeed");
			rate *= +this.get("ResourceGatherer/Rates/" +tstring);
			if (rate)
				return rate;
			return 0;
		}
		return undefined;
	},

	isBuilder: function() { return this.get("Builder") !== undefined; },
	isGatherer: function() { return this.get("ResourceGatherer") !== undefined; },
	canGather: function(type) {
		if (!this.get("ResourceGatherer") || !this.get("ResourceGatherer/Rates"))
			return false;
		for (let r in this.get("ResourceGatherer/Rates"))
			if (r.split('.')[0] === type)
				return true;
		return false;
	},

	isGarrisonHolder: function() { return this.get("GarrisonHolder") !== undefined; },
	garrisoned: function() { return this._entity.garrisoned; },
	canGarrisonInside: function() { return this._entity.garrisoned.length < this.garrisonMax(); },

	move: function(x, z, queued = false) {
		Engine.PostCommand(PlayerID,{"type": "walk", "entities": [this.id()], "x": x, "z": z, "queued": queued });
		return this;
	},

	moveToRange: function(x, z, min, max, queued = false) {
		Engine.PostCommand(PlayerID,{"type": "walk-to-range", "entities": [this.id()], "x": x, "z": z, "min": min, "max": max, "queued": queued });
		return this;
	},

	attackMove: function(x, z, targetClasses, queued = false) {
		Engine.PostCommand(PlayerID,{"type": "attack-walk", "entities": [this.id()], "x": x, "z": z, "targetClasses": targetClasses, "queued": queued });
		return this;
	},

	// violent, aggressive, defensive, passive, standground
	setStance: function(stance, queued = false) {
		Engine.PostCommand(PlayerID,{"type": "stance", "entities": [this.id()], "name" : stance, "queued": queued });
		return this;
	},

	stopMoving: function() {
		Engine.PostCommand(PlayerID,{"type": "stop", "entities": [this.id()], "queued": false});
	},

	unload: function(id) {
		if (!this.get("GarrisonHolder"))
			return undefined;
		Engine.PostCommand(PlayerID,{"type": "unload", "garrisonHolder": this.id(), "entities": [id]});
		return this;
	},

	// Unloads all owned units, don't unload allies
	unloadAll: function() {
		if (!this.get("GarrisonHolder"))
			return undefined;
		Engine.PostCommand(PlayerID,{"type": "unload-all-by-owner", "garrisonHolders": [this.id()]});
		return this;
	},

	garrison: function(target, queued = false) {
		Engine.PostCommand(PlayerID,{"type": "garrison", "entities": [this.id()], "target": target.id(),"queued": queued});
		return this;
	},

	attack: function(unitId, allowCapture = true, queued = false) {
		Engine.PostCommand(PlayerID,{"type": "attack", "entities": [this.id()], "target": unitId, "allowCapture": allowCapture, "queued": queued});
		return this;
	},

	// moveApart from a point in the opposite direction with a distance dist
	moveApart: function(point, dist) {
		if (this.position() !== undefined) {
			var direction = [this.position()[0] - point[0], this.position()[1] - point[1]];
			var norm = m.VectorDistance(point, this.position());
			if (norm === 0)
				direction = [1, 0];
			else
			{
				direction[0] /= norm;
				direction[1] /= norm;
			}
			Engine.PostCommand(PlayerID,{"type": "walk", "entities": [this.id()], "x": this.position()[0] + direction[0]*dist, "z": this.position()[1] + direction[1]*dist, "queued": false});
		}
		return this;
	},

	// Flees from a unit in the opposite direction.
	flee: function(unitToFleeFrom) {
		if (this.position() !== undefined && unitToFleeFrom.position() !== undefined) {
			var FleeDirection = [this.position()[0] - unitToFleeFrom.position()[0],this.position()[1] - unitToFleeFrom.position()[1]];
			var dist = m.VectorDistance(unitToFleeFrom.position(), this.position() );
			FleeDirection[0] = (FleeDirection[0]/dist) * 8;
			FleeDirection[1] = (FleeDirection[1]/dist) * 8;

			Engine.PostCommand(PlayerID,{"type": "walk", "entities": [this.id()], "x": this.position()[0] + FleeDirection[0]*5, "z": this.position()[1] + FleeDirection[1]*5, "queued": false});
		}
		return this;
	},

	gather: function(target, queued = false) {
		Engine.PostCommand(PlayerID,{"type": "gather", "entities": [this.id()], "target": target.id(), "queued": queued});
		return this;
	},

	repair: function(target, autocontinue = false, queued = false) {
		Engine.PostCommand(PlayerID,{"type": "repair", "entities": [this.id()], "target": target.id(), "autocontinue": autocontinue, "queued": queued});
		return this;
	},

	returnResources: function(target, queued = false) {
		Engine.PostCommand(PlayerID,{"type": "returnresource", "entities": [this.id()], "target": target.id(), "queued": queued});
		return this;
	},

	destroy: function() {
		Engine.PostCommand(PlayerID,{"type": "delete-entities", "entities": [this.id()] });
		return this;
	},

	barter: function(buyType, sellType, amount) {
		Engine.PostCommand(PlayerID,{"type": "barter", "sell" : sellType, "buy" : buyType, "amount" : amount });
		return this;
	},

	tradeRoute: function(target, source) {
		Engine.PostCommand(PlayerID,{"type": "setup-trade-route", "entities": [this.id()], "target": target.id(), "source": source.id(), "route": undefined, "queued": false });
		return this;
	},

	setRallyPoint: function(target, command) {
		var data = {"command": command, "target": target.id()};
		Engine.PostCommand(PlayerID, {"type": "set-rallypoint", "entities": [this.id()], "x": target.position()[0], "z": target.position()[1], "data": data});
		return this;
	},

	unsetRallyPoint: function() {
		Engine.PostCommand(PlayerID, {"type": "unset-rallypoint", "entities": [this.id()]});
		return this;
	},

	train: function(civ, type, count, metadata, promotedTypes)
	{
		var trainable = this.trainableEntities(civ);
		if (!trainable)
		{
			error("Called train("+type+", "+count+") on non-training entity "+this);
			return this;
		}
		if (trainable.indexOf(type) === -1)
		{
			error("Called train("+type+", "+count+") on entity "+this+" which can't train that");
			return this;
		}

		Engine.PostCommand(PlayerID,{
			"type": "train",
			"entities": [this.id()],
			"template": type,
			"count": count,
			"metadata": metadata,
			"promoted": promotedTypes
		});
		return this;
	},

	construct: function(template, x, z, angle, metadata) {
		// TODO: verify this unit can construct this, just for internal
		// sanity-checking and error reporting

		Engine.PostCommand(PlayerID,{
			"type": "construct",
			"entities": [this.id()],
			"template": template,
			"x": x,
			"z": z,
			"angle": angle,
			"autorepair": false,
			"autocontinue": false,
			"queued": false,
			"metadata" : metadata	// can be undefined
		});
		return this;
	},

	 research: function(template) {
		Engine.PostCommand(PlayerID,{ "type": "research", "entity": this.id(), "template": template });
		return this;
	},

	stopProduction: function(id) {
		Engine.PostCommand(PlayerID,{ "type": "stop-production", "entity": this.id(), "id": id });
		return this;
	},

	stopAllProduction: function(percentToStopAt) {
		var queue = this._entity.trainingQueue;
		if (!queue)
			return true;	// no queue, so technically we stopped all production.
		for (var item of queue)
			if (item.progress < percentToStopAt)
				Engine.PostCommand(PlayerID,{ "type": "stop-production", "entity": this.id(), "id": item.id });
		return this;
	}
});

return m;

}(API3);
