g_BackgroundLayerData.push(
	[
		{
			"offset": (time, width) => 0.02 * width * Math.cos(0.05 * time),
			"sprite": "background-kush1-1",
			"tiling": true,
		},
		{
			"offset": (time, width) => 0.04 * width * Math.cos(0.1 * time),
			"sprite": "background-kush1-2",
			"tiling": true,
		},
		{
			"offset": (time, width) => 0.04 * width * Math.cos(0.15 * time),
			"sprite": "background-kush1-3",
			"tiling": true,
		},
		{
			"offset": (time, width) => -40,	
			"sprite": "background-kush1-4",
			"tiling": false,
		},
	]);
