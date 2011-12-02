function EntityCollection(baseAI, entities)
{
	this._ai = baseAI;
	this._entities = entities;

	// Compute length lazily on demand, since it can be
	// expensive for large collections
	var length = undefined;
	Object.defineProperty(this, "length", {
		get: function () {
			if (length === undefined)
			{
				length = 0;
				for (var id in entities)
					++length;
			}
			return length;
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
		ret.push(new Entity(this._ai, ent));
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
		if (ent.position)
			data.push([id, ent, VectorDistance(targetPos, ent.position)]);
	}

	// Sort by increasing distance
	data.sort(function (a, b) { return (a[2] - b[2]); });

	// Extract the first n
	var ret = {};
	for each (var val in data.slice(0, n))
		ret[val[0]] = val[1];

	return new EntityCollection(this._ai, ret);
};

EntityCollection.prototype.filter = function(callback, thisp)
{
	var ret = {};
	for (var id in this._entities)
	{
		var ent = this._entities[id];
		var val = new Entity(this._ai, ent);
		if (callback.call(thisp, val, id, this))
			ret[id] = ent;
	}
	return new EntityCollection(this._ai, ret);
};

EntityCollection.prototype.filter_raw = function(callback, thisp)
{
	var ret = {};
	for (var id in this._entities)
	{
		var ent = this._entities[id];
		if (callback.call(thisp, ent, id, this))
			ret[id] = ent;
	}
	return new EntityCollection(this._ai, ret);
};

EntityCollection.prototype.forEach = function(callback, thisp)
{
	for (var id in this._entities)
	{
		var ent = this._entities[id];
		var val = new Entity(this._ai, ent);
		callback.call(thisp, val, id, this);
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
