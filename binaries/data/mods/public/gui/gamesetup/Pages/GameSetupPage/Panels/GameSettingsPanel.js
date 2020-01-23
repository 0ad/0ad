class GameSettingsPanel
{
	constructor(setupWindow, gameSettingTabs, gameSettingControlManager)
	{
		this.centerRightPanel = Engine.GetGUIObjectByName("centerRightPanel");
		this.settingTabButtonsFrame = Engine.GetGUIObjectByName("settingTabButtonsFrame");
		this.settingsPanelFrame = Engine.GetGUIObjectByName("settingsPanelFrame");

		this.gameSettingControlManager = gameSettingControlManager;
		this.gameSettingsPanelResizeHandlers = new Set();

		this.setupWindow = Engine.GetGUIObjectByName("setupWindow");
		this.setupWindow.onWindowResized = this.onWindowResized.bind(this);

		this.settingsPanel = Engine.GetGUIObjectByName("settingsPanel");

		this.enabled = Engine.ConfigDB_GetValue("user", this.ConfigNameSlide) == "true";
		this.slideSpeed = this.enabled ? this.SlideSpeed : Infinity;
		this.lastTickTime = undefined;

		gameSettingTabs.registerTabSelectHandler(this.updateSize.bind(this));
		setupWindow.controls.gameSettingsControl.registerGameAttributesBatchChangeHandler(this.updateSize.bind(this));
		setupWindow.registerLoadHandler(this.triggerResizeHandlers.bind(this));
	}

	registerGameSettingsPanelResizeHandler(handler)
	{
		this.gameSettingsPanelResizeHandlers.add(handler);
	}

	triggerResizeHandlers()
	{
		for (let handler of this.gameSettingsPanelResizeHandlers)
			handler(this.settingsPanelFrame);
	}

	onWindowResized()
	{
		this.updateSize();
		this.triggerResizeHandlers();
	}

	updateSize()
	{
		this.gameSettingControlManager.updateSettingVisibility();
		this.positionSettings();

		this.lastTickTime = undefined;
		this.settingsPanelFrame.onTick = this.onTick.bind(this);
	}

	onTick()
	{
		let now = Date.now();
		let tickLength = now - this.lastTickTime;
		let previousTime = this.lastTickTime;
		this.lastTickTime = now;
		if (previousTime === undefined)
			return;

		let distance = this.slideSpeed * tickLength;
		let rightBorder = this.settingTabButtonsFrame.size.left;
		let offset = 0;
		if (g_TabCategorySelected === undefined)
		{
			let maxOffset = rightBorder - this.settingsPanelFrame.size.left;
			if (maxOffset > 0)
				offset = Math.min(distance, maxOffset);
		}
		else if (rightBorder > this.settingsPanelFrame.size.right)
		{
			offset = Math.min(distance, rightBorder - this.settingsPanelFrame.size.right);
		}
		else
		{
			let maxOffset = this.settingsPanelFrame.size.left - rightBorder + (this.settingsPanelFrame.size.right - this.settingsPanelFrame.size.left);
			if (maxOffset > 0)
				offset = -Math.min(distance, maxOffset);
		}

		if (offset)
			this.changePanelWidth(offset);
		else
		{
			delete this.settingsPanelFrame.onTick;
			this.lastTickTime = undefined;
		}
	}

	changePanelWidth(offset)
	{
		if (!offset)
			return;

		let size = this.settingsPanelFrame.size;
		size.left += offset;
		size.right += offset;
		this.settingsPanelFrame.size = size;

		this.triggerResizeHandlers();
	}

	/**
	 * Distribute the currently visible settings over the settings panel.
	 * First calculate the number of columns required, then place the setting frames.
	 */
	positionSettings()
	{
		let setupWindowSize = this.setupWindow.getComputedSize();

		let columnWidth = Math.min(
			this.MaxColumnWidth,
			(setupWindowSize.right - setupWindowSize.left + this.centerRightPanel.size.left) / 2);

		let settingsPerColumn;
		{
			let settingPanelSize = this.settingsPanel.getComputedSize();
			let maxSettingsPerColumn = Math.floor((settingPanelSize.bottom - settingPanelSize.top) / this.SettingHeight);
			let settingCount = this.settingsPanel.children.filter(child => !child.children[0].hidden).length;
			settingsPerColumn = settingCount / Math.ceil(settingCount / maxSettingsPerColumn);
		}

		let yPos = this.SettingMarginBottom;
		let column = 0;
		let settingsThisColumn = 0;

		let selectedTab = g_GameSettingsLayout[g_TabCategorySelected];
		if (!selectedTab)
			return;

		for (let name of selectedTab.settings)
		{
			let settingFrame = this.gameSettingControlManager.gameSettingControls[name].frame;
			if (settingFrame.hidden)
				continue;

			if (settingsThisColumn >= settingsPerColumn)
			{
				yPos = this.SettingMarginBottom;
				++column;
				settingsThisColumn = 0;
			}

			settingFrame.size = new GUISize(
				columnWidth * column,
				yPos,
				columnWidth * (column + 1) - this.SettingMarginRight,
				yPos + this.SettingHeight - this.SettingMarginBottom);

			yPos += this.SettingHeight;
			++settingsThisColumn;
		}

		{
			let size = this.settingsPanelFrame.size;
			size.right = size.left + (column + 1) * columnWidth;
			this.settingsPanelFrame.size = size;
		}
	}
}

GameSettingsPanel.prototype.ConfigNameSlide =
	"gui.gamesetup.settingsslide";

/**
 * Maximum width of a column in the settings panel.
 */
GameSettingsPanel.prototype.MaxColumnWidth = 470;

/**
 * Pixels per millisecond the settings panel slides when opening/closing.
 */
GameSettingsPanel.prototype.SlideSpeed = 1.2;

/**
 * Vertical size of a setting frame.
 */
GameSettingsPanel.prototype.SettingHeight = 36;

/**
 * Horizontal space between two setting frames.
 */
GameSettingsPanel.prototype.SettingMarginRight = 10;

/**
 * Vertical space between two setting frames.
 */
GameSettingsPanel.prototype.SettingMarginBottom = 2;
