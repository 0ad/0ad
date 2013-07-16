var EntityTemplate = Class({

	_init: function(template)
	{
		this._template = template;
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
						   
	maxGatherers: function()
	{
		if (this._template.ResourceSupply !== undefined)
			return this._template.ResourceSupply.MaxGatherers;
		return 0;
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
		if (!this._template.GarrisonHolder || !this._template.GarrisonHolder.List._string)
			return undefined;
		return this._template.GarrisonHolder.List._string.split(/\s+/);
	},


	/**
	 * Returns whether this is an animal that is too difficult to hunt.
	 * (Currently this includes all non-domestic animals.)
	 */
	isUnhuntable: function() {
		if (!this._template.UnitAI || !this._template.UnitAI.NaturalBehaviour)
			return false;

		// Ideally other animals should be huntable, but e.g. skittish animals
		// must be hunted by ranged units, and some animals may be too tough.
		return (this._template.UnitAI.NaturalBehaviour != "domestic");
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
	}
});



var Entity = Class({
	_super: EntityTemplate,

	_init: function(baseAI, entity)
	{
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
	getMetadata: function(key) {
		return this._ai.getMetadata(this, key);
	},

	/**
	 * Sets extra data to be associated with this entity.
	 */
	setMetadata: function(key, value) {
		this._ai.setMetadata(this, key, value);
	},
	
	deleteMetadata: function() {
		delete this._ai._entityMetadata[this.id()];
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
		if (typeof this._entity.foundationProgress === "undefined")
			return undefined;
		return this._entity.foundationProgress;
	},

	owner: function() {
		return this._entity.owner;
	},
	isOwn: function() {
		if (typeof this._entity.owner === "undefined")
			return false;
		return this._entity.owner === this._ai._player;
	},
	isFriendly: function() {
		return this.isOwn(); // TODO: diplomacy
	},
	isEnemy: function() {
		return !this.isOwn(); // TODO: diplomacy
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

	// TODO: visibility

	move: function(x, z, queued) {
		queued = queued || false;
		Engine.PostCommand({"type": "walk", "entities": [this.id()], "x": x, "z": z, "queued": queued});
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
		Engine.PostCommand({"type": "delete-entities", "entities": [this.id()]});
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

