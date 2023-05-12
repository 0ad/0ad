// Setting this to true will display some warnings when commands
//	are likely to fail, which may be useful for debugging AIs
var g_DebugCommands = false;

function ProcessCommand(player, cmd)
{
	let cmpPlayer = QueryPlayerIDInterface(player);
	if (!cmpPlayer)
		return;

	let data = {
		"cmpPlayer": cmpPlayer,
		"controlAllUnits": cmpPlayer.CanControlAllUnits()
	};

	if (cmd.entities)
		data.entities = FilterEntityList(cmd.entities, player, data.controlAllUnits);

	// TODO: queuing order and forcing formations doesn't really work.
	// To play nice, we'll still no-formation queued order if units are in formation
	// but the opposite perhaps ought to be implemented.
	if (!cmd.queued || cmd.formation == NULL_FORMATION)
		data.formation = cmd.formation || undefined;

	// Allow focusing the camera on recent commands
	let commandData = {
		"type": "playercommand",
		"players": [player],
		"cmd": cmd
	};

	// Save the position, since the GUI event is received after the unit died
	if (cmd.type == "delete-entities")
	{
		let cmpPosition = cmd.entities[0] && Engine.QueryInterface(cmd.entities[0], IID_Position);
		commandData.position = cmpPosition && cmpPosition.IsInWorld() && cmpPosition.GetPosition2D();
	}

	let cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGuiInterface.PushNotification(commandData);

	// Note: checks of UnitAI targets are not robust enough here, as ownership
	//	can change after the order is issued, they should be checked by UnitAI
	//	when the specific behavior (e.g. attack, garrison) is performed.
	// (Also it's not ideal if a command silently fails, it's nicer if UnitAI
	//	moves the entities closer to the target before giving up.)

	// Now handle various commands
	if (g_Commands[cmd.type])
	{
		var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
		cmpTrigger.CallEvent("OnPlayerCommand", { "player": player, "cmd": cmd });
		g_Commands[cmd.type](player, cmd, data);
	}
	else
		error("Invalid command: unknown command type: "+uneval(cmd));
}

var g_Commands = {

	"aichat": function(player, cmd, data)
	{
		var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		var notification = { "players": [player] };
		for (var key in cmd)
			notification[key] = cmd[key];
		cmpGuiInterface.PushNotification(notification);
	},

	"cheat": function(player, cmd, data)
	{
		Cheat(cmd);
	},

	"collect-treasure": function(player, cmd, data)
	{
		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI => {
			cmpUnitAI.CollectTreasure(cmd.target, cmd.queued);
		});
	},

	"collect-treasure-near-position": function(player, cmd, data)
	{
		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI => {
			cmpUnitAI.CollectTreasureNearPosition(cmd.x, cmd.z, cmd.queued);
		});
	},

	"diplomacy": function(player, cmd, data)
	{
		let cmpCeasefireManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_CeasefireManager);
		if (data.cmpPlayer.GetLockTeams() ||
		        cmpCeasefireManager && cmpCeasefireManager.IsCeasefireActive())
			return;

		switch(cmd.to)
		{
		case "ally":
			data.cmpPlayer.SetAlly(cmd.player);
			break;
		case "neutral":
			data.cmpPlayer.SetNeutral(cmd.player);
			break;
		case "enemy":
			data.cmpPlayer.SetEnemy(cmd.player);
			break;
		default:
			warn("Invalid command: Could not set "+player+" diplomacy status of player "+cmd.player+" to "+cmd.to);
		}

		var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGuiInterface.PushNotification({
			"type": "diplomacy",
			"players": [player],
			"targetPlayer": cmd.player,
			"status": cmd.to
		});
	},

	"tribute": function(player, cmd, data)
	{
		data.cmpPlayer.TributeResource(cmd.player, cmd.amounts);
	},

	"control-all": function(player, cmd, data)
	{
		if (!data.cmpPlayer.GetCheatsEnabled())
			return;

		var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGuiInterface.PushNotification({
			"type": "aichat",
			"players": [player],
			"message": markForTranslation("(Cheat - control all units)")
		});

		data.cmpPlayer.SetControlAllUnits(cmd.flag);
	},

	"reveal-map": function(player, cmd, data)
	{
		if (!data.cmpPlayer.GetCheatsEnabled())
			return;

		var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGuiInterface.PushNotification({
			"type": "aichat",
			"players": [player],
			"message": markForTranslation("(Cheat - reveal map)")
		});

		// Reveal the map for all players, not just the current player,
		// primarily to make it obvious to everyone that the player is cheating
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		cmpRangeManager.SetLosRevealAll(-1, cmd.enable);
	},

	"walk": function(player, cmd, data)
	{
		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI => {
			cmpUnitAI.Walk(cmd.x, cmd.z, cmd.queued, cmd.pushFront);
		});
	},

	"walk-custom": function(player, cmd, data)
	{
		for (let ent in data.entities)
			GetFormationUnitAIs([data.entities[ent]], player, cmd, data.formation).forEach(cmpUnitAI => {
				cmpUnitAI.Walk(cmd.targetPositions[ent].x, cmd.targetPositions[ent].y, cmd.queued, cmd.pushFront);
			});
	},

	"walk-to-range": function(player, cmd, data)
	{
		// Only used by the AI
		for (let ent of data.entities)
		{
			var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
			if (cmpUnitAI)
				cmpUnitAI.WalkToPointRange(cmd.x, cmd.z, cmd.min, cmd.max, cmd.queued, cmd.pushFront);
		}
	},

	"attack-walk": function(player, cmd, data)
	{
		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI => {
			cmpUnitAI.WalkAndFight(cmd.x, cmd.z, cmd.targetClasses, cmd.allowCapture, cmd.queued, cmd.pushFront);
		});
	},

	"attack-walk-custom": function(player, cmd, data)
	{
		for (let ent in data.entities)
			GetFormationUnitAIs([data.entities[ent]], player, cmd, data.formation).forEach(cmpUnitAI => {
				cmpUnitAI.WalkAndFight(cmd.targetPositions[ent].x, cmd.targetPositions[ent].y, cmd.targetClasses, cmd.allowCapture, cmd.queued, cmd.pushFront);
			});
	},

	"attack": function(player, cmd, data)
	{
		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI => {
			cmpUnitAI.Attack(cmd.target, cmd.allowCapture, cmd.queued, cmd.pushFront);
		});
	},

	"patrol": function(player, cmd, data)
	{
		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI =>
			cmpUnitAI.Patrol(cmd.x, cmd.z, cmd.targetClasses, cmd.allowCapture, cmd.queued)
		);
	},

	"heal": function(player, cmd, data)
	{
		if (g_DebugCommands && !(IsOwnedByPlayer(player, cmd.target) || IsOwnedByAllyOfPlayer(player, cmd.target)))
			warn("Invalid command: heal target is not owned by player "+player+" or their ally: "+uneval(cmd));

		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI => {
			cmpUnitAI.Heal(cmd.target, cmd.queued, cmd.pushFront);
		});
	},

	"repair": function(player, cmd, data)
	{
		// This covers both repairing damaged buildings, and constructing unfinished foundations
		if (g_DebugCommands && !IsOwnedByAllyOfPlayer(player, cmd.target))
			warn("Invalid command: repair target is not owned by ally of player "+player+": "+uneval(cmd));

		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI => {
			cmpUnitAI.Repair(cmd.target, cmd.autocontinue, cmd.queued, cmd.pushFront);
		});
	},

	"gather": function(player, cmd, data)
	{
		if (g_DebugCommands && !(IsOwnedByPlayer(player, cmd.target) || IsOwnedByGaia(cmd.target)))
			warn("Invalid command: resource is not owned by gaia or player "+player+": "+uneval(cmd));

		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI => {
			cmpUnitAI.Gather(cmd.target, cmd.queued, cmd.pushFront);
		});
	},

	"gather-near-position": function(player, cmd, data)
	{
		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI => {
			cmpUnitAI.GatherNearPosition(cmd.x, cmd.z, cmd.resourceType, cmd.resourceTemplate, cmd.queued, cmd.pushFront);
		});
	},

	"returnresource": function(player, cmd, data)
	{
		if (g_DebugCommands && !IsOwnedByPlayer(player, cmd.target))
			warn("Invalid command: dropsite is not owned by player "+player+": "+uneval(cmd));

		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI => {
			cmpUnitAI.ReturnResource(cmd.target, cmd.queued, cmd.pushFront);
		});
	},

	"back-to-work": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
			if(!cmpUnitAI || !cmpUnitAI.BackToWork())
				notifyBackToWorkFailure(player);
		}
	},

	"call-to-arms": function(player, cmd, data)
	{
		const unitsToMove = data.entities.filter(ent =>
			MatchesClassList(Engine.QueryInterface(ent, IID_Identity).GetClassesList(),
				["Soldier", "Warship", "Siege", "Healer"])
		);
		GetFormationUnitAIs(unitsToMove, player, cmd, data.formation).forEach(cmpUnitAI => {
			if (cmd.pushFront)
			{
				cmpUnitAI.WalkAndFight(cmd.position.x, cmd.position.z, cmd.targetClasses, cmd.allowCapture, false, cmd.pushFront);
				cmpUnitAI.DropAtNearestDropSite(false, cmd.pushFront);
			}
			else
			{
				cmpUnitAI.DropAtNearestDropSite(cmd.queued, false)
				cmpUnitAI.WalkAndFight(cmd.position.x, cmd.position.z, cmd.targetClasses, cmd.allowCapture, true, false);
			}
		});
	},

	"remove-guard": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
			if (cmpUnitAI)
				cmpUnitAI.RemoveGuard();
		}
	},

	"train": function(player, cmd, data)
	{
		if (!Number.isInteger(cmd.count) || cmd.count <= 0)
		{
			warn("Invalid command: can't train " + uneval(cmd.count) + " units");
			return;
		}

		// Check entity limits
		var template = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager).GetTemplate(cmd.template);
		var unitCategory = null;
		if (template.TrainingRestrictions)
			unitCategory = template.TrainingRestrictions.Category;

		// Verify that the building(s) can be controlled by the player
		if (data.entities.length <= 0)
		{
			if (g_DebugCommands)
				warn("Invalid command: training building(s) cannot be controlled by player "+player+": "+uneval(cmd));
			return;
		}

		for (let ent of data.entities)
		{
			if (unitCategory)
			{
				var cmpPlayerEntityLimits = QueryOwnerInterface(ent, IID_EntityLimits);
				if (cmpPlayerEntityLimits && !cmpPlayerEntityLimits.AllowedToTrain(unitCategory, cmd.count, cmd.template, template.TrainingRestrictions.MatchLimit))
				{
					if (g_DebugCommands)
						warn(unitCategory + " train limit is reached: " + uneval(cmd));
					continue;
				}
			}

			var cmpTechnologyManager = QueryOwnerInterface(ent, IID_TechnologyManager);
			if (cmpTechnologyManager && !cmpTechnologyManager.CanProduce(cmd.template))
			{
				if (g_DebugCommands)
					warn("Invalid command: training requires unresearched technology: " + uneval(cmd));
				continue;
			}

			const cmpTrainer = Engine.QueryInterface(ent, IID_Trainer);
			if (!cmpTrainer)
				continue;

			let templateName = cmd.template;
			// Check if the building can train the unit
			// TODO: the AI API does not take promotion technologies into account for the list
			// of trainable units (taken directly from the unit template). Here is a temporary fix.
			if (data.cmpPlayer.IsAI())
				templateName = GetUpgradedTemplate(player, cmd.template);

			if (cmpTrainer.CanTrain(templateName))
				Engine.QueryInterface(ent, IID_ProductionQueue)?.AddItem(templateName, "unit", +cmd.count, cmd.metadata, cmd.pushFront);
		}
	},

	"research": function(player, cmd, data)
	{
		var cmpTechnologyManager = QueryOwnerInterface(cmd.entity, IID_TechnologyManager);
		if (cmpTechnologyManager && !cmpTechnologyManager.CanResearch(cmd.template))
		{
			if (g_DebugCommands)
				warn("Invalid command: Requirements to research technology are not met: " + uneval(cmd));
			return;
		}

		var queue = Engine.QueryInterface(cmd.entity, IID_ProductionQueue);
		if (queue)
			queue.AddItem(cmd.template, "technology", undefined, cmd.metadata, cmd.pushFront);
	},

	"stop-production": function(player, cmd, data)
	{
		let cmpProductionQueue = Engine.QueryInterface(cmd.entity, IID_ProductionQueue);
		if (cmpProductionQueue)
			cmpProductionQueue.RemoveItem(cmd.id);
	},

	"construct": function(player, cmd, data)
	{
		TryConstructBuilding(player, data.cmpPlayer, data.controlAllUnits, cmd);
	},

	"construct-wall": function(player, cmd, data)
	{
		TryConstructWall(player, data.cmpPlayer, data.controlAllUnits, cmd);
	},

	"delete-entities": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			if (!data.controlAllUnits)
			{
				let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
				if (cmpIdentity && cmpIdentity.IsUndeletable())
					continue;

				let cmpCapturable = QueryMiragedInterface(ent, IID_Capturable);
				if (cmpCapturable &&
				    cmpCapturable.GetCapturePoints()[player] < cmpCapturable.GetMaxCapturePoints() / 2)
					continue;

				let cmpResourceSupply = QueryMiragedInterface(ent, IID_ResourceSupply);
				if (cmpResourceSupply && cmpResourceSupply.GetKillBeforeGather())
					continue;
			}

			let cmpMirage = Engine.QueryInterface(ent, IID_Mirage);
			if (cmpMirage)
			{
				let cmpMiragedHealth = Engine.QueryInterface(cmpMirage.parent, IID_Health);
				if (cmpMiragedHealth)
					cmpMiragedHealth.Kill();
				else
					Engine.DestroyEntity(cmpMirage.parent);

				Engine.DestroyEntity(ent);
				continue;
			}

			let cmpHealth = Engine.QueryInterface(ent, IID_Health);
			if (cmpHealth)
				cmpHealth.Kill();
			else
				Engine.DestroyEntity(ent);
		}
	},

	"set-rallypoint": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			var cmpRallyPoint = Engine.QueryInterface(ent, IID_RallyPoint);
			if (cmpRallyPoint)
			{
				if (!cmd.queued)
					cmpRallyPoint.Unset();

				cmpRallyPoint.AddPosition(cmd.x, cmd.z);
				cmpRallyPoint.AddData(clone(cmd.data));
			}
		}
	},

	"unset-rallypoint": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			var cmpRallyPoint = Engine.QueryInterface(ent, IID_RallyPoint);
			if (cmpRallyPoint)
				cmpRallyPoint.Reset();
		}
	},

	"resign": function(player, cmd, data)
	{
		data.cmpPlayer.Defeat(markForTranslation("%(player)s has resigned."));
	},

	"occupy-turret": function(player, cmd, data)
	{
		GetFormationUnitAIs(data.entities, player).forEach(cmpUnitAI => {
			cmpUnitAI.OccupyTurret(cmd.target, cmd.queued);
		});
	},

	"garrison": function(player, cmd, data)
	{
		if (!CanPlayerOrAllyControlUnit(cmd.target, player, data.controlAllUnits))
		{
			if (g_DebugCommands)
				warn("Invalid command: garrison target cannot be controlled by player "+player+" (or ally): "+uneval(cmd));
			return;
		}

		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI => {
			cmpUnitAI.Garrison(cmd.target, cmd.queued, cmd.pushFront);
		});
	},

	"guard": function(player, cmd, data)
	{
		if (!IsOwnedByPlayerOrMutualAlly(cmd.target, player, data.controlAllUnits))
		{
			if (g_DebugCommands)
				warn("Invalid command: Guard/escort target is not owned by player " + player + " or ally thereof: " + uneval(cmd));
			return;
		}

		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI => {
			cmpUnitAI.Guard(cmd.target, cmd.queued, cmd.pushFront);
		});
	},

	"stop": function(player, cmd, data)
	{
		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI => {
			cmpUnitAI.Stop(cmd.queued);
		});
	},

	"leave-turret": function(player, cmd, data)
	{
		let notUnloaded = 0;
		for (let ent of data.entities)
		{
			let cmpTurretable = Engine.QueryInterface(ent, IID_Turretable);
			if (!cmpTurretable || !cmpTurretable.LeaveTurret())
				++notUnloaded;
		}

		if (notUnloaded)
			notifyUnloadFailure(player);
	},

	"unload-turrets": function(player, cmd, data)
	{
		let notUnloaded = 0;
		for (let ent of data.entities)
		{
			let cmpTurretHolder = Engine.QueryInterface(ent, IID_TurretHolder);
			for (let turret of cmpTurretHolder.GetEntities())
			{
				let cmpTurretable = Engine.QueryInterface(turret, IID_Turretable);
				if (!cmpTurretable || !cmpTurretable.LeaveTurret())
					++notUnloaded;
			}
		}

		if (notUnloaded)
			notifyUnloadFailure(player);
	},

	"unload": function(player, cmd, data)
	{
		if (!CanPlayerOrAllyControlUnit(cmd.garrisonHolder, player, data.controlAllUnits))
		{
			if (g_DebugCommands)
				warn("Invalid command: unload target cannot be controlled by player "+player+" (or ally): "+uneval(cmd));
			return;
		}

		var cmpGarrisonHolder = Engine.QueryInterface(cmd.garrisonHolder, IID_GarrisonHolder);
		var notUngarrisoned = 0;

		// The owner can ungarrison every garrisoned unit
		if (IsOwnedByPlayer(player, cmd.garrisonHolder))
			data.entities = cmd.entities;

		for (let ent of data.entities)
			if (!cmpGarrisonHolder || !cmpGarrisonHolder.Unload(ent))
				++notUngarrisoned;

		if (notUngarrisoned != 0)
			notifyUnloadFailure(player, cmd.garrisonHolder);
	},

	"unload-template": function(player, cmd, data)
	{
		var entities = FilterEntityListWithAllies(cmd.garrisonHolders, player, data.controlAllUnits);
		for (let garrisonHolder of entities)
		{
			var cmpGarrisonHolder = Engine.QueryInterface(garrisonHolder, IID_GarrisonHolder);
			if (cmpGarrisonHolder)
			{
				// Only the owner of the garrisonHolder may unload entities from any owners
				if (!IsOwnedByPlayer(player, garrisonHolder) && !data.controlAllUnits
				    && player != +cmd.owner)
						continue;

				if (!cmpGarrisonHolder.UnloadTemplate(cmd.template, cmd.owner, cmd.all))
					notifyUnloadFailure(player, garrisonHolder);
			}
		}
	},

	"unload-all-by-owner": function(player, cmd, data)
	{
		var entities = FilterEntityListWithAllies(cmd.garrisonHolders, player, data.controlAllUnits);
		for (let garrisonHolder of entities)
		{
			var cmpGarrisonHolder = Engine.QueryInterface(garrisonHolder, IID_GarrisonHolder);
			if (!cmpGarrisonHolder || !cmpGarrisonHolder.UnloadAllByOwner(player))
				notifyUnloadFailure(player, garrisonHolder);
		}
	},

	"unload-all": function(player, cmd, data)
	{
		var entities = FilterEntityList(cmd.garrisonHolders, player, data.controlAllUnits);
		for (let garrisonHolder of entities)
		{
			var cmpGarrisonHolder = Engine.QueryInterface(garrisonHolder, IID_GarrisonHolder);
			if (!cmpGarrisonHolder || !cmpGarrisonHolder.UnloadAll())
				notifyUnloadFailure(player, garrisonHolder);
		}
	},

	"alert-raise": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			var cmpAlertRaiser = Engine.QueryInterface(ent, IID_AlertRaiser);
			if (cmpAlertRaiser)
				cmpAlertRaiser.RaiseAlert();
		}
	},

	"alert-end": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			var cmpAlertRaiser = Engine.QueryInterface(ent, IID_AlertRaiser);
			if (cmpAlertRaiser)
				cmpAlertRaiser.EndOfAlert();
		}
	},

	"formation": function(player, cmd, data)
	{
		GetFormationUnitAIs(data.entities, player, cmd, data.formation, true).forEach(cmpUnitAI => {
			cmpUnitAI.MoveIntoFormation(cmd);
		});
	},

	"promote": function(player, cmd, data)
	{
		if (!data.cmpPlayer.GetCheatsEnabled())
			return;

		var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGuiInterface.PushNotification({
			"type": "aichat",
			"players": [player],
			"message": markForTranslation("(Cheat - promoted units)"),
			"translateMessage": true
		});

		for (let ent of cmd.entities)
		{
			var cmpPromotion = Engine.QueryInterface(ent, IID_Promotion);
			if (cmpPromotion)
				cmpPromotion.IncreaseXp(cmpPromotion.GetRequiredXp() - cmpPromotion.GetCurrentXp());
		}
	},

	"stance": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
			if (cmpUnitAI && !cmpUnitAI.IsTurret())
				cmpUnitAI.SwitchToStance(cmd.name);
		}
	},

	"lock-gate": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			var cmpGate = Engine.QueryInterface(ent, IID_Gate);
			if (!cmpGate)
				continue;

			if (cmd.lock)
				cmpGate.LockGate();
			else
				cmpGate.UnlockGate();
		}
	},

	"setup-trade-route": function(player, cmd, data)
	{
		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI => {
			cmpUnitAI.SetupTradeRoute(cmd.target, cmd.source, cmd.route, cmd.queued);
		});
	},

	"cancel-setup-trade-route": function(player, cmd, data)
	{
		GetFormationUnitAIs(data.entities, player, cmd, data.formation).forEach(cmpUnitAI => {
			cmpUnitAI.CancelSetupTradeRoute(cmd.target);
		});
	},

	"set-trading-goods": function(player, cmd, data)
	{
		data.cmpPlayer.SetTradingGoods(cmd.tradingGoods);
	},

	"barter": function(player, cmd, data)
	{
		var cmpBarter = Engine.QueryInterface(SYSTEM_ENTITY, IID_Barter);
		cmpBarter.ExchangeResources(player, cmd.sell, cmd.buy, cmd.amount);
	},

	"set-shading-color": function(player, cmd, data)
	{
		// Prevent multiplayer abuse
		if (!data.cmpPlayer.IsAI())
			return;

		// Debug command to make an entity brightly colored
		for (let ent of cmd.entities)
		{
			var cmpVisual = Engine.QueryInterface(ent, IID_Visual);
			if (cmpVisual)
				cmpVisual.SetShadingColor(cmd.rgb[0], cmd.rgb[1], cmd.rgb[2], 0); // alpha isn't used so just send 0
		}
	},

	"pack": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
			if (!cmpUnitAI)
				continue;

			if (cmd.pack)
				cmpUnitAI.Pack(cmd.queued, cmd.pushFront);
			else
				cmpUnitAI.Unpack(cmd.queued, cmd.pushFront);
		}
	},

	"cancel-pack": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
			if (!cmpUnitAI)
				continue;

			if (cmd.pack)
				cmpUnitAI.CancelPack(cmd.queued, cmd.pushFront);
			else
				cmpUnitAI.CancelUnpack(cmd.queued, cmd.pushFront);
		}
	},

	"upgrade": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			var cmpUpgrade = Engine.QueryInterface(ent, IID_Upgrade);

			if (!cmpUpgrade || !cmpUpgrade.CanUpgradeTo(cmd.template))
				continue;

			if (cmpUpgrade.WillCheckPlacementRestrictions(cmd.template) && ObstructionsBlockingTemplateChange(ent, cmd.template))
			{
				var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
				cmpGUIInterface.PushNotification({
					"players": [player],
					"message": markForTranslation("Cannot upgrade as distance requirements are not verified or terrain is obstructed.")
				});
				continue;
			}

			// Check entity limits
			var cmpEntityLimits = QueryPlayerIDInterface(player, IID_EntityLimits);
			if (cmpEntityLimits && !cmpEntityLimits.AllowedToReplace(ent, cmd.template))
			{
				if (g_DebugCommands)
					warn("Invalid command: build limits check failed for player " + player + ": " + uneval(cmd));
				continue;
			}

			if (!RequirementsHelper.AreRequirementsMet(cmpUpgrade.GetRequirements(cmd.template), player))
			{
				if (g_DebugCommands)
					warn("Invalid command: upgrading is not possible for this player or requires unresearched technology: " + uneval(cmd));
				continue;
			}

			cmpUpgrade.Upgrade(cmd.template, data.cmpPlayer);
		}
	},

	"cancel-upgrade": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			let cmpUpgrade = Engine.QueryInterface(ent, IID_Upgrade);
			if (cmpUpgrade)
				cmpUpgrade.CancelUpgrade(player);
		}
	},

	"attack-request": function(player, cmd, data)
	{
		// Send a chat message to human players
		var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGuiInterface.PushNotification({
			"type": "aichat",
			"players": [player],
			"message": "/allies " + markForTranslation("Attack against %(_player_)s requested."),
			"translateParameters": ["_player_"],
			"parameters": { "_player_": cmd.player }
		});

		// And send an attackRequest event to the AIs
		let cmpAIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_AIInterface);
		if (cmpAIInterface)
			cmpAIInterface.PushEvent("AttackRequest", cmd);
	},

	"spy-request": function(player, cmd, data)
	{
		let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		let ent = pickRandom(cmpRangeManager.GetEntitiesByPlayer(cmd.player).filter(ent => {
			let cmpVisionSharing = Engine.QueryInterface(ent, IID_VisionSharing);
			return cmpVisionSharing && cmpVisionSharing.IsBribable() && !cmpVisionSharing.ShareVisionWith(player);
		}));

		let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGUIInterface.PushNotification({
			"type": "spy-response",
			"players": [player],
			"target": cmd.player,
			"entity": ent
		});
		if (ent)
			Engine.QueryInterface(ent, IID_VisionSharing).AddSpy(cmd.source);
		else
		{
			let template = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager).GetTemplate("special/spy");
			IncurBribeCost(template, player, cmd.player, true);
			// update statistics for failed bribes
			let cmpBribesStatisticsTracker = QueryPlayerIDInterface(player, IID_StatisticsTracker);
			if (cmpBribesStatisticsTracker)
				cmpBribesStatisticsTracker.IncreaseFailedBribesCounter();
			cmpGUIInterface.PushNotification({
				"type": "text",
				"players": [player],
				"message": markForTranslation("There are no bribable units"),
				"translateMessage": true
			});
		}
	},

	"diplomacy-request": function(player, cmd, data)
	{
		let cmpAIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_AIInterface);
		if (cmpAIInterface)
			cmpAIInterface.PushEvent("DiplomacyRequest", cmd);
	},

	"tribute-request": function(player, cmd, data)
	{
		let cmpAIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_AIInterface);
		if (cmpAIInterface)
			cmpAIInterface.PushEvent("TributeRequest", cmd);
	},

	"dialog-answer": function(player, cmd, data)
	{
		// Currently nothing. Triggers can read it anyway, and send this
		// message to any component you like.
	},

	"set-dropsite-sharing": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			let cmpResourceDropsite = Engine.QueryInterface(ent, IID_ResourceDropsite);
			if (cmpResourceDropsite && cmpResourceDropsite.IsSharable())
				cmpResourceDropsite.SetSharing(cmd.shared);
		}
	},

	"map-flare": function(player, cmd, data)
	{
		let cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGuiInterface.PushNotification({
			"type": "map-flare",
			"players": [player],
			"position": cmd.position
		});
	},

	"autoqueue-on": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			let cmpProductionQueue = Engine.QueryInterface(ent, IID_ProductionQueue);
			if (cmpProductionQueue)
				cmpProductionQueue.EnableAutoQueue();
		}
	},

	"autoqueue-off": function(player, cmd, data)
	{
		for (let ent of data.entities)
		{
			let cmpProductionQueue = Engine.QueryInterface(ent, IID_ProductionQueue);
			if (cmpProductionQueue)
				cmpProductionQueue.DisableAutoQueue();
		}
	},

};

/**
 * Sends a GUI notification about unit(s) that failed to ungarrison.
 */
function notifyUnloadFailure(player)
{
	let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.PushNotification({
		"type": "text",
		"players": [player],
		"message": markForTranslation("Unable to unload unit(s)."),
		"translateMessage": true
	});
}

/**
 * Sends a GUI notification about worker(s) that failed to go back to work.
 */
function notifyBackToWorkFailure(player)
{
	var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.PushNotification({
		"type": "text",
		"players": [player],
		"message": markForTranslation("Some unit(s) can't go back to work"),
		"translateMessage": true
	});
}

/**
 * Sends a GUI notification about entities that can't be controlled.
 * @param {number} player - The player-ID of the player that needs to receive this message.
 */
function notifyOrderFailure(entity, player)
{
	let cmpIdentity = Engine.QueryInterface(entity, IID_Identity);
	if (!cmpIdentity)
		return;

	let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.PushNotification({
		"type": "text",
		"players": [player],
		"message": markForTranslation("%(unit)s can't be controlled."),
		"parameters": { "unit": cmpIdentity.GetGenericName() },
		"translateParameters": ["unit"],
		"translateMessage": true
	});
}

/**
 * Get some information about the formations used by entities.
 */
function ExtractFormations(ents)
{
	let entities = []; // Entities with UnitAI.
	let members = {}; // { formationentity: [ent, ent, ...], ... }
	let templates = {};  // { formationentity: template }
	for (let ent of ents)
	{
		let cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		if (!cmpUnitAI)
			continue;

		entities.push(ent);

		let fid = cmpUnitAI.GetFormationController();
		if (fid == INVALID_ENTITY)
			continue;

		if (!members[fid])
		{
			members[fid] = [];
			templates[fid] = cmpUnitAI.GetFormationTemplate();
		}
		members[fid].push(ent);
	}

	return {
		"entities": entities,
		"members": members,
		"templates": templates
	};
}

/**
 * Tries to find the best angle to put a dock at a given position
 * Taken from GuiInterface.js
 */
function GetDockAngle(template, x, z)
{
	var cmpTerrain = Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain);
	var cmpWaterManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_WaterManager);
	if (!cmpTerrain || !cmpWaterManager)
		return undefined;

	// Get footprint size
	var halfSize = 0;
	if (template.Footprint.Square)
		halfSize = Math.max(template.Footprint.Square["@depth"], template.Footprint.Square["@width"])/2;
	else if (template.Footprint.Circle)
		halfSize = template.Footprint.Circle["@radius"];

	/* Find direction of most open water, algorithm:
	 *	1. Pick points in a circle around dock
	 *	2. If point is in water, add to array
	 *	3. Scan array looking for consecutive points
	 *	4. Find longest sequence of consecutive points
	 *	5. If sequence equals all points, no direction can be determined,
	 *		expand search outward and try (1) again
	 *	6. Calculate angle using average of sequence
	 */
	const numPoints = 16;
	for (var dist = 0; dist < 4; ++dist)
	{
		var waterPoints = [];
		for (var i = 0; i < numPoints; ++i)
		{
			var angle = (i/numPoints)*2*Math.PI;
			var d = halfSize*(dist+1);
			var nx = x - d*Math.sin(angle);
			var nz = z + d*Math.cos(angle);

			if (cmpTerrain.GetGroundLevel(nx, nz) < cmpWaterManager.GetWaterLevel(nx, nz))
				waterPoints.push(i);
		}
		var consec = [];
		var length = waterPoints.length;
		if (!length)
			continue;
		for (var i = 0; i < length; ++i)
		{
			var count = 0;
			for (let j = 0; j < length - 1; ++j)
			{
				if ((waterPoints[(i + j) % length] + 1) % numPoints == waterPoints[(i + j + 1) % length])
					++count;
				else
					break;
			}
			consec[i] = count;
		}
		var start = 0;
		var count = 0;
		for (var c in consec)
		{
			if (consec[c] > count)
			{
				start = c;
				count = consec[c];
			}
		}

		// If we've found a shoreline, stop searching
		if (count != numPoints-1)
			return -((waterPoints[start] + consec[start]/2) % numPoints) / numPoints * 2 * Math.PI;
	}
	return undefined;
}

/**
 * Attempts to construct a building using the specified parameters.
 * Returns true on success, false on failure.
 */
function TryConstructBuilding(player, cmpPlayer, controlAllUnits, cmd)
{
	// Message structure:
	// {
	//   "type": "construct",
	//   "entities": [...],                 // entities that will be ordered to construct the building (if applicable)
	//   "template": "...",                 // template name of the entity being constructed
	//   "x": ...,
	//   "z": ...,
	//   "angle": ...,
	//   "metadata": "...",                 // AI metadata of the building
	//   "actorSeed": ...,
	//   "autorepair": true,                // whether to automatically start constructing/repairing the new foundation
	//   "autocontinue": true,              // whether to automatically gather/build/etc after finishing this
	//   "queued": true,                    // whether to add the construction/repairing of this foundation to entities' queue (if applicable)
	//   "obstructionControlGroup": ...,    // Optional; the obstruction control group ID that should be set for this building prior to obstruction
	//                                      // testing to determine placement validity. If specified, must be a valid control group ID (> 0).
	//   "obstructionControlGroup2": ...,   // Optional; secondary obstruction control group ID that should be set for this building prior to obstruction
	//                                      // testing to determine placement validity. May be INVALID_ENTITY.
	// }

	/*
	 * Construction process:
	 *  . Take resources away immediately.
	 *  . Create a foundation entity with 1hp, 0% build progress.
	 *  . Increase hp and build progress up to 100% when people work on it.
	 *  . If it's destroyed, an appropriate fraction of the resource cost is refunded.
	 *  . If it's completed, it gets replaced with the real building.
	 */

	// Check whether we can control these units
	var entities = FilterEntityList(cmd.entities, player, controlAllUnits);
	if (!entities.length)
		return false;

	var foundationTemplate = "foundation|" + cmd.template;

	// Tentatively create the foundation (we might find later that it's a invalid build command)
	var ent = Engine.AddEntity(foundationTemplate);
	if (ent == INVALID_ENTITY)
	{
		// Error (e.g. invalid template names)
		error("Error creating foundation entity for '" + cmd.template + "'");
		return false;
	}

	// If it's a dock, get the right angle.
	var template = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager).GetTemplate(cmd.template);
	var angle = cmd.angle;
	if (template.BuildRestrictions.PlacementType === "shore")
	{
		let angleDock = GetDockAngle(template, cmd.x, cmd.z);
		if (angleDock !== undefined)
			angle = angleDock;
	}

	// Move the foundation to the right place
	var cmpPosition = Engine.QueryInterface(ent, IID_Position);
	cmpPosition.JumpTo(cmd.x, cmd.z);
	cmpPosition.SetYRotation(angle);

	// Set the obstruction control group if needed
	if (cmd.obstructionControlGroup || cmd.obstructionControlGroup2)
	{
		var cmpObstruction = Engine.QueryInterface(ent, IID_Obstruction);

		// primary control group must always be valid
		if (cmd.obstructionControlGroup)
		{
			if (cmd.obstructionControlGroup <= 0)
				warn("[TryConstructBuilding] Invalid primary obstruction control group " + cmd.obstructionControlGroup + " received; must be > 0");

			cmpObstruction.SetControlGroup(cmd.obstructionControlGroup);
		}

		if (cmd.obstructionControlGroup2)
			cmpObstruction.SetControlGroup2(cmd.obstructionControlGroup2);
	}

	// Make it owned by the current player
	var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
	cmpOwnership.SetOwner(player);

	// Check whether building placement is valid
	var cmpBuildRestrictions = Engine.QueryInterface(ent, IID_BuildRestrictions);
	if (cmpBuildRestrictions)
	{
		var ret = cmpBuildRestrictions.CheckPlacement();
		if (!ret.success)
		{
			if (g_DebugCommands)
				warn("Invalid command: build restrictions check failed with '"+ret.message+"' for player "+player+": "+uneval(cmd));

			var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
			ret.players = [player];
			cmpGuiInterface.PushNotification(ret);

			// Remove the foundation because the construction was aborted
			// move it out of world because it's not destroyed immediately.
			cmpPosition.MoveOutOfWorld();
			Engine.DestroyEntity(ent);
			return false;
		}
	}
	else
		error("cmpBuildRestrictions not defined");

	// Check entity limits
	var cmpEntityLimits = QueryPlayerIDInterface(player, IID_EntityLimits);
	if (cmpEntityLimits && !cmpEntityLimits.AllowedToBuild(cmpBuildRestrictions.GetCategory()))
	{
		if (g_DebugCommands)
			warn("Invalid command: build limits check failed for player "+player+": "+uneval(cmd));

		// Remove the foundation because the construction was aborted
		cmpPosition.MoveOutOfWorld();
		Engine.DestroyEntity(ent);
		return false;
	}

	var cmpTechnologyManager = QueryPlayerIDInterface(player, IID_TechnologyManager);
	if (cmpTechnologyManager && !cmpTechnologyManager.CanProduce(cmd.template))
	{
		if (g_DebugCommands)
			warn("Invalid command: required technology check failed for player "+player+": "+uneval(cmd));

		var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGuiInterface.PushNotification({
			"type": "text",
			"players": [player],
			"message": markForTranslation("The building's technology requirements are not met."),
			"translateMessage": true
		});

		// Remove the foundation because the construction was aborted
		cmpPosition.MoveOutOfWorld();
		Engine.DestroyEntity(ent);
	}

	// We need the cost after tech and aura modifications.
	let cmpCost = Engine.QueryInterface(ent, IID_Cost);
	let costs = cmpCost.GetResourceCosts();

	if (!cmpPlayer.TrySubtractResources(costs))
	{
		if (g_DebugCommands)
			warn("Invalid command: building cost check failed for player "+player+": "+uneval(cmd));

		Engine.DestroyEntity(ent);
		cmpPosition.MoveOutOfWorld();
		return false;
	}

	var cmpVisual = Engine.QueryInterface(ent, IID_Visual);
	if (cmpVisual && cmd.actorSeed !== undefined)
		cmpVisual.SetActorSeed(cmd.actorSeed);

	// Initialise the foundation
	var cmpFoundation = Engine.QueryInterface(ent, IID_Foundation);
	cmpFoundation.InitialiseConstruction(cmd.template);

	// send Metadata info if any
	if (cmd.metadata)
		Engine.PostMessage(ent, MT_AIMetadata, { "id": ent, "metadata" : cmd.metadata, "owner" : player } );

	// Tell the units to start building this new entity
	if (cmd.autorepair)
	{
		ProcessCommand(player, {
			"type": "repair",
			"entities": entities,
			"target": ent,
			"autocontinue": cmd.autocontinue,
			"queued": cmd.queued,
			"pushFront": cmd.pushFront,
			"formation": cmd.formation || undefined
		});
	}

	return ent;
}

function TryConstructWall(player, cmpPlayer, controlAllUnits, cmd)
{
	// 'cmd' message structure:
	// {
	//   "type": "construct-wall",
	//   "entities": [...],           // entities that will be ordered to construct the wall (if applicable)
	//   "pieces": [                  // ordered list of information about the pieces making up the wall (towers, wall segments, ...)
	//      {
	//         "template": "...",     // one of the templates from the wallset
	//         "x": ...,
	//         "z": ...,
	//         "angle": ...,
	//      },
	//      ...
	//   ],
	//   "wallSet": {
	//      "templates": {
	//        "tower":                // tower template name
	//        "long":                 // long wall segment template name
	//        ...                     // etc.
	//      },
	//      "maxTowerOverlap": ...,
	//      "minTowerOverlap": ...,
	//   },
	//   "startSnappedEntity":        // optional; entity ID of tower being snapped to at the starting side of the wall
	//   "endSnappedEntity":          // optional; entity ID of tower being snapped to at the ending side of the wall
	//   "autorepair": true,          // whether to automatically start constructing/repairing the new foundation
	//   "autocontinue": true,        // whether to automatically gather/build/etc after finishing this
	//   "queued": true,              // whether to add the construction/repairing of this wall's pieces to entities' queue (if applicable)
	// }

	if (cmd.pieces.length <= 0)
		return;

	if (cmd.startSnappedEntity && cmd.pieces[0].template == cmd.wallSet.templates.tower)
	{
		error("[TryConstructWall] Starting wall piece cannot be a tower (" + cmd.wallSet.templates.tower + ") when snapping at the starting side");
		return;
	}

	if (cmd.endSnappedEntity && cmd.pieces[cmd.pieces.length - 1].template == cmd.wallSet.templates.tower)
	{
		error("[TryConstructWall] Ending wall piece cannot be a tower (" + cmd.wallSet.templates.tower + ") when snapping at the ending side");
		return;
	}

	// Assign obstruction control groups to allow the wall pieces to mutually overlap during foundation placement
	// and during construction. The scheme here is that whatever wall pieces are inbetween two towers inherit the control
	// groups of both of the towers they are connected to (either newly constructed ones as part of the wall, or existing
	// towers in the case of snapping). The towers themselves all keep their default unique control groups.

	// To support this, every non-tower piece registers the entity ID of the towers (or foundations thereof) that neighbour
	// it on either side. Specifically, each non-tower wall piece has its primary control group set equal to that of the
	// first tower encountered towards the starting side of the wall, and its secondary control group set equal to that of
	// the first tower encountered towards the ending side of the wall (if any).

	// We can't build the whole wall at once by linearly stepping through the wall pieces and build them, because the
	// wall segments may/will need the entity IDs of towers that come afterwards. So, build it in two passes:
	//
	//   FIRST PASS:
	//    - Go from start to end and construct wall piece foundations as far as we can without running into a piece that
	//        cannot be built (e.g. because it is obstructed). At each non-tower, set the most recently built tower's ID
	//        as the primary control group, thus allowing it to be built overlapping the previous piece.
	//    - If we encounter a new tower along the way (which will gain its own control group), do the following:
	//        o First build it using temporarily the same control group of the previous (non-tower) piece
	//        o Set the previous piece's secondary control group to the tower's entity ID
	//        o Restore the primary control group of the constructed tower back its original (unique) value.
	//      The temporary control group is necessary to allow the newer tower with its unique control group ID to be able
	//        to be placed while overlapping the previous piece.
	//
	//   SECOND PASS:
	//    - Go end to start from the last successfully placed wall piece (which might be a tower we backtracked to), this
	//      time registering the right neighbouring tower in each non-tower piece.

	// first pass; L -> R

	var lastTowerIndex = -1; // index of the last tower we've encountered in cmd.pieces
	var lastTowerControlGroup = null; // control group of the last tower we've encountered, to assign to non-tower pieces

	// If we're snapping to an existing entity at the starting end, set lastTowerControlGroup to its control group ID so that
	// the first wall piece can be built while overlapping it.
	if (cmd.startSnappedEntity)
	{
		var cmpSnappedStartObstruction = Engine.QueryInterface(cmd.startSnappedEntity, IID_Obstruction);
		if (!cmpSnappedStartObstruction)
		{
			error("[TryConstructWall] Snapped entity on starting side does not have an obstruction component");
			return;
		}

		lastTowerControlGroup = cmpSnappedStartObstruction.GetControlGroup();
		//warn("setting lastTowerControlGroup to control group of start snapped entity " + cmd.startSnappedEntity + ": " + lastTowerControlGroup);
	}

	var i = 0;
	var queued = cmd.queued;
	var pieces = clone(cmd.pieces);
	for (; i < pieces.length; ++i)
	{
		var piece = pieces[i];

		// All wall pieces after the first must be queued.
		if (i > 0 && !queued)
			queued = true;

		// 'lastTowerControlGroup' must always be defined and valid here, except if we're at the first piece and we didn't do
		// start position snapping (implying that the first entity we build must be a tower)
		if (lastTowerControlGroup === null || lastTowerControlGroup == INVALID_ENTITY)
		{
			if (!(i == 0 && piece.template == cmd.wallSet.templates.tower && !cmd.startSnappedEntity))
			{
    			error("[TryConstructWall] Expected last tower control group to be available, none found (1st pass, iteration " + i + ")");
    			break;
			}
		}

		var constructPieceCmd = {
			"type": "construct",
			"entities": cmd.entities,
			"template": piece.template,
			"x": piece.x,
			"z": piece.z,
			"angle": piece.angle,
			"autorepair": cmd.autorepair,
			"autocontinue": cmd.autocontinue,
			"queued": queued,
			// Regardless of whether we're building a tower or an intermediate wall piece, it is always (first) constructed
			// using the control group of the last tower (see comments above).
			"obstructionControlGroup": lastTowerControlGroup,
		};

		// If we're building the last piece and we're attaching to a snapped entity, we need to add in the snapped entity's
		// control group directly at construction time (instead of setting it in the second pass) to allow it to be built
		// while overlapping the snapped entity.
		if (i == pieces.length - 1 && cmd.endSnappedEntity)
		{
			var cmpEndSnappedObstruction = Engine.QueryInterface(cmd.endSnappedEntity, IID_Obstruction);
			if (cmpEndSnappedObstruction)
				constructPieceCmd.obstructionControlGroup2 = cmpEndSnappedObstruction.GetControlGroup();
		}

		var pieceEntityId = TryConstructBuilding(player, cmpPlayer, controlAllUnits, constructPieceCmd);
		if (pieceEntityId)
		{
			// wall piece foundation successfully built, save the entity ID in the piece info object so we can reference it later
			piece.ent = pieceEntityId;

			// if we built a tower, do the control group dance (see outline above) and update lastTowerControlGroup and lastTowerIndex
			if (piece.template == cmd.wallSet.templates.tower)
			{
				var cmpTowerObstruction = Engine.QueryInterface(pieceEntityId, IID_Obstruction);
				var newTowerControlGroup = pieceEntityId;

				if (i > 0)
				{
					//warn("   updating previous wall piece's secondary control group to " + newTowerControlGroup);
					var cmpPreviousObstruction = Engine.QueryInterface(pieces[i-1].ent, IID_Obstruction);
					// TODO: ensure that cmpPreviousObstruction exists
					// TODO: ensure that the previous obstruction does not yet have a secondary control group set
					cmpPreviousObstruction.SetControlGroup2(newTowerControlGroup);
				}

				// TODO: ensure that cmpTowerObstruction exists
				cmpTowerObstruction.SetControlGroup(newTowerControlGroup); // give the tower its own unique control group

				lastTowerIndex = i;
				lastTowerControlGroup = newTowerControlGroup;
			}
		}
		else // failed to build wall piece, abort
			break;
	}

	var lastBuiltPieceIndex = i - 1;
	var wallComplete = (lastBuiltPieceIndex == pieces.length - 1);

	// At this point, 'i' is the index of the last wall piece that was successfully constructed (which may or may not be a tower).
	// Now do the second pass going right-to-left, registering the control groups of the towers to the right of each piece (if any)
	// as their secondary control groups.

	lastTowerControlGroup = null; // control group of the last tower we've encountered, to assign to non-tower pieces

	// only start off with the ending side's snapped tower's control group if we were able to build the entire wall
	if (cmd.endSnappedEntity && wallComplete)
	{
		var cmpSnappedEndObstruction = Engine.QueryInterface(cmd.endSnappedEntity, IID_Obstruction);
		if (!cmpSnappedEndObstruction)
		{
			error("[TryConstructWall] Snapped entity on ending side does not have an obstruction component");
			return;
		}

		lastTowerControlGroup = cmpSnappedEndObstruction.GetControlGroup();
	}

	for (var j = lastBuiltPieceIndex; j >= 0; --j)
	{
		var piece = pieces[j];

		if (!piece.ent)
		{
			error("[TryConstructWall] No entity ID set for constructed entity of template '" + piece.template + "'");
			continue;
		}

		var cmpPieceObstruction = Engine.QueryInterface(piece.ent, IID_Obstruction);
		if (!cmpPieceObstruction)
		{
			error("[TryConstructWall] Wall piece of template '" + piece.template + "' has no Obstruction component");
			continue;
		}

		if (piece.template == cmd.wallSet.templates.tower)
		{
			// encountered a tower entity, update the last tower control group
			lastTowerControlGroup = cmpPieceObstruction.GetControlGroup();
		}
		else
		{
			// Encountered a non-tower entity, update its secondary control group to 'lastTowerControlGroup'.
			// Note that the wall piece may already have its secondary control group set to the tower's entity ID from a control group
			// dance during the first pass, in which case we should validate it against 'lastTowerControlGroup'.

			var existingSecondaryControlGroup = cmpPieceObstruction.GetControlGroup2();
			if (existingSecondaryControlGroup == INVALID_ENTITY)
			{
				if (lastTowerControlGroup != null && lastTowerControlGroup != INVALID_ENTITY)
				{
					cmpPieceObstruction.SetControlGroup2(lastTowerControlGroup);
				}
			}
			else if (existingSecondaryControlGroup != lastTowerControlGroup)
			{
				error("[TryConstructWall] Existing secondary control group of non-tower entity does not match expected value (2nd pass, iteration " + j + ")");
				break;
			}
		}
	}
}

/**
 * Remove the given list of entities from their current formations.
 */
function RemoveFromFormation(ents)
{
	let formation = ExtractFormations(ents);
	for (let fid in formation.members)
	{
		let cmpFormation = Engine.QueryInterface(+fid, IID_Formation);
		if (cmpFormation)
			cmpFormation.RemoveMembers(formation.members[fid]);
	}
}

/**
 * Returns a list of UnitAI components, each belonging either to a
 * selected unit or to a formation entity for groups of the selected units.
 */
function GetFormationUnitAIs(ents, player, cmd, formationTemplate, forceTemplate)
{
	// If an individual was selected, remove it from any formation
	// and command it individually.
	if (ents.length == 1)
	{
		let cmpUnitAI = Engine.QueryInterface(ents[0], IID_UnitAI);
		if (!cmpUnitAI)
			return [];

		RemoveFromFormation(ents);

		return [ cmpUnitAI ];
	}

	let formationUnitAIs = [];
	// Find what formations the selected entities are currently in,
	// and default to that unless the formation is forced or it's the null formation
	// (we want that to reset whatever formations units are in).
	if (formationTemplate != NULL_FORMATION)
	{
		let formation = ExtractFormations(ents);
		let formationIds = Object.keys(formation.members);
		if (formationIds.length == 1)
		{
			// Selected units either belong to this formation or have no formation.
			let fid = formationIds[0];
			let cmpFormation = Engine.QueryInterface(+fid, IID_Formation);
			if (cmpFormation && cmpFormation.GetMemberCount() == formation.members[fid].length &&
			    cmpFormation.GetMemberCount() == formation.entities.length)
			{
				cmpFormation.DeleteTwinFormations();

				// The whole formation was selected, so reuse its controller for this command.
				if (!forceTemplate || formationTemplate == formation.templates[fid])
				{
					formationTemplate = formation.templates[fid];
					formationUnitAIs = [Engine.QueryInterface(+fid, IID_UnitAI)];
				}
				else if (formationTemplate && CanMoveEntsIntoFormation(formation.entities, formationTemplate))
					formationUnitAIs = [cmpFormation.LoadFormation(formationTemplate)];
			}
			else if (cmpFormation && !forceTemplate)
			{
				// Just reuse the template.
				formationTemplate = formation.templates[fid];
			}
		}
		else if (formationIds.length)
		{
			// Check if all entities share a common formation, if so reuse this template.
			let template = formation.templates[formationIds[0]];
			for (let i = 1; i < formationIds.length; ++i)
				if (formation.templates[formationIds[i]] != template)
				{
					template = null;
					break;
				}
			if (template && !forceTemplate)
				formationTemplate = template;
		}
	}

	// Separate out the units that don't support the chosen formation.
	let formedUnits = [];
	let nonformedUnitAIs = [];
	for (let ent of ents)
	{
		let cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		let cmpPosition = Engine.QueryInterface(ent, IID_Position);
		if (!cmpUnitAI || !cmpPosition || !cmpPosition.IsInWorld())
			continue;

		// TODO: We only check if the formation is usable by some units
		// if we move them to it. We should check if we can use formations
		// for the other cases.
		let nullFormation = (formationTemplate || cmpUnitAI.GetFormationTemplate()) == NULL_FORMATION;
		if (nullFormation || !cmpUnitAI.CanUseFormation(formationTemplate || NULL_FORMATION))
		{
			if (nullFormation && cmpUnitAI.GetFormationController())
				cmpUnitAI.LeaveFormation(cmd.queued || false);
			nonformedUnitAIs.push(cmpUnitAI);
		}
		else
			formedUnits.push(ent);
	}
	if (nonformedUnitAIs.length == ents.length)
	{
		// No units support the formation.
		return nonformedUnitAIs;
	}

	if (!formationUnitAIs.length)
	{
		// We need to give the selected units a new formation controller.

		// TODO replace the fixed 60 with something sensible, based on vision range f.e.
		let formationSeparation = 60;
		let clusters = ClusterEntities(formedUnits, formationSeparation);
		let formationEnts = [];
		for (let cluster of clusters)
		{
			RemoveFromFormation(cluster);

			if (!formationTemplate || !CanMoveEntsIntoFormation(cluster, formationTemplate))
			{
				for (let ent of cluster)
					nonformedUnitAIs.push(Engine.QueryInterface(ent, IID_UnitAI));

				continue;
			}

			// Create the new controller.
			let formationEnt = Engine.AddEntity(formationTemplate);
			let cmpFormation = Engine.QueryInterface(formationEnt, IID_Formation);
			formationUnitAIs.push(Engine.QueryInterface(formationEnt, IID_UnitAI));
			cmpFormation.SetFormationSeparation(formationSeparation);
			cmpFormation.SetMembers(cluster);

			for (let ent of formationEnts)
				cmpFormation.RegisterTwinFormation(ent);

			formationEnts.push(formationEnt);
			let cmpOwnership = Engine.QueryInterface(formationEnt, IID_Ownership);
			cmpOwnership.SetOwner(player);
		}
	}

	return nonformedUnitAIs.concat(formationUnitAIs);
}

/**
 * Group a list of entities in clusters via single-links
 */
function ClusterEntities(ents, separationDistance)
{
	let clusters = [];
	if (!ents.length)
		return clusters;

	let distSq = separationDistance * separationDistance;
	let positions = [];
	// triangular matrix with the (squared) distances between the different clusters
	// the other half is not initialised
	let matrix = [];
	for (let i = 0; i < ents.length; ++i)
	{
		matrix[i] = [];
		clusters.push([ents[i]]);
		let cmpPosition = Engine.QueryInterface(ents[i], IID_Position);
		positions.push(cmpPosition.GetPosition2D());
		for (let j = 0; j < i; ++j)
			matrix[i][j] = positions[i].distanceToSquared(positions[j]);
	}
	while (clusters.length > 1)
	{
		// search two clusters that are closer than the required distance
		let closeClusters = undefined;

		for (let i = matrix.length - 1; i >= 0 && !closeClusters; --i)
			for (let j = i - 1; j >= 0 && !closeClusters; --j)
				if (matrix[i][j] < distSq)
					closeClusters = [i,j];

		// if no more close clusters found, just return all found clusters so far
		if (!closeClusters)
			return clusters;

		// make a new cluster with the entities from the two found clusters
		let newCluster = clusters[closeClusters[0]].concat(clusters[closeClusters[1]]);

		// calculate the minimum distance between the new cluster and all other remaining
		// clusters by taking the minimum of the two distances.
		let distances = [];
		for (let i = 0; i < clusters.length; ++i)
		{
			let a = closeClusters[1];
			let b = closeClusters[0];
			if (i == a || i == b)
				continue;
			let dist1 = matrix[a][i] !== undefined ? matrix[a][i] : matrix[i][a];
			let dist2 = matrix[b][i] !== undefined ? matrix[b][i] : matrix[i][b];
			distances.push(Math.min(dist1, dist2));
		}
		// remove the rows and columns in the matrix for the merged clusters,
		// and the clusters themselves from the cluster list
		clusters.splice(closeClusters[0],1);
		clusters.splice(closeClusters[1],1);
		matrix.splice(closeClusters[0],1);
		matrix.splice(closeClusters[1],1);
		for (let i = 0; i < matrix.length; ++i)
		{
			if (matrix[i].length > closeClusters[0])
				matrix[i].splice(closeClusters[0],1);
			if (matrix[i].length > closeClusters[1])
				matrix[i].splice(closeClusters[1],1);
		}
		// add a new row of distances to the matrix and the new cluster
		clusters.push(newCluster);
		matrix.push(distances);
	}
	return clusters;
}

function GetFormationRequirements(formationTemplate)
{
	var template = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager).GetTemplate(formationTemplate);
	if (!template.Formation)
		return false;

	return { "minCount": +template.Formation.RequiredMemberCount };
}


function CanMoveEntsIntoFormation(ents, formationTemplate)
{
	// TODO: should check the player's civ is allowed to use this formation
	// See simulation/components/Player.js GetFormations() for a list of all allowed formations

	const requirements = GetFormationRequirements(formationTemplate);
	if (!requirements)
		return false;

	let count = 0;
	for (const ent of ents)
		if (Engine.QueryInterface(ent, IID_UnitAI)?.CanUseFormation(formationTemplate))
			++count;

	return count >= requirements.minCount;
}

/**
 * Check if player can control this entity
 * returns: true if the entity is owned by the player and controllable
 *          or control all units is activated, else false
 */
function CanControlUnit(entity, player, controlAll)
{
	let cmpIdentity = Engine.QueryInterface(entity, IID_Identity);
	let canBeControlled = IsOwnedByPlayer(player, entity) &&
		(!cmpIdentity || cmpIdentity.IsControllable()) ||
		controlAll;

	if (!canBeControlled)
		notifyOrderFailure(entity, player);

	return canBeControlled;
}

/**
 * @param {number} entity - The entityID to verify.
 * @param {number} player - The playerID to check against.
 * @return {boolean}.
 */
function IsOwnedByPlayerOrMutualAlly(entity, player)
{
	return IsOwnedByPlayer(player, entity) || IsOwnedByMutualAllyOfPlayer(player, entity);
}

/**
 * Check if player can control this entity
 * @return {boolean} - True if the entity is valid and controlled by the player
 *          or the entity is owned by an mutualAlly and can be controlled
 *          or control all units is activated, else false.
 */
function CanPlayerOrAllyControlUnit(entity, player, controlAll)
{
	return CanControlUnit(player, entity, controlAll) ||
		IsOwnedByMutualAllyOfPlayer(player, entity) && CanOwnerControlEntity(entity);
}

/**
 * @return {boolean} - Whether the owner of this entity can control the entity.
 */
function CanOwnerControlEntity(entity)
{
	let cmpOwner = QueryOwnerInterface(entity);
	return cmpOwner && CanControlUnit(entity, cmpOwner.GetPlayerID());
}

/**
 * Filter entities which the player can control.
 */
function FilterEntityList(entities, player, controlAll)
{
	return entities.filter(ent => CanControlUnit(ent, player, controlAll));
}

/**
 * Filter entities which the player can control or are mutualAlly
 */
function FilterEntityListWithAllies(entities, player, controlAll)
{
	return entities.filter(ent => CanPlayerOrAllyControlUnit(ent, player, controlAll));
}

/**
 * Incur the player with the cost of a bribe, optionally multiply the cost with
 * the additionalMultiplier
 */
function IncurBribeCost(template, player, playerBribed, failedBribe)
{
	let cmpPlayerBribed = QueryPlayerIDInterface(playerBribed);
	if (!cmpPlayerBribed)
		return false;

	let costs = {};
	// Additional cost for this owner
	let multiplier = cmpPlayerBribed.GetSpyCostMultiplier();
	if (failedBribe)
		multiplier *= template.VisionSharing.FailureCostRatio;

	for (let res in template.Cost.Resources)
		costs[res] = Math.floor(multiplier * ApplyValueModificationsToTemplate("Cost/Resources/" + res, +template.Cost.Resources[res], player, template));

	let cmpPlayer = QueryPlayerIDInterface(player);
	return cmpPlayer && cmpPlayer.TrySubtractResources(costs);
}

Engine.RegisterGlobal("GetFormationRequirements", GetFormationRequirements);
Engine.RegisterGlobal("CanMoveEntsIntoFormation", CanMoveEntsIntoFormation);
Engine.RegisterGlobal("GetDockAngle", GetDockAngle);
Engine.RegisterGlobal("ProcessCommand", ProcessCommand);
Engine.RegisterGlobal("g_Commands", g_Commands);
Engine.RegisterGlobal("IncurBribeCost", IncurBribeCost);
