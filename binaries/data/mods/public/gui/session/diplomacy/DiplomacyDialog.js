/**
 * This class is concerned with opening, closing and resizing the diplomacy dialog and
 * relaying events to the classes that update individual elements of the dialog.
 */
class DiplomacyDialog
{
	constructor(diplomacyColors)
	{
		this.diplomacyDialogCeasefireCounter = new DiplomacyDialogCeasefireCounter();
		this.diplomacyDialogColorsButton = new DiplomacyDialogColorsButton(diplomacyColors);
		this.diplomacyDialogPlayerControlManager = undefined;

		this.diplomacyDialogPanel = Engine.GetGUIObjectByName("diplomacyDialogPanel");
		Engine.GetGUIObjectByName("diplomacyClose").onPress = this.close.bind(this);
	}

	onPlayerInit()
	{
		this.diplomacyDialogPlayerControlManager = new DiplomacyDialogPlayerControlManager();
		this.resize();
	}

	onSpyResponse(notification, player)
	{
		this.diplomacyDialogPlayerControlManager.onSpyResponse(notification, player);
	}

	update()
	{
		if (!this.isOpen())
			return;

		if (g_ViewedPlayer >= 1)
			this.updatePanels();
		else
			this.close();
	}

	updatePanels()
	{
		this.diplomacyDialogCeasefireCounter.update();
		this.diplomacyDialogColorsButton.update();
		this.diplomacyDialogPlayerControlManager.update();
	}

	open()
	{
		closeOpenDialogs();

		if (g_ViewedPlayer < 1)
			return;

		this.updatePanels();
		this.diplomacyDialogPanel.hidden = false;
	}

	close()
	{
		this.diplomacyDialogPanel.hidden = true;
	}

	isOpen()
	{
		return !this.diplomacyDialogPanel.hidden;
	}

	toggle()
	{
		let open = this.isOpen();

		closeOpenDialogs();

		if (!open)
			this.open();
	}

	resize()
	{
		let widthOffset = DiplomacyDialogPlayerControl.prototype.TributeButtonManager.getWidthOffset() / 2;
		let heightOffset = DiplomacyDialogPlayerControl.prototype.DiplomacyPlayerText.getHeightOffset() / 2;

		let size = this.diplomacyDialogPanel.size;
		size.left -= widthOffset;
		size.right += widthOffset;
		size.top -= heightOffset;
		size.bottom += heightOffset;
		this.diplomacyDialogPanel.size = size;
	}
}
