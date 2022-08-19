class StartGameButton
{
	constructor(setupWindow, loadGameButton)
	{
		this.setupWindow = setupWindow;
		this.gameStarted = false;

		this.buttonHiddenChangeHandlers = new Set();

		this.startGameButton = Engine.GetGUIObjectByName("startGameButton");
		this.startGameButton.caption = this.Caption;
		this.startGameButton.onPress = this.onPress.bind(this);

		setupWindow.registerLoadHandler(this.onLoad.bind(this));
		setupWindow.controls.playerAssignmentsController.registerPlayerAssignmentsChangeHandler(this.update.bind(this));

		this.buttonPositions = Engine.GetGUIObjectByName("bottomRightPanel").children;
		loadGameButton.registerButtonHiddenChangeHandler(this.onNeighborButtonHiddenChange.bind(this));
	}

	registerButtonHiddenChangeHandler(handler)
	{
		this.buttonHiddenChangeHandlers.add(handler);
	}

	onNeighborButtonHiddenChange()
	{
		// Resizing the start game button if the load button is hidden
		// (although is shouldn't be displayed), because in theory the player
		// isn't the controller
		this.startGameButton.size = this.buttonPositions[
			this.buttonPositions[2].children.every(button => button.hidden) ? 1 : 0].size;
		
		for (let handler of this.buttonHiddenChangeHandlers)
			handler();
	}

	onLoad()
	{
		this.startGameButton.hidden = !g_IsController;

		for (let handler of this.buttonHiddenChangeHandlers)
			handler();
	}

	update()
	{
		let isEveryoneReady = this.isEveryoneReady();
		this.startGameButton.enabled = !this.gameStarted && isEveryoneReady;
		this.startGameButton.tooltip =
			!g_IsNetworked || isEveryoneReady ?
				this.ReadyTooltip :
				this.ReadyTooltipWaiting;
	}

	isEveryoneReady()
	{
		if (!g_IsNetworked)
			return true;

		for (let guid in g_PlayerAssignments)
			if (g_PlayerAssignments[guid].player != -1 &&
				g_PlayerAssignments[guid].status == this.setupWindow.controls.readyController.NotReady)
				return false;

		return true;
	}

	onPress()
	{
		if (this.gameStarted)
			return;

		this.gameStarted = true;
		this.update();
		this.setupWindow.controls.gameSettingsController.launchGame();
	}
}

StartGameButton.prototype.Caption =
	translate("Start Game!");

StartGameButton.prototype.ReadyTooltip =
	translate("Start a new game with the current settings.");

StartGameButton.prototype.ReadyTooltipWaiting =
	translate("Start a new game with the current settings (disabled until all players are ready).");
