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

	//console.write("worldClickHandler: button "+event.button+", clicks "+event.clicks);

	if (isSelecting())
	{
		getGlobal().selectionWorldClickHandler(event);
		return;
	}


	// Right button single- or double-clicks
	if (event.button == SDL_BUTTON_RIGHT && event.clicks <= 2)
	{
		var controlling = true;
		for (var i = 0; i < selection.length; i++) {
			if (! (selection[i].player.name == localPlayer.name)) {
				controlling = false;
				break;
			}
		}
		
		if (!controlling) {
			return;
		}
	
		if (event.clicks == 1) {
			cmd = event.order;
		}
		else if (event.clicks == 2)
		{
			// Use the secondary order, or, if the entites didn't vote for any order
			// (i.e. we are using the default of NMT_GOTO), use the NMT_RUN
		  if (event.order == NMT_GOTO )
				cmd = NMT_RUN;
			else
				cmd = event.secondaryOrder;
		}
	}
	else
	{
		return;
	}

	//setting rally point
	if ( getCursorName() == "cursor-rally" )
	{	
		for ( var i=0; i<selection.length; i++ )
			selection[i].setRallyPointAtCursor();	
		setCursor("arrow-default");
		return;
	}
	switch (cmd)
	{
	// location target commands
	case NMT_GOTO:
	case NMT_RUN:
	case NMT_PATROL:
		if (event.queued)
			cmd = NMT_ADD_WAYPOINT;
		args[0]=event.x;
		args[1]=event.y;
		break;
	case NMT_ADD_WAYPOINT:
		args[0]=event.x;
		args[1]=event.y;
		break;
	// entity target commands
	case NMT_CONTACT_ACTION:
		args[0]=event.entity;
		if ( event.clicks == 1)
			args[1]=event.action;
		else
			args[1]=event.secondaryAction;
		break;
	//TODO:  get rid of order() calls.
	case NMT_NOTIFY_REQUEST:
		if (event.clicks == 1)
			action = event.action;
		else
			action = event.secondaryAction;
		
		if (event.entity.isIdle())
		{
			for (var i=0; i<selection.length;i++)			
			{
				if (!selection[i].actions)
					continue;
				//console.write("Requesting notification for " + event.entity);
				//selection[i].requestNotification( event.entity, action, true, false );
				//selection[i].order( ORDER_GOTO, event.entity.position.x, event.entity.position.z -							selection[i].actions.escort.distance, true);
			}
		}
		else
		{
			for (var i=0; i<selection.length;i++)	
			{
				if (!selection[i].actions)
					continue;
				//console.write("Requesting notification for " + event.entity);		
				//selection[i].requestNotification( event.entity, action, true, false );
			}
		}	
		
		return;
	default:
		console.write("worldClickHandler: Unknown order: "+cmd);
		return;
	}
	
	if (cmd == NMT_CONTACT_ACTION) // For NMT_CONTACT_ACTION, add a third argument - whether to run
		issueCommand (selection, isOrderQueued(), cmd, args[0], args[1], event.clicks > 1);
	else
		issueCommand (selection, isOrderQueued(), cmd, args[0], args[1]);
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

	var dudeSpawnPoint = new Vector3D(x, y, z);
	new Entity(getEntityTemplate(MakeUnitName), dudeSpawnPoint, 1.0);
	// writeConsole(MakeUnitName + " created at " + DudeSpawnPoint);
}

// ====================================================================

function validProperty (propertyName)
{
	// Accepts a string representing an entity property (eg "selection[0].traits.id.generic")
	// and checks if all the elements (selection[0].traits, selection[0].traits.id, etc) are valid properties.
	// Returns false if any invalids are found. Returns true if the whole property is valid.

	// An empty string is always successful.
	if (propertyName == "") return true;
	
	// An undefined string is always unsuccessful.
	if (propertyName == undefined) return false;

	// Store elements of the property as an array of strings.
	var splitArray = propertyName.toString().split (".");
	
	// Seek through elements in array.
	var arrayString = "";
	for (var i = 0; i < splitArray.length; i++)
	{
		// Test each element combination of the string to ensure they are all valid.
		if (i > 0) arrayString += ".";
		arrayString += splitArray[i];

		// If the property name is not valid, return false.
		if (!(eval (arrayString)))
			return false;
	}
	// If we managed to check them all, every element in the property is valid. Return true.
	return true;
}


function killSelectedEntities()
{
      var allOwned = true;
      for (var i=0; i<selection.length && allOwned; i++)
      {
          if (localPlayer != selection[i].player)
          {
              allOwned = false;
          }
      }
      
      if (allOwned)
      {
		for (i=0; i<selection.length; i++)
		{
			//TODO: send network msg, so the unit is killed everywhere
			console.write('killing....');
			selection[i].kill();
			issueCommand(selection[i], NMT_REMOVE_OBJECT);
		}
      }
}

