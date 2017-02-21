g_BackgroundLayerData.push(
	[
		{
			"offset": (time, width) => 0.05 * width * Math.cos(0.02 * time),
			"sprite": "background-seleucid1_1",
			"tiling": true,
		},
		{
			"offset": (time, width) => 0.10 * width * Math.cos(0.04 * time),
			"sprite": "background-seleucid1_2",
			"tiling": true,
		},
		{
			"offset": (time, width) => 0.17 * width * Math.cos(0.05 * time) + width/8,
			"sprite": "background-seleucid1_3",
			"tiling": false,
		},
	]);
