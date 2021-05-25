function setupBiome_sahara()
{
	[g_Gaia.tree1, g_Gaia.tree2] = pickRandom([
		[
			"gaia/tree/cretan_date_palm_short",
			"gaia/tree/date_palm"
		],
		[
			"gaia/tree/date_palm",
			"gaia/tree/cretan_date_palm_tall"
		]
	]);

	[g_Gaia.tree4, g_Gaia.tree5] = new Array(2).fill(pickRandom([
		"gaia/tree/date_palm",
		"gaia/tree/cretan_date_palm_patch"
	]));
}
