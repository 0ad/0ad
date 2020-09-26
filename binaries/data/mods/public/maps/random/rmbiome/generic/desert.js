function setupBiome_desert()
{
	[g_Gaia.tree1, g_Gaia.tree2] = pickRandom([
		[
			"gaia/tree/cretan_date_palm_short",
			"gaia/tree/cretan_date_palm_tall"
		],
		[
			"gaia/tree/date_palm",
			"gaia/tree/date_palm"
		]
	]);

	[g_Gaia.tree4, g_Gaia.tree5] = new Array(2).fill(pickRandom([
		"gaia/tree/tamarix",
		"gaia/tree/senegal_date_palm"
	]));
}
