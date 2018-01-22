/**
 * Specifies which template should indicate the target location of a player command,
 * given a command type.
 */
var g_TargetMarker = {
	"move": "special/target_marker"
};

/**
 * Which enemy entity types will be attacked on sight when patroling.
 */
var g_PatrolTargets = ["Unit"];

/**
 * List of different actions units can execute,
 * this is mostly used to determine which actions can be executed
 *
 * "execute" is meant to send the command to the engine
 *
 * The next functions will always return false
 * in case you have to continue to seek
 * (i.e. look at the next entity for getActionInfo, the next
 * possible action for the actionCheck ...)
 * They will return an object when the searching is finished
 *
 * "getActionInfo" is used to determine if the action is possible,
 * and also give visual feedback to the user (tooltips, cursors, ...)
 *
 * "preSelectedActionCheck" is used to select actions when the gui buttons
 * were used to set them, but still require a target (like the guard button)
 *
 * "hotkeyActionCheck" is used to check the possibility of actions when
 * a hotkey is pressed
 *
 * "actionCheck" is used to check the possibilty of actions without specific
 * command. For that, the specificness variable is used
 *
 * "specificness" is used to determine how specific an action is,
 * The lower the number, the more specific an action is, and the bigger
 * the chance of selecting that action when multiple actions are possible
 */
var g_UnitActions =
{
	"move":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({
				"type": "walk",
				"entities": selection,
				"x": target.x,
				"z": target.z,
				"queued": queued
			});

			DrawTargetMarker(target);

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_walk",
				"entity": selection[0]
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			return { "possible": true };
		},
		"hotkeyActionCheck": function(target, selection)
		{
			if (!someUnitAI(selection) ||
			    !Engine.HotkeyIsPressed("session.move") ||
			    !getActionInfo("move", target, selection).possible)
				return false;

			return { "type": "move" };
		},
		"actionCheck": function(target, selection)
		{
			if (!someUnitAI(selection) || !getActionInfo("move", target, selection).possible)
				return false;

			return { "type": "move" };
		},
		"specificness": 12,
	},

	"attack-move":
	{
		"execute": function(target, action, selection, queued)
		{
			let targetClasses;
			if (Engine.HotkeyIsPressed("session.attackmoveUnit"))
				targetClasses = { "attack": ["Unit"] };
			else
				targetClasses = { "attack": ["Unit", "Structure"] };

			Engine.PostNetworkCommand({
				"type": "attack-walk",
				"entities": selection,
				"x": target.x,
				"z": target.z,
				"targetClasses": targetClasses,
				"queued": queued
			});

			DrawTargetMarker(target);

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_walk",
				"entity": selection[0]
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			return { "possible": true };
		},
		"hotkeyActionCheck": function(target, selection)
		{
			if (!someUnitAI(selection) ||
			    !Engine.HotkeyIsPressed("session.attackmove") ||
			    !getActionInfo("attack-move", target, selection).possible)
				return false;

			return {
				"type": "attack-move",
				"cursor": "action-attack-move"
			};
		},
		"specificness": 30,
	},

	"capture":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({
				"type": "attack",
				"entities": selection,
				"target": action.target,
				"allowCapture": true,
				"queued": queued
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_attack",
				"entity": selection[0]
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.attack || !targetState.hitpoints)
				return false;

			return {
				"possible": Engine.GuiInterfaceCall("CanAttack", {
					"entity": entState.id,
					"target": targetState.id,
					"types": ["Capture"]
				})
			};
		},
		"actionCheck": function(target, selection)
		{
			if (!getActionInfo("capture", target, selection).possible)
				return false;

			return {
				"type": "capture",
				"cursor": "action-capture",
				"target": target
			};
		},
		"specificness": 9,
	},

	"attack":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({
				"type": "attack",
				"entities": selection,
				"target": action.target,
				"queued": queued,
				"allowCapture": false
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_attack",
				"entity": selection[0]
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.attack || !targetState.hitpoints)
				return false;

			return {
				"possible": Engine.GuiInterfaceCall("CanAttack", {
					"entity": entState.id,
					"target": targetState.id,
					"types": ["!Capture"]
				})
			};
		},
		"hotkeyActionCheck": function(target, selection)
		{
			if (!Engine.HotkeyIsPressed("session.attack") ||
			    !getActionInfo("attack", target, selection).possible)
				return false;

			return {
				"type": "attack",
				"cursor": "action-attack",
				"target": target
			};
		},
		"actionCheck": function(target, selection)
		{
			if (!getActionInfo("attack", target, selection).possible)
				return false;

			return {
				"type": "attack",
				"cursor": "action-attack",
				"target": target
			};
		},
		"specificness": 10,
	},

	"patrol":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({
				"type": "patrol",
				"entities": selection,
				"x": target.x,
				"z": target.z,
				"target": action.target,
				"targetClasses": { "attack": g_PatrolTargets },
				"queued": queued,
				"allowCapture": false
			});

			DrawTargetMarker(target);

			Engine.GuiInterfaceCall("PlaySound", { "name": "order_patrol", "entity": selection[0] });
			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.unitAI || !entState.unitAI.canPatrol)
				return false;

			return { "possible": true };
		},
		"hotkeyActionCheck": function(target, selection)
		{
			if (!someCanPatrol(selection) ||
			    !Engine.HotkeyIsPressed("session.patrol") ||
			    !getActionInfo("patrol", target, selection).possible)
				return false;
			return {
				"type": "patrol",
				"cursor": "action-patrol",
				"target": target
			};
		},
		"preSelectedActionCheck": function(target, selection)
		{
			if (preSelectedAction != ACTION_PATROL || !getActionInfo("patrol", target, selection).possible)
				return false;
			return {
				"type": "patrol",
				"cursor": "action-patrol",
				"target": target
			};
		},
		"specificness": 37,
	},

	"heal":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({
				"type": "heal",
				"entities": selection,
				"target": action.target,
				"queued": queued
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_heal",
				"entity": selection[0]
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.heal ||
			    !hasClass(targetState, "Unit") || !targetState.needsHeal ||
			    !playerCheck(entState, targetState, ["Player", "Ally"]) ||
			    entState.id == targetState.id) // Healers can't heal themselves.
				return false;

			let unhealableClasses = entState.heal.unhealableClasses;
			if (MatchesClassList(targetState.identity.classes, unhealableClasses))
				return false;

			let healableClasses = entState.heal.healableClasses;
			if (!MatchesClassList(targetState.identity.classes, healableClasses))
				return false;

			return { "possible": true };
		},
		"actionCheck": function(target, selection)
		{
			if (!getActionInfo("heal", target, selection).possible)
				return false;

			return {
				"type": "heal",
				"cursor": "action-heal",
				"target": target
			};
		},
		"specificness": 7,
	},

	"repair":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({
				"type": "repair",
				"entities": selection,
				"target": action.target,
				"autocontinue": true,
				"queued": queued
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_repair",
				"entity": selection[0]
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.builder ||
			    !targetState.needsRepair && !targetState.foundation ||
			    !playerCheck(entState, targetState, ["Player", "Ally"]))
				return false;

			return { "possible": true };
		},
		"preSelectedActionCheck": function(target, selection)
		{
			if (preSelectedAction != ACTION_REPAIR)
				return false;

			if (getActionInfo("repair", target, selection).possible)
				return {
					"type": "repair",
					"cursor": "action-repair",
					"target": target
				};

			return {
				"type": "none",
				"cursor": "action-repair-disabled",
				"target": null
			};
		},
		"hotkeyActionCheck": function(target, selection)
		{
			if (!Engine.HotkeyIsPressed("session.repair") ||
			    !getActionInfo("repair", target, selection).possible)
				return false;

			return {
				"type": "repair",
				"cursor": "action-repair",
				"target": target
			};
		},
		"actionCheck": function(target, selection)
		{
			if (!getActionInfo("repair", target, selection).possible)
				return false;

			return {
				"type": "repair",
				"cursor": "action-repair",
				"target": target
			};
		},
		"specificness": 11,
	},

	"gather":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({
				"type": "gather",
				"entities": selection,
				"target": action.target,
				"queued": queued
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_gather",
				"entity": selection[0]
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!targetState.resourceSupply)
				return false;

			let resource = findGatherType(entState, targetState.resourceSupply);
			if (!resource)
				return false;

			return {
				"possible": true,
				"cursor": "action-gather-" + resource
			};
		},
		"actionCheck": function(target, selection)
		{
			let actionInfo = getActionInfo("gather", target, selection);

			if (!actionInfo.possible)
				return false;

			return {
				"type": "gather",
				"cursor": actionInfo.cursor,
				"target": target
			};
		},
		"specificness": 1,
	},

	"returnresource":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({
				"type": "returnresource",
				"entities": selection,
				"target": action.target,
				"queued": queued
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_gather",
				"entity": selection[0]
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!targetState.resourceDropsite)
				return false;

			let playerState = GetSimState().players[entState.player];
			if (playerState.hasSharedDropsites && targetState.resourceDropsite.shared)
			{
				if (!playerCheck(entState, targetState, ["Player", "MutualAlly"]))
					return false;
			}
			else if (!playerCheck(entState, targetState, ["Player"]))
				return false;

			if (!entState.resourceCarrying || !entState.resourceCarrying.length)
				return false;

			let carriedType = entState.resourceCarrying[0].type;
			if (targetState.resourceDropsite.types.indexOf(carriedType) == -1)
				return false;

			return {
				"possible": true,
				"cursor": "action-return-" + carriedType
			};
		},
		"actionCheck": function(target, selection)
		{
			let actionInfo = getActionInfo("returnresource", target, selection);
			if (!actionInfo.possible)
				return false;

			return {
				"type": "returnresource",
				"cursor": actionInfo.cursor,
				"target": target
			};
		},
		"specificness": 2,
	},

	"setup-trade-route":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({
				"type": "setup-trade-route",
				"entities": selection,
				"target": action.target,
				"source": null,
				"route": null,
				"queued": queued
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_trade",
				"entity": selection[0]
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (targetState.foundation || !entState.trader || !targetState.market ||
			    playerCheck(entState, targetState, ["Enemy"]) ||
			    !(targetState.market.land && hasClass(entState, "Organic") ||
			      targetState.market.naval && hasClass(entState, "Ship")))
				return false;

			let tradingDetails = Engine.GuiInterfaceCall("GetTradingDetails", {
				"trader": entState.id,
				"target": targetState.id
			});

			if (!tradingDetails)
				return false;

			let tooltip;
			switch (tradingDetails.type)
			{
			case "is first":
				tooltip = translate("Origin trade market.") + "\n";
				if (tradingDetails.hasBothMarkets)
					tooltip += sprintf(translate("Gain: %(gain)s"), {
						"gain": getTradingTooltip(tradingDetails.gain)
					});
				else
					tooltip += translate("Right-click on another market to set it as a destination trade market.");
				break;

			case "is second":
				tooltip = translate("Destination trade market.") + "\n" +
					sprintf(translate("Gain: %(gain)s"), {
						"gain": getTradingTooltip(tradingDetails.gain)
					});
				break;

			case "set first":
				tooltip = translate("Right-click to set as origin trade market");
				break;

			case "set second":
				if (tradingDetails.gain.traderGain == 0) // markets too close
					return false;

				tooltip = translate("Right-click to set as destination trade market.") + "\n" +
					sprintf(translate("Gain: %(gain)s"), {
						"gain": getTradingTooltip(tradingDetails.gain)
					});

				break;
			}

			return {
				"possible": true,
				"tooltip": tooltip
			};
		},
		"actionCheck": function(target, selection)
		{
			let actionInfo = getActionInfo("setup-trade-route", target, selection);
			if (!actionInfo.possible)
				return false;

			return {
				"type": "setup-trade-route",
				"cursor": "action-setup-trade-route",
				"tooltip": actionInfo.tooltip,
				"target": target
			};
		},
		"specificness": 0,
	},

	"garrison":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({
				"type": "garrison",
				"entities": selection,
				"target": action.target,
				"queued": queued
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_garrison",
				"entity": selection[0]
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.canGarrison || !targetState.garrisonHolder ||
			    !playerCheck(entState, targetState, ["Player", "MutualAlly"]))
				return false;

			let tooltip = sprintf(translate("Current garrison: %(garrisoned)s/%(capacity)s"), {
				"garrisoned": targetState.garrisonHolder.garrisonedEntitiesCount,
				"capacity": targetState.garrisonHolder.capacity
			});

			let extraCount = 0;
			if (entState.garrisonHolder)
				extraCount += entState.garrisonHolder.garrisonedEntitiesCount;

			if (targetState.garrisonHolder.garrisonedEntitiesCount + extraCount >= targetState.garrisonHolder.capacity)
				tooltip = coloredText(tooltip, "orange");

			if (!MatchesClassList(entState.identity.classes, targetState.garrisonHolder.allowedClasses))
				return false;

			return {
				"possible": true,
				"tooltip": tooltip
			};
		},
		"preSelectedActionCheck": function(target, selection)
		{
			if (preSelectedAction != ACTION_GARRISON)
				return false;

			let actionInfo = getActionInfo("garrison", target, selection);
			if (!actionInfo.possible)
				return {
					"type": "none",
					"cursor": "action-garrison-disabled",
					"target": null
				};

			return {
				"type": "garrison",
				"cursor": "action-garrison",
				"tooltip": actionInfo.tooltip,
				"target": target
			};
		},
		"hotkeyActionCheck": function(target, selection)
		{
			let actionInfo = getActionInfo("garrison", target, selection);

			if (!Engine.HotkeyIsPressed("session.garrison") || !actionInfo.possible)
				return false;

			return {
				"type": "garrison",
				"cursor": "action-garrison",
				"tooltip": actionInfo.tooltip,
				"target": target
			};
		},
		"specificness": 20,
	},

	"guard":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({
				"type": "guard",
				"entities": selection,
				"target": action.target,
				"queued": queued
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_guard",
				"entity": selection[0]
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!targetState.guard ||
			    !playerCheck(entState, targetState, ["Player", "Ally"]) ||
			    !entState.unitAI || !entState.unitAI.canGuard ||
			    targetState.unitAI && targetState.unitAI.isGuarding)
				return false;

			return { "possible": true };
		},
		"preSelectedActionCheck": function(target, selection)
		{
			if (preSelectedAction != ACTION_GUARD)
				return false;

			if (getActionInfo("guard", target, selection).possible)
				return {
					"type": "guard",
					"cursor": "action-guard",
					"target": target
				};

			return {
				"type": "none",
				"cursor": "action-guard-disabled",
				"target": null
			};
		},
		"hotkeyActionCheck": function(target, selection)
		{
			if (!Engine.HotkeyIsPressed("session.guard") ||
			    !getActionInfo("guard", target, selection).possible)
				return false;

			return {
				"type": "guard",
				"cursor": "action-guard",
				"target": target
			};
		},
		"specificness": 40,
	},

	"remove-guard":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({
				"type": "remove-guard",
				"entities": selection,
				"target": action.target,
				"queued": queued
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_guard",
				"entity": selection[0]
			});

			return true;
		},
		"hotkeyActionCheck": function(target, selection)
		{
			if (!Engine.HotkeyIsPressed("session.guard") ||
			    !getActionInfo("remove-guard", target, selection).possible ||
			    !someGuarding(selection))
				return false;

			return {
				"type": "remove-guard",
				"cursor": "action-remove-guard"
			};
		},

		"specificness": 41,
	},

	"set-rallypoint":
	{
		"execute": function(target, action, selection, queued)
		{
			// if there is a position set in the action then use this so that when setting a
			// rally point on an entity it is centered on that entity
			if (action.position)
				target = action.position;

			Engine.PostNetworkCommand({
				"type": "set-rallypoint",
				"entities": selection,
				"x": target.x,
				"z": target.z,
				"data": action.data,
				"queued": queued
			});

			// Display rally point at the new coordinates, to avoid display lag
			Engine.GuiInterfaceCall("DisplayRallyPoint", {
				"entities": selection,
				"x": target.x,
				"z": target.z,
				"queued": queued
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			let tooltip;
			// default to walking there (or attack-walking if hotkey pressed)
			let data = { "command": "walk" };
			let cursor = "";

			if (Engine.HotkeyIsPressed("session.attackmove"))
			{
				let targetClasses;
				if (Engine.HotkeyIsPressed("session.attackmoveUnit"))
					targetClasses = { "attack": ["Unit"] };
				else
					targetClasses = { "attack": ["Unit", "Structure"] };

				data.command = "attack-walk";
				data.targetClasses = targetClasses;
				cursor = "action-attack-move";
			}

			if (Engine.HotkeyIsPressed("session.repair") &&
			    (targetState.needsRepair || targetState.foundation) &&
			    playerCheck(entState, targetState, ["Player", "Ally"]))
			{
				data.command = "repair";
				data.target = targetState.id;
				cursor = "action-repair";
			}
			else if (targetState.garrisonHolder &&
			    playerCheck(entState, targetState, ["Player", "MutualAlly"]))
			{
				data.command = "garrison";
				data.target = targetState.id;
				cursor = "action-garrison";

				tooltip = sprintf(translate("Current garrison: %(garrisoned)s/%(capacity)s"), {
					"garrisoned": targetState.garrisonHolder.garrisonedEntitiesCount,
					"capacity": targetState.garrisonHolder.capacity
				});

				if (targetState.garrisonHolder.garrisonedEntitiesCount >=
				    targetState.garrisonHolder.capacity)
					tooltip = coloredText(tooltip, "orange");
			}
			else if (targetState.resourceSupply)
			{
				let resourceType = targetState.resourceSupply.type;
				if (resourceType.generic == "treasure")
					cursor = "action-gather-" + resourceType.generic;
				else
					cursor = "action-gather-" + resourceType.specific;

				data.command = "gather-near-position";
				data.resourceType = resourceType;
				data.resourceTemplate = targetState.template;
				if (!targetState.speed)
				{
					data.command = "gather";
					data.target = targetState.id;
				}
			}
			else if (entState.market && targetState.market &&
			         entState.id != targetState.id &&
			         (!entState.market.naval || targetState.market.naval) &&
			         !playerCheck(entState, targetState, ["Enemy"]))
			{
				// Find a trader (if any) that this building can produce.
				let trader;
				if (entState.production && entState.production.entities.length)
					for (let i = 0; i < entState.production.entities.length; ++i)
						if ((trader = GetTemplateData(entState.production.entities[i]).trader))
							break;

				let traderData = {
					"firstMarket": entState.id,
					"secondMarket": targetState.id,
					"template": trader
				};

				let gain = Engine.GuiInterfaceCall("GetTradingRouteGain", traderData);
				if (gain && gain.traderGain)
				{
					data.command = "trade";
					data.target = traderData.secondMarket;
					data.source = traderData.firstMarket;
					cursor = "action-setup-trade-route";

					tooltip = translate("Right-click to establish a default route for new traders.") + "\n" +
						sprintf(
							trader ?
								translate("Gain: %(gain)s") :
								translate("Expected gain: %(gain)s"),
							{ "gain": getTradingTooltip(gain) });
				}
			}
			else if ((targetState.needsRepair || targetState.foundation) && playerCheck(entState, targetState, ["Ally"]))
			{
				data.command = "repair";
				data.target = targetState.id;
				cursor = "action-repair";
			}
			else if (playerCheck(entState, targetState, ["Enemy"]))
			{
				data.target = targetState.id;
				data.command = "attack";
				cursor = "action-attack";
			}

			// Don't allow the rally point to be set on any of the currently selected entities (used for unset)
			// except if the autorallypoint hotkey is pressed and the target can produce entities
			if (!Engine.HotkeyIsPressed("session.autorallypoint") ||
			    !targetState.production ||
			    !targetState.production.entities.length)
				for (let ent in g_Selection.selected)
					if (targetState.id == +ent)
						return false;

			return {
				"possible": true,
				"data": data,
				"position": targetState.position,
				"cursor": cursor,
				"tooltip": tooltip
			};

		},
		"actionCheck": function(target, selection)
		{
			if (someUnitAI(selection) || !someRallyPoints(selection))
				return false;

			let actionInfo = getActionInfo("set-rallypoint", target, selection);
			if (!actionInfo.possible)
				return false;

			return {
				"type": "set-rallypoint",
				"cursor": actionInfo.cursor,
				"data": actionInfo.data,
				"tooltip": actionInfo.tooltip,
				"position": actionInfo.position
			};
		},
		"specificness": 6,
	},

	"unset-rallypoint":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({
				"type": "unset-rallypoint",
				"entities": selection
			});

			// Remove displayed rally point
			Engine.GuiInterfaceCall("DisplayRallyPoint", {
				"entities": []
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (entState.id != targetState.id ||
			    !entState.rallyPoint || !entState.rallyPoint.position)
				return false;

			return { "possible": true };
		},
		"actionCheck": function(target, selection)
		{
			if (someUnitAI(selection) || !someRallyPoints(selection) ||
			    !getActionInfo("unset-rallypoint", target, selection).possible)
				return false;

			return {
				"type": "unset-rallypoint",
				"cursor": "action-unset-rally"
			};
		},
		"specificness": 11,
	},

	"none":
	{
		"execute": function(target, action, selection, queued)
		{
			return true;
		},
		"specificness": 100,
	},
};

/**
 * Info and actions for the entity commands
 * Currently displayed in the bottom of the central panel
 */
var g_EntityCommands =
{
	"unload-all": {
		"getInfo": function(entStates)
		{
			let count = 0;
			for (let entState of entStates)
				if (entState.garrisonHolder)
					count += entState.garrisonHolder.entities.length;

			if (!count)
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "session.unload") +
				           translate("Unload All."),
				"icon": "garrison-out.png",
				"count": count,
			};
		},
		"execute": function()
		{
			unloadAll();
		},
	},

	"delete": {
		"getInfo": function(entStates)
		{
			return entStates.some(entState => !isUndeletable(entState)) ?
				{
					"tooltip":
						colorizeHotkey("%(hotkey)s" + " ", "session.kill") +
						translate("Destroy the selected units or buildings.") + "\n" +
						colorizeHotkey(
							translate("Use %(hotkey)s to avoid the confirmation dialog."),
							"session.noconfirmation"
						),
					"icon": "kill_small.png"
				} :
				{
					// Get all delete reasons and remove duplications
					"tooltip": entStates.map(entState => isUndeletable(entState))
						.filter((reason, pos, self) =>
							self.indexOf(reason) == pos && reason
						).join("\n"),
					"icon": "kill_small_disabled.png"
				};
		},
		"execute": function(entStates)
		{
			if (!entStates.length || entStates.every(entState => isUndeletable(entState)))
				return;

			if (Engine.HotkeyIsPressed("session.noconfirmation"))
				Engine.PostNetworkCommand({
					"type": "delete-entities",
					"entities": entStates.map(entState => entState.id)
				});
			else
				openDeleteDialog(entStates.map(entState => entState.id));
		},
	},

	"stop": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState => !entState.unitAI))
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "session.stop") +
				           translate("Abort the current order."),
				"icon": "stop.png"
			};
		},
		"execute": function(entStates)
		{
			if (entStates.length)
				stopUnits(entStates.map(entState => entState.id));
		},
	},

	"garrison": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState => !entState.unitAI || entState.turretParent || false))
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "session.garrison") +
				           translate("Order the selected units to garrison in a building or unit."),
				"icon": "garrison.png"
			};
		},
		"execute": function()
		{
			inputState = INPUT_PRESELECTEDACTION;
			preSelectedAction = ACTION_GARRISON;
		},
	},

	"unload": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState => {
				if (!entState.unitAI || !entState.turretParent)
					return true;
				let parent = GetEntityState(entState.turretParent);
				return !parent || !parent.garrisonHolder || parent.garrisonHolder.entities.indexOf(entState.id) == -1;
			}))
				return false;

			return {
				"tooltip": translate("Unload"),
				"icon": "garrison-out.png"
			};
		},
		"execute": function()
		{
			unloadSelection();
		},
	},

	"repair": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState => !entState.builder))
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "session.repair") +
				           translate("Order the selected units to repair a building or mechanical unit."),
				"icon": "repair.png"
			};
		},
		"execute": function()
		{
			inputState = INPUT_PRESELECTEDACTION;
			preSelectedAction = ACTION_REPAIR;
		},
	},

	"focus-rally": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState => !entState.rallyPoint))
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "camera.rallypointfocus") +
				           translate("Focus on Rally Point."),
				"icon": "focus-rally.png"
			};
		},
		"execute": function(entStates)
		{
			// TODO: Would be nicer to cycle between the rallypoints of multiple entities instead of just using the first
			let focusTarget;
			for (let entState of entStates)
				if (entState.rallyPoint && entState.rallyPoint.position)
				{
					focusTarget = entState.rallyPoint.position;
					break;
				}
			if (!focusTarget)
				for (let entState of entStates)
					if (entState.position)
					{
						focusTarget = entState.position;
						break;
					}

			if (focusTarget)
				Engine.CameraMoveTo(focusTarget.x, focusTarget.z);
		},
	},

	"back-to-work": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState => !entState.unitAI || !entState.unitAI.hasWorkOrders))
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "session.backtowork") +
				           translate("Back to Work"),
				"icon": "production.png"
			};
		},
		"execute": function()
		{
			backToWork();
		},
	},

	"add-guard": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState =>
				!entState.unitAI || !entState.unitAI.canGuard || entState.unitAI.isGuarding))
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "session.guard") +
				           translate("Order the selected units to guard a building or unit."),
				"icon": "add-guard.png"
			};
		},
		"execute": function()
		{
			inputState = INPUT_PRESELECTEDACTION;
			preSelectedAction = ACTION_GUARD;
		},
	},

	"remove-guard": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState => !entState.unitAI || !entState.unitAI.isGuarding))
				return false;

			return {
				"tooltip": translate("Remove guard"),
				"icon": "remove-guard.png"
			};
		},
		"execute": function()
		{
			removeGuard();
		},
	},

	"select-trading-goods": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState => !entState.market))
				return false;

			return {
				"tooltip": translate("Barter & Trade"),
				"icon": "economics.png"
			};
		},
		"execute": function()
		{
			toggleTrade();
		},
	},

	"patrol": {
		"getInfo": function(entStates)
		{
			if (!entStates.some(entState => entState.unitAI && entState.unitAI.canPatrol))
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "session.patrol") +
				           translate("Patrol") + "\n" +
				           translate("Attack all encountered enemy units while avoiding buildings."),
				"icon": "patrol.png"
			};
		},
		"execute": function()
		{
			inputState = INPUT_PRESELECTEDACTION;
			preSelectedAction = ACTION_PATROL;
		},
	},

	"share-dropsite": {
		"getInfo": function(entStates)
		{
			let sharableEntities = entStates.filter(
				entState => entState.resourceDropsite && entState.resourceDropsite.sharable);
			if (!sharableEntities.length)
				return false;

			// Returns if none of the entities belong to a player with a mutual ally
			if (entStates.every(entState => !GetSimState().players[entState.player].isMutualAlly.some(
				(isAlly, playerId) => isAlly && playerId != entState.player)))
				return false;

			return sharableEntities.some(entState => !entState.resourceDropsite.shared) ?
				{
					"tooltip": translate("Press to allow allies to use this dropsite"),
					"icon": "locked_small.png"
				} :
				{
					"tooltip": translate("Press to prevent allies from using this dropsite"),
					"icon": "unlocked_small.png"
				};
		},
		"execute": function(entStates)
		{
			let sharableEntities = entStates.filter(
				entState => entState.resourceDropsite && entState.resourceDropsite.sharable);
			Engine.PostNetworkCommand({
				"type": "set-dropsite-sharing",
				"entities": sharableEntities.map(entState => entState.id),
				"shared": sharableEntities.some(entState => !entState.resourceDropsite.shared)
			});
		},
	}
};

var g_AllyEntityCommands =
{
	"unload-all": {
		"getInfo": function(entState)
		{
			if (!entState.garrisonHolder)
				return false;

			let player = Engine.GetPlayerID();

			let count = 0;
			for (let ent in g_Selection.selected)
			{
				let selectedEntState = GetEntityState(+ent);
				if (!selectedEntState.garrisonHolder)
					continue;

				for (let entity of selectedEntState.garrisonHolder.entities)
				{
					let state = GetEntityState(entity);
					if (state.player == player)
						++count;
				}
			}

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "session.unload") +
				           translate("Unload All."),
				"icon": "garrison-out.png",
				"count": count,
			};
		},
		"execute": function(entState)
		{
			unloadAll();
		},
	},

	"share-dropsite": {
		"getInfo": function(entState)
		{
			if (Engine.GetPlayerID() == -1 ||
			    !GetSimState().players[Engine.GetPlayerID()].hasSharedDropsites ||
			    !entState.resourceDropsite || !entState.resourceDropsite.sharable)
				return false;

			if (entState.resourceDropsite.shared)
				return {
					"tooltip": translate("You are allowed to use this dropsite"),
					"icon": "unlocked_small.png"
				};

			return {
				"tooltip": translate("The use of this dropsite is prohibited"),
				"icon": "locked_small.png"
			};
		},
		"execute": function(entState)
		{
			// This command button is always disabled
		},
	}
};

function playerCheck(entState, targetState, validPlayers)
{
	let playerState = GetSimState().players[entState.player];
	for (let player of validPlayers)
		if (player == "Gaia" && targetState.player == 0 ||
		    player == "Player" && targetState.player == entState.player ||
		    playerState["is" + player] && playerState["is" + player][targetState.player])
			return true;

	return false;
}

function hasClass(entState, className)
{
	// note: use the functions in globalscripts/Templates.js for more versatile matching
	return entState.identity && entState.identity.classes.indexOf(className) != -1;
}

/**
 * Work out whether at least part of the selected entities have UnitAI.
 */
function someUnitAI(entities)
{
	return entities.some(ent => {
		let entState = GetEntityState(ent);
		return entState && entState.unitAI || false;
	});
}

function someRallyPoints(entities)
{
	return entities.some(ent => {
		let entState = GetEntityState(ent);
		return entState && entState.rallyPoint || false;
	});
}

function someGuarding(entities)
{
	return entities.some(ent => {
		let entState = GetEntityState(ent);
		return entState && entState.unitAI && entState.unitAI.isGuarding;
	});
}

function someCanPatrol(entities)
{
	return entities.some(ent => {
		let entState = GetEntityState(ent);
		return entState && entState.unitAI && entState.unitAI.canPatrol;
	});
}

/**
 * Keep in sync with Commands.js.
 */
function isUndeletable(entState)
{
	if (g_DevSettings.controlAll)
		return false;

	if (entState.resourceSupply && entState.resourceSupply.killBeforeGather)
		return translate("The entity has to be killed before it can be gathered from");

	if (entState.capturePoints && entState.capturePoints[entState.player] < entState.maxCapturePoints / 2)
		return translate("You cannot destroy this entity as you own less than half the capture points");

	if (!entState.identity.canDelete)
		return translate("This entity is undeletable");

	return false;
}

function DrawTargetMarker(target)
{
	Engine.GuiInterfaceCall("AddTargetMarker", {
		"template": g_TargetMarker.move,
		"x": target.x,
		"z": target.z
	});
}

function findGatherType(gatherer, supply)
{
	if (!gatherer.resourceGatherRates || !supply)
		return undefined;

	if (gatherer.resourceGatherRates[supply.type.generic + "." + supply.type.specific])
		return supply.type.specific;

	if (gatherer.resourceGatherRates[supply.type.generic])
		return supply.type.generic;

	return undefined;
}

function getActionInfo(action, target, selection)
{
	let simState = GetSimState();

	// If the selection doesn't exist, no action
	if (!GetEntityState(selection[0]))
		return { "possible": false };

	if (!target) // TODO move these non-target actions to an object like unit_actions.js
	{
		if (action == "set-rallypoint")
		{
			let cursor = "";
			let data = { "command": "walk" };
			if (Engine.HotkeyIsPressed("session.attackmove"))
			{
				data.command = "attack-walk";
				data.targetClasses = Engine.HotkeyIsPressed("session.attackmoveUnit") ?
					{ "attack": ["Unit"] } : { "attack": ["Unit", "Structure"] };
				cursor = "action-attack-move";
			}
			else if (Engine.HotkeyIsPressed("session.patrol"))
			{
				data.command = "patrol";
				data.targetClasses = { "attack": g_PatrolTargets };
				cursor = "action-patrol";
			}
			return {
				"possible": true,
				"data": data,
				"cursor": cursor
			};
		}

		return {
			"possible": ["move", "attack-move", "remove-guard", "patrol"].indexOf(action) != -1
		};
	}

	// Look at the first targeted entity
	// (TODO: maybe we eventually want to look at more, and be more context-sensitive?
	// e.g. prefer to attack an enemy unit, even if some friendly units are closer to the mouse)
	let targetState = GetEntityState(target);
	if (!targetState)
		return { "possible": false };

	// Check if any entities in the selection can do some of the available actions with target
	for (let entityID of selection)
	{
		let entState = GetEntityState(entityID);
		if (!entState)
			continue;

		if (g_UnitActions[action] && g_UnitActions[action].getActionInfo)
		{
			let r = g_UnitActions[action].getActionInfo(entState, targetState, simState);
			if (r && r.possible) // return true if it's possible for one of the entities
				return r;
		}
	}
	return { "possible": false };
}
