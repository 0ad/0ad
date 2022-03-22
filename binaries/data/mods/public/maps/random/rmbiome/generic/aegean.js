function setupBiome_aegean()
{
	[g_Gaia.tree1, g_Gaia.tree2] = pickRandom([[
			"gaia/tree/cypress_wild",
			"gaia/tree/pine_maritime_short",
			"gaia/tree/cretan_date_palm_tall"
		]]);

	g_Gaia.tree3 = pickRandom([
		"gaia/tree/olive",
		"gaia/tree/juniper_prickly",
		"gaia/tree/date_palm",
		"gaia/tree/cretan_date_palm_short",
		"gaia/tree/medit_fan_palm"
	]);

	[g_Gaia.tree4, g_Gaia.tree5] = pickRandom([[
		"gaia/tree/poplar_lombardy",
		"gaia/tree/carob",
		"gaia/tree/medit_fan_palm",
		"gaia/tree/cretan_date_palm_tall"
	]]);

	g_Gaia.fruitBush = pickRandom([
		"gaia/fruit/berry_01",
		"gaia/fruit/grapes"
	]);
}
