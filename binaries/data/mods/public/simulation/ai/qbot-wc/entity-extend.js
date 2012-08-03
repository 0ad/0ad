EntityTemplate.prototype.genericName = function() {
	if (!this._template.Identity || !this._template.Identity.GenericName)
		return undefined;
	return this._template.Identity.GenericName;
};
EntityTemplate.prototype.walkSpeed = function() {
	if (!this._template.UnitMotion || !this._template.UnitMotion.WalkSpeed)
		return undefined;
	return this._template.UnitMotion.WalkSpeed;
};
EntityTemplate.prototype.buildTime = function() {
	if (!this._template.Cost || !this._template.Cost.buildTime)
		return undefined;
	return this._template.Cost.buildTime;
};
EntityTemplate.prototype.getPopulationBonus = function() {
	if (!this._template.Cost || !this._template.Cost.PopulationBonus)
		return undefined;
	
	return this._template.Cost.PopulationBonus;
};
// will return either "food", "wood", "stone", "metal" and not treasure.
EntityTemplate.prototype.getResourceType = function() {
	if (!this._template.ResourceSupply)
		return undefined;
	var [type, subtype] = this._template.ResourceSupply.Type.split('.');
	if (type == "treasure")
		return subtype;
	return type;
};
EntityTemplate.prototype.garrisonMax = function() {
	if (!this._template.GarrisonHolder)
		return undefined;
	return this._template.GarrisonHolder.Max;
};
EntityTemplate.prototype.getMaxStrength = function()
{
	var strength = 0.0;
	var attackTypes = this.attackTypes();
	var armourStrength = this.armourStrengths();
	var hp = this.maxHitpoints() / 100.0;	// some normalization
	for (var typeKey in attackTypes) {
		var type = attackTypes[typeKey];
		var attackStrength = this.attackStrengths(type);
		var attackRange = this.attackRange(type);
		var attackTimes = this.attackTimes(type);
		for (var str in attackStrength) {
			var val = parseFloat(attackStrength[str]);
			switch (str) {
				case "crush":
					strength += (val * 0.085) / 3;
					break;
				case "hack":
					strength += (val * 0.075) / 3;
					break;
				case "pierce":
					strength += (val * 0.065) / 3;
					break;
			}
		}
		if (attackRange){
			strength += (attackRange.max * 0.0125) ;
		}
		for (var str in attackTimes) {
			var val = parseFloat(attackTimes[str]);
			switch (str){
				case "repeat":
					strength += (val / 100000);
					break;
				case "prepare":
					strength -= (val / 100000);
					break;
			}
		}
	}
	for (var str in armourStrength) {
		var val = parseFloat(armourStrength[str]);
		switch (str) {
			case "crush":
				strength += (val * 0.085) / 3;
				break;
			case "hack":
				strength += (val * 0.075) / 3;
				break;
			case "pierce":
				strength += (val * 0.065) / 3;
				break;
		}
	}
	return strength * hp;
};

EntityTemplate.prototype.costSum = function() {
	if (!this._template.Cost)
		return undefined;
	
	var ret = 0;
	for (var type in this._template.Cost.Resources)
		ret += +this._template.Cost.Resources[type];
	return ret;
};




Entity.prototype.deleteMetadata = function(id) {
	delete this._ai._entityMetadata[this.id()];
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

Entity.prototype.garrison = function(target) {
	Engine.PostCommand({"type": "garrison", "entities": [this.id()], "target": target.id(),"queued": false});
	return this;
};

Entity.prototype.attack = function(unitId)
{
	Engine.PostCommand({"type": "attack", "entities": [this.id()], "target": unitId, "queued": false});
	return this;
};

Entity.prototype.stopMoving = function() {
	if (this.position() !== undefined)
		Engine.PostCommand({"type": "walk", "entities": [this.id()], "x": this.position()[0], "z": this.position()[1], "queued": false});
};
// from from a unit in the opposite direction.
Entity.prototype.flee = function(unitToFleeFrom) {
	if (this.position() !== undefined && unitToFleeFrom.position() !== undefined) {
		var FleeDirection = [unitToFleeFrom.position()[0] - this.position()[0],unitToFleeFrom.position()[1] - this.position()[1]];
		
		Engine.PostCommand({"type": "walk", "entities": [this.id()], "x": this.position()[0] + FleeDirection[0]*5, "z": this.position()[1] + FleeDirection[1]*5, "queued": false});
	}
	return this;
};
Entity.prototype.barter = function(buyType, sellType, amount) {
	
	Engine.PostCommand({"type": "barter", "sell" : sellType, "buy" : buyType, "amount" : amount });
	return this;
};
