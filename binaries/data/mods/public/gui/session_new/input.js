const SDL_BUTTON_LEFT = 1;
const SDL_BUTTON_MIDDLE = 2;
const SDL_BUTTON_RIGHT = 3;
// TODO: these constants should be defined somewhere else instead, in
// case any other code wants to use them too


var INPUT_NORMAL = 0;
var INPUT_SELECTING = 1;
var INPUT_BANDBOXING = 2;
var INPUT_BUILDING_PLACEMENT = 3;

var inputState = INPUT_NORMAL;

var placementEntity = "";

var mouseX = 0;
var mouseY = 0;

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

	// Different owner -> attack
	if (entState.attack && targetState.player != entState.player)
		return {"type": "attack", "cursor": "action-attack", "target": targets[0]};

	// Resource -> gather
	var resource = findGatherType(entState.resourceGatherRates, targetState.resourceSupply);
	if (resource)
		return {"type": "gather", "cursor": "action-gather-"+resource, "target": targets[0]};

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

// TODO: it'd probably be nice to have a better state-machine system


function handleInputBeforeGui(ev)
{
	// Capture mouse position so we can use it for displaying cursors
	switch (ev.type)
	{
	case "mousebuttonup":
	case "mousebuttondown":
	case "mousemotion":
		mouseX = ev.x;
		mouseY = ev.y;
		break;
	}

	// State-machine processing:
	//
	// (This is for states which should override the normal GUI processing - events will
	// be processed here before being passed on, and propagation will stop if this function
	// returns true)

	switch (inputState)
	{
	case INPUT_BANDBOXING:
		switch (ev.type)
		{
		case "mousemotion":
			var x0 = selectionDragStart[0];
			var y0 = selectionDragStart[1];
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
				var x0 = selectionDragStart[0];
				var y0 = selectionDragStart[1];
				var x1 = ev.x;
				var y1 = ev.y;
				if (x0 > x1) { var t = x0; x0 = x1; x1 = t; }
				if (y0 > y1) { var t = y0; y0 = y1; y1 = t; }

				var bandbox = getGUIObjectByName("bandbox");
				bandbox.hidden = true;

				var ents = Engine.PickFriendlyEntitiesInRect(x0, y0, x1, y1, Engine.GetPlayerID());
				g_Selection.setHighlightList([]);
				g_Selection.reset();
				g_Selection.addList(ents);
				
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
	}

	return false;
}

var selectionDragStart;

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
				selectionDragStart = [ ev.x, ev.y ];
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
			var dragDeltaX = ev.x - selectionDragStart[0];
			var dragDeltaY = ev.y - selectionDragStart[1];
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
			var angle = Math.PI;
			Engine.GuiInterfaceCall("SetBuildingPlacementPreview", {
				"template": placementEntity,
				"x": target.x,
				"z": target.z,
				"angle": angle
			});

			return false; // continue processing mouse motion

		case "mousebuttondown":
			if (ev.button == SDL_BUTTON_LEFT)
			{
				var selection = g_Selection.toList();
				var target = Engine.GetTerrainAtPoint(ev.x, ev.y);
				var angle = Math.PI;
				// Remove the preview
				Engine.GuiInterfaceCall("SetBuildingPlacementPreview", {"template": ""});
				// Start the construction
				Engine.PostNetworkCommand({
					"type": "construct",
					"template": placementEntity,
					"x": target.x,
					"z": target.z,
					"angle": angle,
					"entities": selection
				});

				inputState = INPUT_NORMAL;
				return true;
			}
			else if (ev.button == SDL_BUTTON_RIGHT)
			{
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

function testBuild(ent)
{
	placementEntity = ent;
	inputState = INPUT_BUILDING_PLACEMENT;
}
