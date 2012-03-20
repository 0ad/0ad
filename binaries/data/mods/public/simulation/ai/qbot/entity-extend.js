Entity.prototype.deleteMetadata = function(id) {
	delete this._ai._entityMetadata[this.id()];
};

Entity.prototype.garrison = function(target) {
	Engine.PostCommand({"type": "garrison", "entities": [this.id()], "target": target.id(),"queued": false});
	return this;
};

Entity.prototype.attack = function(unitId)
{
	Engine.PostCommand({"type": "attack", "entities": [this.id()], "target": unitId, "queued": false});
	return this;
};