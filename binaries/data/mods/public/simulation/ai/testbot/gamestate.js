/**
 * Provides an API for the rest of the AI scripts to query the world state
 * at a higher level than the raw data.
 */
var GameState = Class({

	_init: function(ai)
	{
		this.ai = ai;
		this.timeElapsed = ai.timeElapsed;
		this.templates = ai.templates;
		this.entities = ai.entities;
		this.playerData = ai.playerData;
	},

	getTimeElapsed: function()
	{
		return this.timeElapsed;
	},

	getTemplate: function(type)
	{
		if (!this.templates[type])
			return null;
		return new EntityTemplate(this.templates[type]);
	},

	applyCiv: function(str)
	{
		return str.replace(/\{civ\}/g, this.playerData.civ);
	},

	getResources: function()
	{
		return new Resources(this.playerData.resourceCounts);
	},

	getMap: function()
	{
		return this.ai.map;
	},

	getPassabilityClassMask: function(name)
	{
		if (!(name in this.ai.passabilityClasses))
			error("Tried to use invalid passability class name '"+name+"'");
		return this.ai.passabilityClasses[name];
	},

	getOwnEntities: function()
	{
		return this.entities.filter(function(ent) { return ent.isOwn(); });
	},

	countEntitiesWithType: function(type)
	{
		var count = 0;
		this.getOwnEntities().forEach(function(ent) {
			var t = ent.templateName();
			if (t == type)
				++count;
		});
		return count;
	},

	countEntitiesAndQueuedWithType: function(type)
	{
		var foundationType = "foundation|" + type;
		var count = 0;
		this.getOwnEntities().forEach(function(ent) {

			var t = ent.templateName();
			if (t == type || t == foundationType)
				++count;

			var queue = ent.trainingQueue();
			if (queue)
			{
				queue.forEach(function(item) {
					if (item.template == type)
						count += item.count;
				});
			}
		});
		return count;
	},

	countEntitiesAndQueuedWithRole: function(role)
	{
		var count = 0;
		this.getOwnEntities().forEach(function(ent) {

			if (ent.getMetadata("role") == role)
				++count;

			var queue = ent.trainingQueue();
			if (queue)
			{
				queue.forEach(function(item) {
					if (item.metadata && item.metadata.role == role)
						count += item.count;
				});
			}
		});
		return count;
	},

	/**
	 * Find buildings that are capable of training the given unit type,
	 * and aren't already too busy.
	 */
	findTrainers: function(template)
	{
		var maxQueueLength = 3; // avoid tying up resources in giant training queues

		return this.getOwnEntities().filter(function(ent) {

			var trainable = ent.trainableEntities();
			if (!trainable || trainable.indexOf(template) == -1)
				return false;

			var queue = ent.trainingQueue();
			if (queue)
			{
				if (queue.length >= maxQueueLength)
					return false;
			}

			return true;
		});
	},

	/**
	 * Find units that are capable of constructing the given building type.
	 */
	findBuilders: function(template)
	{
		return this.getOwnEntities().filter(function(ent) {

			var buildable = ent.buildableEntities();
			if (!buildable || buildable.indexOf(template) == -1)
				return false;

			return true;
		});
	},

	findFoundations: function(template)
	{
		return this.getOwnEntities().filter(function(ent) {
			return (typeof ent.foundationProgress() !== "undefined");
		});
	},

	findResourceSupplies: function()
	{
		var supplies = {};
		this.entities.forEach(function(ent) {
			var type = ent.resourceSupplyType();
			if (!type)
				return;
			var amount = ent.resourceSupplyAmount();
			if (!amount)
				return;

			if (!supplies[type.generic])
				supplies[type.generic] = [];

			supplies[type.generic].push({
				"entity": ent,
				"amount": amount,
				"type": type,
				"position": ent.position(),
			});
		});
		return supplies;
	},
});
