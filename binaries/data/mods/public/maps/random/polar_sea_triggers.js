const debugLog = false;

var attackerTemplate = "trigger/fauna_arctic_wolf_attack";

var minWaveSize = 1;
var maxWaveSize = 3;

var firstWaveTime = 5;
var minWaveTime = 2;
var maxWaveTime = 4;

/**
 * Attackers will focus the targetCount closest units that have the targetClasses type.
 */
var targetClasses = "Organic+!Domestic";
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
};

Trigger.prototype.SpawnWolvesAndAttack = function()
{
	let waveSize = Math.round(Math.random() * (maxWaveSize - minWaveSize) + minWaveSize);
	let attackers = TriggerHelper.SpawnUnitsFromTriggerPoints("A", attackerTemplate, waveSize, 0);

	if (debugLog)
		print("Spawned " + waveSize + " " + attackerTemplate + " at " + Object.keys(attackers).length + " points\n");

	let allTargets;

	let cmpDamage = Engine.QueryInterface(SYSTEM_ENTITY, IID_Damage);
	let players = new Array(TriggerHelper.GetNumberOfPlayers()).fill(0).map((v, i) => i + 1);

	for (let spawnPoint in attackers)
	{
		// TriggerHelper.SpawnUnits is guaranteed to spawn
		let firstAttacker = attackers[spawnPoint][0];
		if (!firstAttacker)
			continue;

		let cmpAttackerPos = Engine.QueryInterface(firstAttacker, IID_Position);
		if (!cmpAttackerPos || !cmpAttackerPos.IsInWorld())
			continue;

		let attackerPos = cmpAttackerPos.GetPosition2D();

		// The returned entities are sorted by RangeManager already
		let targets = cmpDamage.EntitiesNearPoint(attackerPos, 200, players).filter(ent => {
			let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
			return cmpIdentity && MatchesClassList(cmpIdentity.GetClassesList(), targetClasses);
		});

		let goodTargets = targets.slice(0, targetCount);

		// Look through all targets if there aren't enough nearby ones
		if (goodTargets.length < targetCount)
		{
			if (!allTargets)
				allTargets = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).GetNonGaiaEntities().filter(ent => {
					let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
					return cmpIdentity && MatchesClassList(cmpIdentity.GetClassesList(), targetClasses);
				});

			let getDistance = target => {
				let cmpPositionTarget = Engine.QueryInterface(target, IID_Position);
				if (!cmpPositionTarget || !cmpPositionTarget.IsInWorld())
					return Infinity;
				return attackerPos.distanceToSquared(cmpPositionTarget.GetPosition2D());
			};

			goodTargets = [];
			let goodDists = [];
			for (let target of allTargets)
			{
				let dist = getDistance(target);
				let i = goodDists.findIndex(element => dist < element);
				if (i != -1 || goodTargets.length < targetCount)
				{
					if (i == -1)
						i = goodTargets.length;
					goodTargets.splice(i, 0, target);
					goodDists.splice(i, 0, dist);
					if (goodTargets.length > targetCount)
					{
						goodTargets.pop();
						goodDists.pop();
					}
				}
			}
		}

		for (let target of goodTargets)
			ProcessCommand(0, {
				"type": "attack",
				"entities": attackers[spawnPoint],
				"target": target,
				"queued": true
			});
	}

	this.DoAfterDelay((Math.random() * (maxWaveTime - minWaveTime) + minWaveTime) * 60 * 1000, "SpawnWolvesAndAttack", {});
};

{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.InitDisableTechnologies();
	cmpTrigger.DoAfterDelay(firstWaveTime * 60 * 1000, "SpawnWolvesAndAttack", {});
}
