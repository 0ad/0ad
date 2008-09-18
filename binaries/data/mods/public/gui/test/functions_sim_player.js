/*
	DESCRIPTION	: Functions for manipulating players and their properties (eg resources).
	NOTES		: 
*/

// ====================================================================

function createResourceCounters()
{
	resourceUIArray = new Array();
	
	addResourceCounter(0, "food");
	addResourceCounter(1, "wood");
	addResourceCounter(2, "stone");
	addResourceCounter(3, "metal");
	addResourceCounter(4, "population");
	
	refreshResource("Food", 0);
	refreshResource("Wood", 1);
	refreshResource("Stone", 2);
	refreshResource("Metal", 3);
	refreshResource("Population", 4);
}

// ====================================================================

function addResourceCounter (index, resourceName)
{
	// Creates a resource counter widget.

	// Ensure resource name is lower-case.
	resourceName = resourceName.toLowerCase();
	
	// We maintain a sorted array of the resource indexes so that the UI counters can be refreshed in centered order.
	// (We don't include Housing in the list, though, as it does not have its own resource counter.)
	if (resourceName != "housing")
	{
		//  If it's an even index,
		if (index % 2 == 0)
			// Add it to the end of the UI array.
			resourceUIArray.push ( index );
		else
			// Add it to the beginning of the UI array,
			resourceUIArray.unshift ( index );
	}
	
	//console.write( "Added " + resourceName /*+ " (" + resourceQty + ")"*/ );
}

// ====================================================================

// HACK: Keep track of old resource values so we only update resource counters as necessary.
// We should really find a *much* faster way to render those counters, since resource values are bound to change quite quickly in a real game.
var oldResources = new Object();

function refreshResources ()
{
	// Refreshes all resource counters after update.

	var resourcePool = localPlayer.resources;
	
	var shouldRefresh = false;
	for (var currResource in resourcePool) 
	{
		if( oldResources[currResource] != localPlayer.resources[currResource] )
		{
			oldResources[currResource] = localPlayer.resources[currResource].valueOf();
			shouldRefresh = true;
		}
	}
	
	if( shouldRefresh )
	{
		for( var i=0; i<2; i++ )	// 2 refreshes seem to be necessary for proper alignment
		{
			var resourceCount = 0;
			for (var currResource in resourcePool) 
			{
				if(currResource != "housing")
				{
					// Pass the array index of the resource as the second parameter (as we'll need that to determine the centered screen position of each counter).
					refreshResource (toTitleCase(currResource), resourceUIArray[resourceCount]);
					resourceCount++;
				}
			}
		}
	}
}

// ====================================================================

function refreshResource (resourceName, resourceIndex)
{
	// Refresh the onscreen counter for a given resource (called to recalculate the size of the coordinates, as these dynamically adjust depending on character length).
	
	// Ensure resource name is title-case.
	resourceName = toTitleCase (resourceName);

	// Ignore the "Housing" resource ... It doesn't work like normal resources and doesn't have a counter to resize.
	if (resourceName == "Housing")
		return;
		
	// Get resource readout object.
	var resourceObject = getGUIObjectByName ("snResourceCounter_" + (resourceIndex + 1));
	// Get resource icon object.
	var resourceIconObject = getGUIObjectByName ("snResourceCounterIcon_" + (resourceIndex + 1));
	
	// Update counter caption (since we need to have up-to-date text to determine the length of the counter).
	var caption = parseInt( localPlayer.resources[resourceName.toLowerCase()] );
	// The Population counter also lists the amount of available housing.		
	if (resourceName == "Population")
		caption	+= "/" + localPlayer.resources["housing"];	
	
	resourceObject.caption = caption;
	
	// Update counter tooltip.
	resourceObject.tooltip = resourceName + ": " + resourceObject.caption;
	
	// Set resource icon.
	resourceIconObject.cell_id = cellGroup["Resource"][resourceName.toLowerCase()].id;
			
	// Get the index of the resource readout to be resized.
	var crdResult = getCrd (resourceObject.name, true);
	// Get the index of the resource icon.
	var crdIconResult = getCrd (resourceIconObject.name, true);	
	
	// For each coordinate group stored for this control,
	for (var coordGroup in Crd[crdResult].coord)
	{
		// Set width of readout based on length of caption.
		Crd[crdResult].coord[coordGroup].width = snConst.MiniIcon.Width+5 + resourceObject.caption.length * 10;
	
		// Set X and Y position and height so that resources always are immediately besides each other. (Except for the first resource (usually Food), which has the initial starting position)
		
		// Determine starting position for the first resource (so that the resources wind up being centered across the screen).
		if (resourceIndex == 0)
		{
			// The first coordinate is in the exact centre of the screen.
			Crd[crdResult].coord[coordGroup].x = Math.round (-(Crd[crdResult].coord[coordGroup].width/2) - 5);
		}
		else
		{	// Resources other than the first one get stacked in sequence to the sides of it.
		
			// If we're dealing with an "even" resource index,
			if (resourceIndex % 2 == 0)
			{
				// Put the counter to the right of the previous odd counter.
				Crd[crdResult].coord[coordGroup].x
					= Crd[crdResult-2].coord[coordGroup].x + Crd[crdResult-2].coord[coordGroup].width + 5;
			}
			else // We're dealing with an "odd" resource index.
			{
				// Put the counter to the left of the previous odd counter.
				if (resourceIndex == 1)
					Crd[crdResult].coord[coordGroup].x
						= Crd[crdResult-1].coord[coordGroup].x - Crd[crdResult].coord[coordGroup].width - 5;
				else
					Crd[crdResult].coord[coordGroup].x
						= Crd[crdResult-2].coord[coordGroup].x - Crd[crdResult].coord[coordGroup].width - 5;
			}
		
			// Set Y position to the same as the previous counter.
			Crd[crdResult].coord[coordGroup].y
				= Crd[crdResult-1].coord[coordGroup].y;
				
			// Set height of readout to the same as the previous counter.
			Crd[crdResult].coord[coordGroup].height
				= Crd[crdResult-1].coord[coordGroup].height;
		}
		
		// Transfer to icon coordinates.
		Crd[crdIconResult].coord[coordGroup].width = 32;
		Crd[crdIconResult].coord[coordGroup].height = 32;
		Crd[crdIconResult].coord[coordGroup].x = Crd[crdResult].coord[coordGroup].x;
		Crd[crdIconResult].coord[coordGroup].y = Crd[crdResult].coord[coordGroup].y;
			
		// Recalculate readout's size coordinates.
		Crd[crdResult].size[coordGroup] = calcCrdArray (Crd[crdResult].coord[coordGroup].rx, Crd[crdResult].coord[coordGroup].ry, Crd[crdResult].coord[coordGroup].x, Crd[crdResult].coord[coordGroup].y, Crd[crdResult].coord[coordGroup].width, Crd[crdResult].coord[coordGroup].height, Crd[crdResult].coord[coordGroup].rx2, Crd[crdResult].coord[coordGroup].ry2);
		
		// Recalculate icon's size coordinates.
		Crd[crdIconResult].size[coordGroup] = calcCrdArray (Crd[crdIconResult].coord[coordGroup].rx, Crd[crdIconResult].coord[coordGroup].ry, Crd[crdIconResult].coord[coordGroup].x, Crd[crdIconResult].coord[coordGroup].y, Crd[crdIconResult].coord[coordGroup].width, Crd[crdIconResult].coord[coordGroup].height, Crd[crdIconResult].coord[coordGroup].rx2, Crd[crdIconResult].coord[coordGroup].ry2);
		
		// If this coordinate is part of the currently active set, update the control onscreen too.
		if (coordGroup == GUIType)
		{
			// Update counter size.
			resourceObject.size = Crd[crdResult].size[GUIType];
			resourceIconObject.size = Crd[crdIconResult].size[GUIType];
		}
	}
}

// ====================================================================

