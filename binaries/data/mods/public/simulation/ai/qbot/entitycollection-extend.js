EntityCollection.prototype.attack = function(unit)
{
	var unitId;
	if (typeOf(unit) === "Entity"){
		unitId = unit.id();
	}else{
		unitId = unit;
	}
	
	Engine.PostCommand({"type": "attack", "entities": this.toIdArray(), "target": unitId, "queued": false});
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