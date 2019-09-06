class BackgroundHandler
{
	constructor(layers)
	{
		this.initTime = Date.now();
		this.layerSet = layers;

		this.initBackgrounds();
	}

	initBackgrounds()
	{
		this.layerSet.forEach((layer, i) => {

			let background = Engine.GetGUIObjectByName("background[" + i + "]");
			background.sprite = layer.sprite;
			background.z = i;
			background.hidden = false;
		});
	}

	onTick()
	{
		let now = Date.now();

		this.layerSet.forEach((layer, i) => {

			let background = Engine.GetGUIObjectByName("background[" + i + "]");

			let screen = background.parent.getComputedSize();
			let h = screen.bottom - screen.top;
			let w = h * 16/9;
			let iw = h * 2;

			let offset = layer.offset((now - this.initTime) / 1000, w);

			if (layer.tiling)
			{
				let left = offset % iw;
				if (left >= 0)
					left -= iw;
				background.size = new GUISize(left, screen.top, screen.right, screen.bottom);
			}
			else
				background.size = new GUISize(screen.right/2 - h + offset, screen.top, screen.right/2 + h + offset, screen.bottom);
		});
	}
}
