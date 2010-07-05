const SDL_BUTTON_LEFT = 1;
const SDL_BUTTON_MIDDLE = 2;
const SDL_BUTTON_RIGHT = 3;
const SDLK_RSHIFT = 303;
const SDLK_LSHIFT = 304;

const MAX_SELECTION_SIZE = 24;

// TODO: these constants should be defined somewhere else instead, in
// case any other code wants to use them too

var INPUT_NORMAL = 0;
var INPUT_SELECTING = 1;
var INPUT_BANDBOXING = 2;
var INPUT_BUILDING_PLACEMENT = 3;
var INPUT_BUILDING_CLICK = 4;
var INPUT_BUILDING_DRAG = 5;
var INPUT_BATCHTRAINING = 6;

var inputState = INPUT_NORMAL;

var defaultPlacementAngle = Math.PI*3/4;
var placementAngle;
var placementPosition;
var placementEntity;

var mouseX = 0;
var mouseY = 0;
var specialKeyStates = {};
specialKeyStates[SDLK_RSHIFT] = 0;
specialKeyStates[SDLK_LSHIFT] = 0;
// (TODO: maybe we should fix the hotkey system to be usable in this situation,
// rather than hardcoding Shift into this code?)

function updateCursor()
{
	if (inputState == INPUT_NORMAL)
	{
		var action = determineAction(mouseX, mouseY);
		if (action)
		{
			if (action.cursor)
			{
				Engine.SetCursor(action.cursor);
				return;
			}
		}
	}

	Engine.SetCursor("arrow-default");
}

function findGatherType(gatherer, supply)
{
	if (!gatherer || !supply)
		return;
	if (gatherer[supply.type.generic+"."+supply.type.specific])
		return supply.type.specific;
	if (gatherer[supply.type.generic])
		return supply.type.generic;
}

/**
 * Determine the context-sensitive action that should be performed when the mouse is at (x,y)
 */
function determineAction(x, y)
{
	var selection = g_Selection.toList();

	// No action if there's no selection
	if (!selection.length)
		return;

	// If the selection doesn't exist, no action
	var entState = Engine.GuiInterfaceCall("GetEntityState", selection[0]);
	if (!entState)
		return;

	// If the selection isn't friendly units, no action
	var player = Engine.GetPlayerID();
	if (entState.player != player && !g_DevSettings.controlAll)
		return;

	var targets = Engine.PickEntitiesAtPoint(x, y);

	// If there's no unit, just walk
	if (!targets.length)
		return {"type": "move"};

	// Look at the first targeted entity
	// (TODO: maybe we eventually want to look at more, and be more context-sensitive?
	// e.g. prefer to attack an enemy unit, even if some friendly units are closer to the mouse)
	var targetState = Engine.GuiInterfaceCall("GetEntityState", targets[0]);

	// Resource -> gather
	var resource = findGatherType(entState.resourceGatherRates, targetState.resourceSupply);
	if (resource)
		return {"type": "gather", "cursor": "action-gather-"+resource, "target": targets[0]};

	// Different owner -> attack
	// (TODO: this should only happen if the target is really targetable, not e.g. a tree that this unit can't gather)
	if (entState.attack && targetState.player != entState.player)
		return {"type": "attack", "cursor": "action-attack", "target": targets[0]};

	// If a builder, then: Foundation -> build
	if (entState.buildEntities && targetState.foundation)
		return {"type": "build", "cursor": "action-build", "target": targets[0]};

	// TODO: need more actions

	// If we don't do anything more specific, just walk
	return {"type": "move"};
}

/*

Selection methods: (not all currently implemented)

- Left-click on entity to select (always chooses the 'closest' one if the mouse is over several).
  Includes non-controllable units (e.g. trees, enemy units).
- Double-left-click to select entity plus all of the same type on the screen.
- Triple-left-click to select entity plus all of the same type in the world.
- Left-click-and-drag to select all in region. Only includes controllable units.
- Left-click on empty space to deselect all.
- Hotkeys to select various groups.
- Shift plus left-click on entity to toggle selection of that unit. Only includes controllable.
- Shift plus any other selection method above, to add them to current selection.

*/

var dragStart; // used for remembering mouse coordinates at start of drag operations

function tryPlaceBuilding()
{
	var selection = g_Selection.toList();

	// Use the preview to check it's a valid build location
	var ok = Engine.GuiInterfaceCall("SetBuildingPlacementPreview", {
		"template": placementEntity,
		"x": placementPosition.x,
		"z": placementPosition.z,
		"angle": placementAngle
	});
	if (!ok)
	{
		// invalid location - don't build it
		return false;
	}

	// Remove the preview
	Engine.GuiInterfaceCall("SetBuildingPlacementPreview", {"template": ""});

	// Start the construction
	Engine.PostNetworkCommand({
		"type": "construct",
		"template": placementEntity,
		"x": placementPosition.x,
		"z": placementPosition.z,
		"angle": placementAngle,
		"entities": selection
	});

	return true;
}

function handleInputBeforeGui(ev)
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
	case "keydown":
		if (ev.keysym.sym in specialKeyStates)
			specialKeyStates[ev.keysym.sym] = 1;
		break;
	case "keyup":
		if (ev.keysym.sym in specialKeyStates)
			specialKeyStates[ev.keysym.sym] = 0;
		break;
	}

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

				var ents = Engine.PickFriendlyEntitiesInRect(x0, y0, x1, y1, Engine.GetPlayerID());

				// Remove non-unit entities from the bandboxed selection (Probably should make buildings work somehow)
				for (var i = ents.length-1; i >= 0; i--)
				{
					var template = Engine.GuiInterfaceCall("GetEntityState", ents[i]).template;
					var firstWord = getTemplateFirstWord(template);
					if (firstWord != "units")
						ents.splice(i, 1);
				}

				// Remove units if selection is too large
				if (ents.length > MAX_SELECTION_SIZE)
					ents = ents.slice(0, MAX_SELECTION_SIZE);

				// Set selection list
				g_Selection.setHighlightList([]);
				g_Selection.reset();
				g_Selection.addList(ents);

				// Create the selection groups
				g_Selection.createSelectionGroups(ents);
				
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
			// then switch to drag-orientatio mode
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
				if (tryPlaceBuilding())
				{
					// If shift is down, let the player continue placing another of the same building
					if (specialKeyStates[SDLK_RSHIFT] || specialKeyStates[SDLK_LSHIFT])
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
				Engine.GuiInterfaceCall("SetBuildingPlacementPreview", {"template": ""});
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
				var target = Engine.GetTerrainAtPoint(ev.x, ev.y);
				placementAngle = Math.atan2(target.x - placementPosition.x, target.z - placementPosition.z);
			}
			else
			{
				// If the mouse is near the center, snap back to the default orientation
				placementAngle = defaultPlacementAngle;
			}

			Engine.GuiInterfaceCall("SetBuildingPlacementPreview", {
				"template": placementEntity,
				"x": placementPosition.x,
				"z": placementPosition.z,
				"angle": placementAngle
			});

			break;

		case "mousebuttonup":
			if (ev.button == SDL_BUTTON_LEFT)
			{
				if (tryPlaceBuilding())
					inputState = INPUT_NORMAL;
				else
					inputState = INPUT_BUILDING_PLACEMENT;
				return true;
			}
			break;

		case "mousebuttondown":
			if (ev.button == SDL_BUTTON_RIGHT)
			{
				// Cancel building
				Engine.GuiInterfaceCall("SetBuildingPlacementPreview", {"template": ""});
				inputState = INPUT_NORMAL;
				return true;
			}
			break;
		}
		break;

	case INPUT_BATCHTRAINING:
		switch (ev.type)
		{
		case "keyup":
			if (ev.keysym.sym == SDLK_RSHIFT || ev.keysym.sym == SDLK_LSHIFT)
			{
				flushTrainingQueueBatch();
				inputState = INPUT_NORMAL;
			}
			break;
		}
	}

	return false;
}

function handleInputAfterGui(ev)
{
	// State-machine processing:

	switch (inputState)
	{
	case INPUT_NORMAL:
		switch (ev.type)
		{
		case "mousemotion":
			var ents = Engine.PickEntitiesAtPoint(ev.x, ev.y);
			g_Selection.setHighlightList(ents);
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

				var selection = g_Selection.toList();

				switch (action.type)
				{
				case "move":
					var target = Engine.GetTerrainAtPoint(ev.x, ev.y);
					Engine.PostNetworkCommand({"type": "walk", "entities": selection, "x": target.x, "z": target.z});
					return true;

				case "attack":
					Engine.PostNetworkCommand({"type": "attack", "entities": selection, "target": action.target});
					return true;

				case "build": // (same command as repair)
				case "repair":
					Engine.PostNetworkCommand({"type": "repair", "entities": selection, "target": action.target});
					return true;

				case "gather":
					Engine.PostNetworkCommand({"type": "gather", "entities": selection, "target": action.target});
					return true;

				default:
					throw new Error("Invalid action.type "+action.type);
				}
			}
			break;
		}
		break;

	case INPUT_SELECTING:
		switch (ev.type)
		{
		case "mousemotion":
			// If the mouse moved further than a limit, switch to bandbox mode
			var dragDeltaX = ev.x - dragStart[0];
			var dragDeltaY = ev.y - dragStart[1];
			var maxDragDelta = 4;
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
					
					inputState = INPUT_NORMAL;
					return true;
				}

				g_Selection.reset();
				g_Selection.addList([ents[0]]);

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
			var target = Engine.GetTerrainAtPoint(ev.x, ev.y);
			Engine.GuiInterfaceCall("SetBuildingPlacementPreview", {
				"template": placementEntity,
				"x": target.x,
				"z": target.z,
				"angle": placementAngle
			});

			return false; // continue processing mouse motion

		case "mousebuttondown":
			if (ev.button == SDL_BUTTON_LEFT)
			{
				placementPosition = Engine.GetTerrainAtPoint(ev.x, ev.y);
				dragStart = [ ev.x, ev.y ];
				inputState = INPUT_BUILDING_CLICK;
				return true;
			}
			else if (ev.button == SDL_BUTTON_RIGHT)
			{
				// Cancel building
				Engine.GuiInterfaceCall("SetBuildingPlacementPreview", {"template": ""});
				inputState = INPUT_NORMAL;
				return true;
			}
			break;
		}
		break;
	}
	return false;
}

// Called by GUI when user clicks construction button
function startBuildingPlacement(buildEntType)
{
	placementEntity = buildEntType;
	placementAngle = defaultPlacementAngle;
	inputState = INPUT_BUILDING_PLACEMENT;
}

// Batch training:
// When the user shift-clicks, we set these variables and switch to INPUT_BATCHTRAINING
// When the user releases shift, or clicks on a different training button, we create the batched units
var batchTrainingEntity;
var batchTrainingType;
var batchTrainingCount;
const batchIncrementSize = 5;

function flushTrainingQueueBatch()
{
	Engine.PostNetworkCommand({"type": "train", "entity": batchTrainingEntity, "template": batchTrainingType, "count": batchTrainingCount});
}

// Called by GUI when user clicks training button
function addToTrainingQueue(entity, trainEntType)
{
	if (specialKeyStates[SDLK_RSHIFT] || specialKeyStates[SDLK_LSHIFT])
	{
		if (inputState == INPUT_BATCHTRAINING)
		{
			// If we're already creating a batch of this unit, then just extend it
			if (batchTrainingEntity == entity && batchTrainingType == trainEntType)
			{
				batchTrainingCount += batchIncrementSize;
				return;
			}
			// Otherwise start a new one
			else
			{
				flushTrainingQueueBatch();
				// fall through to create the new batch
			}
		}
		inputState = INPUT_BATCHTRAINING;
		batchTrainingEntity = entity;
		batchTrainingType = trainEntType;
		batchTrainingCount = batchIncrementSize;
	}
	else
	{
		// Non-batched - just create a single entity
		Engine.PostNetworkCommand({"type": "train", "entity": entity, "template": trainEntType, "count": 1});
	}
}

// Returns the number of units that will be present in a batch if the user clicks
// the training button with shift down
function getTrainingQueueBatchStatus(entity, trainEntType)
{
	if (inputState == INPUT_BATCHTRAINING && batchTrainingEntity == entity && batchTrainingType == trainEntType)
		return [batchTrainingCount, batchIncrementSize];
	else
		return [0, batchIncrementSize];
}

// Called by GUI when user clicks production queue item
function removeFromTrainingQueue(entity, id)
{
	console.write("removeFromTrainingQueue(entity = " + entity + ", id = " + id +")");
	//Engine.PostNetworkCommand({"type": "stop-train", "entity": entity, "id": id});
}

function changePrimarySelectionGroup(entType)
{
	getGUIObjectByName("unitSelectionHighlight[" + g_Selection.groups.primary + "]").hidden = true;
	
	// set primary group
	g_Selection.groups.primary = g_Selection.groups.groupNumbers[entType];
	getGUIObjectByName("unitSelectionHighlight[" + g_Selection.groups.primary + "]").hidden = false;
	
	// set primary selection
	g_Selection.setPrimary(g_Selection.groups.firstOfType[entType]);
}
