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
	// Update GUI resource counter.
	if (resourceName != "Housing")
		getGUIObjectByName ("snResourceCounter_" + resourceName).caption = resourceQty;
	
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

	if ( localPlayer.resource.valueOf()[resourceNameU] )
	{
		// Set resource value.
		localPlayer.resource.valueOf()[resourceNameU] = resourceQty;
		// Update GUI resource counter.
		getGUIObjectByName ("snResourceCounter_" + resourceName).caption = localPlayer.resource.valueOf()[resourceNameU];

		console.write ("Resource set to " + resourceQty + " " + resourceName + ".");
		return ( true );
	}

	// If the resource wasn't in the list, report an error.
	console.write ("Failed to set resource " + resourceName + " to " + resourceQty);
	return ( false ) ;
}

// ====================================================================

function giveResources (resourceName, resourceQty)
{
	// Generic function to add resources to the player's Pool.

	// Ensure resource name is title-case.
	resourceName = toTitleCase (resourceName);
	// Create uppercase name.
	resourceNameU = resourceName.toUpperCase();

	if ( localPlayer.resource.valueOf()[resourceNameU] )
	{
		// Set resource value.
		localPlayer.resource.valueOf()[resourceNameU] += resourceQty;
		// Update GUI resource counter.
		getGUIObjectByName ("snResourceCounter_" + resourceName).caption = localPlayer.resource.valueOf()[resourceNameU];

		console.write ("Earned " + resourceQty + " " + resourceName + ".");
		return ( true );
	}

	// If the resource wasn't in the list, report an error.
	console.write ("Failed to add " + resourceQty + " to resource " + resourceName);
	return ( false );
}

// ====================================================================

function deductResources (resourceName, resourceQty)
{
	// Generic function to remove resources from the player's Pool.

	// Ensure resource name is title-case.
	resourceName = toTitleCase (resourceName);
	// Create uppercase name.
	resourceNameU = resourceName.toUpperCase();

	if( localPlayer.resource.valueOf()[resourceNameU] )
	{
		// Set resource value.
		localPlayer.resource.valueOf()[resourceNameU] -= resourceQty;
		// Update GUI resource counter.
		getGUIObjectByName ("snResourceCounter_" + resourceName).caption = localPlayer.resource.valueOf()[resourceNameU];

		console.write("Deducted " + resourceQty + " " + resourceName + ".");
		return( true );
	}

	// If the resource wasn't in the list, report an error.
	console.write ("Failed to deduct " + resourceQty + " from resource " + resourceName);
	return false;
}

// ====================================================================
