var API3 = function(m)
{

m.EntityCollection = function(sharedAI, entities = new Map(), filters = [])
{
	this._ai = sharedAI;
	this._entities = entities;
	this._filters = filters;
	this.dynamicProp = [];
	for (let filter of this._filters)
		if (filter.dynamicProperties.length)
			this.dynamicProp = this.dynamicProp.concat(filter.dynamicProperties);

	Object.defineProperty(this, "length", { "get": () => this._entities.size });
	this.frozen = false;
};

m.EntityCollection.prototype.Serialize = function()
{
	let filters = [];
	for (let f of this._filters)
		filters.push(uneval(f));
	return {
		"ents": this.toIdArray(),
		"frozen": this.frozen,
		"filters": filters
	};
};

m.EntityCollection.prototype.Deserialize = function(data, sharedAI)
{
	this._ai = sharedAI;
	for (let id of data.ents)
		this._entities.set(id, sharedAI._entities.get(id));

	for (let f of data.filters)
		this._filters.push(eval(f));

	if (data.frozen)
		this.freeze();
	else
		this.defreeze();
};

/**
 * If an entitycollection is frozen, it will never automatically add a unit.
 * But can remove one.
 * this makes it easy to create entity collection that will auto-remove dead units
 * but never add new ones.
 */
m.EntityCollection.prototype.freeze = function()
{
	this.frozen = true;
};

m.EntityCollection.prototype.defreeze = function()
{
	this.frozen = false;
};

m.EntityCollection.prototype.toIdArray = function()
{
	return Array.from(this._entities.keys());
};

m.EntityCollection.prototype.toEntityArray = function()
{
	return Array.from(this._entities.values());
};

m.EntityCollection.prototype.values = function()
{
	return this._entities.values();
};

m.EntityCollection.prototype.toString = function()
{
	return "[EntityCollection " + this.toEntityArray().join(" ") + "]";
};

m.EntityCollection.prototype.filter = function(filter, thisp)
{
	if (typeof filter == "function")
		filter = { "func": filter, "dynamicProperties": [] };

	let ret = new Map();
	for (let [id, ent] of this._entities)
		if (filter.func.call(thisp, ent, id, this))
			ret.set(id, ent);

	return new m.EntityCollection(this._ai, ret, this._filters.concat([filter]));
};

/**
 * Returns the (at most) n entities nearest to targetPos.
 */
m.EntityCollection.prototype.filterNearest = function(targetPos, n)
{
	// Compute the distance of each entity
	let data = []; // [ [id, ent, distance], ... ]
	for (let [id, ent] of this._entities)
		if (ent.position())
			data.push([id, ent, m.SquareVectorDistance(targetPos, ent.position())]);

	// Sort by increasing distance
	data.sort((a, b) => a[2] - b[2]);

	if (n === undefined)
		n = data.length;
	else
		n = Math.min(n, data.length);

	// Extract the first n
	let ret = new Map();
	for (let i = 0; i < n; ++i)
		ret.set(data[i][0], data[i][1]);

	return new m.EntityCollection(this._ai, ret);
};

m.EntityCollection.prototype.filter_raw = function(callback, thisp)
{
	let ret = new Map();
	for (let [id, ent] of this._entities)
	{
		let val = ent._entity;
		if (callback.call(thisp, val, id, this))
			ret.set(id, ent);
	}
	return new m.EntityCollection(this._ai, ret);
};

m.EntityCollection.prototype.forEach = function(callback)
{
	for (let ent of this._entities.values())
		callback(ent);
	return this;
};

m.EntityCollection.prototype.hasEntities = function()
{
	return this._entities.size !== 0;
};

m.EntityCollection.prototype.move = function(x, z, queued = false, pushFront = false)
{
	Engine.PostCommand(PlayerID, {
		"type": "walk",
		"entities": this.toIdArray(),
		"x": x,
		"z": z,
		"queued": queued,
		"pushFront": pushFront
	});
	return this;
};

m.EntityCollection.prototype.moveToRange = function(x, z, min, max, queued = false, pushFront = false)
{
	Engine.PostCommand(PlayerID, {
		"type": "walk-to-range",
		"entities": this.toIdArray(),
		"x": x,
		"z": z,
		"min": min,
		"max": max,
		"queued": queued,
		"pushFront": pushFront
	});
	return this;
};

m.EntityCollection.prototype.attackMove = function(x, z, targetClasses, allowCapture = true, queued = false, pushFront = false)
{
	Engine.PostCommand(PlayerID, {
		"type": "attack-walk",
		"entities": this.toIdArray(),
		"x": x,
		"z": z,
		"targetClasses": targetClasses,
		"allowCapture": allowCapture,
		"queued": queued,
		"pushFront": pushFront
	});
	return this;
};

m.EntityCollection.prototype.moveIndiv = function(x, z, queued = false, pushFront = false)
{
	for (let id of this._entities.keys())
		Engine.PostCommand(PlayerID, {
			"type": "walk",
			"entities": [id],
			"x": x,
			"z": z,
			"queued": queued,
			"pushFront": pushFront
		});
	return this;
};

m.EntityCollection.prototype.garrison = function(target, queued = false, pushFront = false)
{
	Engine.PostCommand(PlayerID, {
		"type": "garrison",
		"entities": this.toIdArray(),
		"target": target.id(),
		"queued": queued,
		"pushFront": pushFront
	});
	return this;
};

m.EntityCollection.prototype.occupyTurret = function(target, queued = false, pushFront = false)
{
	Engine.PostCommand(PlayerID, {
		"type": "occupy-turret",
		"entities": this.toIdArray(),
		"target": target.id(),
		"queued": queued,
		"pushFront": pushFront
	});
	return this;
};

m.EntityCollection.prototype.destroy = function()
{
	Engine.PostCommand(PlayerID, { "type": "delete-entities", "entities": this.toIdArray() });
	return this;
};

m.EntityCollection.prototype.attack = function(unitId, queued = false, pushFront = false)
{
	Engine.PostCommand(PlayerID, {
		"type": "attack",
		"entities": this.toIdArray(),
		"target": unitId,
		"queued": queued,
		"pushFront": pushFront
	});
	return this;
};

/** violent, aggressive, defensive, passive, standground */
m.EntityCollection.prototype.setStance = function(stance)
{
	Engine.PostCommand(PlayerID, {
		"type": "stance",
		"entities": this.toIdArray(),
		"name": stance
	});
	return this;
};

/** Returns the average position of all units */
m.EntityCollection.prototype.getCentrePosition = function()
{
	let sumPos = [0, 0];
	let count = 0;
	for (let ent of this._entities.values())
	{
		if (!ent.position())
			continue;
		sumPos[0] += ent.position()[0];
		sumPos[1] += ent.position()[1];
		count++;
	}

	return count ? [sumPos[0]/count, sumPos[1]/count] : undefined;
};

/**
 * returns the average position from the sample first units.
 * This might be faster for huge collections, but there's
 * always a risk that it'll be unprecise.
 */
m.EntityCollection.prototype.getApproximatePosition = function(sample)
{
	let sumPos = [0, 0];
	let i = 0;
	for (let ent of this._entities.values())
	{
		if (!ent.position())
			continue;
		sumPos[0] += ent.position()[0];
		sumPos[1] += ent.position()[1];
		i++;
		if (i === sample)
			break;
	}

	return i ? [sumPos[0]/i, sumPos[1]/i] : undefined;
};

m.EntityCollection.prototype.hasEntId = function(id)
{
	return this._entities.has(id);
};

/** Removes an entity from the collection, returns true if the entity was a member, false otherwise */
m.EntityCollection.prototype.removeEnt = function(ent)
{
	if (!this._entities.has(ent.id()))
		return false;
	this._entities.delete(ent.id());
	return true;
};

/** Adds an entity to the collection, returns true if the entity was not member, false otherwise */
m.EntityCollection.prototype.addEnt = function(ent)
{
	if (this._entities.has(ent.id()))
		return false;
	this._entities.set(ent.id(), ent);
	return true;
};

/**
 * Checks the entity against the filters, and adds or removes it appropriately, returns true if the
 * entity collection was modified.
 * Force can add a unit despite a freezing.
 * If an entitycollection is frozen, it will never automatically add a unit.
 * But can remove one.
 */
m.EntityCollection.prototype.updateEnt = function(ent, force)
{
	let passesFilters = true;
	for (let filter of this._filters)
		passesFilters = passesFilters && filter.func(ent);

	if (passesFilters)
	{
		if (!force && this.frozen)
			return false;
		return this.addEnt(ent);
	}

	return this.removeEnt(ent);
};

m.EntityCollection.prototype.registerUpdates = function()
{
	this._ai.registerUpdatingEntityCollection(this);
};

m.EntityCollection.prototype.unregister = function()
{
	this._ai.removeUpdatingEntityCollection(this);
};

m.EntityCollection.prototype.dynamicProperties = function()
{
	return this.dynamicProp;
};

m.EntityCollection.prototype.setUID = function(id)
{
	this._UID = id;
};

m.EntityCollection.prototype.getUID = function()
{
	return this._UID;
};

return m;

}(API3);
