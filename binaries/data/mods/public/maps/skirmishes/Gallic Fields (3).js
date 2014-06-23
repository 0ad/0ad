Trigger.prototype.SpawnAndAttack = function()
{
	var rand = Math.random();
	// randomize spawn points
	var spawnPoint = rand > 0.5 ? "B" : "C";
	var intruders = TriggerHelper.SpawnUnitsFromTriggerPoints(spawnPoint, "units/rome_legionnaire_marian", this.attackSize, 0);

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
		cmd.queued = true;
		ProcessCommand(0, cmd);
	}

	// enlarge the attack time and size
	// multiply with a number between 1 and 3
	rand = Math.random() * 2 + 1;
	this.attackTime *= rand;
	this.attackSize = Math.round(this.attackSize * rand);
	this.DoAfterDelay(this.attackTime, "SpawnAndAttack", {});
};

var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);

cmpTrigger.attackSize = 1; // attack with 1 soldier
cmpTrigger.attackTime = 60*1000; // attack in 1 minute
cmpTrigger.DoAfterDelay(cmpTrigger.attackTime, "SpawnAndAttack", {});

