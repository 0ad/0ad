function initResourcePool()
{
	// Resource counter (base).
	crd_resource_counter_width_base = 53;
	crd_resource_counter_height_base = 19;
	// Resource counter (short).
	crd_resource_counter_width = crd_mini_icon_width+crd_resource_counter_width_base+1;
	crd_resource_counter_height = crd_mini_icon_height;
	// Resource counter (long).
	crd_resource_counter_long_width = crd_resource_counter_width+9;
	crd_resource_counter_long_height = crd_resource_counter_height;
	// Resource counter span.
	crd_resource_counter_span = 5;
	crd_resource_food_x = -200;
	crd_resource_food_y = 4;
	crd_resource_wood_x = crd_resource_food_x+crd_resource_counter_width+crd_resource_counter_span;
	crd_resource_wood_y = crd_resource_food_y;
	crd_resource_stone_x = crd_resource_wood_x+crd_resource_counter_width+crd_resource_counter_span;
	crd_resource_stone_y = crd_resource_wood_y;
	crd_resource_ore_x = crd_resource_stone_x+crd_resource_counter_width+crd_resource_counter_span;
	crd_resource_ore_y = crd_resource_stone_y;
	crd_resource_population_x = crd_resource_ore_x+crd_resource_counter_width+crd_resource_counter_span;
	crd_resource_population_y = crd_resource_ore_y;
}

// ====================================================================

function GiveResources(resourceName, resourceQty)
{
	// Generic function to add resources to the player's Pool.

	switch (resourceName)
	{
		case "Food":
			player.resource.food += resourceQty;
		break;
		case "Wood":
			player.resource.wood += resourceQty;
		break;
		case "Stone":
			player.resource.stone += resourceQty;
		break;
		case "Ore":
			player.resource.ore += resourceQty;
		break;
		default:
		break;
	}

	console.write("Earned " + resourceQty + " resources.");
}

// ====================================================================

function UpdateResourcePool()
{
	getGUIObjectByName("resource_food_counter").caption = player.resource.food;
	getGUIObjectByName("resource_wood_counter").caption = player.resource.wood;
	getGUIObjectByName("resource_stone_counter").caption = player.resource.stone;
	getGUIObjectByName("resource_ore_counter").caption = player.resource.ore;
	getGUIObjectByName("resource_population_counter").caption = player.resource.pop.curr  + "/" + player.resource.pop.housing;
}