/*
	DESCRIPTION	: Functions for manipulating players and their properties (eg resources).
	NOTES		: 
*/

// ====================================================================

function createResources()
{
	// Defines all resource types for future use.
	// Assigns the value of game setup resource values as starting values.

	addResource ("Food", Number(getGUIObjectByName ("pgSessionSetupResourceFoodCounter").caption));
	addResource ("Wood", Number(getGUIObjectByName ("pgSessionSetupResourceWoodCounter").caption));
	addResource ("Stone", Number(getGUIObjectByName ("pgSessionSetupResourceStoneCounter").caption));
	addResource ("Ore", Number(getGUIObjectByName ("pgSessionSetupResourceOreCounter").caption));
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
