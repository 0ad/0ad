function EntityCollection(sharedAI, entities, filters)
{
	this._ai = sharedAI;
	this._entities = entities || {};
	this._filters = filters || [];
	
	this._quickIter = false;	// will make the entity collection store an array (not associative) of entities used when calling "foreach".
	// probably should not be usde for very dynamic entity collections.

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
	this.frozen = false;
}

// If an entitycollection is frozen, it will never automatically add a unit.
// But can remove one.
// this makes it easy to create entity collection that will auto-remove dead units
// but never add new ones.
EntityCollection.prototype.freeze = function()
{
	this.frozen = true;
};
EntityCollection.prototype.defreeze = function()
{
	this.frozen = false;
};

EntityCollection.prototype.allowQuickIter = function()
{
	this._quickIter = true;
	this._entitiesArray = [];
	for each (var ent in this._entities)
		this._entitiesArray.push(ent);
};

EntityCollection.prototype.toIdArray = function()
{
	var ret = [];
	for (var id in this._entities)
		ret.push(+id);
	return ret;
};

EntityCollection.prototype.toEntityArray = function()
{
	if (this._quickIter === true)
		return this._entitiesArray;
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
	var data = []; // [id, ent, distance]
	
	if (this._quickIter === true)
	{
		for (var i in this._entitiesArray)
		{
			var ent = this._entitiesArray[i];
			if (ent.position() !== -1)
				data.push([ent.id(), ent, SquareVectorDistance(targetPos, ent.position())]);
		}
	} else {
		for (var id in this._entities)
		{
			var ent = this._entities[id];
			if (ent.position() !== -1)
				data.push([id, ent, SquareVectorDistance(targetPos, ent.position())]);
		}
	}

	// Sort by increasing distance
	data.sort(function (a, b) { return (a[2] - b[2]); });

	// Extract the first n
	var ret = {};
	var length = Math.min(n, entData.length);
	for (var i = 0; i < length; ++i)
		ret[data[i][0]] = data[i][1];

	return new EntityCollection(this._ai, ret);
};

EntityCollection.prototype.filter = function(filter, thisp)
{
	if (typeof(filter) == "function")
		filter = {"func": filter, "dynamicProperties": []};
	
	var ret = {};
	if (this._quickIter === true)
	{
		for (var i in this._entitiesArray)
		{
			var ent = this._entitiesArray[i];
			var id = ent.id();
			if (filter.func.call(thisp, ent, id, this))
				ret[id] = ent;
		}
	} else {
		for (var id in this._entities)
		{
			var ent = this._entities[id];
			if (filter.func.call(thisp, ent, id, this))
				ret[id] = ent;
		}
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

EntityCollection.prototype.forEach = function(callback)
{
	if (this._quickIter === true)
	{
		for (var id in this._entitiesArray)
		{
			callback(this._entitiesArray[id]);
		}
		return this;
	}
	for (var id in this._entities)
	{
		callback(this._entities[id]);
	}
	return this;
};

EntityCollection.prototype.filterNearest = function(targetPos, n)
{
	// Compute the distance of each entity
	var data = []; // [ [id, ent, distance], ... ]
	for (var id in this._entities)
	{
		var ent = this._entities[id];
		if (ent.position())
			data.push([id, ent, SquareVectorDistance(targetPos, ent.position())]);
	}
	
	// Sort by increasing distance
	data.sort(function (a, b) { return (a[2] - b[2]); });
	if (n === undefined)
		n = this._length;
	// Extract the first n
	var ret = {};
	for each (var val in data.slice(0, n))
		ret[val[0]] = val[1];
	
	return new EntityCollection(this._ai, ret);
};

EntityCollection.prototype.move = function(x, z, queued)
{
	queued = queued || false;
	Engine.PostCommand({"type": "walk", "entities": this.toIdArray(), "x": x, "z": z, "queued": queued});
	return this;
};
EntityCollection.prototype.moveIndiv = function(x, z, queued)
{
	queued = queued || false;
	for (var id in this._entities)
		Engine.PostCommand({"type": "walk", "entities": [this._entities[id].id()], "x": x, "z": z, "queued": queued});
	return this;
};
EntityCollection.prototype.destroy = function()
{
	Engine.PostCommand({"type": "delete-entities", "entities": this.toIdArray()});
	return this;
};
EntityCollection.prototype.attack = function(unit)
{
	var unitId;
	if (typeof(unit) === "Entity")
	{
		unitId = unit.id();
	}
	else
	{
		unitId = unit;
	}	
	Engine.PostCommand({"type": "attack", "entities": this.toIdArray(), "target": unitId, "queued": false});
	return this;
};
// violent, aggressive, defensive, passive, standground
EntityCollection.prototype.setStance = function(stance)
{
	Engine.PostCommand({"type": "stance", "entities": this.toIdArray(), "name" : stance, "queued": false});
	return this;
};

// Returns the average position of all units
EntityCollection.prototype.getCentrePosition = function()
{
	var sumPos = [0, 0];
	var count = 0;
	this.forEach(function(ent)
	{
		if (ent.position())
		{
			sumPos[0] += ent.position()[0];
			sumPos[1] += ent.position()[1];
			count ++;
		}
	});
	if (count === 0)
	{
		return undefined;
	}
	else
	{
		return [sumPos[0]/count, sumPos[1]/count];
	}
};

// returns the average position from the sample first units.
// This might be faster for huge collections, but there's
// always a risk that it'll be unprecise.
EntityCollection.prototype.getApproximatePosition = function(sample)
{
	var sumPos = [0, 0];
	var i = 0;
	for (var id in this._entities)
	{
		var ent = this._entities[id];
		if (ent.position())
		{
			sumPos[0] += ent.position()[0];
			sumPos[1] += ent.position()[1];
			i++;
		}
		if (i === sample)
			break;
	}
	if (sample === 0)
	{
		return undefined;
	}
	else
	{
		return [sumPos[0]/i, sumPos[1]/i];
	}
};


// Removes an entity from the collection, returns true if the entity was a member, false otherwise
EntityCollection.prototype.removeEnt = function(ent)
{
	if (this._entities[ent.id()])
	{
		// Checking length may initialize it, so do it before deleting.
		if (this.length !== undefined)
			this._length--;
		if (this._quickIter === true)
			this._entitiesArray.splice(this._entitiesArray.indexOf(ent),1);
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
		if (this._quickIter === true)
			this._entitiesArray.push(ent);
		return true;
	}
};

// Checks the entity against the filters, and adds or removes it appropriately, returns true if the
// entity collection was modified.
// Force can add a unit despite a freezing.
// If an entitycollection is frozen, it will never automatically add a unit.
// But can remove one.
EntityCollection.prototype.updateEnt = function(ent, force)
{	
	var passesFilters = true;
	for each (var filter in this._filters)
	{
		passesFilters = passesFilters && filter.func(ent);
	}

	if (passesFilters)
	{
		if (!force && this.frozen)
			return false;
		return this.addEnt(ent);
	}
	else
	{
		return this.removeEnt(ent);
	}
};

EntityCollection.prototype.registerUpdates = function(noPush)
{
	this._ai.registerUpdatingEntityCollection(this,noPush);
};

EntityCollection.prototype.unregister = function()
{
	this._ai.removeUpdatingEntityCollection(this);
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
