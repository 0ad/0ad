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

const g_DisabledTags = { "color": "255 140 0" };

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
				"queued": queued,
				"formation": g_AutoFormation.getDefault()
			});

			DrawTargetMarker(target);

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_walk",
				"entity": action.firstAbleEntity
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.unitAI)
				return false;
			return { "possible": true };
		},
		"hotkeyActionCheck": function(target, selection)
		{
			return Engine.HotkeyIsPressed("session.move") &&
				this.actionCheck(target, selection);
		},
		"actionCheck": function(target, selection)
		{
			let actionInfo = getActionInfo("move", target, selection);
			return actionInfo.possible && {
				"type": "move",
				"firstAbleEntity": actionInfo.entity
			};
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
				"queued": queued,
				"formation": g_AutoFormation.getNull()
			});

			DrawTargetMarker(target);

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_walk",
				"entity": action.firstAbleEntity
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.unitAI)
				return false;
			return { "possible": true };
		},
		"hotkeyActionCheck": function(target, selection)
		{
			return isAttackMovePressed() &&
				this.actionCheck(target, selection);
		},
		"actionCheck": function(target, selection)
		{
			let actionInfo = getActionInfo("attack-move", target, selection);
			return actionInfo.possible && {
				"type": "attack-move",
				"cursor": "action-attack-move",
				"firstAbleEntity": actionInfo.entity
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
				"queued": queued,
				"formation": g_AutoFormation.getNull()
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_attack",
				"entity": action.firstAbleEntity
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.attack || !targetState || !targetState.capturePoints)
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
			let actionInfo = getActionInfo("capture", target, selection);
			return actionInfo.possible && {
				"type": "capture",
				"cursor": "action-capture",
				"target": target,
				"firstAbleEntity": actionInfo.entity
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
				"allowCapture": false,
				"formation": g_AutoFormation.getNull()
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_attack",
				"entity": action.firstAbleEntity
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.attack || !targetState || !targetState.hitpoints)
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
			return Engine.HotkeyIsPressed("session.attack") &&
				this.actionCheck(target, selection);
		},
		"actionCheck": function(target, selection)
		{
			let actionInfo = getActionInfo("attack", target, selection);
			return actionInfo.possible && {
				"type": "attack",
				"cursor": "action-attack",
				"target": target,
				"firstAbleEntity": actionInfo.entity
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
				"allowCapture": false,
				"formation": g_AutoFormation.getDefault()
			});

			DrawTargetMarker(target);

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_patrol",
				"entity": action.firstAbleEntity
			});
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
			return Engine.HotkeyIsPressed("session.patrol") &&
				this.actionCheck(target, selection);
		},
		"preSelectedActionCheck": function(target, selection)
		{
			return preSelectedAction == ACTION_PATROL &&
				this.actionCheck(target, selection);
		},
		"actionCheck": function(target, selection)
		{
			let actionInfo = getActionInfo("patrol", target, selection);
			return actionInfo.possible && {
				"type": "patrol",
				"cursor": "action-patrol",
				"target": target,
				"firstAbleEntity": actionInfo.entity
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
				"queued": queued,
				"formation": g_AutoFormation.getNull()
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_heal",
				"entity": action.firstAbleEntity
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.heal || !targetState ||
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
			let actionInfo = getActionInfo("heal", target, selection);
			return actionInfo.possible && {
				"type": "heal",
				"cursor": "action-heal",
				"target": target,
				"firstAbleEntity": actionInfo.entity
			};
		},
		"specificness": 7,
	},

	// "Fake" action to check if an entity can be ordered to "construct"
	// which is handled differently from repair as the target does not exist.
	"construct":
	{
		"preSelectedActionCheck": function(target, selection)
		{
			let state = GetEntityState(selection[0]);
			if (state && state.builder &&
			        target && target.constructor && target.constructor.name == "PlacementSupport")
				return { "type": "construct" };
			return false;
		},
		"specificness": 0,
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
				"queued": queued,
				"formation": g_AutoFormation.getNull()
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": action.foundation ? "order_build" : "order_repair",
				"entity": action.firstAbleEntity
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.builder || !targetState ||
			    !targetState.needsRepair && !targetState.foundation ||
			    !playerCheck(entState, targetState, ["Player", "Ally"]))
				return false;

			return {
				"possible": true,
				"foundation": targetState.foundation
			};
		},
		"preSelectedActionCheck": function(target, selection)
		{
			return preSelectedAction == ACTION_REPAIR && (this.actionCheck(target, selection) || {
				"type": "none",
				"cursor": "action-repair-disabled",
				"target": null
			});
		},
		"hotkeyActionCheck": function(target, selection)
		{
			return Engine.HotkeyIsPressed("session.repair") &&
				this.actionCheck(target, selection);
		},
		"actionCheck": function(target, selection)
		{
			let actionInfo = getActionInfo("repair", target, selection);
			return actionInfo.possible && {
				"type": "repair",
				"cursor": "action-repair",
				"target": target,
				"foundation": actionInfo.foundation,
				"firstAbleEntity": actionInfo.entity
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
				"queued": queued,
				"formation": g_AutoFormation.getNull()
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_gather",
				"entity": action.firstAbleEntity
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.resourceGatherRates ||
				!targetState || !targetState.resourceSupply)
				return false;

			let resource;
			if (entState.resourceGatherRates[targetState.resourceSupply.type.generic + "." + targetState.resourceSupply.type.specific])
				resource = targetState.resourceSupply.type.specific;
			else if (entState.resourceGatherRates[targetState.resourceSupply.type.generic])
				resource = targetState.resourceSupply.type.generic;
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
			return actionInfo.possible && {
				"type": "gather",
				"cursor": actionInfo.cursor,
				"target": target,
				"firstAbleEntity": actionInfo.entity
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
				"queued": queued,
				"formation": g_AutoFormation.getNull()
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_gather",
				"entity": action.firstAbleEntity
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!targetState || !targetState.resourceDropsite)
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
			return actionInfo.possible && {
				"type": "returnresource",
				"cursor": actionInfo.cursor,
				"target": target,
				"firstAbleEntity": actionInfo.entity
			};
		},
		"specificness": 2,
	},

	"cancel-setup-trade-route":
	{
		"execute": function(target, action, selection, queued)
		{
			Engine.PostNetworkCommand({
				"type": "cancel-setup-trade-route",
				"entities": selection,
				"target": action.target,
				"queued": queued
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!targetState || targetState.foundation || !entState.trader || !targetState.market ||
			    playerCheck(entState, targetState, ["Enemy"]) ||
			    !(targetState.market.land && hasClass(entState, "Organic") ||
			      targetState.market.naval && hasClass(entState, "Ship")))
				return false;

			let tradingDetails = Engine.GuiInterfaceCall("GetTradingDetails", {
				"trader": entState.id,
				"target": targetState.id
			});

			if (!tradingDetails || !tradingDetails.type)
				return false;

			if (tradingDetails.type == "is first" && !tradingDetails.hasBothMarkets)
				return {
					"possible": true,
					"tooltip": translate("This is the origin trade market.\nRight-click to cancel trade route.")
				};
			return false;
		},
		"actionCheck": function(target, selection)
		{
			let actionInfo = getActionInfo("cancel-setup-trade-route", target, selection);
			return actionInfo.possible && {
				"type": "cancel-setup-trade-route",
				"cursor": "action-cancel-setup-trade-route",
				"tooltip": actionInfo.tooltip,
				"target": target,
				"firstAbleEntity": actionInfo.entity
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
				"queued": queued,
				"formation": g_AutoFormation.getNull()
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_trade",
				"entity": action.firstAbleEntity
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!targetState || targetState.foundation || !entState.trader || !targetState.market ||
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
					return false;
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
				if (tradingDetails.gain.traderGain == 0)
					return {
						"possible": true,
						"tooltip": setStringTags(translate("This market is too close to the origin market."), g_DisabledTags),
						"disabled": true
					};

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
			if (actionInfo.disabled)
				return {
					"type": "none",
					"cursor": "action-setup-trade-route-disabled",
					"target": null,
					"tooltip": actionInfo.tooltip
				};

			return actionInfo.possible && {
				"type": "setup-trade-route",
				"cursor": "action-setup-trade-route",
				"tooltip": actionInfo.tooltip,
				"target": target,
				"firstAbleEntity": actionInfo.entity
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
				"queued": queued,
				"formation": g_AutoFormation.getNull()
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_garrison",
				"entity": action.firstAbleEntity
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.garrisonable || !targetState || !targetState.garrisonHolder ||
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
			return preSelectedAction == ACTION_GARRISON && (this.actionCheck(target, selection) || {
				"type": "none",
				"cursor": "action-garrison-disabled",
				"target": null
			});
		},
		"hotkeyActionCheck": function(target, selection)
		{
			return Engine.HotkeyIsPressed("session.garrison") &&
				this.actionCheck(target, selection);

		},
		"actionCheck": function(target, selection)
		{
			let actionInfo = getActionInfo("garrison", target, selection);
			return actionInfo.possible && {
				"type": "garrison",
				"cursor": "action-garrison",
				"tooltip": actionInfo.tooltip,
				"target": target,
				"firstAbleEntity": actionInfo.entity
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
				"queued": queued,
				"formation": g_AutoFormation.getNull()
			});

			Engine.GuiInterfaceCall("PlaySound", {
				"name": "order_guard",
				"entity": action.firstAbleEntity
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!targetState || !targetState.guard || entState.id == targetState.id ||
			    !playerCheck(entState, targetState, ["Player", "Ally"]) ||
			    !entState.unitAI || !entState.unitAI.canGuard)
				return false;

			return { "possible": true };
		},
		"preSelectedActionCheck": function(target, selection)
		{
			return preSelectedAction == ACTION_GUARD && (this.actionCheck(target, selection) || {
				"type": "none",
				"cursor": "action-guard-disabled",
				"target": null
			});
		},
		"hotkeyActionCheck": function(target, selection)
		{
			return Engine.HotkeyIsPressed("session.guard") &&
				this.actionCheck(target, selection);
		},
		"actionCheck": function(target, selection)
		{
			let actionInfo = getActionInfo("guard", target, selection);
			return actionInfo.possible && {
				"type": "guard",
				"cursor": "action-guard",
				"target": target,
				"firstAbleEntity": actionInfo.entity
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
				"entity": action.firstAbleEntity
			});

			return true;
		},
		"getActionInfo": function(entState, targetState)
		{
			if (!entState.unitAI || !entState.unitAI.isGuarding)
				return false;
			return { "possible": true };
		},
		"hotkeyActionCheck": function(target, selection)
		{
			return Engine.HotkeyIsPressed("session.guard") &&
				this.actionCheck(target, selection);
		},
		"actionCheck": function(target, selection)
		{
			let actionInfo = getActionInfo("remove-guard", target, selection);
			return actionInfo.possible && {
				"type": "remove-guard",
				"cursor": "action-remove-guard",
				"firstAbleEntity": actionInfo.entity
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
			if (!entState.rallyPoint)
				return false;

			// Don't allow the rally point to be set on any of the currently selected entities (used for unset)
			// except if the autorallypoint hotkey is pressed and the target can produce entities.
			if (targetState && (!Engine.HotkeyIsPressed("session.autorallypoint") ||
			    !targetState.production ||
			    !targetState.production.entities.length))
				for (let ent in g_Selection.selected)
					if (targetState.id == +ent)
						return false;

			let tooltip;
			let disabled = false;
			// default to walking there (or attack-walking if hotkey pressed)
			let data = { "command": "walk" };
			let cursor = "";

			if (isAttackMovePressed())
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

			if (Engine.HotkeyIsPressed("session.repair") && targetState &&
			    (targetState.needsRepair || targetState.foundation) &&
			    playerCheck(entState, targetState, ["Player", "Ally"]))
			{
				data.command = "repair";
				data.target = targetState.id;
				cursor = "action-repair";
			}
			else if (targetState && targetState.garrisonHolder &&
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
			else if (targetState && targetState.resourceSupply)
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
			else if (entState.market && targetState && targetState.market &&
			         entState.id != targetState.id &&
			         (!entState.market.naval || targetState.market.naval) &&
			         !playerCheck(entState, targetState, ["Enemy"]))
			{
				// Find a trader (if any) that this structure can train.
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
				if (gain)
				{
					data.command = "trade";
					data.target = traderData.secondMarket;
					data.source = traderData.firstMarket;
					cursor = "action-setup-trade-route";

					if (gain.traderGain)
						tooltip = translate("Right-click to establish a default route for new traders.") + "\n" +
							sprintf(
								trader ?
									translate("Gain: %(gain)s") :
									translate("Expected gain: %(gain)s"),
								{ "gain": getTradingTooltip(gain) });
					else
					{
						disabled = true;
						tooltip = setStringTags(translate("This market is too close to the origin market."), g_DisabledTags);
						cursor = "action-setup-trade-route-disabled";
					}
				}
			}
			else if (targetState && (targetState.needsRepair || targetState.foundation) && playerCheck(entState, targetState, ["Ally"]))
			{
				data.command = "repair";
				data.target = targetState.id;
				cursor = "action-repair";
			}
			else if (targetState && playerCheck(entState, targetState, ["Enemy"]))
			{
				data.target = targetState.id;
				data.command = "attack";
				cursor = "action-attack";
			}

			return {
				"possible": true,
				"data": data,
				"position": targetState && targetState.position,
				"cursor": cursor,
				"disabled": disabled,
				"tooltip": tooltip
			};

		},
		"hotkeyActionCheck": function(target, selection)
		{
			// Hotkeys are checked in the actionInfo.
			return this.actionCheck(target, selection);
		},
		"actionCheck": function(target, selection)
		{
			// We want commands to units take precedence.
			if (selection.some(ent => {
				let entState = GetEntityState(ent);
				return entState && !!entState.unitAI;
			}))
				return false;

			let actionInfo = getActionInfo("set-rallypoint", target, selection);
			if (actionInfo.disabled)
				return {
					"type": "none",
					"cursor": actionInfo.cursor,
					"target": null,
					"tooltip": actionInfo.tooltip
				};

			return actionInfo.possible && {
				"type": "set-rallypoint",
				"cursor": actionInfo.cursor,
				"data": actionInfo.data,
				"tooltip": actionInfo.tooltip,
				"position": actionInfo.position,
				"firstAbleEntity": actionInfo.entity
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
			if (!targetState ||
				entState.id != targetState.id || entState.unitAI ||
				!entState.rallyPoint || !entState.rallyPoint.position)
				return false;

			return { "possible": true };
		},
		"actionCheck": function(target, selection)
		{
			let actionInfo = getActionInfo("unset-rallypoint", target, selection);
			return actionInfo.possible && {
				"type": "unset-rallypoint",
				"cursor": "action-unset-rally",
				"firstAbleEntity": actionInfo.entity
			};
		},
		"specificness": 11,
	},

	// This is a "fake" action to show a failure cursor
	// when only uncontrollable entities are selected.
	"uncontrollable":
	{
		"execute": function(target, action, selection, queued)
		{
			return true;
		},
		"actionCheck": function(target, selection)
		{
			// Only show this action if all entities are marked uncontrollable.
			let playerState = g_SimState.players[g_ViewedPlayer];
			if (playerState && playerState.controlsAll || selection.some(ent => {
				let entState = GetEntityState(ent);
				return entState && entState.identity && entState.identity.controllable;
			}))
				return false;

			return {
				"type": "none",
				"cursor": "cursor-no",
				"tooltip": translatePlural("This entity cannot be controlled.", "These entities cannot be controlled.", selection.length)
			};
		},
		"specificness": 100,
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

var g_UnitActionsSortedKeys = Object.keys(g_UnitActions).sort((a, b) => g_UnitActions[a].specificness - g_UnitActions[b].specificness);

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
			{
				if (!entState.garrisonHolder)
					continue;

				if (allowedPlayersCheck([entState], ["Player"]))
					count += entState.garrisonHolder.entities.length;
				else
					for (let entity of entState.garrisonHolder.entities)
						if (allowedPlayersCheck([GetEntityState(entity)], ["Player"]))
							++count;
			}

			if (!count)
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "session.unload") +
				           translate("Unload All."),
				"icon": "garrison-out.png",
				"count": count,
				"enabled": true
			};
		},
		"execute": function()
		{
			unloadAll();
		},
		"allowedPlayers": ["Player", "Ally"]
	},

	"delete": {
		"getInfo": function(entStates)
		{
			return entStates.some(entState => !isUndeletable(entState)) ?
				{
					"tooltip":
						colorizeHotkey("%(hotkey)s" + " ", "session.kill") +
						translate("Destroy the selected units or structures.") + "\n" +
						colorizeHotkey(
							translate("Use %(hotkey)s to avoid the confirmation dialog."),
							"session.noconfirmation"
						),
					"icon": "kill_small.png",
					"enabled": true
				} :
				{
					// Get all delete reasons and remove duplications
					"tooltip": entStates.map(entState => isUndeletable(entState))
						.filter((reason, pos, self) =>
							self.indexOf(reason) == pos && reason
						).join("\n"),
					"icon": "kill_small_disabled.png",
					"enabled": false
				};
		},
		"execute": function(entStates)
		{
			let entityIDs = entStates.reduce(
				(ids, entState) => {
					if (!isUndeletable(entState))
						ids.push(entState.id);
					return ids;
				},
				[]);

			if (!entityIDs.length)
				return;

			let deleteSelection = () => Engine.PostNetworkCommand({
				"type": "delete-entities",
				"entities": entityIDs
			});

			if (Engine.HotkeyIsPressed("session.noconfirmation"))
				deleteSelection();
			else
				(new DeleteSelectionConfirmation(deleteSelection)).display();
		},
		"allowedPlayers": ["Player"]
	},

	"stop": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState => !entState.unitAI))
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "session.stop") +
				           translate("Abort the current order."),
				"icon": "stop.png",
				"enabled": true
			};
		},
		"execute": function(entStates)
		{
			if (entStates.length)
				stopUnits(entStates.map(entState => entState.id));
		},
		"allowedPlayers": ["Player"]
	},

	"garrison": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState => !entState.unitAI || entState.turretParent || false))
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "session.garrison") +
				           translate("Order the selected units to garrison in a structure or unit."),
				"icon": "garrison.png",
				"enabled": true
			};
		},
		"execute": function()
		{
			inputState = INPUT_PRESELECTEDACTION;
			preSelectedAction = ACTION_GARRISON;
		},
		"allowedPlayers": ["Player"]
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
				"icon": "garrison-out.png",
				"enabled": true
			};
		},
		"execute": function()
		{
			unloadSelection();
		},
		"allowedPlayers": ["Player"]
	},

	"repair": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState => !entState.builder))
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "session.repair") +
				           translate("Order the selected units to repair a structure, ship, or siege engine."),
				"icon": "repair.png",
				"enabled": true
			};
		},
		"execute": function()
		{
			inputState = INPUT_PRESELECTEDACTION;
			preSelectedAction = ACTION_REPAIR;
		},
		"allowedPlayers": ["Player"]
	},

	"focus-rally": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState => !entState.rallyPoint))
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "camera.rallypointfocus") +
				           translate("Focus on Rally Point."),
				"icon": "focus-rally.png",
				"enabled": true
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
		"allowedPlayers": ["Player", "Observer"]
	},

	"back-to-work": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState => !entState.unitAI || !entState.unitAI.hasWorkOrders))
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "session.backtowork") +
				           translate("Back to Work"),
				"icon": "back-to-work.png",
				"enabled": true
			};
		},
		"execute": function()
		{
			backToWork();
		},
		"allowedPlayers": ["Player"]
	},

	"add-guard": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState =>
				!entState.unitAI || !entState.unitAI.canGuard || entState.unitAI.isGuarding))
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "session.guard") +
				           translate("Order the selected units to guard a structure or unit."),
				"icon": "add-guard.png",
				"enabled": true
			};
		},
		"execute": function()
		{
			inputState = INPUT_PRESELECTEDACTION;
			preSelectedAction = ACTION_GUARD;
		},
		"allowedPlayers": ["Player"]
	},

	"remove-guard": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState => !entState.unitAI || !entState.unitAI.isGuarding))
				return false;

			return {
				"tooltip": translate("Remove guard"),
				"icon": "remove-guard.png",
				"enabled": true
			};
		},
		"execute": function()
		{
			removeGuard();
		},
		"allowedPlayers": ["Player"]
	},

	"select-trading-goods": {
		"getInfo": function(entStates)
		{
			if (entStates.every(entState => !entState.market))
				return false;

			return {
				"tooltip": translate("Barter & Trade"),
				"icon": "economics.png",
				"enabled": true
			};
		},
		"execute": function()
		{
			g_TradeDialog.toggle();
		},
		"allowedPlayers": ["Player"]
	},

	"patrol": {
		"getInfo": function(entStates)
		{
			if (!entStates.some(entState => entState.unitAI && entState.unitAI.canPatrol))
				return false;

			return {
				"tooltip": colorizeHotkey("%(hotkey)s" + " ", "session.patrol") +
				           translate("Patrol") + "\n" +
				           translate("Attack all encountered enemy units while avoiding structures."),
				"icon": "patrol.png",
				"enabled": true
			};
		},
		"execute": function()
		{
			inputState = INPUT_PRESELECTEDACTION;
			preSelectedAction = ACTION_PATROL;
		},
		"allowedPlayers": ["Player"]
	},

	"share-dropsite": {
		"getInfo": function(entStates)
		{
			let sharableEntities = entStates.filter(
				entState => entState.resourceDropsite && entState.resourceDropsite.sharable);
			if (!sharableEntities.length)
				return false;

			// Returns if none of the entities belong to a player with a mutual ally.
			if (entStates.every(entState => !GetSimState().players[entState.player].isMutualAlly.some(
				(isAlly, playerId) => isAlly && playerId != entState.player)))
				return false;

			return sharableEntities.some(entState => !entState.resourceDropsite.shared) ?
				{
					"tooltip": translate("Press to allow allies to use this dropsite"),
					"icon": "locked_small.png",
					"enabled": true
				} :
				{
					"tooltip": translate("Press to prevent allies from using this dropsite"),
					"icon": "unlocked_small.png",
					"enabled": true
				};
		},
		"execute": function(entStates)
		{
			let sharableEntities = entStates.filter(
				entState => entState.resourceDropsite && entState.resourceDropsite.sharable);
			if (sharableEntities)
				Engine.PostNetworkCommand({
					"type": "set-dropsite-sharing",
					"entities": sharableEntities.map(entState => entState.id),
					"shared": sharableEntities.some(entState => !entState.resourceDropsite.shared)
				});
		},
		"allowedPlayers": ["Player"]
	},

	"is-dropsite-shared": {
		"getInfo": function(entStates)
		{
			let shareableEntities = entStates.filter(
				entState => entState.resourceDropsite && entState.resourceDropsite.sharable);
			if (!shareableEntities.length)
				return false;

			let player = Engine.GetPlayerID();
			let simState = GetSimState();
			if (!g_IsObserver && !simState.players[player].hasSharedDropsites ||
				shareableEntities.every(entState => controlsPlayer(entState.player)))
				return false;

			if (!shareableEntities.every(entState => entState.resourceDropsite.shared))
				return {
					"tooltip": translate("The use of this dropsite is prohibited"),
					"icon": "locked_small.png",
					"enabled": false
				};

			return {
				"tooltip": g_IsObserver ? translate("Allies are allowed to use this dropsite.") :
					translate("You are allowed to use this dropsite"),
				"icon": "unlocked_small.png",
				"enabled": false
			};
		},
		"execute": function(entState)
		{
			// This command button is always disabled.
		},
		"allowedPlayers": ["Ally", "Observer"]
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

/**
 * Checks whether the entities have the right diplomatic status
 * with respect to the currently active player.
 * Also "Observer" can be used.
 *
 * @param {Object[]} entStates - An array containing the entity states to check.
 * @param {string[]} validPlayers - An array containing the diplomatic statuses.
 *
 * @return {boolean} - Whether the currently active player is allowed.
 */
function allowedPlayersCheck(entStates, validPlayers)
{
	// Assume we can only select entities from one player,
	// or it does not matter (e.g. observer).
	let targetState = entStates[0];
	let playerState = GetSimState().players[Engine.GetPlayerID()];

	return validPlayers.some(player =>
		player == "Observer" && g_IsObserver ||
		player == "Player" && controlsPlayer(targetState.player) ||
		playerState && playerState["is" + player] && playerState["is" + player][targetState.player]);
}

function hasClass(entState, className)
{
	// note: use the functions in globalscripts/Templates.js for more versatile matching
	return entState.identity && entState.identity.classes.indexOf(className) != -1;
}

/**
 * Keep in sync with Commands.js.
 */
function isUndeletable(entState)
{
	let playerState = g_SimState.players[entState.player];
	if (playerState && playerState.controlsAll)
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

function getCommandInfo(command, entStates)
{
	return entStates && g_EntityCommands[command] &&
		allowedPlayersCheck(entStates, g_EntityCommands[command].allowedPlayers) &&
		g_EntityCommands[command].getInfo(entStates);
}

function getActionInfo(action, target, selection)
{
	if (!selection || !selection.length || !GetEntityState(selection[0]))
		return { "possible": false };

	// Look at the first targeted entity
	// (TODO: maybe we eventually want to look at more, and be more context-sensitive?
	// e.g. prefer to attack an enemy unit, even if some friendly units are closer to the mouse)
	let targetState = GetEntityState(target);

	let simState = GetSimState();
	let playerState = g_SimState.players[g_ViewedPlayer];

	// Check if any entities in the selection can do some of the available actions.
	for (let entityID of selection)
	{
		let entState = GetEntityState(entityID);
		if (!entState)
			continue;

		if (playerState && !playerState.controlsAll && !entState.identity.controllable)
			continue;

		if (g_UnitActions[action] && g_UnitActions[action].getActionInfo)
		{
			let r = g_UnitActions[action].getActionInfo(entState, targetState, simState);
			if (r && r.possible)
			{
				r.entity = entityID;
				return r;
			}
		}
	}
	return { "possible": false };
}
