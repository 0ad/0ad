class Tooltip
{
	constructor(cancelButton)
	{
		this.onscreenToolTip = Engine.GetGUIObjectByName("onscreenToolTip");
		cancelButton.registerCancelButtonResizeHandler(this.onCancelButtonResize.bind(this));
	}

	onCancelButtonResize(cancelButton)
	{
		let size = this.onscreenToolTip.size;
		size.right = cancelButton.size.left - this.Margin;
		this.onscreenToolTip.size = size;
	}
}

Tooltip.prototype.Margin = 10;
