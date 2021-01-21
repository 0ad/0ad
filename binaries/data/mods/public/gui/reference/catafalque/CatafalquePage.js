class CatafalquePage extends ReferencePage
{
	constructor(data)
	{
		super();

		this.Canvas = Engine.GetGUIObjectByName("canvas");
		this.Emblems = [];
		for (let emblem in this.Canvas.children)
			this.Emblems.push(new Emblem(this, this.Emblems.length));

		let civs = [];
		for (let civCode in this.civData)
			if (this.Emblems[civs.length].setCiv(civCode, this.civData[civCode]))
				civs.push(civCode);

		let canvasSize = this.Canvas.getComputedSize();
		let canvasCenterX = (canvasSize.right - canvasSize.left) / 2;
		let canvasCenterY = (canvasSize.bottom - canvasSize.top) / 2;
		let radius = Math.min(canvasCenterX, canvasCenterY) / 5 * 4;
		let angle = 2 * Math.PI / civs.length;

		for (let i = 0; i < civs.length; ++i)
			this.Emblems[i].setPosition(
				canvasCenterX + radius * Math.sin(angle * i),
				canvasCenterY + radius * -Math.cos(angle * i)
			);

		let closeButton = new CloseButton(this);
	}

	closePage()
	{
		Engine.PopGuiPage({ "page": "page_catafalque.xml" });
	}

}


CatafalquePage.prototype.CloseButtonTooltip =
	translate("%(hotkey)s: Close Catafalque Bonuses.");
