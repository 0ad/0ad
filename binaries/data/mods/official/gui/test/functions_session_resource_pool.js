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

function GiveResources(resourceName, resourceQty)
{
	// Generic function to add resources to the player's Pool.

	switch (resourceName)
	{
		case "Food":
			localPlayer.resource.food += resourceQty;
		break;
		case "Wood":
			localPlayer.resource.wood += resourceQty;
		break;
		case "Stone":
			localPlayer.resource.stone += resourceQty;
		break;
		case "Ore":
			localPlayer.resource.ore += resourceQty;
		break;
		default:
		break;
	}

	console.write("Earned " + resourceQty + " resources.");
}

// ====================================================================

function UpdateResourcePool()
{
	getGUIObjectByName("SN_RESOURCE_COUNTER_FOOD").caption = localPlayer.resource.food;
	getGUIObjectByName("SN_RESOURCE_COUNTER_WOOD").caption = localPlayer.resource.wood;
	getGUIObjectByName("SN_RESOURCE_COUNTER_STONE").caption = localPlayer.resource.stone;
	getGUIObjectByName("SN_RESOURCE_COUNTER_ORE").caption = localPlayer.resource.ore;
	getGUIObjectByName("SN_RESOURCE_COUNTER_POPULATION").caption = localPlayer.resource.pop.curr  + "/" + localPlayer.resource.pop.housing;
}