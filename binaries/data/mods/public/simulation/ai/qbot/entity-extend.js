Entity.prototype.deleteMetadata = function(id) {
	delete this._ai._entityMetadata[this.id()];
};

Entity.prototype.garrisonMax = function() {
	if (!this._template.GarrisonHolder)
		return undefined;
	return this._template.GarrisonHolder.Max;
};

Entity.prototype.garrison = function(target) {
	Engine.PostCommand({"type": "garrison", "entities": [this.id()], "target": target.id(),"queued": false});
	return this;
};

Entity.prototype.unload = function(id) {
	if (!this._template.GarrisonHolder)
		return undefined;
	Engine.PostCommand({"type": "unload", "garrisonHolder": this.id(), "entity": id});
	return this;
};

Entity.prototype.unloadAll = function() {
	if (!this._template.GarrisonHolder)
		return undefined;
	Engine.PostCommand({"type": "unload-all", "garrisonHolder": this.id()});
	return this;
};
