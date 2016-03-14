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

var unitActions = 
{
	"move":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({"type": "walk", "entities": selection, "x": target.x, "z": target.z, "queued": queued});
			Engine.GuiInterfaceCall("PlaySound", { "name": "order_walk", "entity": selection[0] });
			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			return {"possible": true};
		},
		"actionCheck": function(target, selection)
		{
			// Work out whether at least part of the selection have UnitAI
			var haveUnitAI = selection.some(function(ent) {
				var entState = GetEntityState(ent);
				return entState && entState.unitAI;
			});

			if (haveUnitAI && getActionInfo("move", target).possible)
				return {"type": "move"};
			return false;
		},
		"specificness": 12,
	},

	"attack-move": 
	{
		"execute": function(target, action, selection, queued)
		{
			if (Engine.HotkeyIsPressed("session.attackmoveUnit"))
				var targetClasses = { "attack": ["Unit"] };
			else
				var targetClasses = { "attack": ["Unit", "Structure"] };

			Engine.PostNetworkCommand({"type": "attack-walk", "entities": selection, "x": target.x, "z": target.z, "targetClasses": targetClasses, "queued": queued});
			Engine.GuiInterfaceCall("PlaySound", { "name": "order_walk", "entity": selection[0] });
			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			return {"possible": true};
		},
		"hotkeyActionCheck": function(target, selection)
		{
			// Work out whether at least part of the selection have UnitAI
			var haveUnitAI = selection.some(function(ent) {
				var entState = GetEntityState(ent);
				return entState && entState.unitAI;
			});
			if (haveUnitAI && Engine.HotkeyIsPressed("session.attackmove") && getActionInfo("attack-move", target).possible)
				return {"type": "attack-move", "cursor": "action-attack-move"};
			return false;
		},
		"specificness": 30,
	},

	"capture": 
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({"type": "attack", "entities": selection, "target": action.target, "allowCapture": true, "queued": queued});
			Engine.GuiInterfaceCall("PlaySound", { "name": "order_attack", "entity": selection[0] });
			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.attack || !targetState.hitpoints)
				return false;
			return {"possible": Engine.GuiInterfaceCall("CanCapture", {"entity": entState.id, "target": targetState.id})};
		},
		"actionCheck": function(target)
		{
			if (getActionInfo("capture", target).possible)
				return {"type": "capture", "cursor": "action-capture", "target": target};
			return false;
		},
		"specificness": 9,
	},

	"attack":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({"type": "attack", "entities": selection, "target": action.target, "queued": queued, "allowCapture": false});
			Engine.GuiInterfaceCall("PlaySound", { "name": "order_attack", "entity": selection[0] });
			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.attack || !targetState.hitpoints)
				return false;
			return {"possible": Engine.GuiInterfaceCall("CanAttack", {"entity": entState.id, "target": targetState.id})};
		},
		"hotkeyActionCheck": function(target)
		{
			if (Engine.HotkeyIsPressed("session.attack") && getActionInfo("attack", target).possible)
				return {"type": "attack", "cursor": "action-attack", "target": target};
			return false;
		},
		"actionCheck": function(target)
		{
			if (getActionInfo("attack", target).possible)
				return {"type": "attack", "cursor": "action-attack", "target": target};
			return false;
		},
		"specificness": 10,
	},

	"heal": 
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({"type": "heal", "entities": selection, "target": action.target, "queued": queued});
			Engine.GuiInterfaceCall("PlaySound", { "name": "order_heal", "entity": selection[0] });
			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.healer)
				return false;
			if (!hasClass(targetState, "Unit") || !targetState.needsHeal)
				return false;
			if (!playerCheck(entState, targetState, ["Player", "Ally"]))
				return false;

			// Healers can't heal themselves.
			if (entState.id == targetState.id)
				return false;
			
			var unhealableClasses = entState.healer.unhealableClasses;
			if (MatchesClassList(targetState.identity.classes, unhealableClasses))
				return false;

			var healableClasses = entState.healer.healableClasses;
			if (!MatchesClassList(targetState.identity.classes, healableClasses))
				return false;

			return {"possible": true};
		},
		"actionCheck": function(target)
		{
			if (getActionInfo("heal", target).possible)
				return {"type": "heal", "cursor": "action-heal", "target": target};
			return false;
		},
		"specificness": 7,
	},

	"build": 
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({"type": "repair", "entities": selection, "target": action.target, "autocontinue": true, "queued": queued});
			Engine.GuiInterfaceCall("PlaySound", { "name": "order_repair", "entity": selection[0] });
			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (targetState.foundation && entState.builder && playerCheck(entState, targetState, ["Player", "Ally"]))
				return {"possible": true};
			return false;
		},
		"actionCheck": function(target)
		{
			if (getActionInfo("build", target).possible)
				return {"type": "build", "cursor": "action-build", "target": target};
			return false;
		},
		"specificness": 3,
	},

	"repair": 
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({"type": "repair", "entities": selection, "target": action.target, "autocontinue": true, "queued": queued});
			Engine.GuiInterfaceCall("PlaySound", { "name": "order_repair", "entity": selection[0] });
			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (entState.builder && targetState.needsRepair && playerCheck(entState, targetState, ["Player", "Ally"]))
				return {"possible": true};
			return false;
		},
		"preSelectedActionCheck" : function(target)
		{
			if (preSelectedAction != ACTION_REPAIR)
				return false;
			if (getActionInfo("repair", target).possible)
				return {"type": "repair", "cursor": "action-repair", "target": target};
			return {"type": "none", "cursor": "action-repair-disabled", "target": null};
		},
		"actionCheck": function(target)
		{
			if (getActionInfo("repair", target).possible)
				return {"type": "build", "cursor": "action-repair", "target": target};
			return false;
		},
		"specificness": 11,
	},

	"gather": 
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({"type": "gather", "entities": selection, "target": action.target, "queued": queued});
			Engine.GuiInterfaceCall("PlaySound", { "name": "order_gather", "entity": selection[0] });
			return true;
		},
		"getActionInfo":  function(entState, targetState)
		{
			if (!targetState.resourceSupply)
				return false;
			var resource = findGatherType(entState, targetState.resourceSupply);
			if (resource)
				return {"possible": true, "cursor": "action-gather-" + resource};
			return false;
		},
		"actionCheck": function(target)
		{
			var actionInfo = getActionInfo("gather", target);
			if (!actionInfo.possible)
				return false;
			return {"type": "gather", "cursor": actionInfo.cursor, "target": target};
		},
		"specificness": 1,
	},

	"returnresource": 
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({"type": "returnresource", "entities": selection, "target": action.target, "queued": queued});
			Engine.GuiInterfaceCall("PlaySound", { "name": "order_gather", "entity": selection[0] });
			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!targetState.resourceDropsite || !targetState.resourceDropsite.sharable)
				return false;
			var playerState = GetSimState().players[entState.player];
			if (playerState.hasSharedDropsites && targetState.resourceDropsite.shared)
			{
				if (!playerCheck(entState, targetState, ["Player", "MutualAlly"]))
					return false;
			}
			else if (!playerCheck(entState, targetState, ["Player"]))
				return false;
			if (!entState.resourceCarrying || !entState.resourceCarrying.length)
				return false;
			var carriedType = entState.resourceCarrying[0].type;
			if (targetState.resourceDropsite.types.indexOf(carriedType) == -1)
				return false;
			return {"possible": true, "cursor": "action-return-" + carriedType};
		},
		"actionCheck": function(target)
		{
			var actionInfo = getActionInfo("returnresource", target);
			if (!actionInfo.possible)
				return false;
			return {"type": "returnresource", "cursor": actionInfo.cursor, "target": target};
		},
		"specificness": 2,
	},

	"setup-trade-route": 
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({"type": "setup-trade-route", "entities": selection, "target": action.target, "source": null, "route": null, "queued": queued});
			Engine.GuiInterfaceCall("PlaySound", { "name": "order_trade", "entity": selection[0] });
			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (targetState.foundation || !entState.trader)
				return false;
			if (!playerCheck(entState, targetState, ["Player", "Ally"]))
				return false;
			if (!(hasClass(entState, "Organic") && hasClass(targetState, "Market")) && !(hasClass(entState, "Ship") && hasClass(targetState, "NavalMarket")))
				return false;

			var tradingData = {"trader": entState.id, "target": targetState.id};
			var tradingDetails = Engine.GuiInterfaceCall("GetTradingDetails", tradingData);

			if (!tradingDetails)
				return false;

			var tooltip;
			switch (tradingDetails.type)
			{
			case "is first":
				tooltip = translate("Origin trade market.");
				if (tradingDetails.hasBothMarkets)
					tooltip += "\n" + sprintf(translate("Gain: %(gain)s"), {
						gain: getTradingTooltip(tradingDetails.gain)
					});
				else
					tooltip += "\n" + translate("Right-click on another market to set it as a destination trade market.");
				break;
			case "is second":
				tooltip = translate("Destination trade market.") + "\n" + sprintf(translate("Gain: %(gain)s"), {
					gain: getTradingTooltip(tradingDetails.gain)
				});
				break;
			case "set first":
				tooltip = translate("Right-click to set as origin trade market");
				break;
			case "set second":
				if (tradingDetails.gain.traderGain == 0)   // markets too close
					return false;
				tooltip = translate("Right-click to set as destination trade market.") + "\n" + sprintf(translate("Gain: %(gain)s"), {
					gain: getTradingTooltip(tradingDetails.gain)
				});
				break;
			}
			return {"possible": true, "tooltip": tooltip};

		},
		"actionCheck": function(target)
		{
			var actionInfo = getActionInfo("setup-trade-route", target);
			if (!actionInfo.possible)
				return false;
			return {"type": "setup-trade-route", "cursor": "action-setup-trade-route", "tooltip": actionInfo.tooltip, "target": target};
		},
		"specificness": 0,
	},

	"garrison": 
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({"type": "garrison", "entities": selection, "target": action.target, "queued": queued});
			Engine.GuiInterfaceCall("PlaySound", { "name": "order_garrison", "entity": selection[0] });
			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!hasClass(entState, "Unit") || !targetState.garrisonHolder)
				return false;
			if (!playerCheck(entState, targetState, ["Player", "MutualAlly"]))
				return false;
			var tooltip = sprintf(translate("Current garrison: %(garrisoned)s/%(capacity)s"), {
				garrisoned: targetState.garrisonHolder.garrisonedEntitiesCount,
				capacity: targetState.garrisonHolder.capacity
			});
			var extraCount = 0;
			if (entState.garrisonHolder)
				extraCount += entState.garrisonHolder.garrisonedEntitiesCount;
			if (targetState.garrisonHolder.garrisonedEntitiesCount + extraCount >= targetState.garrisonHolder.capacity)
				tooltip = "[color=\"orange\"]" + tooltip + "[/color]";
			if (MatchesClassList(entState.identity.classes, targetState.garrisonHolder.allowedClasses))
				return {"possible": true, "tooltip": tooltip};
			return false;

		},
		"preSelectedActionCheck": function(target)
		{
			if (preSelectedAction != ACTION_GARRISON)
				return false;
			var actionInfo =  getActionInfo("garrison", target);
			if (actionInfo.possible)
				return {"type": "garrison", "cursor": "action-garrison", "tooltip": actionInfo.tooltip, "target": target};
			return {"type": "none", "cursor": "action-garrison-disabled", "target": null};
		},
		"hotkeyActionCheck": function(target)
		{
			var actionInfo = getActionInfo("garrison", target);
			if (Engine.HotkeyIsPressed("session.garrison") && actionInfo.possible)
				return {"type": "garrison", "cursor": "action-garrison", "tooltip": actionInfo.tooltip, "target": target};
			return false;
		},
		"specificness": 20,
	},

	"guard": 
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({"type": "guard", "entities": selection, "target": action.target, "queued": queued});
			Engine.GuiInterfaceCall("PlaySound", { "name": "order_guard", "entity": selection[0] });
			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!targetState.guard)
				return false;
			if (!playerCheck(entState, targetState, ["Player", "Ally"]))
				return false;
			if (!entState.unitAI || !entState.unitAI.canGuard)
				return false;
			if (targetState.unitAI && targetState.unitAI.isGuarding)
				return false;
			return {"possible": true};
		},
		"preSelectedActionCheck" : function(target)
		{
			if (preSelectedAction != ACTION_GUARD)
				return false;
			if (getActionInfo("guard", target).possible)
				return {"type": "guard", "cursor": "action-guard", "target": target};
			return {"type": "none", "cursor": "action-guard-disabled", "target": null};
		},
		"hotkeyActionCheck": function(target)
		{
			 if (Engine.HotkeyIsPressed("session.guard") && getActionInfo("guard", target).possible)
				return {"type": "guard", "cursor": "action-guard", "target": target};
			return false;
		},
		"specificness": 40,
	},

	"remove-guard": 
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({"type": "remove-guard", "entities": selection, "target": action.target, "queued": queued});
			Engine.GuiInterfaceCall("PlaySound", { "name": "order_guard", "entity": selection[0] });
			return true;
		},
		"hotkeyActionCheck": function(target, selection)
		{
			if (Engine.HotkeyIsPressed("session.guard") && getActionInfo("remove-guard", target).possible)
			{
				var isGuarding = selection.some(function(ent) {
					var entState = GetEntityState(ent);
					return entState && entState.unitAI && entState.unitAI.isGuarding;
				});
				if (isGuarding)
					return {"type": "remove-guard", "cursor": "action-remove-guard"};
			}
			return false;
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

			Engine.PostNetworkCommand({"type": "set-rallypoint", "entities": selection, "x": target.x, "z": target.z, "data": action.data, "queued": queued});
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
			var tooltip;
			// default to walking there (or attack-walking if hotkey pressed)
			var data = {command: "walk"};
			var cursor = "";
			if (Engine.HotkeyIsPressed("session.attackmove"))
			{
				if (Engine.HotkeyIsPressed("session.attackmoveUnit"))
					var targetClasses = { "attack": ["Unit"] };
				else
					var targetClasses = { "attack": ["Unit", "Structure"] };
				data.command = "attack-walk";
				data.targetClasses = targetClasses;
				cursor = "action-attack-move";
			}

			if (targetState.garrisonHolder && playerCheck(entState, targetState, ["Player", "MutualAlly"]))
			{
				data.command = "garrison";
				data.target = targetState.id;
				cursor = "action-garrison";
				tooltip = sprintf(translate("Current garrison: %(garrisoned)s/%(capacity)s"), {
					garrisoned: targetState.garrisonHolder.garrisonedEntitiesCount,
					capacity: targetState.garrisonHolder.capacity
				});
				if (targetState.garrisonHolder.garrisonedEntitiesCount >= targetState.garrisonHolder.capacity)
					tooltip = "[color=\"orange\"]" + tooltip + "[/color]";
			}
			else if (targetState.resourceSupply)
			{
				var resourceType = targetState.resourceSupply.type;
				if (resourceType.generic == "treasure")
					cursor = "action-gather-" + resourceType.generic;
				else
					cursor = "action-gather-" + resourceType.specific;
				data.command = "gather";
				data.resourceType = resourceType;
				data.resourceTemplate = targetState.template;
			}
			else if (hasClass(entState, "Market") && hasClass(targetState, "Market") && entState.id != targetState.id &&
					(!hasClass(entState, "NavalMarket") || hasClass(targetState, "NavalMarket")) && !playerCheck(entState, targetState, ["Enemy"]))
			{
				// Find a trader (if any) that this building can produce.
				var trader;
				if (entState.production && entState.production.entities.length)
					for (var i = 0; i < entState.production.entities.length; ++i)
						if ((trader = GetTemplateData(entState.production.entities[i]).trader))
							break;

				var traderData = { "firstMarket": entState.id, "secondMarket": targetState.id, "template": trader };
				var gain = Engine.GuiInterfaceCall("GetTradingRouteGain", traderData);
				if (gain && gain.traderGain)
				{
					data.command = "trade";
					data.target = traderData.secondMarket;
					data.source = traderData.firstMarket;
					cursor = "action-setup-trade-route";
					tooltip = translate("Right-click to establish a default route for new traders.");
					if (trader)
						tooltip += "\n" + sprintf(translate("Gain: %(gain)s"), { gain: getTradingTooltip(gain) });
					else // Foundation or cannot produce traders
						tooltip += "\n" + sprintf(translate("Expected gain: %(gain)s"), { gain: getTradingTooltip(gain) });
				}
			}
			else if (targetState.foundation && playerCheck(entState, targetState, ["Ally"]))
			{
				data.command = "build";
				data.target = targetState.id;
				cursor = "action-build";
			}
			else if (targetState.needsRepair && playerCheck(entState, targetState, ["Ally"]))
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
			if (!Engine.HotkeyIsPressed("session.autorallypoint") || !targetState.production || !targetState.production.entities.length)
			{
				for each (var ent in g_Selection.selected)
					if (targetState.id === ent)
						return false;
			}

			return {"possible": true, "data": data, "position": targetState.position, "cursor": cursor, "tooltip": tooltip};
	
		},
		"actionCheck": function(target, selection)
		{
			// Work out whether at least part of the selection have UnitAI
			var haveUnitAI = selection.some(function(ent) {
				var entState = GetEntityState(ent);
				return entState && entState.unitAI;
			});
			if (haveUnitAI)
				return false;

			// Work out whether at least part the selection have rally points
			// while none have UnitAI
			var haveRallyPoints = selection.some(function(ent) {
				var entState = GetEntityState(ent);
				return entState && ("rallyPoint" in entState) && entState.rallyPoint;
			});
			if (!haveRallyPoints)
				return false;
			
			var actionInfo = getActionInfo("set-rallypoint", target);
			if (!actionInfo.possible)
				return false;
			return {"type": "set-rallypoint", "cursor": actionInfo.cursor, "data": actionInfo.data, "tooltip": actionInfo.tooltip, "position": actionInfo.position};
		},
		"specificness": 6,
	},

	"unset-rallypoint": 
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({"type": "unset-rallypoint", "entities": selection});
			// Remove displayed rally point
			Engine.GuiInterfaceCall("DisplayRallyPoint", {
				"entities": []
			});
			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (entState.id != targetState.id)
				return false;
			if (!entState.rallyPoint || !entState.rallyPoint.position)
				return false;
			return {"possible": true};
		},
		"actionCheck": function(target, selection)
		{
			// Work out whether at least part of the selection have UnitAI
			var haveUnitAI = selection.some(function(ent) {
				var entState = GetEntityState(ent);
				return entState && entState.unitAI;
			});

			// Work out whether at least part the selection have rally points
			// while none have UnitAI
			var haveRallyPoints = selection.some(function(ent) {
				var entState = GetEntityState(ent);
				return entState && ("rallyPoint" in entState) && entState.rallyPoint;
			});

			if (!haveUnitAI && haveRallyPoints && getActionInfo("unset-rallypoint", target).possible)
				return {"type": "unset-rallypoint", "cursor": "action-unset-rally"};
			return false;
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
	// Unload
	"unload-all": {
		"getInfo": function(entState)
		{
			if (!entState.garrisonHolder)
				return false;
			var count = 0;
			for each (var ent in g_Selection.selected)
			{
				var state = GetEntityState(ent);
				if (state.garrisonHolder)
					count += state.garrisonHolder.entities.length;
			}
			return {
				"tooltip": translate("Unload All"),
				"icon": "garrison-out.png",
				"count": count,
			};
		},
		"execute": function(entState)
		{
			unloadAll();
		},
	},
	// Delete
	"delete": {
		"getInfo": function(entState)
		{
			if (!entState.canDelete)
				return false;

			if (entState.mirage)
				return {
					"tooltip": translate("You cannot destroy this entity because it is in the fog-of-war"),
					"icon": "kill_small_disabled.png"
				};

			if (entState.capturePoints && entState.capturePoints[entState.player] < entState.maxCapturePoints / 2)
				return {
					"tooltip": translate("You cannot destroy this entity as you own less than half the capture points"),
					"icon": "kill_small_disabled.png"
				};
					

			return {
				"tooltip": translate("Delete"),
				"icon": "kill_small.png"
			};
		},
		"execute": function(entState)
		{
			if (entState.mirage)
				return;

			if (entState.capturePoints && entState.capturePoints[entState.player] < entState.maxCapturePoints / 2)
				return;

			var selection = g_Selection.toList();
			if (selection.length < 1)
				return;
			if (!entState.resourceSupply || !entState.resourceSupply.killBeforeGather)
				openDeleteDialog(selection);
		},
	},
	// Stop
	"stop": {
		"getInfo": function(entState)
		{
			if (!entState.unitAI)
				return false;
			return {
				"tooltip": translate("Stop"),
				"icon": "stop.png"
			};
		},
		"execute": function(entState)
		{
			var selection = g_Selection.toList();
			if (selection.length > 0)
				stopUnits(selection);
		},
	},
	// Garrison
	 "garrison": {
		"getInfo": function(entState)
		{
			if (!entState.unitAI || entState.turretParent)
				return false;
			return {
				"tooltip": translate("Garrison"),
				"icon": "garrison.png"
			};
		},
		"execute": function(entState)
		{
			inputState = INPUT_PRESELECTEDACTION;
			preSelectedAction = ACTION_GARRISON;
		},
	},
	// Ungarrison
	"unload": {
		"getInfo": function(entState)
		{
			if (!entState.unitAI || !entState.turretParent)
				return false;

			var p = GetEntityState(entState.turretParent);
			if (!p.garrisonHolder || p.garrisonHolder.entities.indexOf(entState.id) == -1)
				return false;
			return {
				"tooltip": translate("Unload"),
				"icon": "garrison-out.png"
			};
		},
		"execute": function(entState)
		{
			unloadSelection();
		},
	},
	// Repair
	"repair": {
		"getInfo": function(entState)
		{
			if (!entState.builder)
				return false;
			return {
				"tooltip": translate("Repair"),
				"icon": "repair.png"
			};
		},
		"execute": function(entState)
		{
			inputState = INPUT_PRESELECTEDACTION;
			preSelectedAction = ACTION_REPAIR;
		},
	},
	// Focus on rally point
	"focus-rally": {
		"getInfo": function(entState)
		{
			if (!entState.rallyPoint)
				return false;
			return {
				"tooltip": translate("Focus on Rally Point"),
				"icon": "focus-rally.png"
			};
		},
		"execute": function(entState)
		{
			var focusTarget = null;
			if (entState.rallyPoint && entState.rallyPoint.position)
				focusTarget = entState.rallyPoint.position;
			else if (entState.position)
				focusTarget = entState.position;
			
			if (focusTarget)
				Engine.CameraMoveTo(focusTarget.x, focusTarget.z);
		},
	},
	// Back to work
	"back-to-work": {
		"getInfo": function(entState)
		{
			if (!entState.unitAI || !entState.unitAI.hasWorkOrders)
				return false;
			return {
				"tooltip": translate("Back to Work"),
				"icon": "production.png"
			};
		},
		"execute": function(entState)
		{
			backToWork();
		},
	},
	// Guard
	"add-guard": {
		"getInfo": function(entState)
		{
			if (!entState.unitAI || !entState.unitAI.canGuard || entState.unitAI.isGuarding)
				return false;
			return {
				"tooltip": translate("Guard"),
				"icon": "add-guard.png"
			};
		},
		"execute": function(entState)
		{
			inputState = INPUT_PRESELECTEDACTION;
			preSelectedAction = ACTION_GUARD;
		},
	},
	// Remove guard
	"remove-guard": {
		"getInfo": function(entState)
		{
			if (!entState.unitAI || !entState.unitAI.isGuarding)
				return false;
			return {
				"tooltip": translate("Remove guard"),
				"icon": "remove-guard.png"
			};
		},
		"execute": function(entState)
		{
			removeGuard();
		},
	},
	// Trading
	"select-trading-goods": {
		"getInfo": function(entState)
		{
			if (!hasClass(entState, "Market"))
				return false;
			return {
				"tooltip": translate("Select trading goods"),
				"icon": "economics.png"
			};
		},
		"execute": function(entState)
		{
			toggleTrade();
		},
	},
	// Dropsite sharing
	"share-dropsite": {
		"getInfo": function(entState)
		{
			if (!entState.resourceDropsite || !entState.resourceDropsite.sharable)
				return false;
			let playerState = GetSimState().players[entState.player];
			if (!playerState.isMutualAlly.some((e, i) => e && i != entState.player))
				return false;
			if (entState.resourceDropsite.shared)
				return {
					"tooltip": translate("Press to prevent allies from using this dropsite"),
					"icon": "lock_unlocked.png"
				};
			return {
				"tooltip": translate("Press to allow allies to use this dropsite"),
				"icon": "lock_locked.png"
			};
		},
		"execute": function(entState)
		{
			Engine.PostNetworkCommand({
				"type": "set-dropsite-sharing",
				"entities": [entState.id],
				"shared": !entState.resourceDropsite.shared
			});
		},
	}
};

var g_AllyEntityCommands =
{
	// Unload
	"unload-all": {
		"getInfo": function(entState)
		{
			if (!entState.garrisonHolder)
				return false;
			var count = 0;
			for each (var ent in g_Selection.selected)
			{
				var selectedEntState = GetEntityState(ent);
				if (selectedEntState.garrisonHolder)
				{	
					var player = Engine.GetPlayerID();
					for (var entity of selectedEntState.garrisonHolder.entities)
					{
						var state = GetEntityState(entity);
						if (state.player == player)
							count++;
					}
				}
			}
			return {
				"tooltip": translate("Unload All"),
				"icon": "garrison-out.png",
				"count": count,
			};
		},
		"execute": function(entState)
		{
			unloadAllByOwner();
		},
	},
	// Dropsite sharing
	"share-dropsite": {
		"getInfo": function(entState)
		{
			if (Engine.GetPlayerID() == -1 || !GetSimState().players[Engine.GetPlayerID()].hasSharedDropsites)
				return false;
			if (!entState.resourceDropsite || !entState.resourceDropsite.sharable)
				return false;
			if (entState.resourceDropsite.shared)
				return {
					"tooltip": translate("You are allowed to use this dropsite"),
					"icon": "lock_unlocked.png"
				};
			return {
				"tooltip": translate("The use of this dropsite is prohibited"),
				"icon": "lock_locked.png"
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
	var playerState = GetSimState().players[entState.player];
	for (var player of validPlayers)
	{
		if (player == "Gaia" && targetState.player == 0)
			return true;
		if (player == "Player" && targetState.player == entState.player)
			return true;
		if (playerState["is"+player] && playerState["is"+player][targetState.player])
			return true;
	}
	return false;
}
