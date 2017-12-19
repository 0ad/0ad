Trigger.prototype.SpawnAndAttack = function()
{
	var intruders = TriggerHelper.SpawnUnitsFromTriggerPoints(
			pickRandom(["B", "C"]), "units/rome_legionnaire_marian", this.attackSize, 0);

	for (var origin in intruders)
	{
		var playerID = TriggerHelper.GetOwner(+origin);
		var cmd = null;
		for (var target of this.GetTriggerPoints("A"))
		{
			if (TriggerHelper.GetOwner(target) != playerID)
				continue;
			var cmpPosition = Engine.QueryInterface(target, IID_Position);
			if (!cmpPosition || !cmpPosition.IsInWorld)
				continue;
			// store the x and z coordinates in the command
			cmd = cmpPosition.GetPosition();
			break;
		}
		if (!cmd)
			continue;
		cmd.type = "attack-walk";
		cmd.entities = intruders[origin];
		cmd.targetClasses = { "attack": ["Unit", "Structure"] };
		cmd.allowCapture = false;
		cmd.queued = true;
		ProcessCommand(0, cmd);
	}

	// enlarge the attack time and size
	var rand = randFloat(1, 3);
	this.attackTime *= rand;
	this.attackSize = Math.round(this.attackSize * rand);
	this.DoAfterDelay(this.attackTime, "SpawnAndAttack", {});
};

{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.attackSize = 1; // attack with 1 soldier
	cmpTrigger.attackTime = 60 * 1000; // attack in 1 minute
	cmpTrigger.DoAfterDelay(cmpTrigger.attackTime, "SpawnAndAttack", {});
}
