function setupBiome_desert()
{
	[g_Gaia.tree1, g_Gaia.tree2] = pickRandom([
		[
			"gaia/flora_tree_cretan_date_palm_short",
			"gaia/flora_tree_cretan_date_palm_tall"
		],
		[
			"gaia/flora_tree_date_palm",
			"gaia/flora_tree_date_palm"
		]
	]);

	[g_Gaia.tree4, g_Gaia.tree5] = new Array(2).fill(pickRandom([
		"gaia/flora_tree_tamarix",
		"gaia/flora_tree_senegal_date_palm"
	]));
}
