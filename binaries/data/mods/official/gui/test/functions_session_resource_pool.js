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
