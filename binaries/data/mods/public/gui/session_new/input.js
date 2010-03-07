const SDL_BUTTON_LEFT = 1;
const SDL_BUTTON_MIDDLE = 2;
const SDL_BUTTON_RIGHT = 3;
// TODO: these constants should be defined somewhere else instead, in
// case any other code wants to use them too


var INPUT_NORMAL = 0;
var INPUT_DRAGGING = 1;
var INPUT_BUILDING_PLACEMENT = 2;

var inputState = INPUT_NORMAL;

var placementEntity = "";

var mouseX = 0;
var mouseY = 0;

function updateCursor()
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
	var selection = getEntitySelection();

	// No action if there's no selection
	if (!selection.length)
		return;

	// If the selection doesn't exist, no action
	var entState = Engine.GuiInterfaceCall("GetEntityState", selection[0]);
	if (!entState)
		return;

	// If the selection isn't friendly units, no action
	var player = Engine.GetPlayerID();
	if (entState.player != player)
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
	if (entState.attack && targetState.player != player)
		return {"type": "attack", "cursor": "action-attack", "target": targets[0]};

	var resource = findGatherType(entState.resourceGatherRates, targetState.resourceSupply);
	if (resource)
		return {"type": "gather", "cursor": "action-gather-"+resource, "target": targets[0]};

	// TODO: need more actions

	// If we don't do anything more specific, just walk
	return {"type": "move"};
}

function handleInputBeforeGui(ev)
{
	switch (ev.type)
	{
	case "mousebuttonup":
	case "mousebuttondown":
	case "mousemotion":
		mouseX = ev.x;
		mouseY = ev.y;
		break;
	}

	return false;
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

function handleInputAfterGui(ev)
{
	switch (inputState)
	{
	case INPUT_NORMAL:
		switch (ev.type)
		{
		case "mousebuttondown":
			if (ev.button == SDL_BUTTON_LEFT)
			{
				var ents = Engine.PickEntitiesAtPoint(ev.x, ev.y);
				if (!ents.length)
				{
					resetEntitySelection();
					return true;
				}

				resetEntitySelection();
				addEntitySelection([ents[0]]);

				return true;
			}
			else if (ev.button == SDL_BUTTON_RIGHT)
			{
				var action = determineAction(ev.x, ev.y);
				if (!action)
					break;

				var selection = getEntitySelection();

				switch (action.type)
				{
				case "move":
					var target = Engine.GetTerrainAtPoint(ev.x, ev.y);
					Engine.PostNetworkCommand({"type": "walk", "entities": selection, "x": target.x, "z": target.z});
					return true;

				case "attack":
					Engine.PostNetworkCommand({"type": "attack", "entities": selection, "target": action.target});
					return true;

				case "gather":
					Engine.PostNetworkCommand({"type": "gather", "entities": selection, "target": action.target});
					return true;
				}
			}
		}
		break;

	case INPUT_BUILDING_PLACEMENT:
		switch (ev.type)
		{
		case "mousemotion":
			var target = Engine.GetTerrainAtPoint(ev.x, ev.y);
			var angle = Math.PI;
			Engine.GuiInterfaceCall("SetBuildingPlacementPreview", {"template": placementEntity, "x": target.x, "z": target.z, "angle": angle});

			return false; // continue processing mouse motion

		case "mousebuttondown":
			if (ev.button == SDL_BUTTON_LEFT)
			{
				var target = Engine.GetTerrainAtPoint(ev.x, ev.y);
				var angle = Math.PI;
				Engine.GuiInterfaceCall("SetBuildingPlacementPreview", {"template": ""});
				Engine.PostNetworkCommand({"type": "construct", "template": placementEntity, "x": target.x, "z": target.z, "angle": angle});

				inputState = INPUT_NORMAL;
				return true;
			}
			else if (ev.button == SDL_BUTTON_RIGHT)
			{
				Engine.GuiInterfaceCall("SetBuildingPlacementPreview", {"template": ""});

				inputState = INPUT_NORMAL;
				return true;
			}
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
