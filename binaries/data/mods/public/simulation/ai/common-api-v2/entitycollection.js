function EntityCollection(baseAI, entities, filters)
{
	this._ai = baseAI;
	this._entities = entities;

	this._filters = filters || [];

	// Compute length lazily on demand, since it can be
	// expensive for large collections
	this._length = undefined;
	Object.defineProperty(this, "length", {
		get: function () {
			if (this._length === undefined)
			{
				this._length = 0;
				for (var id in entities)
					++this._length;
			}
			return this._length;
		}
	});
}

EntityCollection.prototype.toIdArray = function()
{
	var ret = [];
	for (var id in this._entities)
		ret.push(+id);
	return ret;
};

EntityCollection.prototype.toEntityArray = function()
{
	var ret = [];
	for each (var ent in this._entities)
		ret.push(ent);
	return ret;
};

EntityCollection.prototype.toString = function()
{
	return "[EntityCollection " + this.toEntityArray().join(" ") + "]";
};

/**
 * Returns the (at most) n entities nearest to targetPos.
 */
EntityCollection.prototype.filterNearest = function(targetPos, n)
{
	// Compute the distance of each entity
	var data = []; // [ [id, ent, distance], ... ]
	for (var id in this._entities)
	{
		var ent = this._entities[id];
		if (ent.position())
			data.push([id, ent, VectorDistance(targetPos, ent.position())]);
	}

	// Sort by increasing distance
	data.sort(function (a, b) { return (a[2] - b[2]); });

	// Extract the first n
	var ret = {};
	for each (var val in data.slice(0, n))
		ret[val[0]] = val[1];

	return new EntityCollection(this._ai, ret);
};

EntityCollection.prototype.filter = function(filter, thisp)
{
	if (typeof(filter) == "function")
		filter = {"func": filter, "dynamicProperties": []};
	
	var ret = {};
	for (var id in this._entities)
	{
		var ent = this._entities[id];
		if (filter.func.call(thisp, ent, id, this))
			ret[id] = ent;
	}
	
	return new EntityCollection(this._ai, ret, this._filters.concat([filter]));
};

EntityCollection.prototype.filter_raw = function(callback, thisp)
{
	var ret = {};
	for (var id in this._entities)
	{
		var ent = this._entities[id];
		var val = this._entities[id]._entity;
		if (callback.call(thisp, val, id, this))
			ret[id] = ent;
	}
	return new EntityCollection(this._ai, ret);
};

EntityCollection.prototype.forEach = function(callback, thisp)
{
	for (var id in this._entities)
	{
		var ent = this._entities[id];
		callback.call(thisp, ent, id, this);
	}
	return this;
};

EntityCollection.prototype.move = function(x, z, queued)
{
	queued = queued || false;
	Engine.PostCommand({"type": "walk", "entities": this.toIdArray(), "x": x, "z": z, "queued": queued});
	return this;
};

EntityCollection.prototype.destroy = function()
{
	Engine.PostCommand({"type": "delete-entities", "entities": this.toIdArray()});
	return this;
};

// Removes an entity from the collection, returns true if the entity was a member, false otherwise
EntityCollection.prototype.removeEnt = function(ent)
{
	if (this._entities[ent.id()])
	{
		// Checking length may initialize it, so do it before deleting.
		if (this.length !== undefined)
			this._length--;
		delete this._entities[ent.id()];
		return true;
	}
	else
	{
		return false;
	}
};

// Adds an entity to the collection, returns true if the entity was not member, false otherwise
EntityCollection.prototype.addEnt = function(ent)
{
	if (this._entities[ent.id()])
	{
		return false;
	}
	else
	{
		// Checking length may initialize it, so do it before adding.
		if (this.length !== undefined)
			this._length++;
		this._entities[ent.id()] = ent;
		return true;
	}
};

// Checks the entity against the filters, and adds or removes it appropriately, returns true if the
// entity collection was modified.
EntityCollection.prototype.updateEnt = function(ent)
{
	var passesFilters = true;
	for each (var filter in this._filters)
	{
		passesFilters = passesFilters && filter.func(ent);
	}

	if (passesFilters)
	{
		return this.addEnt(ent);
	}
	else
	{
		return this.removeEnt(ent);
	}
};

EntityCollection.prototype.registerUpdates = function()
{
	this._ai.registerUpdatingEntityCollection(this);
};

EntityCollection.prototype.dynamicProperties = function()
{
	var ret = [];
	for each (var filter in this._filters)
	{
		ret = ret.concat(filter.dynamicProperties);
	}

	return ret;
};

EntityCollection.prototype.setUID = function(id)
{
	this._UID = id;
};

EntityCollection.prototype.getUID = function()
{
	return this._UID;
};
