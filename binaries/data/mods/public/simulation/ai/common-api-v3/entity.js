var EntityTemplate = Class({

	// techModifications should be the tech modifications of only one player.
	// gamestates handle "GetTemplate" and should push the player's
	// while entities should push the owner's
	_init: function(template, techModifications)
	{
		this._techModifications = techModifications;
		this._template = template;
	},
	
	genericName: function() {
		if (!this._template.Identity || !this._template.Identity.GenericName)
			return undefined;
		return this._template.Identity.GenericName;
	},
						   
	rank: function() {
		if (!this._template.Identity)
			return undefined;
		return this._template.Identity.Rank;
	},

	classes: function() {
		if (!this._template.Identity || !this._template.Identity.Classes || !this._template.Identity.Classes._string)
			return undefined;
		return this._template.Identity.Classes._string.split(/\s+/);
	},
	
	requiredTech: function() {
		if (!this._template.Identity || !this._template.Identity.RequiredTechnology)
			return undefined;
		return this._template.Identity.RequiredTechnology;
	},
						   
	available: function(gameState) {
		if (!this._template.Identity || !this._template.Identity.RequiredTechnology)
			return true;
		return gameState.isResearched(this._template.Identity.RequiredTechnology);
	},
						   
	// specifically
	phase: function() {
		if (!this._template.Identity || !this._template.Identity.RequiredTechnology)
			return 0;
		if (this.template.Identity.RequiredTechnology == "phase_village")
			return 1;
		if (this.template.Identity.RequiredTechnology == "phase_town")
			return 2;
		if (this.template.Identity.RequiredTechnology == "phase_city")
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
		if (!this._template.Identity)
			return undefined;
		return this._template.Identity.Civ;
	},

	cost: function() {
		if (!this._template.Cost)
			return undefined;

		var ret = {};
		for (var type in this._template.Cost.Resources)
			ret[type] = GetTechModifiedProperty(this._techModifications, this._template, "Cost/Resources/"+type, +this._template.Cost.Resources[type]);
		return ret;
	},
	
	costSum: function() {
		if (!this._template.Cost)
			return undefined;
		
		var ret = 0;
		for (var type in this._template.Cost.Resources)
			ret += +this._template.Cost.Resources[type];
		return ret;
	},

	/**
	 * Returns the radius of a circle surrounding this entity's
	 * obstruction shape, or undefined if no obstruction.
	 */
	obstructionRadius: function() {
		if (!this._template.Obstruction)
			return undefined;

		if (this._template.Obstruction.Static)
		{
			var w = +this._template.Obstruction.Static["@width"];
			var h = +this._template.Obstruction.Static["@depth"];
			return Math.sqrt(w*w + h*h) / 2;
		}

		if (this._template.Obstruction.Unit)
			return +this._template.Obstruction.Unit["@radius"];

		return 0; // this should never happen
	},
						   
	/**
	 * Returns the radius of a circle surrounding this entity's
	 * footprint.
	 */
	footprintRadius: function() {
		if (!this._template.Footprint)
			return undefined;
		
		if (this._template.Footprint.Square)
		{
			var w = +this._template.Footprint.Square["@width"];
			var h = +this._template.Footprint.Square["@depth"];
			return Math.sqrt(w*w + h*h) / 2;
		}
		
		if (this._template.Footprint.Circle)
			return +this._template.Footprint.Circle["@radius"];
		
		return 0; // this should never happen
	},

	maxHitpoints: function()
	{
		if (this._template.Health !== undefined)
			return this._template.Health.Max;
		return 0;
	},
	isHealable: function()
	{
		if (this._template.Health !== undefined)
			return this._template.Health.Unhealable !== "true";
		return false;
	},
	isRepairable: function()
	{
		if (this._template.Health !== undefined)
			return this._template.Health.Repairable === "true";
		return false;
	},

	getPopulationBonus: function() {
		if (!this._template.Cost || !this._template.Cost.PopulationBonus)
			return undefined;
		return this._template.Cost.PopulationBonus;
	},

	armourStrengths: function() {
		if (!this._template.Armour)
			return undefined;

		return {
			hack: GetTechModifiedProperty(this._techModifications, this._template, "Armour/Hack", +this._template.Armour.Hack),
			pierce: GetTechModifiedProperty(this._techModifications, this._template, "Armour/Pierce", +this._template.Armour.Pierce),
			crush: GetTechModifiedProperty(this._techModifications, this._template, "Armour/Crush", +this._template.Armour.Crush)
		};
	},

	attackTypes: function() {
		if (!this._template.Attack)
			return undefined;

		var ret = [];
		for (var type in this._template.Attack)
			ret.push(type);

		return ret;
	},

	attackRange: function(type) {
		if (!this._template.Attack || !this._template.Attack[type])
			return undefined;

		return {
				max: GetTechModifiedProperty(this._techModifications, this._template, "Attack/MaxRange", +this._template.Attack[type].MaxRange),
				min: GetTechModifiedProperty(this._techModifications, this._template, "Attack/MinRange", +(this._template.Attack[type].MinRange || 0))
		};
	},

	attackStrengths: function(type) {
		if (!this._template.Attack || !this._template.Attack[type])
			return undefined;

		return {
			hack: GetTechModifiedProperty(this._techModifications, this._template, "Attack/"+type+"/Hack", +(this._template.Attack[type].Hack || 0)),
			pierce: GetTechModifiedProperty(this._techModifications, this._template, "Attack/"+type+"/Pierce", +(this._template.Attack[type].Pierce || 0)),
			crush: GetTechModifiedProperty(this._techModifications, this._template, "Attack/"+type+"/Crush", +(this._template.Attack[type].Crush || 0))
		};
	},
	
	attackTimes: function(type) {
		if (!this._template.Attack || !this._template.Attack[type])
			return undefined;

		return {
			prepare: GetTechModifiedProperty(this._techModifications, this._template, "Attack/"+type+"/PrepareTime", +(this._template.Attack[type].PrepareTime || 0)),
			repeat: GetTechModifiedProperty(this._techModifications, this._template, "Attack/"+type+"/RepeatTime", +(this._template.Attack[type].RepeatTime || 1000))
		};
	},

	// returns the classes this templates counters:
	// Return type is [ [-neededClasses-] , multiplier ].
	getCounteredClasses: function() {
		if (!this._template.Attack)
			return undefined;
		
		var Classes = [];
		for (var i in this._template.Attack) {
			if (!this._template.Attack[i].Bonuses)
				continue;
			for (var o in this._template.Attack[i].Bonuses)
				if (this._template.Attack[i].Bonuses[o].Classes)
					Classes.push([this._template.Attack[i].Bonuses[o].Classes.split(" "), +this._template.Attack[i].Bonuses[o].Multiplier]);
		}
		return Classes;
	},

	// returns true if the entity counters those classes.
	// TODO: refine using the multiplier
	countersClasses: function(classes) {
		if (!this._template.Attack)
			return false;
		var mcounter = [];
		for (var i in this._template.Attack) {
			if (!this._template.Attack[i].Bonuses)
				continue;
			for (var o in this._template.Attack[i].Bonuses)
				if (this._template.Attack[i].Bonuses[o].Classes)
					mcounter.concat(this._template.Attack[i].Bonuses[o].Classes.split(" "));
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
		if (!this._template.Attack || !this._template.Attack[type])
			return undefined;

		if (this._template.Attack[type].Bonuses)
			for (var o in this._template.Attack[type].Bonuses) {
				if (!this._template.Attack[type].Bonuses[o].Classes)
					continue;
				var total = this._template.Attack[type].Bonuses[o].Classes.split(" ");
				for (var j in total)
					if (total[j] === againstClass)
						return this._template.Attack[type].Bonuses[o].Multiplier;
			}
		return 1;
	},

	// returns true if the entity can attack the given class
	canAttackClass: function(saidClass) {
		if (!this._template.Attack)
			return false;
		
		for (var i in this._template.Attack) {
			if (!this._template.Attack[i].RestrictedClasses || !this._template.Attack[i].RestrictedClasses._string)
				continue;
			var cannotAttack = this._template.Attack[i].RestrictedClasses._string.split(" ");
			if (cannotAttack.indexOf(saidClass) !== -1)
				return false;
		}
		return true;
	},

	buildableEntities: function() {
		if (!this._template.Builder)
			return undefined;
		if (!this._template.Builder.Entities._string)
			return [];
		var civ = this.civ();
		var templates = this._template.Builder.Entities._string.replace(/\{civ\}/g, civ).split(/\s+/);
		return templates; // TODO: map to Entity?
	},

	trainableEntities: function() {
		if (!this._template.ProductionQueue || !this._template.ProductionQueue.Entities || !this._template.ProductionQueue.Entities._string)
			return undefined;
		var civ = this.civ();
		var templates = this._template.ProductionQueue.Entities._string.replace(/\{civ\}/g, civ).split(/\s+/);
		return templates;
	},

	researchableTechs: function() {
		if (!this._template.ProductionQueue || !this._template.ProductionQueue.Technologies || !this._template.ProductionQueue.Technologies._string)
			return undefined;
		var templates = this._template.ProductionQueue.Technologies._string.split(/\s+/);
		return templates;
	},

	resourceSupplyType: function() {
		if (!this._template.ResourceSupply)
			return undefined;
		var [type, subtype] = this._template.ResourceSupply.Type.split('.');
		return { "generic": type, "specific": subtype };
	},
	// will return either "food", "wood", "stone", "metal" and not treasure.
	getResourceType: function() {
		if (!this._template.ResourceSupply)
			return undefined;
		var [type, subtype] = this._template.ResourceSupply.Type.split('.');
		if (type == "treasure")
			return subtype;
		return type;
	},

	resourceSupplyMax: function() {
		if (!this._template.ResourceSupply)
			return undefined;
		return +this._template.ResourceSupply.Amount;
	},

	maxGatherers: function()
	{
		if (this._template.ResourceSupply !== undefined)
			return +this._template.ResourceSupply.MaxGatherers;
		return 0;
	},

	resourceGatherRates: function() {
		if (!this._template.ResourceGatherer)
			return undefined;
		var ret = {};
		var baseSpeed = GetTechModifiedProperty(this._techModifications, this._template, "ResourceGatherer/BaseSpeed", +this._template.ResourceGatherer.BaseSpeed);
		for (var r in this._template.ResourceGatherer.Rates)
			ret[r] = GetTechModifiedProperty(this._techModifications, this._template, "ResourceGatherer/Rates/"+r, +this._template.ResourceGatherer.Rates[r]) * baseSpeed;
		return ret;
	},

	resourceDropsiteTypes: function() {
		if (!this._template.ResourceDropsite)
			return undefined;
		return this._template.ResourceDropsite.Types.split(/\s+/);
	},


	garrisonableClasses: function() {
		if (!this._template.GarrisonHolder || !this._template.GarrisonHolder.List._string)
			return undefined;
		return this._template.GarrisonHolder.List._string.split(/\s+/);
	},

	garrisonMax: function() {
		if (!this._template.GarrisonHolder)
			return undefined;
		return this._template.GarrisonHolder.Max;
	},
	
	/**
	 * Returns whether this is an animal that is too difficult to hunt.
	 * (Any non domestic currently.)
	 */
	isUnhuntable: function() {
		if (!this._template.UnitAI || !this._template.UnitAI.NaturalBehaviour)
			return false;

		// only attack domestic animals since they won't flee nor retaliate.
		return this._template.UnitAI.NaturalBehaviour !== "domestic";
	},
						   
	walkSpeed: function() {
		if (!this._template.UnitMotion || !this._template.UnitMotion.WalkSpeed)
			 return undefined;
		return this._template.UnitMotion.WalkSpeed;
	},

	buildCategory: function() {
		if (!this._template.BuildRestrictions || !this._template.BuildRestrictions.Category)
			return undefined;
		return this._template.BuildRestrictions.Category;
	},
	
	buildTime: function() {
		if (!this._template.Cost || !this._template.Cost.BuildTime)
			return undefined;
		return this._template.Cost.BuildTime;
	},

	buildDistance: function() {
		if (!this._template.BuildRestrictions || !this._template.BuildRestrictions.Distance)
			return undefined;
		return this._template.BuildRestrictions.Distance;
	},

	buildPlacementType: function() {
		if (!this._template.BuildRestrictions || !this._template.BuildRestrictions.PlacementType)
			return undefined;
		return this._template.BuildRestrictions.PlacementType;
	},

	buildTerritories: function() {
		if (!this._template.BuildRestrictions || !this._template.BuildRestrictions.Territory)
			return undefined;
		return this._template.BuildRestrictions.Territory.split(/\s+/);
	},

	hasBuildTerritory: function(territory) {
		var territories = this.buildTerritories();
		return (territories && territories.indexOf(territory) != -1);
	},

	hasTerritoryInfluence: function() {
		return (this._template.TerritoryInfluence !== undefined);
	},

	territoryInfluenceRadius: function() {
		if (this._template.TerritoryInfluence !== undefined)
			return (this._template.TerritoryInfluence.Radius);
		else
			return -1;
	},

	territoryInfluenceWeight: function() {
		if (this._template.TerritoryInfluence !== undefined)
			return (this._template.TerritoryInfluence.Weight);
		else
			return -1;
	},

	visionRange: function() {
		if (!this._template.Vision)
			return undefined;
		return this._template.Vision.Range;
	}
});



var Entity = Class({
	_super: EntityTemplate,

	_init: function(sharedAI, entity)
	{
		this._super.call(this, sharedAI.GetTemplate(entity.template), sharedAI._techModifications[entity.owner]);

		this._ai = sharedAI;
		this._templateName = entity.template;
		this._entity = entity;
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
	 * (This data is not shared with any other AI scripts.)
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
	hitpoints: function() { if (this._entity.hitpoints !== undefined) return this._entity.hitpoints; return undefined; },
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
		if (this._entity.foundationProgress == undefined)
			return undefined;
		return this._entity.foundationProgress;
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

	garrisoned: function() { return new EntityCollection(this._ai, this._entity.garrisoned); },
	
	canGarrisonInside: function() { return this._entity.garrisoned.length < this.garrisonMax(); },

	// TODO: visibility

	move: function(x, z, queued) {
		queued = queued || false;
		Engine.PostCommand({"type": "walk", "entities": [this.id()], "x": x, "z": z, "queued": queued });
		return this;
	},
	
	attackMove: function(x, z, queued) {
		queued = queued || false;
		Engine.PostCommand({"type": "attack-walk", "entities": [this.id()], "x": x, "z": z, "queued": queued });
		return this;
	},

	// violent, aggressive, defensive, passive, standground
	setStance: function(stance,queued){
		Engine.PostCommand({"type": "stance", "entities": [this.id()], "name" : stance, "queued": queued });
		return this;
	},

	// TODO: replace this with the proper "STOP" command
	stopMoving: function() {
		if (this.position() !== undefined)
			Engine.PostCommand({"type": "walk", "entities": [this.id()], "x": this.position()[0], "z": this.position()[1], "queued": false});
	},

	unload: function(id) {
		if (!this._template.GarrisonHolder)
			return undefined;
		Engine.PostCommand({"type": "unload", "garrisonHolder": this.id(), "entities": [id]});
		return this;
	},

	// Unloads all owned units, don't unload allies
	unloadAll: function() {
		if (!this._template.GarrisonHolder)
			return undefined;
		Engine.PostCommand({"type": "unload-all-own", "garrisonHolders": [this.id()]});
		return this;
	},

	garrison: function(target) {
		Engine.PostCommand({"type": "garrison", "entities": [this.id()], "target": target.id(),"queued": false});
		return this;
	},

	attack: function(unitId) {
		Engine.PostCommand({"type": "attack", "entities": [this.id()], "target": unitId, "queued": false});
		return this;
	},
	
	// Flees from a unit in the opposite direction.
	flee: function(unitToFleeFrom) {
		if (this.position() !== undefined && unitToFleeFrom.position() !== undefined) {
			var FleeDirection = [this.position()[0] - unitToFleeFrom.position()[0],this.position()[1] - unitToFleeFrom.position()[1]];
			var dist = VectorDistance(unitToFleeFrom.position(), this.position() );
			FleeDirection[0] = (FleeDirection[0]/dist) * 8;
			FleeDirection[1] = (FleeDirection[1]/dist) * 8;
			
			Engine.PostCommand({"type": "walk", "entities": [this.id()], "x": this.position()[0] + FleeDirection[0]*5, "z": this.position()[1] + FleeDirection[1]*5, "queued": false});
		}
		return this;
	},

	gather: function(target, queued) {
		queued = queued || false;
		Engine.PostCommand({"type": "gather", "entities": [this.id()], "target": target.id(), "queued": queued});
		return this;
	},

	repair: function(target, queued) {
		queued = queued || false;
		Engine.PostCommand({"type": "repair", "entities": [this.id()], "target": target.id(), "autocontinue": false, "queued": queued});
		return this;
	},
	
	returnResources: function(target, queued) {
		queued = queued || false;
		Engine.PostCommand({"type": "returnresource", "entities": [this.id()], "target": target.id(), "queued": queued});
		return this;
	},

	destroy: function() {
		Engine.PostCommand({"type": "delete-entities", "entities": [this.id()] });
		return this;
	},
	
	barter: function(buyType, sellType, amount) {
		Engine.PostCommand({"type": "barter", "sell" : sellType, "buy" : buyType, "amount" : amount });
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

		Engine.PostCommand({
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

		Engine.PostCommand({
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
		Engine.PostCommand({ "type": "research", "entity": this.id(), "template": template });
		return this;
	},

	stopProduction: function(id) {
		Engine.PostCommand({ "type": "stop-production", "entity": this.id(), "id": id });
		return this;
	},
	
	stopAllProduction: function(percentToStopAt) {
		var queue = this._entity.trainingQueue;
		if (!queue)
			return true;	// no queue, so technically we stopped all production.
		for (var i in queue)
		{
			if (queue[i].progress < percentToStopAt)
				Engine.PostCommand({ "type": "stop-production", "entity": this.id(), "id": queue[i].id });
		}
		return this;
	}
});

