/*
	DESCRIPTION	: Functions for manipulating players and their properties (eg resources).
	NOTES		: 
*/

// ====================================================================

function createResources()
{
	// Defines all resource types for future use.
	// Assigns the value of game setup resource values as starting values.

	// Numbers for resource modes Low/Normal/High - tweak as needed for balancing
	var resLowValue = Array(100,50,0,0);
	var resNormalValue = Array(200,200,100,100);
	var resHighValue = Array(1000,1000,1000,1000);
	
	if (getCurrItemValue ("pgSessionSetupResources") == "Low") {
		// Give low resources
		var resValue = resLowValue;
	} else if (getCurrItemValue("pgSessionSetupResources") == "Normal") {
		// Give normal resources
		var resValue = resNormalValue;
	} else if (getCurrItemValue ("pgSessionSetupResources") == "High") {
		// Give high resources
		var resValue = resHighValue;
	} else {
		// Do not give any resources
		var resValue = Array(0,0,0,0);
	}
	
	addResource ("Food", Number(resValue[0]));
	addResource ("Wood", Number(resValue[1]));
	addResource ("Stone", Number(resValue[2]));
	addResource ("Ore", Number(resValue[3]));
	addResource ("Population", 0);
	addResource ("Housing", 0);
}

// ====================================================================

function addResource (resourceName, resourceQty)
{
	// Creates a resource type.

	// Ensure resource name is title-case.
	resourceName = toTitleCase (resourceName);

	if (!localPlayer.resource)
	{
		// Define the base resource group if it does not exist.
		localPlayer.resource = new Array();
		// Set the length of the array.
		localPlayer.resource.length = 0;
		// Create the UI resource array container if it does not exist.
		resourceUIArray = new Array;
	}
	
	// Store resource's name and starting value.
	localPlayer.resource.valueOf()[resourceName] = resourceQty;
	
	// The array is now one index longer.
	localPlayer.resource.length++;
	
	// We maintain a sorted array of the resource indexes so that the UI counters can be refreshed in centered order.
	// (We don't include Housing in the list, though, as it does not have its own resource counter.)
	if (resourceName != "Housing")
	{
		//  If it's an even index,
		if ((localPlayer.resource.length-1) % 2 == 0)
			// Add it to the end of the UI array.
			resourceUIArray.push ((localPlayer.resource.length-1));
		else
			// Add it to the beginning of the UI array,
			resourceUIArray.unshift ((localPlayer.resource.length-1));
	}
		
	// Dynamically adjust width of resource counter based on caption length.
	refreshResources();		
	
	console.write( "Added " + resourceName + " (" + resourceQty + ")" );
}

// ====================================================================

function setResources (resourceName, resourceQty)
{
	// Generic function to set the value of a resource in the player's pool.

	// Ensure resource name is title-case.
	resourceName = toTitleCase (resourceName);

//	if ( localPlayer.resource.valueOf()[resourceName] )
//	{
		// Set resource value.
		localPlayer.resource.valueOf()[resourceName] = resourceQty;
		
		// Dynamically adjust width of resource counter based on caption length.
		refreshResources();		

		console.write ("Resource set to " + resourceQty + " " + resourceName + ".");
		return ( true );
//	}

	// If the resource wasn't in the list, report an error.
//	console.write ("Failed to set resource " + resourceName + " to " + resourceQty);
//	return ( false ) ;
}

// ====================================================================

function giveResources (resourceName, resourceQty)
{
	// Generic function to add resources to the player's Pool.

	// Ensure resource name is title-case.
	resourceName = toTitleCase (resourceName);

//	if ( localPlayer.resource.valueOf()[resourceName] )
//	{
		// Set resource value.
		localPlayer.resource.valueOf()[resourceName] += resourceQty;
	
		// Dynamically adjust width of resource counter based on caption length.
		refreshResources();		

		if (resourceName != "Housing" && resourceName != "Population")
			console.write ("Earned " + resourceQty + " " + resourceName + ".");
		return ( true );
//	}

//	// If the resource wasn't in the list, report an error.
//	console.write ("Failed to add " + resourceQty + " to resource " + resourceName);
//	return ( false );
}

// ====================================================================

function deductResources (resourceName, resourceQty)
{
	// Generic function to remove resources from the player's Pool.

	// Ensure resource name is title-case.
	resourceName = toTitleCase (resourceName);

//	if( localPlayer.resource.valueOf()[resourceName] )
//	{
		// Set resource value.
		localPlayer.resource.valueOf()[resourceName] -= resourceQty;
		
		// Dynamically adjust width of resource counter based on caption length.
		refreshResources();

		if (resourceName != "Housing" && resourceName != "Population")
			console.write("Deducted " + resourceQty + " " + resourceName + ".");
		return( true );
//	}

//	// If the resource wasn't in the list, report an error.
//	console.write ("Failed to deduct " + resourceQty + " from resource " + resourceName);
//	return false;
}

// ====================================================================

function refreshResources ()
{
	// Refreshes all resource counters after update.

	resourcePool = localPlayer.resource;
	resourceCount = 0;
	for (currResource in resourcePool)
	{
		// Pass the array index of the resource as the second parameter (as we'll need that to determine the centered screen position of each counter).
		refreshResource (toTitleCase(currResource), resourceUIArray[resourceCount]);
		resourceCount++;
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
	resourceObject = getGUIObjectByName ("snResourceCounter_" + (resourceIndex + 1));
	// Get resource icon object.
	resourceIconObject = getGUIObjectByName ("snResourceCounterIcon_" + (resourceIndex + 1));
	
	// Update counter caption (since we need to have up-to-date text to determine the length of the counter).
	caption = localPlayer.resource.valueOf()[resourceName];
	// The Population counter also lists the amount of available housing.		
	if (resourceName == "Population")
		caption	+= "/" + localPlayer.resource.valueOf()["Housing"];	
	resourceObject.caption = caption;
	
	// Update counter tooltip.
	resourceObject.tooltip = resourceName + ": " + resourceObject.caption;
	
	// Set resource icon.
	resourceIconObject.cell_id = cellGroup["Resource"][resourceName.toLowerCase()].id;
			
	// Get the index of the resource readout to be resized.
	crdResult = getCrd (resourceObject.name, true);
	// Get the index of the resource icon.
	crdIconResult = getCrd (resourceIconObject.name, true);	
	
	// For each coordinate group stored for this control,
	for (coordGroup in Crd[crdResult].coord)
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

