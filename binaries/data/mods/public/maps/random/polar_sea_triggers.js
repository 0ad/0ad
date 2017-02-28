var attackerTemplate = "gaia/fauna_wolf_snow";

var minWaveSize = 1;
var maxWaveSize = 3;

var firstWaveTime = 3;
var minWaveTime = 2;
var maxWaveTime = 4;

/**
 * Attackers will focus the targetCount closest units that have the targetClasses type.
 */
var targetClasses = "Organic";
var targetCount = 3;

var disabledTechnologies = [
	"gather_lumbering_ironaxes",
	"gather_lumbering_sharpaxes",
	"gather_lumbering_strongeraxes"
];

Trigger.prototype.InitDisableTechnologies = function()
{
	for (let i = 1; i < TriggerHelper.GetNumberOfPlayers(); ++i)
		QueryPlayerIDInterface(i).SetDisabledTechnologies(disabledTechnologies);
}

Trigger.prototype.SpawnWolvesAndAttack = function()
{
	let waveSize = Math.round(Math.random() * (maxWaveSize - minWaveSize) + minWaveSize);
	let attackers = TriggerHelper.SpawnUnitsFromTriggerPoints("A", attackerTemplate, waveSize, 0);
	print("Spawned " + waveSize + " " + attackerTemplate + " at " + Object.keys(attackers).length + " points\n");

	let targets = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).GetNonGaiaEntities().filter(ent => {

		// TODO: This shouldn't occur by definition of GetNonGaiaEntities
		let cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
		if (cmpOwnership.GetOwner() == 0)
			return false;

		let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
		return cmpIdentity && MatchesClassList(cmpIdentity.GetClassesList(), targetClasses)
	});

	let getDistance = (attacker, target) => {

		let cmpPositionAttacker = Engine.QueryInterface(attacker, IID_Position);
		if (!cmpPositionAttacker || !cmpPositionAttacker.IsInWorld())
			return Infinity;

		let cmpPositionTarget = Engine.QueryInterface(target, IID_Position);
		if (!cmpPositionTarget || !cmpPositionTarget.IsInWorld())
			return Infinity;

		return cmpPositionAttacker.GetPosition().distanceToSquared(cmpPositionTarget.GetPosition());
	};

	for (let spawnPoint in attackers)
		for (let attacker of attackers[spawnPoint])
			for (let target of targets.sort((ent1, ent2) => getDistance(attacker, ent1) - getDistance(attacker, ent2)).slice(0, targetCount))
				ProcessCommand(0, {
					"type": "attack",
					"entities": [attacker],
					"queued": true,
					"target": target
				});

	this.DoAfterDelay((Math.random() * (maxWaveTime - minWaveTime) + minWaveTime) * 60 * 1000, "SpawnWolvesAndAttack", {});
};

{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.InitDisableTechnologies();
	cmpTrigger.DoAfterDelay(firstWaveTime * 60 * 1000, "SpawnWolvesAndAttack", {});
}
