function setupBiome_nubia()
{
	g_Gaia.mainHuntableAnimal = pickRandom([
		"gaia/fauna_wildebeest",
		"gaia/fauna_zebra",
		"gaia/fauna_giraffe",
		"gaia/fauna_elephant_african_bush",
		"gaia/fauna_gazelle"
	]);

	g_Gaia.tree3 = pickRandom([
			"gaia/tree/baobab_4_dead",
			"gaia/tree/baobab_3_mature"
	]);

	[g_Gaia.tree4, g_Gaia.tree5] = pickRandom([
		[
			"gaia/tree/date_palm",
			"gaia/tree/bush_tropic"
		],
		[
			"gaia/tree/bush_tropic",
			"gaia/tree/palm_doum"
		]
	]);
}
