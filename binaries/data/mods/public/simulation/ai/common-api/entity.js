var API3 = function(m)
{

// defines a template.
// It's completely raw data, except it's slightly cleverer now and then.
m.Template = m.Class({

	_init: function(template)
	{
		this._template = template;
		this._tpCache = {};
	},
	
	// helper function to return a template value, optionally adjusting for tech.
	// TODO: there's no support for "_string" values here.
	get: function(string)
	{
		var value = this._template;
		if (this._auraTemplateModif && this._auraTemplateModif[string])		{
			return this._auraTemplateModif[string];
		} else if (this._techModif && this._techModif[string]) {
			return this._techModif[string];
		} else {
			if (this._tpCache[string] === undefined)
			{
				var args = string.split("/");
				for (var i = 0; i < args.length; ++i)
					if (value[args[i]])
						value = value[args[i]];
					else
					{
						value = undefined;
						break;
					}
				this._tpCache[string] = value;
			}
			return this._tpCache[string];
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
		if (!this.get("Identity") || !this.get("Identity/Classes") || !this.get("Identity/Classes/_string"))
			return undefined;
		return this.get("Identity/Classes/_string").split(/\s+/);
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
		var classes = this.classes();
		return (classes && classes.indexOf(name) != -1);
	},
	
	hasClasses: function(array) {
		var classes = this.classes();
		if (!classes)
			return false;
		
		for (var i in array)
			if (classes.indexOf(array[i]) === -1)
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
		if (this.get("Health") !== undefined)
			return this.get("Health/Repairable") === "true";
		return false;
	},

	getPopulationBonus: function() {
		return this.get("Cost/PopulationBonus");
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
		for (var i in this.get("Attack")) {
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
		for (var i in this.get("Attack")) {
			if (!this.get("Attack/" + i + "/Bonuses"))
				continue;
			for (var o in this.get("Attack/" + i + "/Bonuses"))
				if (this.get("Attack/" + i + "/Bonuses/" + o + "/Classes"))
					mcounter.concat(this.get("Attack/" + i + "/Bonuses/" + o + "/Classes").split(" "));
		}
		for (var i in classes)
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
			for (var o in this.get("Attack/" + type + "/Bonuses")) {
				if (!this.get("Attack/" + type + "/Bonuses/" + o + "/Classes"))
					continue;
				var total = this.get("Attack/" + type + "/Bonuses/" + o + "/Classes").split(" ");
				for (var j in total)
					if (total[j] === againstClass)
						return this.get("Attack/" + type + "/Bonuses/" + o + "/Multiplier");
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

	trainableEntities: function() {
		if (!this.get("ProductionQueue/Entities/_string"))
			return undefined;
		var civ = this.civ();
		var templates = this.get("ProductionQueue/Entities/_string").replace(/\{civ\}/g, civ).split(/\s+/);
		return templates;
	},

	researchableTechs: function() {
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
		return this.get("ResourceDropsite/Types").split(/\s+/);
	},


	garrisonableClasses: function() {
		if (!this.get("GarrisonHolder") || !this.get("GarrisonHolder/List/_string"))
			return undefined;
		return this.get("GarrisonHolder/List/_string").split(/\s+/);
	},

	garrisonMax: function() {
		if (!this.get("GarrisonHolder"))
			return undefined;
		return this.get("GarrisonHolder/Max");
	},
	
	/**
	 * Returns whether this is an animal that is too difficult to hunt.
	 * (Any non domestic currently.)
	 */
	isUnhuntable: function() {
		if (!this.get("UnitAI") || !this.get("UnitAI/NaturalBehaviour"))
			return false;

		// only attack domestic animals since they won't flee nor retaliate.
		return this.get("UnitAI/NaturalBehaviour") !== "domestic";
	},
						  
	walkSpeed: function() {
		if (!this.get("UnitMotion") || !this.get("UnitMotion/WalkSpeed"))
			 return undefined;
		return this.get("UnitMotion/WalkSpeed");
	},

	buildCategory: function() {
		if (!this.get("BuildRestrictions") || !this.get("BuildRestrictions/Category"))
			return undefined;
		return this.get("BuildRestrictions/Category");
	},
	
	buildTime: function() {
		if (!this.get("Cost") || !this.get("Cost/BuildTime"))
			return undefined;
		return this.get("Cost/BuildTime");
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

	territoryInfluenceRadius: function() {
		if (this.get("TerritoryInfluence") !== undefined)
			return (this.get("TerritoryInfluence/Radius"));
		else
			return -1;
	},

	territoryInfluenceWeight: function() {
		if (this.get("TerritoryInfluence") !== undefined)
			return (this.get("TerritoryInfluence/Weight"));
		else
			return -1;
	},

	visionRange: function() {
		return this.get("Vision/Range");
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
		this._auraTemplateModif = {};	// template modification from auras. this is only for this entity.
		this._ai = sharedAI;
		if (!sharedAI._techModifications[entity.owner][this._templateName])
			sharedAI._techModifications[entity.owner][this._templateName] = {};
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
	
	unitAIState: function() { return this._entity.unitAIState; },
	unitAIOrderData: function() { return this._entity.unitAIOrderData; },
	
	hitpoints: function() {if (this._entity.hitpoints !== undefined) return this._entity.hitpoints; return undefined; },
	isHurt: function() { return this.hitpoints() < this.maxHitpoints(); },
	healthLevel: function() { return (this.hitpoints() / this.maxHitpoints()); },
	needsHeal: function() { return this.isHurt() && this.isHealable(); },
	needsRepair: function() { return this.isHurt() && this.isRepairable(); },

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
		// TODO: compute total time for units in training queue
		return queue.length;
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
	
	isFriendly: function(player) {
		return this.isOwn(player); // TODO: diplomacy
	},
	
	isEnemy: function(player) {
		return !this.isOwn(player); // TODO: diplomacy
	},

	resourceSupplyAmount: function() {
		if(this._entity.resourceSupplyAmount === undefined)
			return undefined;
		return this._entity.resourceSupplyAmount;
	},
				  
	resourceSupplyGatherers: function()
	{
		if (this._entity.resourceSupplyGatherers !== undefined)
			return this._entity.resourceSupplyGatherers;
		return [];
	},
				  
	isFull: function()
	{
		if (this._entity.resourceSupplyGatherers !== undefined)
			return (this.maxGatherers() === this._entity.resourceSupplyGatherers.length);

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
			var ress = undefined;
			// this is an abuse of "_ai" but it works.
			if (this.unitAIState().split(".")[1] === "GATHER" && this.unitAIOrderData()[0]["target"] !== undefined)
				ress = this._ai._entities[this.unitAIOrderData()[0]["target"]];
			else if (this.unitAIOrderData()[1] !== undefined && this.unitAIOrderData()[1]["target"] !== undefined)
				ress = this._ai._entities[this.unitAIOrderData()[1]["target"]];
			
			if (ress == undefined)
				return undefined;
			
			var type = ress.resourceSupplyType();
			var tstring = type.generic + "." + type.specific;
				  
			if (type.generic == "treasure")
				return 1000;
				
			var speed = +this.get("ResourceGatherer/BaseSpeed");
			speed *= +this.get("ResourceGatherer/Rates/" +tstring);
				  
			if (speed)
				return speed;
			return 0;
		}
		return undefined;
	},

	garrisoned: function() { return new m.EntityCollection(this._ai, this._entity.garrisoned); },
	
	canGarrisonInside: function() { return this._entity.garrisoned.length < this.garrisonMax(); },

	// TODO: visibility

	move: function(x, z, queued) {
		queued = queued || false;
		Engine.PostCommand(PlayerID,{"type": "walk", "entities": [this.id()], "x": x, "z": z, "queued": queued });
		return this;
	},
	
	attackMove: function(x, z, queued) {
		queued = queued || false;
		Engine.PostCommand(PlayerID,{"type": "attack-walk", "entities": [this.id()], "x": x, "z": z, "queued": queued });
		return this;
	},

	// violent, aggressive, defensive, passive, standground
	setStance: function(stance,queued){
		Engine.PostCommand(PlayerID,{"type": "stance", "entities": [this.id()], "name" : stance, "queued": queued });
		return this;
	},

	// TODO: replace this with the proper "STOP" command
	stopMoving: function() {
		if (this.position() !== undefined)
			Engine.PostCommand(PlayerID,{"type": "walk", "entities": [this.id()], "x": this.position()[0], "z": this.position()[1], "queued": false});
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
		Engine.PostCommand(PlayerID,{"type": "unload-all-own", "garrisonHolders": [this.id()]});
		return this;
	},

	garrison: function(target) {
		Engine.PostCommand(PlayerID,{"type": "garrison", "entities": [this.id()], "target": target.id(),"queued": false});
		return this;
	},

	attack: function(unitId) {
		Engine.PostCommand(PlayerID,{"type": "attack", "entities": [this.id()], "target": unitId, "queued": false});
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

	gather: function(target, queued) {
		queued = queued || false;
		Engine.PostCommand(PlayerID,{"type": "gather", "entities": [this.id()], "target": target.id(), "queued": queued});
		return this;
	},

	repair: function(target, queued) {
		queued = queued || false;
		Engine.PostCommand(PlayerID,{"type": "repair", "entities": [this.id()], "target": target.id(), "autocontinue": false, "queued": queued});
		return this;
	},
	
	returnResources: function(target, queued) {
		queued = queued || false;
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
	
	train: function(type, count, metadata)
	{
		var trainable = this.trainableEntities();
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
			"metadata": metadata
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
		for (var i in queue)
		{
			if (queue[i].progress < percentToStopAt)
				Engine.PostCommand(PlayerID,{ "type": "stop-production", "entity": this.id(), "id": queue[i].id });
		}
		return this;
	}
});

return m;

}(API3);
