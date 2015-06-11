g_BackgroundLayerData.push(
	[
		{
			"offset": (time, width) => 0.02 * width * Math.cos(0.05 * time),
			"sprite": "background-hellenes1-1",
			"tiling": true,
		},
		{
			"offset": (time, width) => 0.12 * width * Math.cos(0.05 * time) - width/10,
			"sprite": "background-hellenes1-2",
			"tiling": false,
		},
		{
			"offset": (time, width) => 0.16 * width * Math.cos(0.05 * time) + width/4,
			"sprite": "background-hellenes1-3",
			"tiling": false,
		},
	]);
