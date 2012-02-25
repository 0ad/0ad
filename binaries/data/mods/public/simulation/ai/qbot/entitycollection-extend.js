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

EntityCollection.prototype.attackMove = function(x, z){
	Engine.PostCommand({"type": "attack-move", "entities": this.toIdArray(), "x": x, "z": z, "queued": false});
	return this;
};

// Do naughty stuff to replace the entity collection constructor for updating entity collections
var tmpEntityCollection = function(baseAI, entities, filter, gameState){
	this._ai = baseAI;
	this._entities = entities;
	if (filter){
		this.filterFunc = filter;
		this._entities = this.filter(function(ent){
				return filter(ent, gameState);
			})._entities;
		this._ai.registerUpdate(this);
	}

	// Compute length lazily on demand, since it can be
	// expensive for large collections
	// This is updated by the update() function. 
	this._length = undefined;
	Object.defineProperty(this, "length", {
		get: function () {
			if (this._length === undefined)
			{
				this._length = 0;
				for (var id in this._entities)
					++this._length;
			}
			return this._length;
		}
	});
};

tmpEntityCollection.prototype = new EntityCollection;
EntityCollection = tmpEntityCollection;

// Keeps an EntityCollection with a filter function up to date by watching for events
EntityCollection.prototype.update = function(gameState, events){
	if (!this.filterFunc)
		return;
	for (var i in events){
		if (events[i].type === "Create"){
			var ent = gameState.getEntityById(events[i].msg.entity);
			if (ent){
				var raw_ent = ent._entity;
				if (ent && this.filterFunc(ent, gameState)){
					this._entities[events[i].msg.entity] = raw_ent;
					if (this._length !== undefined)
						this._length ++;
				}
			}
		}else if(events[i].type === "Destroy"){
			if (this._entities[events[i].msg.entity]){
				delete this._entities[events[i].msg.entity];
				if (this._length !== undefined)
					this._length --;
			}
		}
	}
};

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