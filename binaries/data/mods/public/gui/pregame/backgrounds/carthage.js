g_BackgroundLayerData.push(
	[
		{
			"offset": (time, width) => 0.02 * width * Math.cos(0.05 * time),
			"sprite": "background-carthage1-1",
			"tiling": true,
		},
		{
			"offset": (time, width) => 0.04 * width * Math.cos(0.05 * time),
			"sprite": "background-carthage1-2",
			"tiling": true,
		},
		{
			"offset": (time, width) => 0.10 * width * Math.cos(0.05 * time),
			"sprite": "background-carthage1-3",
			"tiling": false,
		},
		{
			"offset": (time, width) => 0.18 * width * Math.cos(0.05 * time),
			"sprite": "background-carthage1-4",
			"tiling": false,
		},
	]);
