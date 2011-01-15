function EntityCollection(baseAI, entities)
{
	this._ai = baseAI;
	this._entities = entities;
}

EntityCollection.prototype.ToIdArray = function()
{
	var ret = [];
	for (var id in this._entities)
		ret.push(+id);
	return ret;
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

EntityCollection.prototype.move = function(x, z)
{
	Engine.PostCommand({"type": "walk", "entities": this.ToIdArray(), "x": x, "z": z, "queued": false});
	return this;
};

EntityCollection.prototype.destroy = function()
{
	Engine.PostCommand({"type": "delete-entities", "entities": this.ToIdArray()});
	return this;
};
