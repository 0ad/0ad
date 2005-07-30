/*
	DESCRIPTION	: Functions for manipulating players and their properties (eg resources).
	NOTES		: 
*/

// ====================================================================

function createResources()
{
	// Defines all resource types for future use.

	addResource ("Food", 0);
	addResource ("Wood", 0);
	addResource ("Stone", 0);
	addResource ("Ore", 0);
	addResource ("Population", 0);
	addResource ("Housing", 0);
}

// ====================================================================

function addResource (resourceName, resourceQty)
{
	// Creates a resource type.

	// MT: Rewritten to use JavaScript's nice associative-array-alikes. Requires the valueOf() hack - I'm looking into this.

	if (!localPlayer.resource)
	{
		// Define the base resource group if it does not exist.
		localPlayer.resource = new Array();
	}
	
	// Set resource name to upper-case to ensure it matches resource control name.
	resourceName = resourceName.toUpperCase();

	// Store resource's name and starting value.
	localPlayer.resource.valueOf()[resourceName] = resourceQty;
	
	console.write("Added " + resourceName );
}

// ====================================================================

function giveResources (resourceName, resourceQty)
{
	// Generic function to add resources to the player's Pool.

	if ( localPlayer.resource.valueOf()[resourceName] )
	{
	    localPlayer.resource.valueOf()[resourceName] += resourceQty;
	    console.write ("Earned " + resourceQty + " resources.");
	    return ( true );
	}

	// If the resource wasn't in the list, report an error.
	return false;
}

// ====================================================================

function deductResources (resourceName, resourceQty)
{
	// Generic function to remove resources from the player's Pool.

	if( localPlayer.resource.valueOf()[resourceName] )
	{
	    localPlayer.resource.valueOf()[resourceName] -= resourceQty;
	    console.write("Deducted " + resourceQty + " resources.");
	    return( true );
	}

	// If the resource wasn't in the list, report an error.
	return false;
}

// ====================================================================