EntityCollection.prototype.attack = function(unit)
{
	var unitId;
	if (typeof(unit) === "Entity"){
		unitId = unit.id();
	}else{
		unitId = unit;
	}
	
	Engine.PostCommand({"type": "attack", "entities": this.toIdArray(), "target": unitId, "queued": false});
	return this;
};
// violent, aggressive, defensive, passive, standground
EntityCollection.prototype.setStance = function(stance){
	Engine.PostCommand({"type": "stance", "entities": this.toIdArray(), "name" : stance, "queued": false});
	return this;
};

function EntityCollectionFromIds(gameState, idList){
	var ents = {};
	for (var i in idList){
		var id = idList[i];
		if (gameState.entities._entities[id]) {
			ents[id] = gameState.entities._entities[id];
		}
	}
	return new EntityCollection(gameState.ai, ents);
}

EntityCollection.prototype.getCentrePosition = function(){
	var sumPos = [0, 0];
	var count = 0;
	this.forEach(function(ent){
		if (ent.position()){
			sumPos[0] += ent.position()[0];
			sumPos[1] += ent.position()[1];
			count ++;
		}
	});
	if (count === 0){
		return undefined;
	}else{
		return [sumPos[0]/count, sumPos[1]/count];
	}
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
