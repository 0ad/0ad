function setupBiome_mediterranean()
{
	[g_Gaia.tree1, g_Gaia.tree2] = pickRandom([
		[
			"gaia/flora_tree_cretan_date_palm_short",
			"gaia/flora_tree_cretan_date_palm_tall"
		],
		[
			"gaia/flora_tree_carob",
			"gaia/flora_tree_carob"
		],
		[
			"gaia/flora_tree_medit_fan_palm",
			"gaia/flora_tree_medit_fan_palm"
		]]);

	g_Gaia.tree3 = pickRandom([
		"gaia/flora_tree_apple",
		"gaia/flora_tree_poplar_lombardy"
	]);

	[g_Gaia.tree4, g_Gaia.tree5] = new Array(2).fill(pickRandom([
		"gaia/flora_tree_cypress",
		"gaia/flora_tree_aleppo_pine"
	]));

	g_Gaia.fruitBush = pickRandom([
		"gaia/flora_bush_berry",
		"gaia/flora_bush_grapes"
	]);
}
