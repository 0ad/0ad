/*
	DESCRIPTION	: Functions for the world click handler and manipulating entities.
	NOTES		: 
*/

// ====================================================================

addGlobalHandler ("worldClick", worldClickHandler);

// ====================================================================

// The world-click handler - called whenever the user clicks the terrain
function worldClickHandler(event)
{
	args=new Array(null, null);

	console.write("worldClickHandler: button "+event.button+", clicks "+event.clicks);

	if (isSelecting())
	{
		getGlobal().selectionWorldClickHandler(event);
		return;
	}


	// Right button single- or double-clicks
	if (event.button == SDL_BUTTON_RIGHT && event.clicks <= 2)
	{
		if (event.clicks == 1)
			cmd = event.order;
		else if (event.clicks == 2)
		{
			console.write("Issuing secondary order");
			cmd = event.secondaryOrder;
		}
	}
	else
	{
		return;
	}

	switch (cmd)
	{
		// location target commands
		case NMT_Goto:
		case NMT_Patrol:
			if (event.queued)
			{
				cmd = NMT_AddWaypoint;
			}
			args[0]=event.x;
			args[1]=event.y;
		break;
		case NMT_AddWaypoint:
			args[0]=event.x;
			args[1]=event.y;
		break;
		// entity target commands
		// I'm guessing we no longer require these now that they have become generic events?
//		case NMT_AttackMelee:
//		case NMT_Gather:
//		case NMT_Heal:
//			args[0]=event.entity;
//			args[1]=null;
//		break;
		case NMT_Generic:
			args[0]=event.entity;
			args[1]=event.action;
		break;
		default:
			console.write("worldClickHandler: Unknown order: "+cmd);
			return;
		break;
	}

	issueCommand (selection, cmd, args[0], args[1]);
}

// ====================================================================

function selectEntity(handler)
{
	endSelection();
	startSelection(function (event) {
			// Selection is performed when single-clicking the right mouse
			// button.
			if (event.button == SDL_BUTTON_RIGHT && event.clicks == 1)
			{
				handler(event.entity);
			}
			// End selection on first mouse-click
			endSelection();
		});
}

// ====================================================================

function selectLocation(handler)
{
	endSelection();
	startSelection(function (event) {
			// Selection is performed when single-clicking the right mouse
			// button.
			if (event.button == SDL_BUTTON_RIGHT && event.clicks == 1)
			{
				handler(event.x, event.y);
			}
			// End selection on first mouse-click
			endSelection();
		});
}

// ====================================================================

function startSelection(handler)
{
	gameView.startCustomSelection();
	getGlobal().selectionWorldClickHandler=handler;
	console.write("isSelecting(): "+isSelecting());
}

// ====================================================================

function endSelection()
{
	if (!isSelecting())
		return;
	
	gameView.endCustomSelection();
	getGlobal().selectionWorldClickHandler = null;
}

// ====================================================================

function isSelecting()
{
	return getGlobal().selectionWorldClickHandler != null;
}

// ====================================================================

function makeUnit (x, y, z, MakeUnitName)
{
        // Spawn an entity at the given coordinates.

        DudeSpawnPoint = new Vector3D(x, y, z);
        new Entity(getEntityTemplate(MakeUnitName), DudeSpawnPoint, 1.0);
        // writeConsole(MakeUnitName + " created at " + DudeSpawnPoint);
}

// ====================================================================

function selected()
{
        // Returns how many units selected.

        if( selection.length > 0 )
                return( selection[0] );
        return( null );
}

// ====================================================================




