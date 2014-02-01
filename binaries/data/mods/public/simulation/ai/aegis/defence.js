var AEGIS = function(m)
{

m.Defence = function(Config)
{
	this.armies = [];	// array of "army" Objects. See defence-helper.js
	this.Config = Config;
}


m.Defence.prototype.init = function(gameState)
{
	this.armyMergeSize = this.Config.Defence.armyMergeSize;
	
	this.dangerMap = new API3.Map(gameState.sharedScript);
}

m.Defence.prototype.update = function(gameState, events)
{
	Engine.ProfileStart("Defence Manager");

	this.territoryMap = m.createTerritoryMap(gameState);
	
	this.releasedDefenders = [];	// array of defenders released by armies this turn.
	this.checkEnemyArmies(gameState,events);
	this.checkEnemyUnits(gameState);
	this.assignDefenders(gameState);
	
	// debug
	/*debug ("");
	debug ("");
	debug ("Armies: " +this.armies.length);
	for (var i in this.armies)
		this.armies[i].debug(gameState);
	*/
	
	this.MessageProcess(gameState,events);

	Engine.ProfileStop();
};

m.Defence.prototype.makeIntoArmy = function(gameState, entityID)
{
	// Try to add it to an existing army.
	for (var o in this.armies)
	{
		if (this.armies[o].addFoe(gameState,entityID))
			return;	// over
	}
	// Create a new army for it.
	var army = new m.DefenseArmy(gameState, this, [], [entityID]);
	this.armies.push(army);
}

// TODO: this algorithm needs to be improved, sorta.
m.Defence.prototype.isDangerous = function(gameState, entity)
{
	if (!entity.position())
		return false;
	if (this.territoryMap.getOwner(entity.position()) === entity.owner() || entity.attackTypes() === undefined)
		return false;
	
	var myBuildings = gameState.getOwnStructures();
	for (var i in myBuildings._entities)
		if (API3.SquareVectorDistance(myBuildings._entities[i].position(), entity.position()) < 6000)
			return true;

	return false;
}


m.Defence.prototype.checkEnemyUnits = function(gameState)
{
	var self = this;
	
	// loop through enemy units
	var nbPlayers = gameState.sharedScript.playersData.length - 1;
	var i = 1 + gameState.ai.playedTurn % nbPlayers;
	if (i === PlayerID && i !== nbPlayers)
		i++;
	else if (i === PlayerID)
		i = 1;

	if (gameState.isPlayerAlly(i))
		return;
	
	var filter = API3.Filters.and(API3.Filters.byClass("Unit"), API3.Filters.byOwner(i));
	var enemyUnits = gameState.updatingGlobalCollection("player-" +i + "-units", filter);
	
	enemyUnits.forEach( function (ent) {
		// first check: is this unit already part of an army.
		if (ent.getMetadata(PlayerID, "PartOfArmy") !== undefined)
			return;
		
		if (ent.attackTypes() === undefined || ent.hasClass("Support") || ent.hasClass("Ship"))
			return;
		
		// check if unit is dangerous "a priori"
		if (self.isDangerous(gameState,ent))
			self.makeIntoArmy(gameState,ent.id());
	});
}

m.Defence.prototype.checkEnemyArmies = function(gameState, events)
{
	var self = this;

	for (var o = 0; o < this.armies.length; ++o)
	{
		var army = this.armies[o];
		army.checkEvents(gameState, events);	// must be called every turn for all armies
		
		// this returns a list of IDs: the units that broke away from the army for being too far.
		var breakaways = army.update(gameState);

		for (var u in breakaways)
		{
			// assume dangerosity
			this.makeIntoArmy(gameState,breakaways[u]);
		}
		
		if (army.getState(gameState) === 0)
		{
			army.clear(gameState);
			this.armies.splice(o--,1);
			continue;
		}
	}
	// Check if we can't merge it with another.
	for (var o = 0; o < this.armies.length; ++o)
	{
		var army = this.armies[o];
		for (var p = o+1; p < this.armies.length; ++p)
		{
			var otherArmy = this.armies[p];
			if (otherArmy.state !== army.state)
				continue;
			
			if (API3.SquareVectorDistance(army.foePosition, otherArmy.foePosition) < this.armyMergeSize)
			{
				// no need to clear here.
				army.merge(gameState, otherArmy);
				this.armies.splice(p--,1);
			}
		}
	}
}

m.Defence.prototype.assignDefenders = function(gameState, events)
{
	if (this.armies.length === 0)
		return;
	
	var armiesNeeding = [];
	// Okay, let's add defenders
	// TODO: this is dumb.
	for (var i in this.armies)
	{
		var army = this.armies[i];
		var needsDef = army.needsDefenders(gameState);
		if (needsDef === false)
			continue;
		
		// Okay for now needsDef is the total needed strength.
		// we're dumb so we don't choose if we have a defender shortage.
		armiesNeeding.push([army, needsDef]);
	}

	if (armiesNeeding.length === 0)
		return;

	// let's get our potential units
	// TODO: this should rather be a HQ function that returns viable plans.
	var filter = API3.Filters.and(API3.Filters.and(API3.Filters.byHasMetadata(PlayerID,"plan"),
										 API3.Filters.not(API3.Filters.byHasMetadata(PlayerID, "PartOfArmy"))),
							 API3.Filters.and(API3.Filters.not(API3.Filters.byMetadata(PlayerID,"subrole","walking")),
										 API3.Filters.not(API3.Filters.byMetadata(PlayerID,"subrole","attacking"))));
	var potentialDefendersOne = gameState.getOwnUnits().filter(filter).toIdArray();
	
	filter = API3.Filters.and(API3.Filters.and(API3.Filters.not(API3.Filters.byHasMetadata(PlayerID,"plan")),
									 API3.Filters.not(API3.Filters.byHasMetadata(PlayerID, "PartOfArmy"))),
						 API3.Filters.byClassesOr(["Infantry","Cavalry"]));
	var potentialDefendersTwo = gameState.getOwnUnits().filter(filter).toIdArray();
	
	var potDefs = this.releasedDefenders.concat(potentialDefendersOne).concat(potentialDefendersTwo);
	
	for (var i in armiesNeeding)
	{
		var army = armiesNeeding[i][0];
		var need = armiesNeeding[i][1];

		// TODO: this is what I'll want to improve, along with the choice above.
		while (need > 0)
		{
			if (potDefs.length === 0)
				return;	// won't do anything anymore.
			var ent = gameState.getEntityById(potDefs[0]);

			if (ent.getMetadata(PlayerID, "PartOfArmy") !== undefined)
			{
				potDefs.splice(0,1);
				continue;
			}
			var str = m.getMaxStrength(ent);
			need -= str;
			
			army.addOwn(gameState,potDefs[0]);
			army.assignUnit(gameState, potDefs[0]);
			potDefs.splice(0,1);
		}
	}
}

// this processes the attackmessages
// So that a unit that gets attacked will not be completely dumb.
// warning: big levels of indentation coming.
m.Defence.prototype.MessageProcess = function(gameState,events) {
/*	var self = this;
	var attackedEvents = events["Attacked"];
	for (var key in attackedEvents){
		var e = attackedEvents[key];
		if (gameState.isEntityOwn(gameState.getEntityById(e.target))) {
			var attacker = gameState.getEntityById(e.attacker);
			var ourUnit = gameState.getEntityById(e.target);
			
			// the attacker must not be already dead, and it must not be me (think catapults that miss).
			if (attacker === undefined || attacker.owner() === PlayerID || attacker.position() === undefined)
				continue;
			
			var mapPos = this.dangerMap.gamePosToMapPos(attacker.position());
			this.dangerMap.addInfluence(mapPos[0], mapPos[1], 4, 1, 'constant');

			// disregard units from attack plans and defence.
			if (ourUnit.getMetadata(PlayerID, "role") == "defence" || ourUnit.getMetadata(PlayerID, "role") == "attack")
				continue;
			
			var territory = this.territoryMap.getOwner(attacker.position());

			if (attacker.owner() == 0)
			{
				if (ourUnit !== undefined && ourUnit.hasClass("Unit") && !ourUnit.hasClass("Support"))
					ourUnit.attack(e.attacker);
				else
				{
					ourUnit.flee(attacker);
					ourUnit.setMetadata(PlayerID,"fleeing", gameState.getTimeElapsed());
				}
				if (territory === PlayerID)
				{
					// anyway we'll register the animal as dangerous, and attack it (note: only on our territory. Don't care otherwise).
					this.listOfWantedUnits[attacker.id()] = new EntityCollection(gameState.sharedScript);
					this.listOfWantedUnits[attacker.id()].addEnt(attacker);
					this.listOfWantedUnits[attacker.id()].freeze();
					this.listOfWantedUnits[attacker.id()].registerUpdates();
					
					var filter = Filters.byTargetedEntity(attacker.id());
					this.WantedUnitsAttacker[attacker.id()] = this.myUnits.filter(filter);
					this.WantedUnitsAttacker[attacker.id()].registerUpdates();
				}
			} // Disregard military units except in our territory. Disregard all calls in enemy territory.
			else if (territory == PlayerID || (territory != attacker.owner() && ourUnit.hasClass("Support")))
			{
				// TODO: this does not differentiate with buildings...
				// These ought to be treated differently.
				// units in attack plans will react independently, but we still list the attacks here.
				if (attacker.hasClass("Structure")) {
					// todo: we ultimately have to check wether it's a danger point or an isolated area, and if it's a danger point, mark it as so.
					
					// Right now, to make the AI less gameable, we'll mark any surrounding resource as inaccessible.
					// usual tower range is 80. Be on the safe side.
					var close = gameState.getResourceSupplies("wood").filter(Filters.byDistance(attacker.position(), 90));
					close.forEach(function (supply) { //}){
								  supply.setMetadata(PlayerID, "inaccessible", true);
								  });
				} else {
					// TODO: right now a soldier always retaliate... Perhaps it should be set in "Defence" mode.

					// TODO: handle the ship case
					if (attacker.hasClass("Ship"))
						continue;

					// This unit is dangerous. if it's in an army, it's being dealt with.
					// if it's not in an army, it means it's either a lone raider, or it has got friends.
					// In which case we must check for other dangerous units around, and perhaps armify them.
					// TODO: perhaps someday army detection will have improved and this will require change.
					var armyID = attacker.getMetadata(PlayerID, "inArmy");
					if (armyID == undefined || !this.enemyArmy[attacker.owner()] || !this.enemyArmy[attacker.owner()][armyID]) {
						if (this.reevaluateEntity(gameState, attacker))
						{
							var position = attacker.position();
							var close = HQ.enemyWatchers[attacker.owner()].enemySoldiers.filter(Filters.byDistance(position, self.armyCompactSize));
							
							if (close.length > 2 || ourUnit.hasClass("Support") || attacker.hasClass("Siege"))
							{
								// armify it, then armify units close to him.
								this.armify(gameState,attacker);
								armyID = attacker.getMetadata(PlayerID, "inArmy");
								
								close.forEach(function (ent) { //}){
											  if (API3.SquareVectorDistance(position, ent.position()) < self.armyCompactSize)
											  {
											  ent.setMetadata(PlayerID, "inArmy", armyID);
											  self.enemyArmy[ent.owner()][armyID].addEnt(ent);
											  }
											  });
								return;	// don't use too much processing power. If there are other cases, they'll be processed soon enough.
							}
						}
						// Defencemanager will deal with them in the next turn.
					}
					if (ourUnit !== undefined && ourUnit.hasClass("Unit")) {
						if (ourUnit.hasClass("Support")) {
							// let's try to garrison this support unit.
							if (ourUnit.position())
							{
								var buildings = gameState.getOwnEntities().filter(Filters.byCanGarrison()).filterNearest(ourUnit.position(),4).toEntityArray();
								var garrisoned = false;
								for (var i in buildings)
								{
									var struct = buildings[i];
									if (struct.garrisoned() && struct.garrisonMax() - struct.garrisoned().length > 0)
									{
										garrisoned = true;
										ourUnit.garrison(struct);
										break;
									}
								}
								if (!garrisoned) {
									ourUnit.flee(attacker);
									ourUnit.setMetadata(PlayerID,"fleeing", gameState.getTimeElapsed());
								}
							}
						} else {
							// It's a soldier. Right now we'll retaliate
							// TODO: check for stronger units against this type, check for fleeing options, etc.
							// Check also for neighboring towers and garrison there perhaps?
							ourUnit.attack(e.attacker);
						}
					}
				}
			}
		}
	}
 */
}; // nice sets of closing brackets, isn't it?

return m;
}(AEGIS);
