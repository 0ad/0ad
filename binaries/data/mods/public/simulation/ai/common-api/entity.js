var EntityTemplate = Class({

	_init: function(template) {
		this._template = template;
	},

	rank: function() {
		if (!this._template.Identity)
			return undefined;
		return this._template.Identity.Rank;
	},

	classes: function() {
		if (!this._template.Identity || !this._template.Identity.Classes)
			return undefined;
		return this._template.Identity.Classes._string.split(/\s+/);
	},

	hasClass: function(name) {
		var classes = this.classes();
		return (classes && classes.indexOf(name) != -1);
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
			ret[type] = +this._template.Cost.Resources[type];
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

	maxHitpoints: function() { return this._template.Health.Max; },
	isHealable: function() { return this._template.Health.Healable === "true"; },
	isRepairable: function() { return this._template.Health.Repairable === "true"; },


	armourStrengths: function() {
		if (!this._template.Armour)
			return undefined;

		return {
			hack: +this._template.Armour.Hack,
			pierce: +this._template.Armour.Pierce,
			crush: +this._template.Armour.Crush
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
			max: +this._template.Attack[type].MaxRange,
			min: +(this._template.Attack[type].MinRange || 0)
		};
	},

	attackStrengths: function(type) {
		if (!this._template.Attack || !this._template.Attack[type])
			return undefined;

		return {
			hack: +(this._template.Attack[type].Hack || 0),
			pierce: +(this._template.Attack[type].Pierce || 0),
			crush: +(this._template.Attack[type].Crush || 0)
		};
	},
	
	attackTimes: function(type) {
		if (!this._template.Attack || !this._template.Attack[type])
			return undefined;

		return {
			prepare: +(this._template.Attack[type].PrepareTime || 0),
			repeat: +(this._template.Attack[type].RepeatTime || 1000)
		};
	},


	buildableEntities: function() {
		if (!this._template.Builder)
			return undefined;
		var civ = this.civ();
		var templates = this._template.Builder.Entities._string.replace(/\{civ\}/g, civ).split(/\s+/);
		return templates; // TODO: map to Entity?
	},

	trainableEntities: function() {
		if (!this._template.TrainingQueue)
			return undefined;
		var civ = this.civ();
		var templates = this._template.TrainingQueue.Entities._string.replace(/\{civ\}/g, civ).split(/\s+/);
		return templates;
	},


	resourceSupplyType: function() {
		if (!this._template.ResourceSupply)
			return undefined;
		var [type, subtype] = this._template.ResourceSupply.Type.split('.');
		return { "generic": type, "specific": subtype };
	},

	resourceSupplyMax: function() {
		if (!this._template.ResourceSupply)
			return undefined;
		return +this._template.ResourceSupply.Amount;
	},

	resourceGatherRates: function() {
		if (!this._template.ResourceGatherer)
			return undefined;
		var ret = {};
		for (var r in this._template.ResourceGatherer.Rates)
			ret[r] = this._template.ResourceGatherer.Rates[r] * this._template.ResourceGatherer.BaseSpeed;
		return ret;
	},

	resourceDropsiteTypes: function() {
		if (!this._template.ResourceDropsite)
			return undefined;
		return this._template.ResourceDropsite.Types.split(/\s+/);
	},
	
	garrisonableClasses: function() {
		if (!this._template.GarrisonHolder)
			return undefined;
		return this._template.GarrisonHolder.List._string.split(/\s+/);
	},
	
	garrisonMax: function() {
		if (!this._template.GarrisonHolder)
			return undefined;
		
		return this._template.GarrisonHolder.Max;
	},
 
	//"Population Bonus" is how much a building raises your population cap. 
	getPopulationBonus: function() { 
		if (!this._template.Cost) 
			return undefined;
		
		return this._template.Cost.PopulationBonus; 
	}, 
 	
	/**
	 * Returns whether this is an animal that is too difficult to hunt.
	 * (Currently this just includes skittish animals, which are probably
	 * too fast to chase.)
	 */
	isUnhuntable: function() {
		if (!this._template.UnitAI || !this._template.UnitAI.NaturalBehaviour)
			return false;

		// return (this._template.UnitAI.NaturalBehaviour == "skittish");
		// Actually, since the AI is currently rubbish at hunting, skip all animals
		// that aren't really weak:
		return this._template.Health.Max >= 10;
	},

	buildCategory: function() {
		if (!this._template.BuildRestrictions || !this._template.BuildRestrictions.Category)
			return undefined;
		return this._template.BuildRestrictions.Category;
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
	
	visionRange: function() {
		if (!this._template.Vision)
			return undefined;
		return this._template.Vision.Range;
	},
});



var Entity = Class({
	_super: EntityTemplate,

	_init: function(baseAI, entity) {
		this._super.call(this, baseAI.GetTemplate(entity.template));

		this._ai = baseAI;
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
	getMetadata: function(id) {
		var metadata = this._ai._entityMetadata[this.id()];
		if (!metadata || !(id in metadata))
			return undefined;
		return metadata[id];
	},

	/**
	 * Sets extra data to be associated with this entity.
	 */
	setMetadata: function(id, value) {
		var metadata = this._ai._entityMetadata[this.id()];
		if (!metadata)
			metadata = this._ai._entityMetadata[this.id()] = {};
		metadata[id] = value;
	},

	position: function() { return this._entity.position; },

	isIdle: function() {
		if (this._entity.idle === undefined)
			return undefined; // Prevent warning about reference to undefined property
		return this._entity.idle;
	},

	hitpoints: function() { return this._entity.hitpoints; },
	isHurt: function() { return this.hitpoints() < this.maxHitpoints(); },
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
			return undefined; // Prevent warning about reference to undefined property
		return this._entity.foundationProgress;
	},

	owner: function() {
		return this._entity.owner;
	},

	isOwn: function() {
		if (this._entity.owner === undefined)
			return false;
		return this._entity.owner === this._ai._player;
	},

	isFriendly: function() {
		return this.isOwn(); // TODO: diplomacy
	},

	isEnemy: function() {
		return !this.isOwn(); // TODO: diplomacy
	},

	resourceSupplyAmount: function() { return this._entity.resourceSupplyAmount; },

	resourceCarrying: function() { return this._entity.resourceCarrying; },

//Garrison related

	garrisoned: function() { return new EntityCollection(this._ai, this._entity.garrisoned); },

	garrisonSpaceAvailable: function() 
	{
		return (this.garrisonMax() - this.garrisoned().length);
	},

	canGarrisonInto: function(target) {
		var allowedClasses = target.garrisonableClasses();
		// return false if the target is full or doesn't have any allowed classes 
		if (target.garrisonSpaceAvaliable() <=0 || allowedClasses === undefined)
			return false;
		
		// Check that this unit is a member of at least one of the allowed garrison classes
		var hasClasses = this.classes();
		for (var i = 0; i < hasClasses.length; i++)
		{ 
			var hasClass = hasClasses[i];
			if (allowedClasses.indexOf(hasClass) >= 0)
				return true;
		}
		
		return false;
	}, 

	// TODO: visibility

	attack: function(target) {
		Engine.PostCommand({"type": "attack", "entities": [this.id()], "target": target.id(), "queued": false});  
		return this;  
	}, 

	move: function(x, z, queued) {
		queued = queued || false;
		Engine.PostCommand({"type": "walk", "entities": [this.id()], "x": x, "z": z, "queued": queued});
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

	destroy: function() {
		Engine.PostCommand({"type": "delete-entities", "entities": [this.id()]});
		return this;
	},

	train: function(type, count, metadata) {
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
			"entity": this.id(),
			"template": type,
			"count": count,
			"metadata": metadata
		});
		return this;
	},

	//ungarrison a specific unit in this building 
	unload: function(unit) {
		if (!this._template.GarrisonHolder) 
			return;
		
		Engine.PostCommand({"type": "unload", "garrisonHolder": this.id(), "entity": unit.id()}); 
	}, 

	unloadAll: function() {
		if (!this._template.GarrisonHolder)
			return;
		
		Engine.PostCommand({"type": "unload-all", "garrisonHolder": this.id()});
	}, 

	construct: function(template, x, z, angle) {
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
			"queued": false
		});
		return this;
	},
});

