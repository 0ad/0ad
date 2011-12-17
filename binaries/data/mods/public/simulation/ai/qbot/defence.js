function Defence(){
	this.AQUIRE_DIST = 220;
	this.RELEASE_DIST = 250;
	
	this.GROUP_RADIUS = 20; // units will be added to a group if they are within this radius
	this.GROUP_BREAK_RADIUS = 40; // units will leave a group if they are outside of this radius
	this.GROUP_MERGE_RADIUS = 10; // Two groups with centres this far apart will be merged
	
	this.DEFENCE_RATIO = 2; // How many defenders we want per attacker.  Need to balance fewer losses vs. lost economy
	
	// These are objects with the keys being entity ids and values being the entity objects
	// NOTE: It is assumed that all attackers have a valid position, the attackers list must be kept up to date so this 
	// property is maintained 
	this.attackers = {}; // Enemy soldiers which are attacking our base
	this.defenders = {}; // Our soldiers currently being used for defence
	
	// A list of groups, enemy soldiers are clumped together in groups.
	this.groups = [];
}

Defence.prototype.update = function(gameState, events, militaryManager){
	Engine.ProfileStart("Defence Manager");
	var enemyTroops = militaryManager.getEnemySoldiers();
	
	this.updateAttackers(gameState, events, enemyTroops);
	this.updateGroups();
	var unassignedDefenders = this.updateDefenders(gameState);
	this.assignDefenders(gameState, militaryManager, unassignedDefenders);	
	
	Engine.ProfileStop();
};

Defence.prototype.assignDefenders = function(gameState, militaryManager, unassignedDefenders){
	var numAttackers = Object.keys(this.attackers).length;
	var numDefenders = Object.keys(this.defenders).length;
	var numUnassignedDefenders = unassignedDefenders.length;
	var numAssignedDefenders = numDefenders - numUnassignedDefenders;
	
	// TODO: this is non optimal, we may have unevenly distributed defenders
	// Unassign defenders which aren't needed
	if (numAttackers * this.DEFENCE_RATIO <= numAssignedDefenders){
		militaryManager.unassignUnits(unassignedDefenders);
		
		var CCs = gameState.getOwnEntities().filter(Filters.byClass("CivCentre"));
		
		for (var i in unassignedDefenders){
			var pos = this.defenders[unassignedDefenders[i]].position();
			
			// Move back to nearest CC
			if (pos){
				var nearestCCArray = CCs.filterNearest(pos, 1).toEntityArray();
				if (nearestCCArray.length > 0){
					var movePos = nearestCCArray[0].position();
					this.defenders[unassignedDefenders[i]].move(movePos[0], movePos[1]);
				}
			}
			delete this.defenders[unassignedDefenders[i]];
		}
		return;
	}
	
	// Check to see if we need to recruit more defenders
	if (numAttackers * this.DEFENCE_RATIO > numDefenders){
		var numNeeded = Math.ceil(numAttackers * this.DEFENCE_RATIO - numDefenders);
		var numIdleAvailable = militaryManager.countAvailableUnits(Filters.isIdle());
		
		if (numIdleAvailable > numNeeded){
			var newUnits = militaryManager.getAvailableUnits(numNeeded, Filters.isIdle());
			for (var i in newUnits){
				var ent = gameState.getEntityById(newUnits[i]);
			}
			unassignedDefenders = unassignedDefenders.concat(newUnits);
		}else{
			var newUnits = militaryManager.getAvailableUnits(numNeeded);
			for (var i in newUnits){
				var ent = gameState.getEntityById(newUnits[i]);
				ent.setMetadata("initialPosition", ent.position());
			}
			unassignedDefenders = unassignedDefenders.concat(newUnits);
		}
	}
	
	// Now distribute the unassigned defenders among the attacking groups.
	for (var i in unassignedDefenders){
		var id = unassignedDefenders[i];
		var ent = gameState.getEntityById(id);
		if (!ent.position()){
			debug("Defender with no position! (shouldn't happen)");
			debug(ent);
			continue;
		}
		
		var minDist = Math.min();
		var closestGroup = undefined;
		for (var j in this.groups){
			var dist = VectorDistance(this.groups[j].position, ent.position());
			if (dist < minDist && this.groups[j].members.length * this.DEFENCE_RATIO > this.groups[j].defenders.length){
				minDist = dist;
				closestGroup = this.groups[j];
			}
		}
		
		if (closestGroup !== undefined){
			var rand = Math.floor(Math.random()*closestGroup.members.length);
			ent.attack(closestGroup.members[rand]);
			this.defenders[id] = ent;
			closestGroup.defenders.push(id);
		}
	}
};

Defence.prototype.updateDefenders = function(gameState){
	var newDefenders = {};
	var unassignedDefenders = [];
	
	for (var i in this.groups){
		this.removeDestroyed(gameState, this.groups[i].defenders);
		for (j in this.groups[i].defenders){
			var id = this.groups[i].defenders[j];
			newDefenders[id] = this.defenders[id];
			var ent = gameState.getEntityById(id);
			
			// If the defender is idle then set it to attack another member of the group it is targetting
			if (ent && ent.isIdle()){
				var rand = Math.floor(Math.random()*this.groups[i].members.length);
				ent.attack(this.groups[i].members[rand]);
			}
		}
	}
	
	for (var id in this.defenders){
		if (!gameState.getEntityById(id)){
			delete this.defenders[id];
		} else if (!newDefenders[id]){
			unassignedDefenders.push(id);
		}
	}
	
	return unassignedDefenders;
};

// Returns an entity collection of key buildings which should be defended.
// Currently just returns civ centres
Defence.prototype.getKeyBuildings = function(gameState){
	return gameState.getOwnEntities().filter(Filters.byClass("CivCentre"));
};

/*
 * This function puts all attacking enemy troops into this.attackers, the list from the turn before is put into 
 * this.oldAttackers, also any new attackers have their id's listed in this.newAttackers.
 */
Defence.prototype.updateAttackers = function(gameState, events, enemyTroops){
	var self = this;
	
	var keyBuildings = this.getKeyBuildings(gameState);
	
	this.newAttackers = [];
	this.oldAttackers = this.attackers;
	this.attackers = {};
	
	enemyTroops.forEach(function(ent){
		if (ent.position()){
			var minDist = Math.min();
			keyBuildings.forEach(function(building){
				if (building.position() && VectorDistance(ent.position(), building.position()) < minDist){
					minDist = VectorDistance(ent.position(), building.position());
				}
			});
			
			if (self.oldAttackers[ent.id()]){
				if (minDist < self.RELEASE_DIST){
					self.attackers[ent.id()] = ent;
				}
			}else{
				if (minDist < self.AQUIRE_DIST){
					self.attackers[ent.id()] = ent;
					self.newAttackers.push(ent.id());
				}
			}
		}
	});
};

Defence.prototype.removeDestroyed = function(gameState, entList){
	for (var i = 0; i < entList.length; i++){
		if (!gameState.getEntityById(entList[i])){
			entList.splice(i, 1);
			i--;
		}
	}
};

Defence.prototype.updateGroups = function(){
	// clean up groups by removing members and removing empty groups 
	for (var i = 0; i < this.groups.length; i++){
		var group = this.groups[i];
		// remove members which are no longer attackers
		for (var j = 0; j < group.members.length; j++){
			if (!this.attackers[group.members[j]]){
				group.members.splice(j, 1);
				j--;
			}
		}
		// recalculate centre of group
		group.sumPosition = [0,0];
		for (var j = 0; j < group.members.length; j++){
			group.sumPosition[0] += this.attackers[group.members[j]].position()[0];
			group.sumPosition[1] += this.attackers[group.members[j]].position()[1];
		}
		group.position[0] = group.sumPosition[0]/group.members.length;
		group.position[1] = group.sumPosition[1]/group.members.length;
		
		// remove members that are too far away
		for (var j = 0; j < group.members.length; j++){
			if ( VectorDistance(this.attackers[group.members[j]].position(), group.position) > this.GROUP_BREAK_RADIUS){
				this.newAttackers.push(group.members[j]);
				group.sumPosition[0] -= this.attackers[group.members[j]].position()[0];
				group.sumPosition[1] -= this.attackers[group.members[j]].position()[1];
				group.members.splice(j, 1);
				j--;
			}
		}
		
		if (group.members.length === 0){
			this.groups.splice(i, 1);
			i--;
		}
		
		group.position[0] = group.sumPosition[0]/group.members.length;
		group.position[1] = group.sumPosition[1]/group.members.length;
	}
	
	// add ungrouped attackers to groups
	for (var j in this.newAttackers){
		var ent = this.attackers[this.newAttackers[j]];
		var foundGroup = false;
		for (var i in this.groups){
			if (VectorDistance(ent.position(), this.groups[i].position) <= this.GROUP_RADIUS){
				this.groups[i].members.push(ent.id());
				
				this.groups[i].sumPosition[0] += ent.position()[0];
				this.groups[i].sumPosition[1] += ent.position()[1];
				this.groups[i].position[0] = this.groups[i].sumPosition[0]/this.groups[i].members.length;
				this.groups[i].position[1] = this.groups[i].sumPosition[1]/this.groups[i].members.length;
				
				foundGroup = true;
				break;
			}
		}
		if (!foundGroup){
			this.groups.push({"members": [ent.id()],
			                  "position": [ent.position()[0], ent.position()[1]],
			                  "sumPosition": [ent.position()[0], ent.position()[1]],
			                  "defenders": []});
		}
	}
	
	// merge groups which are close together
	for (var i = 0; i < this.groups.length; i++){
		for (var j = 0; j < this.groups.length; j++){
			if (this.groups[i].members.length < this.groups[j].members.length){
				if (VectorDistance(this.groups[i].position, this.groups[j].position) < this.GROUP_MERGE_RADIUS){
					this.groups[j].members = this.groups[i].members.concat(this.groups[j].members);
					this.groups[j].defenders = this.groups[i].defenders.concat(this.groups[j].defenders);
					
					this.groups[j].sumPosition[0] += this.groups[i].sumPosition[0];
					this.groups[j].sumPosition[1] += this.groups[i].sumPosition[1];
					this.groups[j].position[0] = this.groups[j].sumPosition[0]/this.groups[j].members.length;
					this.groups[j].position[1] = this.groups[j].sumPosition[1]/this.groups[j].members.length;
					
					this.groups.splice(i, 1);
					i--;
					break; 
				}
			}
		}
	}
};
