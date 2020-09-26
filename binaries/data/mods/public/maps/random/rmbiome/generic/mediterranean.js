function setupBiome_mediterranean()
{
	[g_Gaia.tree1, g_Gaia.tree2] = pickRandom([
		[
			"gaia/tree/cretan_date_palm_short",
			"gaia/tree/cretan_date_palm_tall"
		],
		[
			"gaia/tree/carob",
			"gaia/tree/carob"
		],
		[
			"gaia/tree/medit_fan_palm",
			"gaia/tree/medit_fan_palm"
		]]);

	g_Gaia.tree3 = pickRandom([
		"gaia/fruit/apple",
		"gaia/tree/poplar_lombardy"
	]);

	[g_Gaia.tree4, g_Gaia.tree5] = new Array(2).fill(pickRandom([
		"gaia/tree/cypress",
		"gaia/tree/aleppo_pine"
	]));

	g_Gaia.fruitBush = pickRandom([
		"gaia/fruit/berry_01",
		"gaia/fruit/grapes"
	]);
}
