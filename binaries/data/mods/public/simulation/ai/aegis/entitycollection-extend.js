function EntityCollectionFromIds(gameState, idList){
	var ents = {};
	for (var i in idList){
		var id = idList[i];
		if (gameState.entities._entities[id]) {
			ents[id] = gameState.entities._entities[id];
		}
	}
	return new EntityCollection(gameState.sharedScript, ents);
}
