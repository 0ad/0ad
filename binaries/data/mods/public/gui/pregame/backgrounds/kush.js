g_BackgroundLayerData.push(
	[
		{
			"offset": (time, width) => 0.07 * width * Math.cos(0.1 * time),
			"sprite": "background-kush1-1",
			"tiling": true
		},
		{
			"offset": (time, width) => 0.05 * width * Math.cos(0.1 * time),
			"sprite": "background-kush1-2",
			"tiling": true
		},
		{
			"offset": (time, width) => 0.04 * width * Math.cos(0.1 * time) + 0.01 * width * Math.cos(0.04 * time),
			"sprite": "background-kush1-3",
			"tiling": true
		},
		{
			"offset": (time, width) => -0.1,
			"sprite": "background-kush1-4",
			"tiling": true
		}
	]);
