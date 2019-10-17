/**
 * This class is concerned with opening, closing and resizing the diplomacy dialog and
 * relaying events to the classes that update individual elements of the dialog.
 */
class DiplomacyDialog
{
	constructor(playerViewControl, diplomacyColors)
	{
		this.diplomacyDialogCeasefireCounter = new DiplomacyDialogCeasefireCounter();
		this.diplomacyDialogColorsButton = new DiplomacyDialogColorsButton(diplomacyColors);
		this.diplomacyDialogPlayerControlManager = undefined;

		this.diplomacyDialogPanel = Engine.GetGUIObjectByName("diplomacyDialogPanel");
		Engine.GetGUIObjectByName("diplomacyClose").onPress = this.close.bind(this);

		registerPlayersInitHandler(this.onPlayersInit.bind(this));
		registerSimulationUpdateHandler(this.onViewedPlayerChange.bind(this));
		playerViewControl.registerViewedPlayerChangeHandler(this.updateIfOpen.bind(this));
	}

	onPlayersInit()
	{
		this.diplomacyDialogPlayerControlManager = new DiplomacyDialogPlayerControlManager();
		this.resize();
	}

	onViewedPlayerChange()
	{
		if (g_ViewedPlayer >= 1)
			this.updateIfOpen();
		else
			this.close();
	}

	onSpyResponse(notification, player)
	{
		this.diplomacyDialogPlayerControlManager.onSpyResponse(notification, player);
	}

	updateIfOpen()
	{
		if (this.isOpen())
			this.updatePanels();
	}

	updatePanels()
	{
		this.diplomacyDialogCeasefireCounter.update();
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
