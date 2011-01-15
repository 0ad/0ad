function Entity(baseAI, entity)
{
	this._ai = baseAI;
	this._entity = entity;
	this._template = baseAI._templates[entity.template];
}

Entity.prototype = {
	get rank() {
		if (!this._template.Identity)
			return undefined;
		return this._template.Identity.Rank;
	},

	get classes() {
		if (!this._template.Identity || !this._template.Identity.Classes)
			return undefined;
		return this._template.Identity.Classes._string.split(/\s+/);
	},

	get civ() {
		if (!this._template.Identity)
			return undefined;
		return this._template.Identity.Civ;
	},


	get position() { return this._entity.position; },


	get hitpoints() { return this._entity.hitpoints; },
	get maxHitpoints() { return this._template.Health.Max; },
	get isHurt() { return this.hitpoints < this.maxHitpoints; },
	get needsHeal() { return this.isHurt && (this._template.Health.Healable == "true"); },
	get needsRepair() { return this.isHurt && (this._template.Health.Repairable == "true"); },


	// TODO: attack, armour


	get buildableEntities() {
		if (!this._template.Builder)
			return undefined;
		var templates = this._template.Builder.Entities._string.replace(/\{civ\}/g, this.civ).split(/\s+/);
		return templates; // TODO: map to Entity?
	},

	get trainableEntities() {
		if (!this._template.TrainingQueue)
			return undefined;
		var templates = this._template.TrainingQueue.Entities._string.replace(/\{civ\}/g, this.civ).split(/\s+/);
		return templates;
	},


	get trainingQueue() { return this._entity.trainingQueue; },

	get foundationProgress() { return this._entities.foundationProgress; },


	get owner() { return this._entity.owner; },
	get isOwn() { return this._entity.owner == this._ai._player; },
	get isFriendly() { return this.isOwn; }, // TODO: diplomacy
	get isEnemy() { return !this.isOwn; }, // TODO: diplomacy


	get resourceSupplyType() {
		if (!this._template.ResourceSupply)
			return undefined;
		var [type, subtype] = this._template.ResourceSupply.Type.split('.');
		return { "generic": type, "specific": subtype };
	},

	get resourceSupplyMax() {
		if (!this._template.ResourceSupply)
			return undefined;
		return +this._template.ResourceSupply.Amount;
	},

	get resourceSupplyAmount() { return this._entity.resourceSupplyAmount; },


	get resourceGatherRates() {
		if (!this._template.ResourceGatherer)
			return undefined;
		var ret = {};
		for (var r in this._template.ResourceGatherer.Rates)
			ret[r] = this._template.ResourceGatherer.Rates[r] * this._template.ResourceGatherer.BaseSpeed;
		return ret;
	},

	get resourceCarrying() { return this._entity.resourceCarrying; },


	get resourceDropsiteTypes() {
		if (!this._template.ResourceDropsite)
			return undefined;
		return this._template.ResourceDropsite.Types.split(/\s+/);
	},


	get garrisoned() { return new EntityCollection(this._ai, this._entity.garrisoned); },

	get garrisonableClasses() {
		if (!this._template.GarrisonHolder)
			return undefined;
		return this._template.GarrisonHolder.List._string.split(/\s+/);
	},


	// TODO: visibility


	move: function(x, z) {
		Engine.PostCommand({"type": "walk", "entities": [this.entity.id], "x": x, "z": z, "queued": false});
		return this;
	},

	destroy: function() {
		Engine.PostCommand({"type": "delete-entities", "entities": [this.entity.id]});
		return this;
	},
};
