/**
 * Provides an API for the rest of the AI scripts to query the world state
 * at a higher level than the raw data.
 */
var GameState = Class({

	_init: function(ai)
	{
		MemoizeInit(this);

		this.ai = ai;
		this.timeElapsed = ai.timeElapsed;
		this.templates = ai.templates;
		this.entities = ai.entities;
		this.player = ai.player;
		this.playerData = ai.playerData;
	
		if (!this.ai._gameStateStore){
			this.ai._gameStateStore = {};
		}
		this.store = this.ai._gameStateStore;
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

	displayCiv: function()
	{
		return this.playerData.civ;
	},

	getResources: function()
	{
		return new Resources(this.playerData.resourceCounts);
	},

	getPassabilityMap: function()
	{
		return this.ai.passabilityMap;
	},

	getPassabilityClassMask: function(name)
	{
		if (!(name in this.ai.passabilityClasses))
			error("Tried to use invalid passability class name '"+name+"'");
		return this.ai.passabilityClasses[name];
	},

	getTerritoryMap: function()
	{
		return this.ai.territoryMap;
	},

	getEntities: function() {
		return this.entities;
	},

	updatingCollection: function(id, filter, collection){
	if (!this.store[id]){
		this.store[id] = collection.filter(filter);
		this.store[id].registerUpdates();
	}
	
	return this.store[id];
	},

	getOwnEntities: function()
	{
	if (!this.store.ownEntities){
		this.store.ownEntities = this.getEntities().filter(Filters.byOwner(this.player));
		this.store.ownEntities.registerUpdates();
	}
	
	return this.store.ownEntities;
	},

	getOwnRoleGroup: function(role) {
	return this.updatingCollection("RoleGroup" + role, Filters.byMetadata("role", role), this.getOwnEntities());
	},

	getOwnWithClass: function(aclass) {
	return this.updatingCollection("RoleGroup" + aclass, Filters.byClass(aclass), this.getOwnEntities());
	},

	getNotGaia: function() {
	var collection = this.updatingCollection("NotGaia", Filters.byNotOwner(0), this.getEntities());
	return collection;
	},

	getJustEnemies: function() {
	var collection = this.updatingCollection("JustEnemies", Filters.byNotOwner(this.player), this.getNotGaia());
	//warn(collection.length + " enemy unit objects")
	return collection;
	},
	
	// These get enemies things are copied from qbot, to avoid duplication of a ton of effort (yay laziness)
	getEnemies: function(){
	var ret = [];
	for (i in this.playerData.isEnemy){
		if (this.playerData.isEnemy[i]){
			ret.push(i);
		}
	}
	return ret;
	},

	getEnemyEntities: function() {
	var diplomacyChange = false;
	var enemies = this.getEnemies();
	warn(enemies.length + " enemy factions")
	if (this.store.enemies){
		if (this.store.enemies.length != enemies.length){
			diplomacyChange = true;
		}
		else{
			for (var i  = 0; i < enemies.length; i++){
				if (enemies[i] !== this.store.enemies[i]){
					diplomacyChange = true;
				}
			}
		}
	}
	if (!this.store.enemyEntities){
		var filter = Filters.byOwners(enemies);
		this.store.enemyEntities = this.getEntities().filter(filter);
		this.store.enemyEntities.registerUpdates();
		this.store.enemies = enemies;
	}
	if (diplomacyChange == true){
		var filter = Filters.byOwners(enemies);
		this.store.enemyEntities = this.getEntities().filter(filter);
		this.store.enemyEntities.registerUpdates();
		this.store.enemies = enemies;
	}
	warn(this.store.enemyEntities.length + " enemy objects")
	return this.store.enemyEntities;
},
/// Laziness ends here.

	getOwnEntitiesWithRole: Memoize('getOwnEntitiesWithRole', function(role)
	{
		var metas = this.ai._entityMetadata;
		if (role === undefined)
			return this.getOwnEntities().filter_raw(function(ent) {
				var metadata = metas[ent.id];
				if (!metadata || !('role' in metadata))
					return true;
				return (metadata.role === undefined);
			});
		else
			return this.getOwnEntities().filter_raw(function(ent) {
				var metadata = metas[ent.id];
				if (!metadata || !('role' in metadata))
					return false;
				return (metadata.role === role);
			});
	}),

	getOwnEntitiesWithTwoRoles: Memoize('getOwnEntitiesWithRole', function(role, role2)
	{
		var metas = this.ai._entityMetadata;
		if (role === undefined)
			return this.getOwnEntities().filter_raw(function(ent) {
				var metadata = metas[ent.id];
				if (!metadata || !('role' in metadata))
					return true;
				return (metadata.role === undefined);
			});
		else if (role2 === undefined)
			return this.getOwnEntities().filter_raw(function(ent) {
				var metadata = metas[ent.id];
				if (!metadata || !('role' in metadata))
					return true;
				return (metadata.role === undefined);
			});
		else
			return this.getOwnEntities().filter_raw(function(ent) {
				var metadata = metas[ent.id];
				if (!metadata || !('role' in metadata))
					return false;
				return (metadata.role === role || metadata.role === role2);
			});
	}),

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

	countEntitiesAndQueuedWithRoles: function(role, role2)
	{
		var count = 0;
		this.getOwnEntities().forEach(function(ent) {

			if (ent.getMetadata("role") == role)
				++count;
			else if (ent.getMetadata("role") == role2)
				++count;

			var queue = ent.trainingQueue();
			if (queue)
			{
				queue.forEach(function(item) {
					if (item.metadata && item.metadata.role == role)
						count += item.count;
					else if (item.metadata && item.metadata.role == role2)
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
			return (ent.foundationProgress() !== undefined);
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

			var reportedType;
			if (type.generic == "treasure")
				reportedType = type.specific;
			else
				reportedType = type.generic;

			if (!supplies[reportedType])
				supplies[reportedType] = [];

			supplies[reportedType].push({
				"entity": ent,
				"amount": amount,
				"type": type,
				"position": ent.position(),
			});
		});
		return supplies;
	},

	getPopulationLimit: function()
	{
		return this.playerData.popLimit;
	},

	getPopulation: function()
	{
		return this.playerData.popCount;
	},

	getPlayerID: function()
	{
		return this.player;
	},

	/**
	 * Checks whether the player with the given id is an ally of the AI player
	 */
	isPlayerAlly: function(id)
	{
		return this.playerData.isAlly[id];
	},

	/**
	 * Checks whether the player with the given id is an enemy of the AI player
	 */
	isPlayerEnemy: function(id)
	{
		return this.playerData.isEnemy[id];
	},

	/**
	 * Checks whether an Entity object is owned by an ally of the AI player (or self)
	 */
	isEntityAlly: function(ent)
	{
		return (ent && ent.owner() !== undefined && this.playerData.isAlly[ent.owner()]);
	},

	/**
	 * Checks whether an Entity object is owned by an enemy of the AI player
	 */
	isEntityEnemy: function(ent)
	{
		return (ent && ent.owner() !== undefined && this.playerData.isEnemy[ent.owner()]);
	},

	/**
	 * Checks whether an Entity object is owned by the AI player
	 */
	isEntityOwn: function(ent)
	{
		return (ent && ent.owner() !== undefined && ent.owner() == this.player);
	},
	
	/**
	 * Returns player build limits
	 * an object where each key is a category corresponding to a build limit for the player.
	 */
	getBuildLimits: function()
	{
		return this.playerData.buildLimits;
	},
	
	/**
	 * Returns player build counts
	 * an object where each key is a category corresponding to the current building count for the player.
	 */
	getBuildCounts: function()
	{
		return this.playerData.buildCounts;
	},
	
	/**
	 * Checks if the player's build limit has been reached for the given category.
	 * The category comes from the entity tenplate, specifically the BuildRestrictions component.
	 */
	isBuildLimitReached: function(category)
	{
		if (this.playerData.buildLimits[category] === undefined || this.playerData.buildCounts[category] === undefined)
			return false;
		
		// There's a special case of build limits per civ centre, so check that first
		if (this.playerData.buildLimits[category].LimitPerCivCentre !== undefined)
			return (this.playerData.buildCounts[category] >= this.playerData.buildCounts["CivilCentre"]*this.playerData.buildLimits[category].LimitPerCivCentre);
		else
			return (this.playerData.buildCounts[category] >= this.playerData.buildLimits[category]);
	},
});
