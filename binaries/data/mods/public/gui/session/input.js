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
// case any other code wants to use them too.

const ACTION_NONE = 0;
const ACTION_GARRISON = 1;
const ACTION_REPAIR = 2;
const ACTION_GUARD = 3;
const ACTION_PATROL = 4;
const ACTION_OCCUPY_TURRET = 5;
const ACTION_CALLTOARMS = 6;
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
const INPUT_UNIT_POSITION_START = 10;
const INPUT_UNIT_POSITION = 11;
const INPUT_FLARE = 12;

var inputState = INPUT_NORMAL;

const INVALID_ENTITY = 0;

var mouseX = 0;
var mouseY = 0;
var mouseIsOverObject = false;

/**
 * Containing the ingame position which span the line.
 */
var g_FreehandSelection_InputLine = [];

/**
 * Minimum squared distance when a mouse move is called a drag.
 */
const g_FreehandSelection_ResolutionInputLineSquared = 1;

/**
 * Minimum length a dragged line should have to use the freehand selection.
 */
const g_FreehandSelection_MinLengthOfLine = 8;

/**
 * To start the freehandSelection function you need a minimum number of units.
 * Minimum must be 2, for better performance you could set it higher.
 */
const g_FreehandSelection_MinNumberOfUnits = 2;

/**
 * Used for remembering mouse coordinates at start of drag operations.
 */
var g_DragStart;

/**
 * Store the clicked entity on mousedown or mouseup for single/double/triple clicks to select entities.
 * If any mousedown or mouseup of a sequence of clicks lands on a unit,
 * that unit will be selected, which makes it easier to click on moving units.
 */
var clickedEntity = INVALID_ENTITY;

/**
 * Store the last time the flare functionality was used to prevent overusage.
 */
var g_LastFlareTime;

/**
 * The duration in ms for which we disable flaring after each flare to prevent overusage.
 */
const g_FlareCooldown = 3000;

// Same double-click behaviour for hotkey presses.
const doublePressTime = 500;
var doublePressTimer = 0;
var prevHotkey = 0;

function getMaxDragDelta()
{
	return Engine.ConfigDB_GetValue("user", "gui.session.dragdelta");
}

function updateCursorAndTooltip()
{
	let cursorSet = false;
	let tooltipSet = false;
	let informationTooltip = Engine.GetGUIObjectByName("informationTooltip");
	if (inputState == INPUT_FLARE || inputState == INPUT_NORMAL && Engine.HotkeyIsPressed("session.flare") && !g_IsObserver)
	{
		Engine.SetCursor("action-flare");
		cursorSet = true;
	}
	else if (!mouseIsOverObject && (inputState == INPUT_NORMAL || inputState == INPUT_PRESELECTEDACTION) || g_MiniMapPanel.isMouseOverMiniMap())
	{
		let action = determineAction(mouseX, mouseY, g_MiniMapPanel.isMouseOverMiniMap());
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

	if (!cursorSet)
		Engine.ResetCursor();

	if (!tooltipSet)
		informationTooltip.hidden = true;

	let placementTooltip = Engine.GetGUIObjectByName("placementTooltip");
	if (placementSupport.tooltipMessage)
		placementTooltip.sprite = placementSupport.tooltipError ? "BackgroundErrorTooltip" : "BackgroundInformationTooltip";

	placementTooltip.caption = placementSupport.tooltipMessage || "";
	placementTooltip.hidden = !placementSupport.tooltipMessage;
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
			let result = Engine.GuiInterfaceCall("SetBuildingPlacementPreview", {
				"template": placementSupport.template,
				"x": placementSupport.position.x,
				"z": placementSupport.position.z,
				"angle": placementSupport.angle,
				"actorSeed": placementSupport.actorSeed
			});

			placementSupport.tooltipError = !result.success;
			placementSupport.tooltipMessage = "";

			if (!result.success)
			{
				if (result.message && result.parameters)
				{
					let message = result.message;
					if (result.translateMessage)
						if (result.pluralMessage)
							message = translatePlural(result.message, result.pluralMessage, result.pluralCount);
						else
							message = translate(message);
					let parameters = result.parameters;
					if (result.translateParameters)
						translateObjectKeys(parameters, result.translateParameters);
					placementSupport.tooltipMessage = sprintf(message, parameters);
				}
				return false;
			}

			if (placementSupport.attack && placementSupport.attack.Ranged)
			{
				const cmd = {
					"x": placementSupport.position.x,
					"z": placementSupport.position.z,
					"range": placementSupport.attack.Ranged.maxRange,
					"yOrigin": placementSupport.attack.Ranged.yOrigin
				};
				const averageRange = Math.round(Engine.GuiInterfaceCall("GetAverageRangeForBuildings", cmd) - cmd.range);
				const range = Math.round(cmd.range);
				placementSupport.tooltipMessage = sprintf(translatePlural("Basic range: %(range)s meter", "Basic range: %(range)s meters", range), { "range": range }) + "\n" +
					sprintf(translatePlural("Average bonus range: %(range)s meter", "Average bonus range: %(range)s meters", averageRange), { "range": averageRange });
			}
			return true;
		}
	}
	else if (placementSupport.mode === "wall" &&
		placementSupport.wallSet && placementSupport.position)
	{
		placementSupport.wallSnapEntities = Engine.PickSimilarPlayerEntities(
			placementSupport.wallSet.templates.tower,
			placementSupport.wallSnapEntitiesIncludeOffscreen,
			true, // require exact template match
			true  // include foundations
		);

		return Engine.GuiInterfaceCall("SetWallPlacementPreview", {
			"wallSet": placementSupport.wallSet,
			"start": placementSupport.position,
			"end": placementSupport.wallEndPosition,
			"snapEntities": placementSupport.wallSnapEntities // snapping entities (towers) for starting a wall segment
		});
	}

	return false;
}

/**
 * Determine the context-sensitive action that should be performed when the mouse is at (x,y)
 */
function determineAction(x, y, fromMiniMap)
{
	let selection = g_Selection.toList();
	if (!selection.length)
	{
		preSelectedAction = ACTION_NONE;
		return undefined;
	}

	let entState = GetEntityState(selection[0]);
	if (!entState)
		return undefined;

	if (!selection.every(ownsEntity) &&
	    !(g_SimState.players[g_ViewedPlayer] &&
	      g_SimState.players[g_ViewedPlayer].controlsAll))
		return undefined;

	let target;
	if (!fromMiniMap)
	{
		let ent = Engine.PickEntityAtPoint(x, y);
		if (ent != INVALID_ENTITY)
			target = ent;
	}

	// Decide between the following ordered actions,
	// if two actions are possible, the first one is taken
	// thus the most specific should appear first.

	if (preSelectedAction != ACTION_NONE)
	{
		for (let action of g_UnitActionsSortedKeys)
			if (g_UnitActions[action].preSelectedActionCheck)
			{
				let r = g_UnitActions[action].preSelectedActionCheck(target, selection);
				if (r)
					return r;
			}

		return { "type": "none", "cursor": "", "target": target };
	}

	for (let action of g_UnitActionsSortedKeys)
		if (g_UnitActions[action].hotkeyActionCheck)
		{
			let r = g_UnitActions[action].hotkeyActionCheck(target, selection);
			if (r)
				return r;
		}

	for (let action of g_UnitActionsSortedKeys)
		if (g_UnitActions[action].actionCheck)
		{
			let r = g_UnitActions[action].actionCheck(target, selection);
			if (r)
				return r;
		}

	return { "type": "none", "cursor": "", "target": target };
}

function ownsEntity(ent)
{
	let entState = GetEntityState(ent);
	return entState && entState.player == g_ViewedPlayer;
}

function isAttackMovePressed()
{
	return Engine.HotkeyIsPressed("session.attackmove") ||
		Engine.HotkeyIsPressed("session.attackmoveUnit");
}

function isSnapToEdgesEnabled()
{
	let config = Engine.ConfigDB_GetValue("user", "gui.session.snaptoedges");
	let hotkeyPressed = Engine.HotkeyIsPressed("session.snaptoedges");
	return hotkeyPressed == (config == "disabled");
}

function tryPlaceBuilding(queued, pushFront)
{
	if (placementSupport.mode !== "building")
	{
		error("tryPlaceBuilding expected 'building', got '" + placementSupport.mode + "'");
		return false;
	}

	if (!updateBuildingPlacementPreview())
	{
		Engine.GuiInterfaceCall("PlaySound", {
			"name": "invalid_building_placement",
			"entity": g_Selection.getFirstSelected()
		});
		return false;
	}

	let selection = Engine.HotkeyIsPressed("session.orderone") &&
		popOneFromSelection({ "type": "construct", "target": placementSupport }) || g_Selection.toList();

	Engine.PostNetworkCommand({
		"type": "construct",
		"template": placementSupport.template,
		"x": placementSupport.position.x,
		"z": placementSupport.position.z,
		"angle": placementSupport.angle,
		"actorSeed": placementSupport.actorSeed,
		"entities": selection,
		"autorepair": true,
		"autocontinue": true,
		"queued": queued,
		"pushFront": pushFront,
		"formation": g_AutoFormation.getNull()
	});
	Engine.GuiInterfaceCall("PlaySound", { "name": "order_build", "entity": selection[0] });

	if (!queued || !g_Selection.size())
		placementSupport.Reset();
	else
		placementSupport.RandomizeActorSeed();

	return true;
}

function tryPlaceWall(queued)
{
	if (placementSupport.mode !== "wall")
	{
		error("tryPlaceWall expected 'wall', got '" + placementSupport.mode + "'");
		return false;
	}

	let wallPlacementInfo = updateBuildingPlacementPreview(); // entities making up the wall (wall segments, towers, ...)
	if (!(wallPlacementInfo === false || typeof wallPlacementInfo === "object"))
	{
		error("Invalid updateBuildingPlacementPreview return value: " + uneval(wallPlacementInfo));
		return false;
	}

	if (!wallPlacementInfo)
		return false;

	let selection = Engine.HotkeyIsPressed("session.orderone") &&
		popOneFromSelection({ "type": "construct", "target": placementSupport }) || g_Selection.toList();

	let cmd = {
		"type": "construct-wall",
		"autorepair": true,
		"autocontinue": true,
		"queued": queued,
		"entities": selection,
		"wallSet": placementSupport.wallSet,
		"pieces": wallPlacementInfo.pieces,
		"startSnappedEntity": wallPlacementInfo.startSnappedEnt,
		"endSnappedEntity": wallPlacementInfo.endSnappedEnt,
		"formation": g_AutoFormation.getNull()
	};

	// Make sure that there's at least one non-tower entity getting built, to prevent silly edge cases where the start and end
	// point are too close together for the algorithm to place a wall segment inbetween, and only the towers are being previewed
	// (this is somewhat non-ideal and hardcode-ish).
	let hasWallSegment = false;
	for (let piece of cmd.pieces)
	{
		if (piece.template != cmd.wallSet.templates.tower) // TODO: hardcode-ish :(
		{
			hasWallSegment = true;
			break;
		}
	}

	if (hasWallSegment)
	{
		Engine.PostNetworkCommand(cmd);
		Engine.GuiInterfaceCall("PlaySound", { "name": "order_build", "entity": selection[0] });
	}

	return true;
}

/**
 * Updates the bandbox object with new positions and visibility.
 * @returns {array} The coordinates of the vertices of the bandbox.
 */
function updateBandbox(bandbox, ev, hidden)
{
	let scale = +Engine.ConfigDB_GetValue("user", "gui.scale");
	let vMin = Vector2D.min(g_DragStart, ev);
	let vMax = Vector2D.max(g_DragStart, ev);

	bandbox.size = new GUISize(vMin.x / scale, vMin.y / scale, vMax.x / scale, vMax.y / scale);
	bandbox.hidden = hidden;

	return [vMin.x, vMin.y, vMax.x, vMax.y];
}

// Define some useful unit filters for getPreferredEntities.
var unitFilters = {
	"isUnit": entity => {
		let entState = GetEntityState(entity);
		return entState && hasClass(entState, "Unit");
	},
	"isDefensive": entity => {
		let entState = GetEntityState(entity);
		return entState && hasClass(entState, "Defensive");
	},
	"isMilitary": entity => {
		let entState = GetEntityState(entity);
		return entState &&
			g_MilitaryTypes.some(c => hasClass(entState, c));
	},
	"isNonMilitary": entity => {
		let entState = GetEntityState(entity);
		return entState &&
			hasClass(entState, "Unit") &&
			!g_MilitaryTypes.some(c => hasClass(entState, c));
	},
	"isIdle": entity => {
		let entState = GetEntityState(entity);
		return entState &&
			hasClass(entState, "Unit") &&
			entState.unitAI &&
			entState.unitAI.isIdle &&
			!hasClass(entState, "Domestic");
	},
	"isWounded": entity => {
		let entState = GetEntityState(entity);
		return entState &&
			hasClass(entState, "Unit") &&
			entState.maxHitpoints &&
			100 * entState.hitpoints <= entState.maxHitpoints * Engine.ConfigDB_GetValue("user", "gui.session.woundedunithotkeythreshold");
	},
	"isAnything": entity => {
		return true;
	}
};

// Choose, inside a list of entities, which ones will be selected.
// We may use several entity filters, until one returns at least one element.
function getPreferredEntities(ents)
{
	let filters = [unitFilters.isUnit, unitFilters.isDefensive, unitFilters.isAnything];

	if (Engine.HotkeyIsPressed("selection.militaryonly"))
		filters = [unitFilters.isMilitary];
	if (Engine.HotkeyIsPressed("selection.nonmilitaryonly"))
		filters = [unitFilters.isNonMilitary];
	if (Engine.HotkeyIsPressed("selection.idleonly"))
		filters = [unitFilters.isIdle];
	if (Engine.HotkeyIsPressed("selection.woundedonly"))
		filters = [unitFilters.isWounded];

	let preferredEnts = [];
	for (let i = 0; i < filters.length; ++i)
	{
		preferredEnts = ents.filter(filters[i]);
		if (preferredEnts.length)
			break;
	}
	return preferredEnts;
}

function handleInputBeforeGui(ev, hoveredObject)
{
	if (GetSimState().cinemaPlaying)
		return false;

	// Capture cursor position so we can use it for displaying cursors,
	// and key states.
	switch (ev.type)
	{
	case "mousebuttonup":
	case "mousebuttondown":
	case "mousemotion":
		mouseX = ev.x;
		mouseY = ev.y;
		break;
	}

	mouseIsOverObject = (hoveredObject != null);

	// Close the menu when interacting with the game world.
	if (!mouseIsOverObject && (ev.type =="mousebuttonup" || ev.type == "mousebuttondown") &&
		(ev.button == SDL_BUTTON_LEFT || ev.button == SDL_BUTTON_RIGHT))
		g_Menu.close();

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
		let bandbox = Engine.GetGUIObjectByName("bandbox");
		switch (ev.type)
		{
		case "mousemotion":
		{
			let rect = updateBandbox(bandbox, ev, false);

			let ents = Engine.PickPlayerEntitiesInRect(rect[0], rect[1], rect[2], rect[3], g_ViewedPlayer);
			let preferredEntities = getPreferredEntities(ents);
			g_Selection.setHighlightList(preferredEntities);

			return false;
		}

		case "mousebuttonup":
			if (ev.button == SDL_BUTTON_LEFT)
			{
				let rect = updateBandbox(bandbox, ev, true);
				let ents = getPreferredEntities(Engine.PickPlayerEntitiesInRect(rect[0], rect[1], rect[2], rect[3], g_ViewedPlayer));
				g_Selection.setHighlightList([]);

				if (Engine.HotkeyIsPressed("selection.add"))
					g_Selection.addList(ents);
				else if (Engine.HotkeyIsPressed("selection.remove"))
					g_Selection.removeList(ents);
				else
				{
					g_Selection.reset();
					g_Selection.addList(ents);
				}

				inputState = INPUT_NORMAL;
				return true;
			}
			if (ev.button == SDL_BUTTON_RIGHT)
			{
				// Cancel selection.
				bandbox.hidden = true;
				g_Selection.setHighlightList([]);

				inputState = INPUT_NORMAL;
				return true;
			}
			break;
		}
		break;

	case INPUT_UNIT_POSITION:
		switch (ev.type)
		{
		case "mousemotion":
			return positionUnitsFreehandSelectionMouseMove(ev);
		case "mousebuttonup":
			return positionUnitsFreehandSelectionMouseUp(ev);
		}
		break;

	case INPUT_BUILDING_CLICK:
		switch (ev.type)
		{
		case "mousemotion":
			// If the mouse moved far enough from the original click location,
			// then switch to drag-orientation mode.
			if (g_DragStart.distanceTo(ev) >= Math.square(getMaxDragDelta()))
			{
				inputState = INPUT_BUILDING_DRAG;
				return false;
			}
			break;

		case "mousebuttonup":
			if (ev.button == SDL_BUTTON_LEFT)
			{
				// If queued, let the player continue placing another of the same building.
				let queued = Engine.HotkeyIsPressed("session.queue");
				if (tryPlaceBuilding(queued, Engine.HotkeyIsPressed("session.pushorderfront")))
				{
					if (queued && g_Selection.size())
						inputState = INPUT_BUILDING_PLACEMENT;
					else
						inputState = INPUT_NORMAL;
				}
				else
					inputState = INPUT_BUILDING_PLACEMENT;
				return true;
			}
			break;

		case "mousebuttondown":
			if (ev.button == SDL_BUTTON_RIGHT)
			{
				// Cancel building.
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
				// Cancel building.
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

			// Update the structure placement preview, and by extension, the list of snapping candidate entities for both (!)
			// the ending point and the starting point to snap to.
			//
			// TODO: Note that here, we need to fetch all similar entities, including any offscreen ones, to support the case
			// where the snap entity for the starting point has moved offscreen, or has been deleted/destroyed, or was a
			// foundation and has been replaced with a completed entity since the user first chose it. Fetching all towers on
			// the entire map instead of only the current screen might get expensive fast since walls all have a ton of towers
			// in them. Might be useful to query only for entities within a certain range around the starting point and ending
			// points.

			placementSupport.wallSnapEntitiesIncludeOffscreen = true;
			let result = updateBuildingPlacementPreview(); // includes an update of the snap entity candidates
			if (result && result.cost)
			{
				let neededResources = Engine.GuiInterfaceCall("GetNeededResources", { "cost": result.cost });
				placementSupport.tooltipMessage = [
					getEntityCostTooltip(result),
					getNeededResourcesTooltip(neededResources)
				].filter(tip => tip).join("\n");
			}

			break;

		case "mousebuttondown":
			if (ev.button == SDL_BUTTON_LEFT)
			{
				let queued = Engine.HotkeyIsPressed("session.queue");
				if (tryPlaceWall(queued))
				{
					if (queued)
					{
						// Continue building, just set a new starting position where we left off.
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
					placementSupport.tooltipMessage = translate("Cannot build wall here!");

				updateBuildingPlacementPreview();
				return true;
			}
			if (ev.button == SDL_BUTTON_RIGHT)
			{
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
			if (g_DragStart.distanceTo(ev) >= Math.square(getMaxDragDelta()))
				// Rotate in the direction of the cursor.
				placementSupport.angle = placementSupport.position.horizAngleTo(Engine.GetTerrainAtScreenPoint(ev.x, ev.y));
			else
				// If the cursor is near the center, snap back to the default orientation.
				placementSupport.SetDefaultAngle();

			let snapData = Engine.GuiInterfaceCall("GetFoundationSnapData", {
				"template": placementSupport.template,
				"x": placementSupport.position.x,
				"z": placementSupport.position.z,
				"angle": placementSupport.angle,
				"snapToEdges": isSnapToEdgesEnabled() && Engine.GetEdgesOfStaticObstructionsOnScreenNearTo(
					placementSupport.position.x, placementSupport.position.z)
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
				// If queued, let the player continue placing another of the same structure.
				let queued = Engine.HotkeyIsPressed("session.queue");
				if (tryPlaceBuilding(queued, Engine.HotkeyIsPressed("session.pushorderfront")))
				{
					if (queued && g_Selection.size())
						inputState = INPUT_BUILDING_PLACEMENT;
					else
						inputState = INPUT_NORMAL;
				}
				else
					inputState = INPUT_BUILDING_PLACEMENT;
				return true;
			}
			break;

		case "mousebuttondown":
			if (ev.button == SDL_BUTTON_RIGHT)
			{
				// Cancel building.
				placementSupport.Reset();
				inputState = INPUT_NORMAL;
				return true;
			}
			break;
		}
		break;

	case INPUT_BATCHTRAINING:
		if (ev.type == "hotkeyup" && ev.hotkey == "session.batchtrain")
		{
			flushTrainingBatch();
			inputState = INPUT_NORMAL;
		}
		break;
	}

	return false;
}

function handleInputAfterGui(ev)
{
	if (GetSimState().cinemaPlaying)
		return false;

	if (ev.hotkey === undefined)
		ev.hotkey = null;

	if (ev.hotkey == "session.highlightguarding")
	{
		g_ShowGuarding = (ev.type == "hotkeypress");
		updateAdditionalHighlight();
	}
	else if (ev.hotkey == "session.highlightguarded")
	{
		g_ShowGuarded = (ev.type == "hotkeypress");
		updateAdditionalHighlight();
	}

	if (inputState != INPUT_NORMAL && inputState != INPUT_SELECTING)
		clickedEntity = INVALID_ENTITY;

	// State-machine processing:

	switch (inputState)
	{
	case INPUT_NORMAL:
		switch (ev.type)
		{
		case "mousemotion":
			let ent = Engine.PickEntityAtPoint(ev.x, ev.y);
			if (ent != INVALID_ENTITY)
				g_Selection.setHighlightList([ent]);
			else
				g_Selection.setHighlightList([]);

			return false;

		case "mousebuttondown":
			if (Engine.HotkeyIsPressed("session.flare") && controlsPlayer(g_ViewedPlayer))
			{
				triggerFlareAction(Engine.GetTerrainAtScreenPoint(ev.x, ev.y));
				return true;
			}
			if (ev.button == SDL_BUTTON_LEFT)
			{
				g_DragStart = new Vector2D(ev.x, ev.y);
				inputState = INPUT_SELECTING;
				// If a single click occured, reset the clickedEntity.
				// Also set it if we're double/triple clicking and missed the unit earlier.
				if (ev.clicks == 1 || clickedEntity == INVALID_ENTITY)
					clickedEntity = Engine.PickEntityAtPoint(ev.x, ev.y);
				return true;
			}
			else if (ev.button == SDL_BUTTON_RIGHT)
			{
				if (!controlsPlayer(g_ViewedPlayer))
					break;
				g_DragStart = new Vector2D(ev.x, ev.y);
				inputState = INPUT_UNIT_POSITION_START;
			}
			break;

		case "hotkeypress":
			if (ev.hotkey.indexOf("selection.group.") == 0)
			{
				let now = Date.now();
				if (now - doublePressTimer < doublePressTime && ev.hotkey == prevHotkey)
				{
					if (ev.hotkey.indexOf("selection.group.select.") == 0)
					{
						let sptr = ev.hotkey.split(".");
						performGroup("snap", sptr[3] - 1);
					}
				}
				else
				{
					let sptr = ev.hotkey.split(".");
					performGroup(sptr[2], sptr[3] - 1);

					doublePressTimer = now;
					prevHotkey = ev.hotkey;
				}
			}
			break;
		}
		break;

	case INPUT_PRESELECTEDACTION:
		switch (ev.type)
		{
		case "mousemotion":
			let ent = Engine.PickEntityAtPoint(ev.x, ev.y);
			if (ent != INVALID_ENTITY)
				g_Selection.setHighlightList([ent]);
			else
				g_Selection.setHighlightList([]);

			return false;

		case "mousebuttondown":
			if (ev.button == SDL_BUTTON_LEFT && preSelectedAction != ACTION_NONE)
			{
				let action = determineAction(ev.x, ev.y);
				if (!action)
					break;
				if (!Engine.HotkeyIsPressed("session.queue") && !Engine.HotkeyIsPressed("session.orderone"))
				{
					preSelectedAction = ACTION_NONE;
					inputState = INPUT_NORMAL;
				}
				return doAction(action, ev);
			}
			if (ev.button == SDL_BUTTON_RIGHT && preSelectedAction != ACTION_NONE)
			{
				preSelectedAction = ACTION_NONE;
				inputState = INPUT_NORMAL;
				break;
			}
		default:
			// Slight hack: If selection is empty, reset the input state.
			if (!g_Selection.size())
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
			if (g_DragStart.distanceTo(ev) >= getMaxDragDelta())
			{
				inputState = INPUT_BANDBOXING;
				return false;
			}

			let ent = Engine.PickEntityAtPoint(ev.x, ev.y);
			if (ent != INVALID_ENTITY)
				g_Selection.setHighlightList([ent]);
			else
				g_Selection.setHighlightList([]);
			return false;

		case "mousebuttonup":
			if (ev.button == SDL_BUTTON_LEFT)
			{
				if (clickedEntity == INVALID_ENTITY)
					clickedEntity = Engine.PickEntityAtPoint(ev.x, ev.y);
				// Abort if we didn't click on an entity or if the entity was removed before the mousebuttonup event.
				if (clickedEntity == INVALID_ENTITY || !GetEntityState(clickedEntity))
				{
					clickedEntity = INVALID_ENTITY;
					if (!Engine.HotkeyIsPressed("selection.add") && !Engine.HotkeyIsPressed("selection.remove"))
					{
						g_Selection.reset();
						resetIdleUnit();
					}
					inputState = INPUT_NORMAL;
					return true;
				}

				if (Engine.GetFollowedEntity() != clickedEntity)
					Engine.CameraFollow(0);

				let ents = [];
				if (ev.clicks == 1)
					ents = [clickedEntity];
				else
				{
					let showOffscreen = Engine.HotkeyIsPressed("selection.offscreen");
					let matchRank = true;
					let templateToMatch;

					if (ev.clicks == 2)
					{
						templateToMatch = GetEntityState(clickedEntity).identity.selectionGroupName;
						if (templateToMatch)
							matchRank = false;
						else
							// No selection group name defined, so fall back to exact match.
							templateToMatch = GetEntityState(clickedEntity).template;

					}
					else
						// Triple click
						// Select units matching exact template name (same rank).
						templateToMatch = GetEntityState(clickedEntity).template;

					// TODO: Should we handle "control all units" here as well?
					ents = Engine.PickSimilarPlayerEntities(templateToMatch, showOffscreen, matchRank, false);
				}

				if (Engine.HotkeyIsPressed("selection.add"))
					g_Selection.addList(ents);
				else if (Engine.HotkeyIsPressed("selection.remove"))
					g_Selection.removeList(ents);
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

	case INPUT_UNIT_POSITION_START:
		switch (ev.type)
		{
		case "mousemotion":
			if (g_DragStart.distanceToSquared(ev) >= Math.square(getMaxDragDelta()))
			{
				inputState = INPUT_UNIT_POSITION;
				return false;
			}
			break;
		case "mousebuttonup":
			inputState = INPUT_NORMAL;
			if (ev.button == SDL_BUTTON_RIGHT)
			{
				let action = determineAction(ev.x, ev.y);
				if (action)
					return doAction(action, ev);
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
				// itself happens in the call to updateBuildingPlacementPreview below.)
				placementSupport.wallSnapEntitiesIncludeOffscreen = false;
			}
			else
			{
				if (placementSupport.template && Engine.GuiInterfaceCall("GetNeededResources", { "cost": GetTemplateData(placementSupport.template).cost }))
				{
					placementSupport.Reset();
					inputState = INPUT_NORMAL;
					return true;
				}

				if (isSnapToEdgesEnabled())
				{
					// We need to reset the angle before the snapping to edges,
					// because we want to get the angle near to the default one.
					placementSupport.SetDefaultAngle();
				}
				let snapData = Engine.GuiInterfaceCall("GetFoundationSnapData", {
					"template": placementSupport.template,
					"x": placementSupport.position.x,
					"z": placementSupport.position.z,
					"angle": placementSupport.angle,
					"snapToEdges": isSnapToEdgesEnabled() && Engine.GetEdgesOfStaticObstructionsOnScreenNearTo(
						placementSupport.position.x, placementSupport.position.z)
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
					let validPlacement = updateBuildingPlacementPreview();
					if (validPlacement !== false)
						inputState = INPUT_BUILDING_WALL_CLICK;
				}
				else
				{
					placementSupport.position = Engine.GetTerrainAtScreenPoint(ev.x, ev.y);

					if (isSnapToEdgesEnabled())
					{
						let snapData = Engine.GuiInterfaceCall("GetFoundationSnapData", {
							"template": placementSupport.template,
							"x": placementSupport.position.x,
							"z": placementSupport.position.z,
							"angle": placementSupport.angle,
							"snapToEdges": Engine.GetEdgesOfStaticObstructionsOnScreenNearTo(
								placementSupport.position.x, placementSupport.position.z)
						});
						if (snapData)
						{
							placementSupport.angle = snapData.angle;
							placementSupport.position.x = snapData.x;
							placementSupport.position.z = snapData.z;
						}
					}

					g_DragStart = new Vector2D(ev.x, ev.y);
					inputState = INPUT_BUILDING_CLICK;
				}
				return true;
			}
			else if (ev.button == SDL_BUTTON_RIGHT)
			{
				// Cancel building.
				placementSupport.Reset();
				inputState = INPUT_NORMAL;
				return true;
			}
			break;

		case "hotkeydown":

			let rotation_step = Math.PI / 12; // 24 clicks make a full rotation

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
	case INPUT_FLARE:
		if (ev.type == "mousebuttondown")
		{
			if (ev.button == SDL_BUTTON_LEFT && controlsPlayer(g_ViewedPlayer))
			{
				triggerFlareAction(Engine.GetTerrainAtScreenPoint(ev.x, ev.y));
				inputState = INPUT_NORMAL;
				return true;
			}
			else if (ev.button == SDL_BUTTON_RIGHT)
			{
				inputState = INPUT_NORMAL;
				return true;
			}
		}
	}
	return false;
}

function doAction(action, ev)
{
	if (!controlsPlayer(g_ViewedPlayer))
		return false;

	return handleUnitAction(Engine.GetTerrainAtScreenPoint(ev.x, ev.y), action);
}

function popOneFromSelection(action)
{
	// Pick the first unit that can do this order.
	let unit = action.firstAbleEntity || g_Selection.find(entity =>
		["preSelectedActionCheck", "hotkeyActionCheck", "actionCheck"].some(method =>
			g_UnitActions[action.type][method] &&
			g_UnitActions[action.type][method](action.target || undefined, [entity])
		));
	if (unit)
	{
		g_Selection.removeList([unit], false);
		return [unit];
	}
	return null;
}

function positionUnitsFreehandSelectionMouseMove(ev)
{
	// Converting the input line into a List of points.
	// For better performance the points must have a minimum distance to each other.
	let target = Vector2D.from3D(Engine.GetTerrainAtScreenPoint(ev.x, ev.y));
	if (!g_FreehandSelection_InputLine.length ||
	    target.distanceToSquared(g_FreehandSelection_InputLine[g_FreehandSelection_InputLine.length - 1]) >=
	    g_FreehandSelection_ResolutionInputLineSquared)
		g_FreehandSelection_InputLine.push(target);
	return false;
}

function positionUnitsFreehandSelectionMouseUp(ev)
{
	inputState = INPUT_NORMAL;
	let inputLine = g_FreehandSelection_InputLine;
	g_FreehandSelection_InputLine = [];
	if (ev.button != SDL_BUTTON_RIGHT)
		return true;

	let lengthOfLine = 0;
	for (let i = 1; i < inputLine.length; ++i)
		lengthOfLine += inputLine[i].distanceTo(inputLine[i - 1]);

	const selection = g_Selection.filter(ent => !!GetEntityState(ent).unitAI).sort((a, b) => a - b);

	// Checking the line for a minimum length to save performance.
	if (lengthOfLine < g_FreehandSelection_MinLengthOfLine || selection.length < g_FreehandSelection_MinNumberOfUnits)
	{
		let action = determineAction(ev.x, ev.y);
		return !!action && doAction(action, ev);
	}

	// Even distribution of the units on the line.
	let p0 = inputLine[0];
	let entityDistribution = [p0];
	let distanceBetweenEnts = lengthOfLine / (selection.length - 1);
	let freeDist = -distanceBetweenEnts;

	for (let i = 1; i < inputLine.length; ++i)
	{
		let p1 = inputLine[i];
		freeDist += inputLine[i - 1].distanceTo(p1);

		while (freeDist >= 0)
		{
			p0 = Vector2D.sub(p0, p1).normalize().mult(freeDist).add(p1);
			entityDistribution.push(p0);
			freeDist -= distanceBetweenEnts;
		}
	}

	// Rounding errors can lead to missing or too many points.
	entityDistribution = entityDistribution.slice(0, selection.length);
	entityDistribution = entityDistribution.concat(new Array(selection.length - entityDistribution.length).fill(inputLine[inputLine.length - 1]));

	if (Vector2D.from3D(GetEntityState(selection[0]).position).distanceTo(entityDistribution[0]) +
	    Vector2D.from3D(GetEntityState(selection[selection.length - 1]).position).distanceTo(entityDistribution[selection.length - 1]) >
	    Vector2D.from3D(GetEntityState(selection[0]).position).distanceTo(entityDistribution[selection.length - 1]) +
	    Vector2D.from3D(GetEntityState(selection[selection.length - 1]).position).distanceTo(entityDistribution[0]))
		entityDistribution.reverse();

	Engine.PostNetworkCommand({
		"type": isAttackMovePressed() ? "attack-walk-custom" : "walk-custom",
		"entities": selection,
		"targetPositions": entityDistribution.map(pos => pos.toFixed(2)),
		"targetClasses": Engine.HotkeyIsPressed("session.attackmoveUnit") ? { "attack": ["Unit"] } : { "attack": ["Unit", "Structure"] },
		"queued": Engine.HotkeyIsPressed("session.queue"),
		"pushFront": Engine.HotkeyIsPressed("session.pushorderfront"),
		"formation": NULL_FORMATION,
	});

	// Add target markers with a minimum distance of 5 to each other.
	let entitiesBetweenMarker = Math.ceil(5 / distanceBetweenEnts);
	for (let i = 0; i < entityDistribution.length; i += entitiesBetweenMarker)
		DrawTargetMarker({ "x": entityDistribution[i].x, "z": entityDistribution[i].y });

	Engine.GuiInterfaceCall("PlaySound", {
		"name": "order_walk",
		"entity": selection[0]
	});

	return true;
}

function triggerFlareAction(target)
{
		let now = Date.now();
		if (g_LastFlareTime && now < g_LastFlareTime + g_FlareCooldown)
			return;

		g_LastFlareTime = now;
		displayFlare(target, Engine.GetPlayerID());
		Engine.PlayUISound(g_FlareSound, false);
		Engine.PostNetworkCommand({
			"type": "map-flare",
			"target": target
		});
}

function handleUnitAction(target, action)
{
	if (!g_UnitActions[action.type] || !g_UnitActions[action.type].execute)
	{
		error("Invalid action.type " + action.type);
		return false;
	}

	let selection = Engine.HotkeyIsPressed("session.orderone") &&
		popOneFromSelection(action) || g_Selection.toList();
	// If the session.queue hotkey is down, add the order to the unit's order queue instead
	// of running it immediately. If the pushorderfront hotkey is down, execute the order
	// immidiately and continue the rest of the queue afterwards.
	return g_UnitActions[action.type].execute(
		target,
		action,
		selection,
		Engine.HotkeyIsPressed("session.queue"),
		Engine.HotkeyIsPressed("session.pushorderfront"));
}

function getEntityLimitAndCount(playerState, entType)
{
	let ret = {
		"entLimit": undefined,
		"entCount": undefined,
		"entLimitChangers": undefined,
		"canBeAddedCount": undefined,
		"matchLimit": undefined,
		"matchCount": undefined,
		"type": undefined
	};
	if (!playerState.entityLimits)
		return ret;
	let template = GetTemplateData(entType);
	let entCategory;
	let matchLimit;
	if (template.trainingRestrictions)
	{
		entCategory = template.trainingRestrictions.category;
		matchLimit = template.trainingRestrictions.matchLimit;
		ret.type = "training";
	}
	else if (template.buildRestrictions)
	{
		entCategory = template.buildRestrictions.category;
		matchLimit = template.buildRestrictions.matchLimit;
		ret.type = "build";
	}

	if (entCategory && playerState.entityLimits[entCategory] !== undefined)
	{
		ret.entLimit = playerState.entityLimits[entCategory] || 0;
		ret.entCount = playerState.entityCounts[entCategory] || 0;
		ret.entLimitChangers = playerState.entityLimitChangers[entCategory];
		ret.canBeAddedCount = Math.max(ret.entLimit - ret.entCount, 0);
	}

	if (matchLimit)
	{
		ret.matchLimit = matchLimit;
		ret.matchCount = playerState.matchEntityCounts[entType] || 0;
		ret.canBeAddedCount = Math.min(Math.max(ret.entLimit - ret.entCount, 0), Math.max(ret.matchLimit - ret.matchCount, 0));
	}
	return ret;
}

/**
 * Called by GUI when user clicks construction button.
 * @param {string} buildTemplate - Template name of the entity the user wants to build.
 */
function startBuildingPlacement(buildTemplate, playerState)
{
	if (getEntityLimitAndCount(playerState, buildTemplate).canBeAddedCount == 0)
		return;

	// TODO: we should clear any highlight selection rings here. If the cursor was over an entity before going onto the GUI
	// to start building a structure, then the highlight selection rings are kept during the construction of the structure.
	// Gives the impression that somehow the hovered-over entity has something to do with the structure you're building.

	placementSupport.Reset();

	let templateData = GetTemplateData(buildTemplate);
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

	if (templateData.attack &&
		templateData.attack.Ranged &&
		templateData.attack.Ranged.maxRange)
		placementSupport.attack = templateData.attack;
}

// Batch training:
// When the user shift-clicks, we set these variables and switch to INPUT_BATCHTRAINING
// When the user releases shift, or clicks on a different training button, we create the batched units
var g_BatchTrainingEntities;
var g_BatchTrainingType;
var g_NumberOfBatches;
var g_BatchTrainingEntityAllowedCount;
var g_BatchSize = getDefaultBatchTrainingSize();

function OnTrainMouseWheel(dir)
{
	if (!Engine.HotkeyIsPressed("session.batchtrain"))
		return;

	g_BatchSize += dir / Engine.ConfigDB_GetValue("user", "gui.session.scrollbatchratio");
	if (g_BatchSize < 1 || !Number.isFinite(g_BatchSize))
		g_BatchSize = 1;

	updateSelectionDetails();
}

function getBuildingsWhichCanTrainEntity(entitiesToCheck, trainEntType)
{
	return entitiesToCheck.filter(entity => {
		const state = GetEntityState(entity);
		return state?.trainer?.entities?.includes(trainEntType) &&
			(!state.upgrade || !state.upgrade.isUpgrading);
	});
}

function initBatchTrain()
{
	registerConfigChangeHandler(changes => {
		if (changes.has("gui.session.batchtrainingsize"))
			updateDefaultBatchSize();
	});
}

function getDefaultBatchTrainingSize()
{
	let num = +Engine.ConfigDB_GetValue("user", "gui.session.batchtrainingsize");
	return Number.isInteger(num) && num > 0 ? num : 5;
}

function getBatchTrainingSize()
{
	return Math.max(Math.round(g_BatchSize), 1);
}

function updateDefaultBatchSize()
{
	g_BatchSize = getDefaultBatchTrainingSize();
}

/**
 * Add the unit shown at position to the training queue for all entities in the selection.
 * @param {number} position - The position of the template to train.
 */
function addTrainingByPosition(position)
{
	let playerState = GetSimState().players[Engine.GetPlayerID()];
	let selection = g_Selection.toList();

	if (!playerState || !selection.length)
		return;

	let trainableEnts = getAllTrainableEntitiesFromSelection();
	let entToTrain = trainableEnts[position];
	if (!entToTrain)
		return;

	addTrainingToQueue(selection, entToTrain, playerState);
}

// Called by GUI when user clicks training button
function addTrainingToQueue(selection, trainEntType, playerState)
{
	let appropriateBuildings = getBuildingsWhichCanTrainEntity(selection, trainEntType);

	let canBeAddedCount = getEntityLimitAndCount(playerState, trainEntType).canBeAddedCount;

	let decrement = Engine.HotkeyIsPressed("selection.remove");
	let template;
	if (!decrement)
		template = GetTemplateData(trainEntType);

	// Batch training only possible if we can train at least 2 units.
	if (Engine.HotkeyIsPressed("session.batchtrain") && (canBeAddedCount == undefined || canBeAddedCount > 1))
	{
		if (inputState == INPUT_BATCHTRAINING)
		{
			// Check if we are training in the same structure(s) as the last batch.
			// NOTE: We just check if the arrays are the same and if the order is the same.
			// If the order changed, we have a new selection and we should create a new batch.
			// If we're already creating a batch of this unit (in the same structure(s)), then just extend it
			// (if training limits allow).
			if (g_BatchTrainingEntities.length == selection.length &&
			    g_BatchTrainingEntities.every((ent, i) => ent == selection[i]) &&
			    g_BatchTrainingType == trainEntType)
			{
				if (decrement)
				{
					--g_NumberOfBatches;
					if (g_NumberOfBatches <= 0)
						inputState = INPUT_NORMAL;
				}
				else if (canBeAddedCount == undefined ||
				         canBeAddedCount > g_NumberOfBatches * getBatchTrainingSize() * appropriateBuildings.length)
				{
					if (Engine.GuiInterfaceCall("GetNeededResources", {
						"cost": multiplyEntityCosts(template, (g_NumberOfBatches + 1) * getBatchTrainingSize())
					}))
						return;

					++g_NumberOfBatches;
				}
				g_BatchTrainingEntityAllowedCount = canBeAddedCount;
				return;
			}
			else if (!decrement)
				flushTrainingBatch();
		}

		if (decrement || Engine.GuiInterfaceCall("GetNeededResources", {
				"cost": multiplyEntityCosts(template, getBatchTrainingSize())
			}))
			return;

		inputState = INPUT_BATCHTRAINING;
		g_BatchTrainingEntities = selection;
		g_BatchTrainingType = trainEntType;
		g_BatchTrainingEntityAllowedCount = canBeAddedCount;
		g_NumberOfBatches = 1;
	}
	else
	{
		let buildingsForTraining = appropriateBuildings;
		if (canBeAddedCount !== undefined)
			buildingsForTraining = buildingsForTraining.slice(0, canBeAddedCount);
		Engine.PostNetworkCommand({
			"type": "train",
			"template": trainEntType,
			"count": 1,
			"entities": buildingsForTraining,
			"pushFront": Engine.HotkeyIsPressed("session.pushorderfront")
		});
	}
}

/**
 * Returns the number of units that will be present in a batch if the user clicks
 * the training button depending on the batch training modifier hotkey.
 */
function getTrainingStatus(selection, trainEntType, playerState)
{
	let appropriateBuildings = getBuildingsWhichCanTrainEntity(selection, trainEntType);
	let nextBatchTrainingCount = 0;

	let canBeAddedCount;
	if (inputState == INPUT_BATCHTRAINING && g_BatchTrainingType == trainEntType)
	{
		nextBatchTrainingCount = g_NumberOfBatches * getBatchTrainingSize();
		canBeAddedCount = g_BatchTrainingEntityAllowedCount;
	}
	else
		canBeAddedCount = getEntityLimitAndCount(playerState, trainEntType).canBeAddedCount;

	// We need to calculate count after the next increment if possible.
	if ((canBeAddedCount == undefined || canBeAddedCount > nextBatchTrainingCount * appropriateBuildings.length) &&
	    Engine.HotkeyIsPressed("session.batchtrain"))
		nextBatchTrainingCount += getBatchTrainingSize();

	nextBatchTrainingCount = Math.max(nextBatchTrainingCount, 1);

	// If training limits don't allow us to train batchedSize in each appropriate structure,
	// train as many full batches as we can and the remainder in one more structure.
	let buildingsCountToTrainFullBatch = appropriateBuildings.length;
	let remainderToTrain = 0;
	if (canBeAddedCount !== undefined &&
	    canBeAddedCount < nextBatchTrainingCount * appropriateBuildings.length)
	{
		buildingsCountToTrainFullBatch = Math.floor(canBeAddedCount / nextBatchTrainingCount);
		remainderToTrain = canBeAddedCount % nextBatchTrainingCount;
	}

	return [buildingsCountToTrainFullBatch, nextBatchTrainingCount, remainderToTrain];
}

function flushTrainingBatch()
{
	let batchedSize = g_NumberOfBatches * getBatchTrainingSize();
	let appropriateBuildings = getBuildingsWhichCanTrainEntity(g_BatchTrainingEntities, g_BatchTrainingType);
	// If training limits don't allow us to train batchedSize in each appropriate structure.
	if (g_BatchTrainingEntityAllowedCount !== undefined &&
		g_BatchTrainingEntityAllowedCount < batchedSize * appropriateBuildings.length)
	{
		// Train as many full batches as we can.
		let buildingsCountToTrainFullBatch = Math.floor(g_BatchTrainingEntityAllowedCount / batchedSize);
		Engine.PostNetworkCommand({
			"type": "train",
			"entities": appropriateBuildings.slice(0, buildingsCountToTrainFullBatch),
			"template": g_BatchTrainingType,
			"count": batchedSize,
			"pushFront": Engine.HotkeyIsPressed("session.pushorderfront")
		});

		// Train remainer in one more structure.
		let remainer = g_BatchTrainingEntityAllowedCount % batchedSize;
		if (remainer)
			Engine.PostNetworkCommand({
				"type": "train",
				"entities": [appropriateBuildings[buildingsCountToTrainFullBatch]],
				"template": g_BatchTrainingType,
				"count": remainer,
				"pushFront": Engine.HotkeyIsPressed("session.pushorderfront")
			});
	}
	else
		Engine.PostNetworkCommand({
			"type": "train",
			"entities": appropriateBuildings,
			"template": g_BatchTrainingType,
			"count": batchedSize,
			"pushFront": Engine.HotkeyIsPressed("session.pushorderfront")
		});
}

function performGroup(action, groupId)
{
	if (g_Groups.groups[groupId] === undefined)
	{
		warn("Invalid groupId " + groupId);
		return;
	}
	switch (action)
	{
	case "snap":
	case "select":
	case "add":
		let toSelect = [];
		g_Groups.update();
		for (let ent in g_Groups.groups[groupId].ents)
			toSelect.push(+ent);

		if (action != "add")
			g_Selection.reset();

		g_Selection.addList(toSelect);

		if (action == "snap" && toSelect.length)
		{
			let entState = GetEntityState(getEntityOrHolder(toSelect[0]));
			let position = entState.position;
			if (position && entState.visibility != "hidden")
				Engine.CameraMoveTo(position.x, position.z);
		}
		break;
	case "save":
	case "breakUp":
		g_Groups.groups[groupId].reset();

		if (action == "save")
			g_Groups.addEntities(groupId, g_Selection.toList());

		updateGroups();
		break;
	}
}

var lastIdleUnit = 0;
var currIdleClassIndex = 0;
var lastIdleClasses = [];

function resetIdleUnit()
{
	lastIdleUnit = 0;
	currIdleClassIndex = 0;
	lastIdleClasses = [];
}

function findIdleUnit(classes)
{
	let append = Engine.HotkeyIsPressed("selection.add");
	let selectall = Engine.HotkeyIsPressed("selection.offscreen");

	// Reset the last idle unit, etc., if the selection type has changed.
	if (selectall || classes.length != lastIdleClasses.length || !classes.every((v, i) => v === lastIdleClasses[i]))
		resetIdleUnit();
	lastIdleClasses = classes;

	let data = {
		"viewedPlayer": g_ViewedPlayer,
		"excludeUnits": append ? g_Selection.toList() : [],
		// If the current idle class index is not 0, put the class at that index first.
		"idleClasses": classes.slice(currIdleClassIndex, classes.length).concat(classes.slice(0, currIdleClassIndex))
	};
	if (!selectall)
	{
		data.limit = 1;
		data.prevUnit = lastIdleUnit;
	}

	let idleUnits = Engine.GuiInterfaceCall("FindIdleUnits", data);
	if (!idleUnits.length)
	{
		// TODO: display a message to indicate no more idle units, or something
		Engine.GuiInterfaceCall("PlaySoundForPlayer", {
			"name": "no_idle_unit"
		});
		resetIdleUnit();
		return;
	}

	if (!append)
		g_Selection.reset();
	g_Selection.addList(idleUnits);

	if (selectall)
		return;

	lastIdleUnit = idleUnits[0];
	let entityState = GetEntityState(lastIdleUnit);
	if (entityState.position)
		Engine.CameraMoveTo(entityState.position.x, entityState.position.z);

	// Move the idle class index to the first class an idle unit was found for.
	let indexChange = data.idleClasses.findIndex(elem => MatchesClassList(entityState.identity.classes, elem));
	currIdleClassIndex = (currIdleClassIndex + indexChange) % classes.length;
}

function clearSelection()
{
	if (inputState==INPUT_BUILDING_PLACEMENT || inputState==INPUT_BUILDING_WALL_PATHING)
	{
		inputState = INPUT_NORMAL;
		placementSupport.Reset();
	}
	else
		g_Selection.reset();
	preSelectedAction = ACTION_NONE;
}
