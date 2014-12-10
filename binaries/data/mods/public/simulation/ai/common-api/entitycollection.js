var API3 = function(m)
{

m.EntityCollection = function(sharedAI, entities, filters)
{
	this._ai = sharedAI;
	if (entities && !(entities instanceof Map))
	{
		this._entities = new Map();
		for (let key in entities)
			this._entities.set(+key, entities[key]);
	}
	else
		this._entities = entities || new Map();
	this._filters = filters || [];

	this.dynamicProp = [];
	for each (var filter in this._filters)
		this.dynamicProp = this.dynamicProp.concat(filter.dynamicProperties);
	
	Object.defineProperty(this, "length", {
		get: function () {
			return this._entities.size;
		}
	});
	this.frozen = false;
};

m.EntityCollection.prototype.Serialize = function()
{
	var filters = [];
	for each (var f in this._filters)
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
		this.freeze;
	else
		this.defreeze;
};

// If an entitycollection is frozen, it will never automatically add a unit.
// But can remove one.
// this makes it easy to create entity collection that will auto-remove dead units
// but never add new ones.
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
	let ret = [];
	for (let id of this._entities.keys())
		ret.push(id);
	return ret;
};

m.EntityCollection.prototype.toEntityArray = function()
{
	let ret = [];
	for (let ent of this._entities.values())
		ret.push(ent);
	return ret;
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
	if (typeof(filter) == "function")
		filter = {"func": filter, "dynamicProperties": []};
	
	var ret = new Map();
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
	var data = []; // [ [id, ent, distance], ... ]
	for (let [id, ent] of this._entities)
		if (ent.position())
			data.push([id, ent, m.SquareVectorDistance(targetPos, ent.position())]);

	// Sort by increasing distance
	data.sort(function (a, b) { return (a[2] - b[2]); });
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
	var ret = new Map();
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

m.EntityCollection.prototype.move = function(x, z, queued)
{
	queued = queued || false;
	Engine.PostCommand(PlayerID,{"type": "walk", "entities": this.toIdArray(), "x": x, "z": z, "queued": queued});
	return this;
};

m.EntityCollection.prototype.attackMove = function(x, z, targetClasses, queued)
{
	queued = queued || false;
	Engine.PostCommand(PlayerID,{"type": "attack-walk", "entities": this.toIdArray(), "x": x, "z": z, "targetClasses": targetClasses, "queued": queued});
	return this;
};

m.EntityCollection.prototype.moveIndiv = function(x, z, queued)
{
	queued = queued || false;
	for (let id of this._entities.keys())
	{
		// The following try {} finally {} block is a workaround for OOS problems in multiplayer games with AI players (check ticket #2000).
		// It disables JIT compiling of this loop. Don't remove it unless you are sure that the underlying issue has been resolved!
		// TODO: Check this again after the SpiderMonkey upgrade.
		try {} finally {}
		Engine.PostCommand(PlayerID,{"type": "walk", "entities": [id], "x": x, "z": z, "queued": queued});
	}
	return this;
};

m.EntityCollection.prototype.garrison = function(target)
{
	Engine.PostCommand(PlayerID,{"type": "garrison", "entities": this.toIdArray(), "target": target.id()});
	return this;
};

m.EntityCollection.prototype.destroy = function()
{
	Engine.PostCommand(PlayerID,{"type": "delete-entities", "entities": this.toIdArray()});
	return this;
};

m.EntityCollection.prototype.attack = function(unit)
{
	var unitId;
	if (typeof(unit) === "Entity")
		unitId = unit.id();
	else
		unitId = unit;
	Engine.PostCommand(PlayerID,{"type": "attack", "entities": this.toIdArray(), "target": unitId, "queued": false});
	return this;
};

// violent, aggressive, defensive, passive, standground
m.EntityCollection.prototype.setStance = function(stance)
{
	Engine.PostCommand(PlayerID,{"type": "stance", "entities": this.toIdArray(), "name" : stance, "queued": false});
	return this;
};

// Returns the average position of all units
m.EntityCollection.prototype.getCentrePosition = function()
{
	var sumPos = [0, 0];
	var count = 0;
	for (let ent of this._entities.values())
	{
		if (!ent.position())
			continue;
		sumPos[0] += ent.position()[0];
		sumPos[1] += ent.position()[1];
		count ++;
	}

	if (count === 0)
		return undefined;
	else
		return [sumPos[0]/count, sumPos[1]/count];
};

// returns the average position from the sample first units.
// This might be faster for huge collections, but there's
// always a risk that it'll be unprecise.
m.EntityCollection.prototype.getApproximatePosition = function(sample)
{
	var sumPos = [0, 0];
	var i = 0;
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
	if (i === 0)
		return undefined;
	else
		return [sumPos[0]/i, sumPos[1]/i];
};


// Removes an entity from the collection, returns true if the entity was a member, false otherwise
m.EntityCollection.prototype.removeEnt = function(ent)
{
	if (!this._entities.has(ent.id()))
		return false;
	this._entities.delete(ent.id());
	return true;
};

// Adds an entity to the collection, returns true if the entity was not member, false otherwise
m.EntityCollection.prototype.addEnt = function(ent)
{
	if (this._entities.has(ent.id()))
		return false;
	this._entities.set(ent.id(), ent);
	return true;
};

// Checks the entity against the filters, and adds or removes it appropriately, returns true if the
// entity collection was modified.
// Force can add a unit despite a freezing.
// If an entitycollection is frozen, it will never automatically add a unit.
// But can remove one.
m.EntityCollection.prototype.updateEnt = function(ent, force)
{	
	var passesFilters = true;
	for each (let filter in this._filters)
		passesFilters = passesFilters && filter.func(ent);

	if (passesFilters)
	{
		if (!force && this.frozen)
			return false;
		return this.addEnt(ent);
	}
	else
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
