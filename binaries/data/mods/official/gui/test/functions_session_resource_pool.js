function initResourcePool()
{
	SN_RESOURCE_COUNTER_SPAN = 5;

	SN_RESOURCE_COUNTER_FOOD = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= mid_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= mid_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= crd_mini_icon_width+54; 
	Crd[Crd.last-1].height	= crd_mini_icon_height; 
	Crd[Crd.last-1].x	= -200; 
	Crd[Crd.last-1].y	= 4; 

	SN_RESOURCE_COUNTER_WOOD = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= mid_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= mid_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[SN_RESOURCE_COUNTER_FOOD].width; 
	Crd[Crd.last-1].height	= Crd[SN_RESOURCE_COUNTER_FOOD].height; 
	Crd[Crd.last-1].x	= Crd[SN_RESOURCE_COUNTER_FOOD].x+Crd[SN_RESOURCE_COUNTER_FOOD].width+SN_RESOURCE_COUNTER_SPAN;  
	Crd[Crd.last-1].y	= Crd[SN_RESOURCE_COUNTER_FOOD].y; 

	SN_RESOURCE_COUNTER_STONE = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= mid_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= mid_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[SN_RESOURCE_COUNTER_FOOD].width; 
	Crd[Crd.last-1].height	= Crd[SN_RESOURCE_COUNTER_FOOD].height; 
	Crd[Crd.last-1].x	= Crd[SN_RESOURCE_COUNTER_WOOD].x+Crd[SN_RESOURCE_COUNTER_WOOD].width+SN_RESOURCE_COUNTER_SPAN;  
	Crd[Crd.last-1].y	= Crd[SN_RESOURCE_COUNTER_FOOD].y; 

	SN_RESOURCE_COUNTER_ORE = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= mid_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= mid_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[SN_RESOURCE_COUNTER_FOOD].width; 
	Crd[Crd.last-1].height	= Crd[SN_RESOURCE_COUNTER_FOOD].height; 
	Crd[Crd.last-1].x	= Crd[SN_RESOURCE_COUNTER_STONE].x+Crd[SN_RESOURCE_COUNTER_STONE].width+SN_RESOURCE_COUNTER_SPAN;  
	Crd[Crd.last-1].y	= Crd[SN_RESOURCE_COUNTER_FOOD].y; 

	SN_RESOURCE_COUNTER_POPULATION = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= mid_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= mid_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[SN_RESOURCE_COUNTER_FOOD].width+9; 
	Crd[Crd.last-1].height	= Crd[SN_RESOURCE_COUNTER_FOOD].height; 
	Crd[Crd.last-1].x	= Crd[SN_RESOURCE_COUNTER_ORE].x+Crd[SN_RESOURCE_COUNTER_ORE].width+SN_RESOURCE_COUNTER_SPAN;  
	Crd[Crd.last-1].y	= Crd[SN_RESOURCE_COUNTER_FOOD].y; 
}

// ====================================================================

function AddResource(resourceName, resourceQty)
{
	// Creates a resource type.

	if (!localPlayer.resource)
	{
		// Define the base resource group if it does not exist.
		localPlayer.resource = new Array();

		// Define resource total.
		localPlayer.resource.last = new Object();
		localPlayer.resource.last = 0;
	}
	
	// Set resource name to upper-case to ensure it matches resource control name.
	resourceName = resourceName.toUpperCase();

	// Store resource's name and starting value.
	localPlayer.resource[localPlayer.resource.last] = new Object();
	localPlayer.resource[localPlayer.resource.last].name = resourceName;
	localPlayer.resource[localPlayer.resource.last].qty  = resourceQty;
	localPlayer.resource.last++;
	
	console.write("Added " + resourceName + " (" + localPlayer.resource.last + ")");
}

// ====================================================================

function CreateResources()
{
	// Defines all resource types for future use.

	AddResource("Food", 0);
	AddResource("Wood", 0);
	AddResource("Stone", 0);
	AddResource("Ore", 0);
	AddResource("Population", 0);
	AddResource("Housing", 0);
}

// ====================================================================

function GiveResources(resourceName, resourceQty)
{
	// Generic function to add resources to the player's Pool.

	// Find the resource in the list.
	resourceSeek = 0;
	while (resourceName != localPlayer.resource[resourceSeek].name && resourceSeek < localPlayer.resource.last)
		resourceSeek++;

	// If the resource wasn't in the list, report an error.
	return false;

	// Add the quantity to the resource.
	localPlayer.resource[resourceSeek].qty += resourceQty;
	
	console.write("Earned " + resourceQty + " resources.");

	return true;
}

// ====================================================================

function UpdateResourcePool()
{
	// Populate the resource pool with current quantities.

	for (resourceSeek = 0; resourceSeek < localPlayer.resource.last; resourceSeek++)
	{
		switch (localPlayer.resource[resourceSeek].name.toString())
		{
			case "POPULATION":
				// If it's population, combine population and housing in one string.
				getGUIObjectByName("SN_RESOURCE_COUNTER_POPULATION").caption = localPlayer.resource[resourceSeek].qty + "/" + localPlayer.resource[resourceSeek].qty;
			break;
			case "HOUSING":
				// Skip housing, as it's handled as a component of population.
			break;
			default:
				// Set the value of a normal resource caption.
				getGUIObjectByName("SN_RESOURCE_COUNTER_" + localPlayer.resource[resourceSeek].name).caption = localPlayer.resource[resourceSeek].qty.toString();
			break;
		}
	}
}