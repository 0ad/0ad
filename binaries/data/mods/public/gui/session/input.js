const SDL_BUTTON_LEFT = 1;
const SDL_BUTTON_MIDDLE = 2;
const SDL_BUTTON_RIGHT = 3;
const SDLK_LEFTBRACKET = 91;
const SDLK_RIGHTBRACKET = 93;
const SDLK_RSHIFT = 303;
const SDLK_LSHIFT = 304;
const SDLK_RCTRL = 305;
const SDLK_LCTRL = 306;
const SDLK_RALT = 307;
const SDLK_LALT = 308;
// TODO: these constants should be defined somewhere else instead, in
// case any other code wants to use them too

const ACTION_NONE = 0;
const ACTION_GARRISON = 1;
const ACTION_REPAIR = 2;
var preSelectedAction = ACTION_NONE;

const INPUT_NORMAL = 0;
const INPUT_SELECTING = 1;
const INPUT_BANDBOXING = 2;
const INPUT_BUILDING_PLACEMENT = 3;
const INPUT_BUILDING_CLICK = 4;
const INPUT_BUILDING_DRAG = 5;
const INPUT_BATCHTRAINING = 6;
const INPUT_PRESELECTEDACTION = 7;
const INPUT_BUILDING_WALL_CLICK = 8;
const INPUT_BUILDING_WALL_PATHING = 9;

var inputState = INPUT_NORMAL;
var placementSupport = new PlacementSupport();

var mouseX = 0;
var mouseY = 0;
var mouseIsOverObject = false;

// Number of pixels the mouse can move before the action is considered a drag
var maxDragDelta = 4;

// Time in milliseconds in which a double click is recognized
const doubleClickTime = 500;
var doubleClickTimer = 0;
var doubleClicked = false;
// Store the previously clicked entity - ensure a double/triple click happens on the same entity
var prevClickedEntity = 0;

// Same double-click behaviour for hotkey presses
const doublePressTime = 500;
var doublePressTimer = 0;
var prevHotkey = 0;

function updateCursorAndTooltip()
{
	var cursorSet = false;
	var tooltipSet = false;
	var informationTooltip = getGUIObjectByName("informationTooltip");
	if (!mouseIsOverObject)
	{
		var action = determineAction(mouseX, mouseY);
		if (inputState == INPUT_NORMAL || inputState == INPUT_PRESELECTEDACTION)
		{
			if (action)
			{
				if (action.cursor)
				{
					Engine.SetCursor(action.cursor);
					cursorSet = true;
				}
				if (action.tooltip)
				{
					tooltipSet = true;
					informationTooltip.caption = action.tooltip;
					informationTooltip.hidden = false;
				}
			}
		}
	}

	if (!cursorSet)
		Engine.SetCursor("arrow-default");
	if (!tooltipSet)
		informationTooltip.hidden = true;
	
	var wallDragTooltip = getGUIObjectByName("wallDragTooltip");
	if (placementSupport.wallDragTooltip)
	{
		wallDragTooltip.caption = placementSupport.wallDragTooltip;
		wallDragTooltip.hidden = false;
	}
	else
	{
		wallDragTooltip.caption = "";
		wallDragTooltip.hidden = true;
	}
}

function updateBuildingPlacementPreview()
{
	// The preview should be recomputed every turn, so that it responds to obstructions/fog/etc moving underneath it, or
	// in the case of the wall previews, in response to new tower foundations getting constructed for it to snap to.
	// See onSimulationUpdate in session.js.

	if (placementSupport.mode === "building")
	{
		if (placementSupport.template && placementSupport.position)
		{
			return Engine.GuiInterfaceCall("SetBuildingPlacementPreview", {
				"template": placementSupport.template,
				"x": placementSupport.position.x,
				"z": placementSupport.position.z,
				"angle": placementSupport.angle,
			});
		}
	}
	else if (placementSupport.mode === "wall")
	{
		if (placementSupport.wallSet && placementSupport.position)
		{
			// Fetch an updated list of snapping candidate entities
			placementSupport.wallSnapEntities = Engine.PickSimilarFriendlyEntities(
				placementSupport.wallSet.templates.tower,
				placementSupport.wallSnapEntitiesIncludeOffscreen,
				true, // require exact template match
				true  // include foundations
			);
			
			return Engine.GuiInterfaceCall("SetWallPlacementPreview", {
				"wallSet": placementSupport.wallSet,
				"start": placementSupport.position,
				"end": placementSupport.wallEndPosition,
				"snapEntities": placementSupport.wallSnapEntities,	// snapping entities (towers) for starting a wall segment
			});
		}
	}

	return false;
}

function findGatherType(gatherer, supply)
{
	if (!gatherer || !supply)
		return undefined;
	if (gatherer[supply.type.generic+"."+supply.type.specific])
		return supply.type.specific;
	if (gatherer[supply.type.generic])
		return supply.type.generic;
	return undefined;
}

function getActionInfo(action, target)
{
	var selection = g_Selection.toList();

	// If the selection doesn't exist, no action
	var entState = GetEntityState(selection[0]);
	if (!entState)
		return {"possible": false};

	// If the selection isn't friendly units, no action
	var playerID = Engine.GetPlayerID();
	var allOwnedByPlayer = selection.every(function(ent) {
		var entState = GetEntityState(ent);
		return entState && entState.player == playerID;
	});

	if (!g_DevSettings.controlAll && !allOwnedByPlayer)
		return {"possible": false};

	// Work out whether the selection can have rally points
	var haveRallyPoints = selection.every(function(ent) {
		var entState = GetEntityState(ent);
		return entState && entState.rallyPoint;
	});

	if (!target)
	{
		if (action == "set-rallypoint" && haveRallyPoints)
			return {"possible": true};
		else if (action == "move")
			return {"possible": true};
		else
			return {"possible": false};
	}

	if (haveRallyPoints && selection.indexOf(target) != -1 && action == "unset-rallypoint")
		return {"possible": true};

	// Look at the first targeted entity
	// (TODO: maybe we eventually want to look at more, and be more context-sensitive?
	// e.g. prefer to attack an enemy unit, even if some friendly units are closer to the mouse)
	var targetState = GetEntityState(target);

	// Check if the target entity is a resource, dropsite, foundation, or enemy unit.
	// Check if any entities in the selection can gather the requested resource,
	// can return to the dropsite, can build the foundation, or can attack the enemy
	var simState = Engine.GuiInterfaceCall("GetSimulationState");

	// Look to see what type of command units going to the rally point should use
	if (haveRallyPoints && action == "set-rallypoint")
	{
		// haveRallyPoints ensures all selected entities can have rally points.
		// We assume that all entities are owned by the same player.
		var entState = GetEntityState(selection[0]);

		var playerState = simState.players[entState.player];
		var playerOwned = (targetState.player == entState.player);
		var allyOwned = playerState.isAlly[targetState.player];
		var enemyOwned = playerState.isEnemy[targetState.player];
		var gaiaOwned = (targetState.player == 0);

		var cursor = "";

		// default to walking there
		var data = {command: "walk"};
		if (targetState.garrisonHolder && playerOwned)
		{
			// Don't allow the rally point to be set on any of the currently selected units
			for (var i = 0; i < selection.length; i++)
			{
				if (target === selection[i])
				{
					return {"possible": false};
				}
			}
			data.command = "garrison";
			data.target = target;
			cursor = "action-garrison";
		}
		else if (targetState.resourceSupply)
		{
			var resourceType = targetState.resourceSupply.type;
			if (resourceType.generic == "treasure")
			{
				cursor = "action-gather-" + resourceType.generic;
			}
			else
			{
				cursor = "action-gather-" + resourceType.specific;
			}
			data.command = "gather";
			data.resourceType = resourceType;
			data.resourceTemplate = targetState.template;
		}
		else if (targetState.foundation && entState.buildEntities)
		{
			data.command = "build";
			data.target = target;
			cursor = "action-build";
		}
		else if (targetState.needsRepair && allyOwned)
		{
			data.command = "repair";
			data.target = target;
			cursor = "action-repair";
		}
		
		return {"possible": true, "data": data, "position": targetState.position, "cursor": cursor};
	}

	for each (var entityID in selection)
	{
		var entState = GetEntityState(entityID);
		if (!entState)
			continue;

		var playerState = simState.players[entState.player];
		var playerOwned = (targetState.player == entState.player);
		var allyOwned = playerState.isAlly[targetState.player];
		var enemyOwned = playerState.isEnemy[targetState.player];
		var gaiaOwned = (targetState.player == 0);

		// Find the resource type we're carrying, if any
		var carriedType = undefined;
		if (entState.resourceCarrying && entState.resourceCarrying.length)
			carriedType = entState.resourceCarrying[0].type;
		switch (action)
		{
		case "garrison":
			if (hasClass(entState, "Unit") && targetState.garrisonHolder && playerOwned)
			{
				var allowedClasses = targetState.garrisonHolder.allowedClasses;
				for each (var unitClass in entState.identity.classes)
				{
					if (allowedClasses.indexOf(unitClass) != -1)
					{
						return {"possible": true};
					}
				}
			}
			break;
		case "setup-trade-route":
			// If ground or sea trade possible
			if (!targetState.foundation && ((entState.trader && hasClass(entState, "Organic") && (playerOwned || allyOwned) && hasClass(targetState, "Market")) ||
				(entState.trader && hasClass(entState, "Ship") && (playerOwned || allyOwned) && hasClass(targetState, "NavalMarket"))))
			{
				var tradingData = {"trader": entState.id, "target": target};
				var tradingDetails = Engine.GuiInterfaceCall("GetTradingDetails", tradingData);
				var tooltip;
				if (tradingDetails === null)
					return {"possible": false};
				switch (tradingDetails.type)
				{
				case "is first":
					tooltip = "First trade market.";
					if (tradingDetails.hasBothMarkets)
						tooltip += " Gain: " + tradingDetails.gain + " " + tradingDetails.goods + ". Click to establish another route."
					else
						tooltip += " Click on another market to establish a trade route."
					break;
				case "is second":
					tooltip = "Second trade market. Gain: " + tradingDetails.gain + " " + tradingDetails.goods + "." + " Click to establish another route.";
					break;
				case "set first":
					tooltip = "Set as first trade market";
					break;
				case "set second":
					tooltip = "Set as second trade market. Gain: " + tradingDetails.gain + " " + tradingDetails.goods + ".";
					break;
				}
				return {"possible": true, "tooltip": tooltip};
			}
			break;
		case "heal":
			// The check if the target is unhealable is done by targetState.needsHeal
			if (entState.Healer && hasClass(targetState, "Unit") && targetState.needsHeal && (playerOwned || allyOwned))
			{
				var unhealableClasses = entState.Healer.unhealableClasses;
				for each (var unitClass in targetState.identity.classes)
				{
					if (unhealableClasses.indexOf(unitClass) != -1)
					{
						return {"possible": false};
					}
				}
				
				var healableClasses = entState.Healer.healableClasses;
				for each (var unitClass in targetState.identity.classes)
				{
					if (healableClasses.indexOf(unitClass) != -1)
					{
						return {"possible": true};
					}
				}
			}
			break;
		case "gather":
			if (targetState.resourceSupply)
			{
				var resource = findGatherType(entState.resourceGatherRates, targetState.resourceSupply);
				if (resource)
					return {"possible": true, "cursor": "action-gather-" + resource};
			}
			break;
		case "returnresource":
			if (targetState.resourceDropsite && playerOwned && carriedType && targetState.resourceDropsite.types.indexOf(carriedType) != -1)
				return {"possible": true, "cursor": "action-return-" + carriedType};
			break;
		case "build":
			if (targetState.foundation && entState.buildEntities && playerOwned)
				return {"possible": true};
			break;
		case "repair":
			if (entState.buildEntities && targetState.needsRepair && allyOwned)
				return {"possible": true};
			break;
		case "attack":
			if (entState.attack && targetState.hitpoints && enemyOwned)
				return {"possible": Engine.GuiInterfaceCall("CanAttack", {"entity": entState.id, "target": target})};
			break;
		}
	}
	if (action == "move")
		return {"possible": true};
	else
		return {"possible": false};
}

/**
 * Determine the context-sensitive action that should be performed when the mouse is at (x,y)
 */
function determineAction(x, y, fromMinimap)
{
	var selection = g_Selection.toList();

	// No action if there's no selection
	if (!selection.length)
	{
		preSelectedAction = ACTION_NONE;
		return undefined;
	}

	// If the selection doesn't exist, no action
	var entState = GetEntityState(selection[0]);
	if (!entState)
		return undefined;

	// If the selection isn't friendly units, no action
	var playerID = Engine.GetPlayerID();
	var allOwnedByPlayer = selection.every(function(ent) {
		var entState = GetEntityState(ent);
		return entState && entState.player == playerID;
	});

	if (!g_DevSettings.controlAll && !allOwnedByPlayer)
		return undefined;

	// Work out whether the selection can have rally points
	var haveRallyPoints = selection.every(function(ent) {
		var entState = GetEntityState(ent);
		return entState && entState.rallyPoint;
	});

	var targets = [];
	var target = undefined;
	var type = "none";
	var cursor = "";
	var targetState = undefined;
	if (!fromMinimap)
		targets = Engine.PickEntitiesAtPoint(x, y);

	if (targets.length)
	{
		target = targets[0];
	}

	if (preSelectedAction != ACTION_NONE)
	{
		switch (preSelectedAction)
		{
		case ACTION_GARRISON:
			if (getActionInfo("garrison", target).possible)
				return {"type": "garrison", "cursor": "action-garrison", "target": target};
			else
				return 	{"type": "none", "cursor": "action-garrison-disabled", "target": undefined};
			break;
		case ACTION_REPAIR:
			if (getActionInfo("repair", target).possible)
				return {"type": "repair", "cursor": "action-repair", "target": target};
			else
				return {"type": "none", "cursor": "action-repair-disabled", "target": undefined};
			break;
		}
	}
	else if (Engine.HotkeyIsPressed("session.garrison"))
	{
		if (getActionInfo("garrison", target).possible)
			return {"type": "garrison", "cursor": "action-garrison", "target": target};
		else
			return 	{"type": "none", "cursor": "action-garrison-disabled", "target": undefined};
	}
	else
	{
		var actionInfo = undefined;
		if ((actionInfo = getActionInfo("setup-trade-route", target)).possible)
			return {"type": "setup-trade-route", "cursor": "action-setup-trade-route", "tooltip": actionInfo.tooltip, "target": target};
		else if ((actionInfo = getActionInfo("gather", target)).possible)
			return {"type": "gather", "cursor": actionInfo.cursor, "target": target};
		else if ((actionInfo = getActionInfo("returnresource", target)).possible)
			return {"type": "returnresource", "cursor": actionInfo.cursor, "target": target};
		else if (getActionInfo("build", target).possible)
			return {"type": "build", "cursor": "action-build", "target": target};
		else if (getActionInfo("repair", target).possible)
			return {"type": "build", "cursor": "action-repair", "target": target};
		else if ((actionInfo = getActionInfo("set-rallypoint", target)).possible)
			return {"type": "set-rallypoint", "cursor": actionInfo.cursor, "data": actionInfo.data, "position": actionInfo.position};
		else if (getActionInfo("heal", target).possible)
			return {"type": "heal", "cursor": "action-heal", "target": target};
		else if (getActionInfo("attack", target).possible)
			return {"type": "attack", "cursor": "action-attack", "target": target};
		else if (getActionInfo("unset-rallypoint", target).possible)
			return {"type": "unset-rallypoint"};
		else if (getActionInfo("move", target).possible)
			return {"type": "move"};
	}
	return {"type": type, "cursor": cursor, "target": target};
}


var dragStart; // used for remembering mouse coordinates at start of drag operations

function tryPlaceBuilding(queued)
{
	if (placementSupport.mode !== "building")
	{
		error("[tryPlaceBuilding] Called while in '"+placementSupport.mode+"' placement mode instead of 'building'");
		return false;
	}
	
	var selection = g_Selection.toList();

	// Use the preview to check it's a valid build location
	if (!updateBuildingPlacementPreview())
	{
		// invalid location - don't build it
		// TODO: play a sound?
		return false;
	}

	// Start the construction
	Engine.PostNetworkCommand({
		"type": "construct",
		"template": placementSupport.template,
		"x": placementSupport.position.x,
		"z": placementSupport.position.z,
		"angle": placementSupport.angle,
		"entities": selection,
		"autorepair": true,
		"autocontinue": true,
		"queued": queued
	});
	Engine.GuiInterfaceCall("PlaySound", { "name": "order_repair", "entity": selection[0] });

	if (!queued)
		placementSupport.Reset();

	return true;
}

function tryPlaceWall()
{
	if (placementSupport.mode !== "wall")
	{
		error("[tryPlaceWall] Called while in '" + placementSupport.mode + "' placement mode; expected 'wall' mode");
		return false;
	}
	
	var wallPlacementInfo = updateBuildingPlacementPreview(); // entities making up the wall (wall segments, towers, ...)
	if (!(wallPlacementInfo === false || typeof(wallPlacementInfo) === "object"))
	{
		error("[tryPlaceWall] Unexpected return value from updateBuildingPlacementPreview: '" + uneval(placementInfo) + "'; expected either 'false' or 'object'");
		return false;
	}
	
	if (!wallPlacementInfo)
		return false;
	
	var selection = g_Selection.toList();
	var cmd = {
		"type": "construct-wall",
		"autorepair": true,
		"autocontinue": true,
		"queued": true,
		"entities": selection,
		"wallSet": placementSupport.wallSet,
		"pieces": wallPlacementInfo.pieces,
		"startSnappedEntity": wallPlacementInfo.startSnappedEnt,
		"endSnappedEntity": wallPlacementInfo.endSnappedEnt,
	};
	
	// make sure that there's at least one non-tower entity getting built, to prevent silly edge cases where the start and end
	// point are too close together for the algorithm to place a wall segment inbetween, and only the towers are being previewed
	// (this is somewhat non-ideal and hardcode-ish)
	var hasWallSegment = false;
	for (var k in cmd.pieces)
	{
		if (cmd.pieces[k].template != cmd.wallSet.templates.tower) // TODO: hardcode-ish :(
		{
			hasWallSegment = true;
			break;
		}
	}
	
	if (hasWallSegment)
	{
		Engine.PostNetworkCommand(cmd);
		Engine.GuiInterfaceCall("PlaySound", {"name": "order_repair", "entity": selection[0] });
	}

	return true;
}

// Limits bandboxed selections to certain types of entities based on priority
function getPreferredEntities(ents)
{
	var entStateList = [];
	var preferredEnts = [];

	// Check if there are units in the selection and get a list of entity states
	for each (var ent in ents)
	{
		var entState = GetEntityState(ent);
		if (!entState)
			continue;
		if (hasClass(entState, "Unit"))
			preferredEnts.push(ent);

		entStateList.push(entState);
	}

	// If there are no units, check if there are defensive entities in the selection
	if (!preferredEnts.length)
		for (var i = 0; i < ents.length; i++)
			if (hasClass(entStateList[i], "Defensive"))
				preferredEnts.push(ents[i]);

	return preferredEnts;
}

// Removes any support units from the passed list of entities
function getMilitaryEntities(ents)
{
	var militaryEnts = [];
	for each (var ent in ents)
	{
		var entState = GetEntityState(ent);
		if (!hasClass(entState, "Support"))
			militaryEnts.push(ent);
	}
	return militaryEnts;
}

function handleInputBeforeGui(ev, hoveredObject)
{
	// Capture mouse position so we can use it for displaying cursors,
	// and key states
	switch (ev.type)
	{
	case "mousebuttonup":
	case "mousebuttondown":
	case "mousemotion":
		mouseX = ev.x;
		mouseY = ev.y;
		break;
	}

	// Remember whether the mouse is over a GUI object or not
	mouseIsOverObject = (hoveredObject != null);

	// Close the menu when interacting with the game world
	if (!mouseIsOverObject && (ev.type =="mousebuttonup" || ev.type == "mousebuttondown")
	    && (ev.button == SDL_BUTTON_LEFT || ev.button == SDL_BUTTON_RIGHT))
		closeMenu();

	// State-machine processing:
	//
	// (This is for states which should override the normal GUI processing - events will
	// be processed here before being passed on, and propagation will stop if this function
	// returns true)
	//
	// TODO: it'd probably be nice to have a better state-machine system, with guaranteed
	// entry/exit functions, since this is a bit broken now

	switch (inputState)
	{
	case INPUT_BANDBOXING:
		switch (ev.type)
		{
		case "mousemotion":
			var x0 = dragStart[0];
			var y0 = dragStart[1];
			var x1 = ev.x;
			var y1 = ev.y;
			if (x0 > x1) { var t = x0; x0 = x1; x1 = t; }
			if (y0 > y1) { var t = y0; y0 = y1; y1 = t; }

			var bandbox = getGUIObjectByName("bandbox");
			bandbox.size = [x0, y0, x1, y1].join(" ");
			bandbox.hidden = false;

			// TODO: Should we handle "control all units" here as well?
			var ents = Engine.PickFriendlyEntitiesInRect(x0, y0, x1, y1, Engine.GetPlayerID());
			g_Selection.setHighlightList(ents);

			return false;

		case "mousebuttonup":
			if (ev.button == SDL_BUTTON_LEFT)
			{
				var x0 = dragStart[0];
				var y0 = dragStart[1];
				var x1 = ev.x;
				var y1 = ev.y;
				if (x0 > x1) { var t = x0; x0 = x1; x1 = t; }
				if (y0 > y1) { var t = y0; y0 = y1; y1 = t; }

				var bandbox = getGUIObjectByName("bandbox");
				bandbox.hidden = true;

				// Get list of entities limited to preferred entities
				// TODO: Should we handle "control all units" here as well?
				var ents = Engine.PickFriendlyEntitiesInRect(x0, y0, x1, y1, Engine.GetPlayerID());
				var preferredEntities = getPreferredEntities(ents)

				if (preferredEntities.length)
				{
 					ents = preferredEntities;

					if (Engine.HotkeyIsPressed("selection.milonly"))
					{
						var militaryEntities = getMilitaryEntities(ents);
						if (militaryEntities.length)
							ents = militaryEntities;
					}
				}

				// Remove the bandbox hover highlighting
				g_Selection.setHighlightList([]);

				// Update the list of selected units
				if (Engine.HotkeyIsPressed("selection.add"))
				{
					g_Selection.addList(ents);
				}
				else if (Engine.HotkeyIsPressed("selection.remove"))
				{
					g_Selection.removeList(ents);
				}
				else
				{
					g_Selection.reset();
					g_Selection.addList(ents);
				}

				inputState = INPUT_NORMAL;
				return true;
			}
			else if (ev.button == SDL_BUTTON_RIGHT)
			{
				// Cancel selection
				var bandbox = getGUIObjectByName("bandbox");
				bandbox.hidden = true;

				g_Selection.setHighlightList([]);

				inputState = INPUT_NORMAL;
				return true;
			}
			break;
		}
		break;

	case INPUT_BUILDING_CLICK:
		switch (ev.type)
		{
		case "mousemotion":
			// If the mouse moved far enough from the original click location,
			// then switch to drag-orientation mode
			var dragDeltaX = ev.x - dragStart[0];
			var dragDeltaY = ev.y - dragStart[1];
			var maxDragDelta = 16;
			if (Math.abs(dragDeltaX) >= maxDragDelta || Math.abs(dragDeltaY) >= maxDragDelta)
			{
				inputState = INPUT_BUILDING_DRAG;
				return false;
			}
			break;

		case "mousebuttonup":
			if (ev.button == SDL_BUTTON_LEFT)
			{
				// If shift is down, let the player continue placing another of the same building
				var queued = Engine.HotkeyIsPressed("session.queue");
				if (tryPlaceBuilding(queued))
				{
					if (queued)
						inputState = INPUT_BUILDING_PLACEMENT;
					else
						inputState = INPUT_NORMAL;
				}
				else
				{
					inputState = INPUT_BUILDING_PLACEMENT;
				}
				return true;
			}
			break;

		case "mousebuttondown":
			if (ev.button == SDL_BUTTON_RIGHT)
			{
				// Cancel building
				placementSupport.Reset();
				inputState = INPUT_NORMAL;
				return true;
			}
			break;
		}
		break;
		
	case INPUT_BUILDING_WALL_CLICK:
		// User is mid-click in choosing a starting point for building a wall. The build process can still be cancelled at this point
		// by right-clicking; releasing the left mouse button will 'register' the starting point and commence endpoint choosing mode. 
		switch (ev.type)
		{
		case "mousebuttonup":
			if (ev.button === SDL_BUTTON_LEFT)
			{
    			inputState = INPUT_BUILDING_WALL_PATHING;
    			return true;
			}
			break;
			
		case "mousebuttondown":
			if (ev.button == SDL_BUTTON_RIGHT)
			{
				// Cancel building
				placementSupport.Reset();
				updateBuildingPlacementPreview();
				
				inputState = INPUT_NORMAL;
				return true;
			}
			break;
		}
		break;

	case INPUT_BUILDING_WALL_PATHING:
		// User has chosen a starting point for constructing the wall, and is now looking to set the endpoint.
		// Right-clicking cancels wall building mode, left-clicking sets the endpoint and builds the wall and returns to
		// normal input mode. Optionally, shift + left-clicking does not return to normal input, and instead allows the 
		// user to continue building walls.
		switch (ev.type)
		{
			case "mousemotion":
				placementSupport.wallEndPosition = Engine.GetTerrainAtScreenPoint(ev.x, ev.y);
				
				// Update the building placement preview, and by extension, the list of snapping candidate entities for both (!)
				// the ending point and the starting point to snap to.
				// 
				// TODO: Note that here, we need to fetch all similar entities, including any offscreen ones, to support the case
				// where the snap entity for the starting point has moved offscreen, or has been deleted/destroyed, or was a
				// foundation and has been replaced with a completed entity since the user first chose it. Fetching all towers on
				// the entire map instead of only the current screen might get expensive fast since walls all have a ton of towers
				// in them. Might be useful to query only for entities within a certain range around the starting point and ending
				// points.
				
				placementSupport.wallSnapEntitiesIncludeOffscreen = true;
				var result = updateBuildingPlacementPreview(); // includes an update of the snap entity candidates
				
				if (result && result.cost)
				{
					placementSupport.wallDragTooltip = "";
					for (var resource in result.cost)
					{
						if (result.cost[resource] > 0)
							placementSupport.wallDragTooltip += getCostComponentDisplayName(resource) + ": " + result.cost[resource] + "\n";
					}
				}
				
				break;
				
			case "mousebuttondown":
				if (ev.button == SDL_BUTTON_LEFT)
				{
					if (tryPlaceWall())
					{
    					if (Engine.HotkeyIsPressed("session.queue"))
    					{
    						// continue building, just set a new starting position where we left off
    						placementSupport.position = placementSupport.wallEndPosition;
    						placementSupport.wallEndPosition = undefined;
    						
    						inputState = INPUT_BUILDING_WALL_CLICK;
    					}
    					else
    					{
    						placementSupport.Reset();
    						inputState = INPUT_NORMAL;
    					}
					}
					else
					{
						placementSupport.wallDragTooltip = "Cannot build wall here!";
					}
					
					updateBuildingPlacementPreview();
					return true;
				}
				else if (ev.button == SDL_BUTTON_RIGHT)
				{
					// reset to normal input mode
					placementSupport.Reset();
					updateBuildingPlacementPreview();
					
					inputState = INPUT_NORMAL;
					return true;
				}
				break;
		}
		break;
		
	case INPUT_BUILDING_DRAG:
		switch (ev.type)
		{
		case "mousemotion":
			var dragDeltaX = ev.x - dragStart[0];
			var dragDeltaY = ev.y - dragStart[1];
			var maxDragDelta = 16;
			if (Math.abs(dragDeltaX) >= maxDragDelta || Math.abs(dragDeltaY) >= maxDragDelta)
			{
				// Rotate in the direction of the mouse
				var target = Engine.GetTerrainAtScreenPoint(ev.x, ev.y);
				placementSupport.angle = Math.atan2(target.x - placementSupport.position.x, target.z - placementSupport.position.z);
			}
			else
			{
				// If the mouse is near the center, snap back to the default orientation
				placementSupport.SetDefaultAngle();
			}

			var snapData = Engine.GuiInterfaceCall("GetFoundationSnapData", {
				"template": placementSupport.template,
				"x": placementSupport.position.x,
				"z": placementSupport.position.z
			});
			if (snapData)
			{
				placementSupport.angle = snapData.angle;
				placementSupport.position.x = snapData.x;
				placementSupport.position.z = snapData.z;
			}

			updateBuildingPlacementPreview();
			break;

		case "mousebuttonup":
			if (ev.button == SDL_BUTTON_LEFT)
			{
				// If shift is down, let the player continue placing another of the same building
				var queued = Engine.HotkeyIsPressed("session.queue");
				if (tryPlaceBuilding(queued))
				{
					if (queued)
						inputState = INPUT_BUILDING_PLACEMENT;
					else
						inputState = INPUT_NORMAL;
				}
				else
				{
					inputState = INPUT_BUILDING_PLACEMENT;
				}
				return true;
			}
			break;

		case "mousebuttondown":
			if (ev.button == SDL_BUTTON_RIGHT)
			{
				// Cancel building
				placementSupport.Reset();
				inputState = INPUT_NORMAL;
				return true;
			}
			break;
		}
		break;

	case INPUT_BATCHTRAINING:
		switch (ev.type)
		{
		case "hotkeyup":
			if (ev.hotkey == "session.batchtrain")
			{
				flushTrainingBatch();
				inputState = INPUT_NORMAL;
			}
			break;
		}
	}

	return false;
}

function handleInputAfterGui(ev)
{
	// Handle the time-warp testing features, restricted to single-player
	if (!g_IsNetworked && getGUIObjectByName("devTimeWarp").checked)
	{
		if (ev.type == "hotkeydown" && ev.hotkey == "timewarp.fastforward")
			Engine.SetSimRate(20.0);
		else if (ev.type == "hotkeyup" && ev.hotkey == "timewarp.fastforward")
			Engine.SetSimRate(1.0);
		else if (ev.type == "hotkeyup" && ev.hotkey == "timewarp.rewind")
			Engine.RewindTimeWarp();
	}

	// State-machine processing:

	switch (inputState)
	{
	case INPUT_NORMAL:
		switch (ev.type)
		{
		case "mousemotion":
			// Highlight the first hovered entity (if any)
			var ents = Engine.PickEntitiesAtPoint(ev.x, ev.y);
			if (ents.length)
				g_Selection.setHighlightList([ents[0]]);
			else
				g_Selection.setHighlightList([]);

			return false;

		case "mousebuttondown":
			if (ev.button == SDL_BUTTON_LEFT)
			{
				dragStart = [ ev.x, ev.y ];
				inputState = INPUT_SELECTING;
				return true;
			}
			else if (ev.button == SDL_BUTTON_RIGHT)
			{
				var action = determineAction(ev.x, ev.y);
				if (!action)
					break;
				return doAction(action, ev);
			}
			break;

		case "hotkeydown":
				if (ev.hotkey.indexOf("selection.group.") == 0)
				{
					var now = new Date();
					if ((now.getTime() - doublePressTimer < doublePressTime) && (ev.hotkey == prevHotkey))
					{
						if (ev.hotkey.indexOf("selection.group.select.") == 0)
						{
							var sptr = ev.hotkey.split(".");
							performGroup("snap", sptr[3]);
						}
					}
					else
					{
						var sptr = ev.hotkey.split(".");
						performGroup(sptr[2], sptr[3]);

						doublePressTimer = now.getTime();
						prevHotkey = ev.hotkey;
					}
				}
				break;
		}
		break;
		
	case INPUT_PRESELECTEDACTION:
		switch (ev.type)
		{
		case "mousebuttondown":
			if (ev.button == SDL_BUTTON_LEFT && preSelectedAction != ACTION_NONE)
			{
				var action = determineAction(ev.x, ev.y);
				if (!action)
					break;
				preSelectedAction = ACTION_NONE;
				inputState = INPUT_NORMAL;
				return doAction(action, ev);
			}
			else if (ev.button == SDL_BUTTON_RIGHT && preSelectedAction != ACTION_NONE)
			{
				preSelectedAction = ACTION_NONE;
				inputState = INPUT_NORMAL;
				break;
			}
			// else
		default:
			// Slight hack: If selection is empty, reset the input state
			if (g_Selection.toList().length == 0)
			{
				preSelectedAction = ACTION_NONE;
				inputState = INPUT_NORMAL;
				break;
			}
		}
		break;
		
	case INPUT_SELECTING:
		switch (ev.type)
		{
		case "mousemotion":
			// If the mouse moved further than a limit, switch to bandbox mode
			var dragDeltaX = ev.x - dragStart[0];
			var dragDeltaY = ev.y - dragStart[1];

			if (Math.abs(dragDeltaX) >= maxDragDelta || Math.abs(dragDeltaY) >= maxDragDelta)
			{
				inputState = INPUT_BANDBOXING;
				return false;
			}

			var ents = Engine.PickEntitiesAtPoint(ev.x, ev.y);
			g_Selection.setHighlightList(ents);
			return false;

		case "mousebuttonup":
			if (ev.button == SDL_BUTTON_LEFT)
			{
				var ents = Engine.PickEntitiesAtPoint(ev.x, ev.y);
				if (!ents.length)
				{
					g_Selection.reset();
					resetIdleUnit();
					inputState = INPUT_NORMAL;
					return true;
				}

				var selectedEntity = ents[0];
				var now = new Date();

				// If camera following and we select different unit, stop
				if (Engine.GetFollowedEntity() != selectedEntity)
				{
					Engine.CameraFollow(0);
				}

				if ((now.getTime() - doubleClickTimer < doubleClickTime) && (selectedEntity == prevClickedEntity))
				{
					// Double click or triple click has occurred
					var showOffscreen = Engine.HotkeyIsPressed("selection.offscreen");
					var matchRank = true;
					var templateToMatch;

					// Check for double click or triple click
					if (!doubleClicked)
					{
						// If double click hasn't already occurred, this is a double click.
						// Select similar units regardless of rank
						templateToMatch = Engine.GuiInterfaceCall("GetEntityState", selectedEntity).identity.selectionGroupName;
						if (templateToMatch)
						{
							matchRank = false;
						}
						else
						{	// No selection group name defined, so fall back to exact match
							templateToMatch = Engine.GuiInterfaceCall("GetEntityState", selectedEntity).template;
						}

						doubleClicked = true;
						// Reset the timer so the user has an extra period 'doubleClickTimer' to do a triple-click
						doubleClickTimer = now.getTime();
					}
					else
					{
						// Double click has already occurred, so this is a triple click.
						// Select units matching exact template name (same rank)
						templateToMatch = Engine.GuiInterfaceCall("GetEntityState", selectedEntity).template;
					}

					// TODO: Should we handle "control all units" here as well?
					ents = Engine.PickSimilarFriendlyEntities(templateToMatch, showOffscreen, matchRank, false);
				}
				else
				{
					// It's single click right now but it may become double or triple click
					doubleClicked = false;
					doubleClickTimer = now.getTime();
					prevClickedEntity = selectedEntity;

					// We only want to include the first picked unit in the selection
					ents = [ents[0]];
				}

				// Update the list of selected units
				if (Engine.HotkeyIsPressed("selection.add"))
				{
					g_Selection.addList(ents);
				}
				else if (Engine.HotkeyIsPressed("selection.remove"))
				{
					g_Selection.removeList(ents);
				}
				else
				{
					g_Selection.reset();
					g_Selection.addList(ents);
				}

				inputState = INPUT_NORMAL;
				return true;
			}
			break;
		}
		break;

	case INPUT_BUILDING_PLACEMENT:
		switch (ev.type)
		{
		case "mousemotion":
			
			placementSupport.position = Engine.GetTerrainAtScreenPoint(ev.x, ev.y);
			
			if (placementSupport.mode === "wall")
			{
				// Including only the on-screen towers in the next snap candidate list is sufficient here, since the user is
				// still selecting a starting point (which must necessarily be on-screen). (The update of the snap entities
				// itself happens in the call to updateBuildingPlacementPreview below).
				placementSupport.wallSnapEntitiesIncludeOffscreen = false;
			}
			else
			{
				var snapData = Engine.GuiInterfaceCall("GetFoundationSnapData", {
					"template": placementSupport.template,
					"x": placementSupport.position.x,
					"z": placementSupport.position.z,
				});
				if (snapData)
				{
					placementSupport.angle = snapData.angle;
					placementSupport.position.x = snapData.x;
					placementSupport.position.z = snapData.z;
				}
			}

			updateBuildingPlacementPreview(); // includes an update of the snap entity candidates
			return false; // continue processing mouse motion

		case "mousebuttondown":
			if (ev.button == SDL_BUTTON_LEFT)
			{
				if (placementSupport.mode === "wall")
				{
					var validPlacement = updateBuildingPlacementPreview();
					if (validPlacement !== false)
					{
						inputState = INPUT_BUILDING_WALL_CLICK;
					}
				}
				else
				{
					placementSupport.position = Engine.GetTerrainAtScreenPoint(ev.x, ev.y);
    				dragStart = [ ev.x, ev.y ];
    				inputState = INPUT_BUILDING_CLICK;
				}
				return true;
			}
			else if (ev.button == SDL_BUTTON_RIGHT)
			{
				// Cancel building
				placementSupport.Reset();
				inputState = INPUT_NORMAL;
				return true;
			}
			break;

		case "hotkeydown":

			var rotation_step = Math.PI / 12; // 24 clicks make a full rotation

			switch (ev.hotkey)
			{
			case "session.rotate.cw":
				placementSupport.angle += rotation_step;
				updateBuildingPlacementPreview();
				break;
			case "session.rotate.ccw":
				placementSupport.angle -= rotation_step;
				updateBuildingPlacementPreview();
				break;
			}
			break;

		}
		break;
	}
	return false;
}

function doAction(action, ev)
{
	var selection = g_Selection.toList();

	// If shift is down, add the order to the unit's order queue instead
	// of running it immediately
	var queued = Engine.HotkeyIsPressed("session.queue");

	switch (action.type)
	{
	case "move":
		var target = Engine.GetTerrainAtScreenPoint(ev.x, ev.y);
		Engine.PostNetworkCommand({"type": "walk", "entities": selection, "x": target.x, "z": target.z, "queued": queued});
		Engine.GuiInterfaceCall("PlaySound", { "name": "order_walk", "entity": selection[0] });
		return true;

	case "attack":
		Engine.PostNetworkCommand({"type": "attack", "entities": selection, "target": action.target, "queued": queued});
		Engine.GuiInterfaceCall("PlaySound", { "name": "order_attack", "entity": selection[0] });
		return true;

	case "heal":
		Engine.PostNetworkCommand({"type": "heal", "entities": selection, "target": action.target, "queued": queued});
		Engine.GuiInterfaceCall("PlaySound", { "name": "order_heal", "entity": selection[0] });
		return true;

	case "build": // (same command as repair)
	case "repair":
		Engine.PostNetworkCommand({"type": "repair", "entities": selection, "target": action.target, "autocontinue": true, "queued": queued});
		Engine.GuiInterfaceCall("PlaySound", { "name": "order_repair", "entity": selection[0] });
		return true;

	case "gather":
		Engine.PostNetworkCommand({"type": "gather", "entities": selection, "target": action.target, "queued": queued});
		Engine.GuiInterfaceCall("PlaySound", { "name": "order_gather", "entity": selection[0] });
		return true;

	case "returnresource":
		Engine.PostNetworkCommand({"type": "returnresource", "entities": selection, "target": action.target, "queued": queued});
		Engine.GuiInterfaceCall("PlaySound", { "name": "order_gather", "entity": selection[0] });
		return true;

	case "setup-trade-route":
		Engine.PostNetworkCommand({"type": "setup-trade-route", "entities": selection, "target": action.target});
		Engine.GuiInterfaceCall("PlaySound", { "name": "order_trade", "entity": selection[0] });
		return true;

	case "garrison":
		Engine.PostNetworkCommand({"type": "garrison", "entities": selection, "target": action.target, "queued": queued});
		Engine.GuiInterfaceCall("PlaySound", { "name": "order_garrison", "entity": selection[0] });
		return true;

	case "set-rallypoint":
		var pos = undefined;
		// if there is a position set in the action then use this so that when setting a 
		// rally point on an entity it is centered on that entity
		if (action.position)
		{
			pos = action.position;
		}
		else
		{
			pos = Engine.GetTerrainAtScreenPoint(ev.x, ev.y);
		}
		Engine.PostNetworkCommand({"type": "set-rallypoint", "entities": selection, "x": pos.x, "z": pos.z, "data": action.data, "queued": queued});
		// Display rally point at the new coordinates, to avoid display lag
		Engine.GuiInterfaceCall("DisplayRallyPoint", {
			"entities": selection,
			"x": pos.x,
			"z": pos.z,
			"queued": queued
		});
		return true;

	case "unset-rallypoint":
		var target = Engine.GetTerrainAtScreenPoint(ev.x, ev.y);
		Engine.PostNetworkCommand({"type": "unset-rallypoint", "entities": selection});
		// Remove displayed rally point
		Engine.GuiInterfaceCall("DisplayRallyPoint", {
			"entities": []
		});
		return true;

	case "none":
		return true;

	default:
		error("Invalid action.type "+action.type);
		return false;
	}
}

function handleMinimapEvent(target)
{
	// Partly duplicated from handleInputAfterGui(), but with the input being
	// world coordinates instead of screen coordinates.

	if (inputState == INPUT_NORMAL)
	{
		var fromMinimap = true;
		var action = determineAction(undefined, undefined, fromMinimap);
		if (!action)
			return false;

		var selection = g_Selection.toList();

		var queued = Engine.HotkeyIsPressed("session.queue");

		switch (action.type)
		{
		case "move":
			Engine.PostNetworkCommand({"type": "walk", "entities": selection, "x": target.x, "z": target.z, "queued": queued});
			Engine.GuiInterfaceCall("PlaySound", { "name": "order_walk", "entity": selection[0] });
			return true;

		case "set-rallypoint":
			Engine.PostNetworkCommand({"type": "set-rallypoint", "entities": selection, "x": target.x, "z": target.z});
			// Display rally point at the new coordinates, to avoid display lag
			Engine.GuiInterfaceCall("DisplayRallyPoint", {
				"entities": selection,
				"x": target.x,
				"z": target.z
			});
			return true;

		default:
			error("Invalid action.type "+action.type);
		}
	}
	return false;
}

// Called by GUI when user clicks construction button
// @param buildTemplate Template name of the entity the user wants to build
function startBuildingPlacement(buildTemplate)
{
	// TODO: we should clear any highlight selection rings here. If the mouse was over an entity before going onto the GUI
	// to start building a structure, then the highlight selection rings are kept during the construction of the building.
	// Gives the impression that somehow the hovered-over entity has something to do with the building you're constructing.
	
	placementSupport.Reset();
	
	// find out if we're building a wall, and change the entity appropriately if so
	var templateData = GetTemplateData(buildTemplate);
	if (templateData.wallSet)
	{
		placementSupport.mode = "wall";
		placementSupport.wallSet = templateData.wallSet;
		inputState = INPUT_BUILDING_PLACEMENT;
	}
	else
	{
		placementSupport.mode = "building";
		placementSupport.template = buildTemplate;
		inputState = INPUT_BUILDING_PLACEMENT;
	}
}

// Called by GUI when user changes preferred trading goods
function selectTradingPreferredGoods(data)
{
	Engine.PostNetworkCommand({"type": "select-trading-goods", "entities": data.entities, "preferredGoods": data.preferredGoods});
}

// Called by GUI when user clicks exchange resources button
function exchangeResources(command)
{
	Engine.PostNetworkCommand({"type": "barter", "sell": command.sell, "buy": command.buy, "amount": command.amount});
}

// Batch training:
// When the user shift-clicks, we set these variables and switch to INPUT_BATCHTRAINING
// When the user releases shift, or clicks on a different training button, we create the batched units
var batchTrainingEntities;
var batchTrainingType;
var batchTrainingCount;
const batchIncrementSize = 5;

function flushTrainingBatch()
{
	Engine.PostNetworkCommand({"type": "train", "entities": batchTrainingEntities, "template": batchTrainingType, "count": batchTrainingCount});
}

// Called by GUI when user clicks training button
function addTrainingToQueue(selection, trainEntType)
{
	if (Engine.HotkeyIsPressed("session.batchtrain"))
	{
		if (inputState == INPUT_BATCHTRAINING)
		{
			// Check if we are training in the same building(s) as the last batch
			var sameEnts = false;
			if (batchTrainingEntities.length == selection.length)
			{
				// NOTE: We just check if the arrays are the same and if the order is the same
				// If the order changed, we have a new selection and we should create a new batch.
				for (var i = 0; i < batchTrainingEntities.length; ++i)
				{
					if (!(sameEnts = batchTrainingEntities[i] == selection[i]))
						break;
				}
			}
			// If we're already creating a batch of this unit (in the same building(s)), then just extend it
			if (sameEnts && batchTrainingType == trainEntType)
			{
				batchTrainingCount += batchIncrementSize;
				return;
			}
			// Otherwise start a new one
			else
			{
				flushTrainingBatch();
				// fall through to create the new batch
			}
		}
		inputState = INPUT_BATCHTRAINING;
		batchTrainingEntities = selection;
		batchTrainingType = trainEntType;
		batchTrainingCount = batchIncrementSize;
	}
	else
	{
		// Non-batched - just create a single entity
		Engine.PostNetworkCommand({"type": "train", "template": trainEntType, "count": 1, "entities": selection});
	}
}

// Called by GUI when user clicks research button
function addResearchToQueue(entity, researchType)
{
	Engine.PostNetworkCommand({"type": "research", "entity": entity, "template": researchType});
}

// Returns the number of units that will be present in a batch if the user clicks
// the training button with shift down
function getTrainingBatchStatus(entity, trainEntType)
{
	if (inputState == INPUT_BATCHTRAINING && batchTrainingEntities.indexOf(entity) != -1 && batchTrainingType == trainEntType)
		return [batchTrainingCount, batchIncrementSize];
	else
		return [0, batchIncrementSize];
}

// Called by GUI when user clicks production queue item
function removeFromProductionQueue(entity, id)
{
	Engine.PostNetworkCommand({"type": "stop-production", "entity": entity, "id": id});
}

// Called by unit selection buttons
function changePrimarySelectionGroup(templateName)
{
	if (Engine.HotkeyIsPressed("session.deselectgroup"))
		g_Selection.makePrimarySelection(templateName, true);
	else
		g_Selection.makePrimarySelection(templateName, false);
}

// Performs the specified command (delete, town bell, repair, etc.)
function performCommand(entity, commandName)
{
	if (entity)
	{
		var entState = GetEntityState(entity);
		var template = GetTemplateData(entState.template);
		var unitName = getEntityName(template);

		var playerID = Engine.GetPlayerID();
		if (entState.player == playerID || g_DevSettings.controlAll)
		{
			switch (commandName)
			{
			case "delete":
				var selection = g_Selection.toList();
				if (selection.length > 0)
					openDeleteDialog(selection);
				break;
			case "garrison":
				inputState = INPUT_PRESELECTEDACTION;
				preSelectedAction = ACTION_GARRISON;
				break;
			case "repair":
				inputState = INPUT_PRESELECTEDACTION;
				preSelectedAction = ACTION_REPAIR;
				break;
			case "unload-all":
				unloadAll(entity);
				break;
			case "focus-rally":
				// if the selected building has a rally point set, move the camera to it; otherwise, move to the building itself
				// (since that's where units will spawn without a rally point)
				var focusTarget = null;
				if (entState.rallyPoint && entState.rallyPoint.position)
				{
					focusTarget = entState.rallyPoint.position;
				}
				else
				{
					if (entState.position)
						focusTarget = entState.position;
				}
				
				if (focusTarget !== null)
					Engine.CameraMoveTo(focusTarget.x, focusTarget.z);
				
				break;
			default:
				break;
			}
		}
	}
}

// Performs the specified formation
function performFormation(entity, formationName)
{
	if (entity)
	{
		var selection = g_Selection.toList();
		Engine.PostNetworkCommand({
			"type": "formation",
			"entities": selection,
			"name": formationName
		});
	}
}

// Performs the specified group
function performGroup(action, groupId)
{
	switch (action)
	{
	case "snap":
	case "select":
	case "add":
		var toSelect = [];
		g_Groups.update();
		for (var ent in g_Groups.groups[groupId].ents)
			toSelect.push(+ent);

		if (action != "add")
			g_Selection.reset();

		g_Selection.addList(toSelect);

		if (action == "snap" && toSelect.length)
			Engine.CameraFollow(toSelect[0]);
		break;
	case "save":
		var selection = g_Selection.toList();
		g_Groups.groups[groupId].reset();
		g_Groups.addEntities(groupId, selection);
		updateGroups();
		break;
	}
}

// Performs the specified stance
function performStance(entity, stanceName)
{
	if (entity)
	{
		var selection = g_Selection.toList();
		Engine.PostNetworkCommand({
			"type": "stance",
			"entities": selection,
			"name": stanceName
		});
	}
}

// Set the camera to follow the given unit
function setCameraFollow(entity)
{
	// Follow the given entity if it's a unit
	if (entity)
	{
		var entState = GetEntityState(entity);
		if (entState && hasClass(entState, "Unit"))
		{
			Engine.CameraFollow(entity);
			return;
		}
	}

	// Otherwise stop following
	Engine.CameraFollow(0);
}

var lastIdleUnit = 0;
var currIdleClass = 0;

function resetIdleUnit()
{
	lastIdleUnit = 0;
	currIdleClass = 0;
}

function findIdleUnit(classes)
{
	// Cycle through idling classes before giving up
	for (var i = 0; i <= classes.length; ++i)
	{
		var data = { prevUnit: lastIdleUnit, idleClass: classes[currIdleClass] };
		var newIdleUnit = Engine.GuiInterfaceCall("FindIdleUnit", data);

		// Check if we have new valid entity
		if (newIdleUnit && newIdleUnit != lastIdleUnit)
		{
			lastIdleUnit = newIdleUnit;
			g_Selection.reset()
			g_Selection.addList([lastIdleUnit]);
			Engine.CameraFollow(lastIdleUnit);

			return;
		}

		lastIdleUnit = 0;
		currIdleClass = (currIdleClass + 1) % classes.length;
	}

	// TODO: display a message or play a sound to indicate no more idle units, or something
	// Reset for next cycle
	resetIdleUnit();
}

function unload(garrisonHolder, entities)
{
	if (Engine.HotkeyIsPressed("session.unloadtype"))
		Engine.PostNetworkCommand({"type": "unload", "entities": entities, "garrisonHolder": garrisonHolder});
	else
		Engine.PostNetworkCommand({"type": "unload", "entities": [entities[0]], "garrisonHolder": garrisonHolder});
}

function unloadAll(garrisonHolder)
{
	Engine.PostNetworkCommand({"type": "unload-all", "garrisonHolder": garrisonHolder});
}
