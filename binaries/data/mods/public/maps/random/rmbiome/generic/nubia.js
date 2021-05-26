function setupBiome_nubia()
{
	g_Gaia.mainHuntableAnimal = pickRandom([
		"gaia/fauna_wildebeest",
		"gaia/fauna_zebra",
		"gaia/fauna_giraffe",
		"gaia/fauna_elephant_african_bush",
		"gaia/fauna_gazelle"
	]);

	[g_Gaia.tree1, g_Gaia.tree2] = pickRandom([
		[
			"gaia/tree/acacia",
			"gaia/tree/acacia"
		],
		[
			"gaia/tree/date_palm",
			"gaia/tree/baobab"
		],
		[
			"gaia/tree/date_palm",
			"gaia/tree/palm_doum"
		]
	]);

	[g_Gaia.tree4, g_Gaia.tree5] = pickRandom([
		[
			"gaia/tree/date_palm_dead",
			"gaia/tree/date_palm"
		],
		[
			"gaia/tree/baobab",
			"gaia/tree/acacia"
		],
		[
			"gaia/tree/acacia",
			"gaia/tree/acacia"
		]
	]);
}
