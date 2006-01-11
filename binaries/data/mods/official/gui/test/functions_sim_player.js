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
	// Create uppercase name.
	resourceNameU = resourceName.toUpperCase();

	if (!localPlayer.resource)
	{
		// Define the base resource group if it does not exist.
		localPlayer.resource = new Array();
	}
	
	// Store resource's name and starting value.
	localPlayer.resource.valueOf()[resourceNameU] = resourceQty;
		
	// Dynamically adjust width of resource counter based on caption length.
	refreshResource (resourceName);		
	
	console.write( "Added " + resourceName + " (" + resourceQty + ")" );
}

// ====================================================================

function setResources (resourceName, resourceQty)
{
	// Generic function to set the value of a resource in the player's pool.

	// Ensure resource name is title-case.
	resourceName = toTitleCase (resourceName);
	// Create uppercase name.
	resourceNameU = resourceName.toUpperCase();

//	if ( localPlayer.resource.valueOf()[resourceNameU] )
//	{
		// Set resource value.
		localPlayer.resource.valueOf()[resourceNameU] = resourceQty;
		
		// Dynamically adjust width of resource counter based on caption length.
		refreshResource (resourceName);		

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
	// Create uppercase name.
	resourceNameU = resourceName.toUpperCase();
console.write (localPlayer.resource.valueOf()[resourceNameU]);
//	if ( localPlayer.resource.valueOf()[resourceNameU] )
//	{
		// Set resource value.
		localPlayer.resource.valueOf()[resourceNameU] += resourceQty;
	
		// Dynamically adjust width of resource counter based on caption length.
		refreshResource (resourceName);		

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
	// Create uppercase name.
	resourceNameU = resourceName.toUpperCase();

//	if( localPlayer.resource.valueOf()[resourceNameU] )
//	{
		// Set resource value.
		localPlayer.resource.valueOf()[resourceNameU] -= resourceQty;
		
		// Dynamically adjust width of resource counter based on caption length.
		refreshResource (resourceName);

		console.write("Deducted " + resourceQty + " " + resourceName + ".");
		return( true );
//	}

//	// If the resource wasn't in the list, report an error.
//	console.write ("Failed to deduct " + resourceQty + " from resource " + resourceName);
//	return false;
}

// ====================================================================

function refreshResource (resourceName)
{
	// Ignore the "Housing" resource ... It doesn't work like normal resources and doesn't have a counter to resize.
	if (resourceName == "Housing")
		resourceName = "Population";
		
	// Update GUI resource counter caption.
	if (resourceName != "Population")
		getGUIObjectByName ("snResourceCounter_" + resourceName).caption = localPlayer.resource.valueOf()[resourceNameU];
	else
		getGUIObjectByName ("snResourceCounter_Population").caption = localPlayer.resource.valueOf()["POPULATION"] + "/" + localPlayer.resource.valueOf()["HOUSING"];		

	// Get the index of the resource readout to be resized.
	crdResult = getCrd ("snResourceCounter_" + resourceName, true);
	// Get resource readout object.
	resourceObject = getGUIObjectByName ("snResourceCounter_" + resourceName);
	
	// For each coordinate group stored for this control,
	for (coordGroup in Crd[crdResult].coord)
	{
		// Set X position so that resources always are immediately next to each other. (Except for Food, which is always the leftmost resource.)
		if (resourceName != "Food")
			Crd[crdResult].coord[coordGroup].x
				= Crd[crdResult-1].coord[coordGroup].x + Crd[crdResult-1].coord[coordGroup].width + 5;
				
		// Set width of readout based on length of caption.
		Crd[crdResult].coord[coordGroup].width = snConst.MiniIcon.Width+5 + resourceObject.caption.length * 10;
		
		// Recalculate readout's size coordinates.
		Crd[crdResult].size[coordGroup] = calcCrdArray (Crd[crdResult].coord[coordGroup].rx, Crd[crdResult].coord[coordGroup].ry, Crd[crdResult].coord[coordGroup].x, Crd[crdResult].coord[coordGroup].y, Crd[crdResult].coord[coordGroup].width, Crd[crdResult].coord[coordGroup].height, Crd[crdResult].coord[coordGroup].rx2, Crd[crdResult].coord[coordGroup].ry2);
	}
}

// ====================================================================
