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


	maxHitpoints: function() { return this._template.Health.Max; },
	isHealable: function() { return this._template.Health.Healable === "true"; },
	isRepairable: function() { return this._template.Health.Repairable === "true"; },


	// TODO: attack, armour


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
});


var Entity = Class({
	_super: EntityTemplate,

	_init: function(baseAI, entity)
	{
		this._super.call(this, baseAI._templates[entity.template]);

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
		if (!metadata)
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
		if (typeof this._entity.idle === "undefined")
			return undefined;
		return this._entity.idle;
	},

	hitpoints: function() { return this._entity.hitpoints; },
	isHurt: function() { return this.hitpoints < this.maxHitpoints; },
	needsHeal: function() { return this.isHurt && this.isHealable; },
	needsRepair: function() { return this.isHurt && this.isRepairable; },

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

	foundationProgress: function() { return this._entity.foundationProgress; },

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

	resourceSupplyAmount: function() { return this._entity.resourceSupplyAmount; },

	resourceCarrying: function() { return this._entity.resourceCarrying; },

	garrisoned: function() { return new EntityCollection(this._ai, this._entity.garrisoned); },


	// TODO: visibility


	move: function(x, z) {
		Engine.PostCommand({"type": "walk", "entities": [this.id()], "x": x, "z": z, "queued": false});
		return this;
	},

	gather: function(target) {
		Engine.PostCommand({"type": "gather", "entities": [this.id()], "target": target.id(), "queued": false});
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
			"entity": this.id(),
			"template": type,
			"count": count,
			"metadata": metadata
		});
		return this;
	},
});

