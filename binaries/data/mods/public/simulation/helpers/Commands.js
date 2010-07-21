function ProcessCommand(player, cmd)
{
//	print("command: " + player + " " + uneval(cmd) + "\n");
	
	// TODO: all of this stuff needs to do checks for valid arguments
	// (e.g. make sure players own the units they're trying to use)

	switch (cmd.type)
	{
	case "debug-print":
		print(cmd.message);
		break;

	case "walk":
		for each (var ent in cmd.entities)
		{
			var ai = Engine.QueryInterface(ent, IID_UnitAI);
			if (ai)
				ai.Walk(cmd.x, cmd.z, cmd.queued);
		}
		break;

	case "attack":
		for each (var ent in cmd.entities)
		{
			var ai = Engine.QueryInterface(ent, IID_UnitAI);
			if (ai)
				ai.Attack(cmd.target, cmd.queued);
		}
		break;

	case "repair":
		// This covers both repairing damaged buildings, and constructing unfinished foundations
		for each (var ent in cmd.entities)
		{
			var ai = Engine.QueryInterface(ent, IID_UnitAI);
			if (ai)
				ai.Repair(cmd.target, cmd.queued);
		}
		break;

	case "gather":
		for each (var ent in cmd.entities)
		{
			var ai = Engine.QueryInterface(ent, IID_UnitAI);
			if (ai)
				ai.Gather(cmd.target, cmd.queued);
		}
		break;

	case "train":
		var queue = Engine.QueryInterface(cmd.entity, IID_TrainingQueue);
		if (queue)
			queue.AddBatch(player, cmd.template, +cmd.count);
		break;

	case "stop-train":
		var queue = Engine.QueryInterface(cmd.entity, IID_TrainingQueue);
		if (queue)
			queue.RemoveBatch(player, cmd.id);
		break;

	case "construct":
		/*
		 * Construction process:
		 *  . Take resources away immediately.
		 *  . Create a foundation entity with 1hp, 0% build progress.
		 *  . Increase hp and build progress up to 100% when people work on it.
		 *  . If it's destroyed, an appropriate fraction of the resource cost is refunded.
		 *  . If it's completed, it gets replaced with the real building.
		 */

		// Find the player
		var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
		var playerEnt = cmpPlayerMan.GetPlayerByID(player);
		var cmpPlayer = Engine.QueryInterface(playerEnt, IID_Player);

		// Tentatively create the foundation (we might find later that it's a invalid build command)
		var ent = Engine.AddEntity("foundation|" + cmd.template);
		// TODO: report errors (e.g. invalid template names)

		// Move the foundation to the right place
		var cmpPosition = Engine.QueryInterface(ent, IID_Position);
		cmpPosition.JumpTo(cmd.x, cmd.z);
		cmpPosition.SetYRotation(cmd.angle);

		var cmpObstruction = Engine.QueryInterface(ent, IID_Obstruction);
		if (cmpObstruction && cmpObstruction.CheckCollisions())
		{
			// TODO: report error to player (the building site was obstructed)

			// Remove the foundation because the construction was aborted
			Engine.DestroyEntity(ent);

			break;
		}

		var cmpCost = Engine.QueryInterface(ent, IID_Cost);
		if (!cmpPlayer.TrySubtractResources(cmpCost.GetResourceCosts()))
		{
			// TODO: report error to player (they ran out of resources)

			// Remove the foundation because the construction was aborted
			Engine.DestroyEntity(ent);

			break;
		}

		// Make it owned by the current player
		var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
		cmpOwnership.SetOwner(player);

		// Initialise the foundation
		var cmpFoundation = Engine.QueryInterface(ent, IID_Foundation);
		cmpFoundation.InitialiseConstruction(player, cmd.template);

		// Tell the units to start building this new entity
		ProcessCommand(player, {
			"type": "repair",
			"entities": cmd.entities,
			"target": ent,
			"queued": cmd.queued
		});

		break;

	default:
		error("Ignoring unrecognised command type '" + cmd.type + "'");
	}
}

Engine.RegisterGlobal("ProcessCommand", ProcessCommand);
