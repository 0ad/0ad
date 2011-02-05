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

	case "chat":
		var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGuiInterface.PushNotification({"type": "chat", "player": player, "message": cmd.message});
		break;

	case "reveal-map":
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		cmpRangeManager.SetLosRevealAll(cmd.enable);
		break;

	case "walk":
		var cmpUnitAI = GetFormationUnitAI(cmd.entities);
		if (cmpUnitAI)
			cmpUnitAI.Walk(cmd.x, cmd.z, cmd.queued);
		break;

	case "attack":
		var cmpUnitAI = GetFormationUnitAI(cmd.entities);
		if (cmpUnitAI)
			cmpUnitAI.Attack(cmd.target, cmd.queued);
		break;

	case "repair":
		// This covers both repairing damaged buildings, and constructing unfinished foundations
		var cmpUnitAI = GetFormationUnitAI(cmd.entities);
		if (cmpUnitAI)
			cmpUnitAI.Repair(cmd.target, cmd.queued);
		break;

	case "gather":
		var cmpUnitAI = GetFormationUnitAI(cmd.entities);
		if (cmpUnitAI)
			cmpUnitAI.Gather(cmd.target, cmd.queued);
		break;

	case "returnresource":
		var cmpUnitAI = GetFormationUnitAI(cmd.entities);
		if (cmpUnitAI)
			cmpUnitAI.ReturnResource(cmd.target, cmd.queued);
		break;

	case "train":
		var queue = Engine.QueryInterface(cmd.entity, IID_TrainingQueue);
		if (queue)
			queue.AddBatch(cmd.template, +cmd.count, cmd.metadata);
		break;

	case "stop-train":
		var queue = Engine.QueryInterface(cmd.entity, IID_TrainingQueue);
		if (queue)
			queue.RemoveBatch(cmd.id);
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

		// Check whether it's obstructed by other entities
		var cmpObstruction = Engine.QueryInterface(ent, IID_Obstruction);
		if (cmpObstruction && cmpObstruction.CheckCollisions())
		{
			// TODO: report error to player (the building site was obstructed)

			// Remove the foundation because the construction was aborted
			Engine.DestroyEntity(ent);

			break;
		}

		// Check whether it's in a visible region
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		var visible = (cmpRangeManager.GetLosVisibility(ent, player) == "visible");
		if (!visible)
		{
			// TODO: report error to player (the building site was not visible)
			Engine.DestroyEntity(ent);
			break;
		}

		var cmpBuildRestrictions = Engine.QueryInterface(ent, IID_BuildRestrictions);
		var cmpBuildLimits = Engine.QueryInterface(playerEnt, IID_BuildLimits);
		if (!cmpBuildLimits.AllowedToBuild(cmpBuildRestrictions.GetCategory()))
		{
			Engine.DestroyEntity(ent);
			break;
		}
		
		var cmpCost = Engine.QueryInterface(ent, IID_Cost);
		if (!cmpPlayer.TrySubtractResources(cmpCost.GetResourceCosts()))
		{
			// TODO: report error to player (they ran out of resources)
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
	
	case "delete-entities":
		for each (var ent in cmd.entities)
		{
			// Verify the player owns the unit
			var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
			if (!cmpOwnership || cmpOwnership.GetOwner() != player)
				continue;

			var cmpHealth = Engine.QueryInterface(ent, IID_Health);
			if (cmpHealth)
				cmpHealth.Kill();
			else
				Engine.DestroyEntity(ent);
		}
		break;

	case "set-rallypoint":
		for each (var ent in cmd.entities)
		{
			var cmpRallyPoint = Engine.QueryInterface(ent, IID_RallyPoint);
			if (cmpRallyPoint)
				cmpRallyPoint.SetPosition(cmd.x, cmd.z);
		}
		break;

	case "unset-rallypoint":
		for each (var ent in cmd.entities)
		{
			var cmpRallyPoint = Engine.QueryInterface(ent, IID_RallyPoint);
			if (cmpRallyPoint)
				cmpRallyPoint.Unset();
		}
		break;
		
	case "defeat-player":
		// Get player entity by playerId
		var cmpPlayerMananager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
		var playerEnt = cmpPlayerManager.GetPlayerByID(cmd.playerId);
		// Send "OnPlayerDefeated" message to player
		Engine.PostMessage(playerEnt, MT_PlayerDefeated, null);
		break;

	case "garrison":
		var targetCmpOwnership = Engine.QueryInterface(cmd.target, IID_Ownership);
		if (!targetCmpOwnership || targetCmpOwnership.GetOwner() != player)
			break;
		var cmpUnitAI = GetFormationUnitAI(cmd.entities);
		if (cmpUnitAI)
			cmpUnitAI.Garrison(cmd.target);
		break;
		
	case "unload":
		var cmpOwnership = Engine.QueryInterface(cmd.garrisonHolder, IID_Ownership);
		if (!cmpOwnership || cmpOwnership.GetOwner() != player)
			break;
		var cmpGarrisonHolder = Engine.QueryInterface(cmd.garrisonHolder, IID_GarrisonHolder);
		if (cmpGarrisonHolder)
			cmpGarrisonHolder.Unload(cmd.entity);
		break;
		
	case "unload-all":
		var cmpOwnership = Engine.QueryInterface(cmd.garrisonHolder, IID_Ownership);
		if (!cmpOwnership || cmpOwnership.GetOwner() != player)
			break;
		
		var cmpGarrisonHolder = Engine.QueryInterface(cmd.garrisonHolder, IID_GarrisonHolder);
		cmpGarrisonHolder.UnloadAll();
		break;
		
	default:
		error("Ignoring unrecognised command type '" + cmd.type + "'");
	}
}

/**
 * Get some information about the formations used by entities.
 */
function ExtractFormations(ents)
{
	var entities = []; // subset of ents that have UnitAI
	var members = {}; // { formationentity: [ent, ent, ...], ... }
	for each (var ent in ents)
	{
		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		if (cmpUnitAI)
		{
			var fid = cmpUnitAI.GetFormationController();
			if (fid != INVALID_ENTITY)
			{
				if (!members[fid])
					members[fid] = [];
				members[fid].push(ent);
			}
			entities.push(ent);
		}
	}

	var ids = [ id for (id in members) ];

	return { "entities": entities, "members": members, "ids": ids };
}

/**
 * Remove the given list of entities from their current formations.
 */
function RemoveFromFormation(ents)
{
	var formation = ExtractFormations(ents);
	for (var fid in formation.members)
	{
		var cmpFormation = Engine.QueryInterface(+fid, IID_Formation);
		if (cmpFormation)
			cmpFormation.RemoveMembers(formation.members[fid]);
	}
}

/**
 * Return null or a UnitAI belonging either to the selected unit
 * or to a formation entity for the selected group of units.
 */
function GetFormationUnitAI(ents)
{
	// If an individual was selected, remove it from any formation
	// and command it individually
	if (ents.length == 1)
	{
		RemoveFromFormation(ents);

		return Engine.QueryInterface(ents[0], IID_UnitAI);
	}
	
	// Find what formations the selected entities are currently in
	var formation = ExtractFormations(ents);

	if (formation.entities.length == 0)
	{
		// No units with AI - nothing to do here
		return null;
	}

	var formationEnt = undefined;
	if (formation.ids.length == 1)
	{
		// Selected units all belong to the same formation.
		// Check that it doesn't have any other members
		var fid = formation.ids[0];
		var cmpFormation = Engine.QueryInterface(+fid, IID_Formation);
		if (cmpFormation && cmpFormation.GetMemberCount() == formation.entities.length)
		{
			// The whole formation was selected, so reuse its controller for this command
			formationEnt = +fid;
		}
	}

	if (!formationEnt)
	{
		// We need to give the selected units a new formation controller

		// Remove selected units from their current formation
		for (var fid in formation.members)
		{
			var cmpFormation = Engine.QueryInterface(+fid, IID_Formation);
			if (cmpFormation)
				cmpFormation.RemoveMembers(formation.members[fid]);
		}

		// Create the new controller
		formationEnt = Engine.AddEntity("special/formation");
		var cmpFormation = Engine.QueryInterface(formationEnt, IID_Formation);
		cmpFormation.SetMembers(formation.entities);
	}

	return Engine.QueryInterface(formationEnt, IID_UnitAI);
}

Engine.RegisterGlobal("ProcessCommand", ProcessCommand);
