/**
 * This class constructs and positions the menu buttons and assigns the handlers defined in MenuButtons.
 */
class Menu
{
	constructor(pauseControl, playerViewControl, chat)
	{
		this.menuButton = Engine.GetGUIObjectByName("menuButton");
		this.menuButton.onPress = this.toggle.bind(this);
		registerHotkeyChangeHandler(this.rebuild.bind(this));

		this.isOpen = false;
		this.lastTick = undefined;

		this.menuButtonPanel = Engine.GetGUIObjectByName("menuButtonPanel");
		let menuButtons = this.menuButtonPanel.children;
		this.margin = menuButtons[0].size.top;
		this.buttonHeight = menuButtons[0].size.bottom;

		let handlerNames = this.getHandlerNames();
		if (handlerNames.length > menuButtons.length)
			throw new Error(
				"There are " + handlerNames.length + " menu buttons defined, " +
				"but only " + menuButtons.length  + " objects!");

		this.buttons = handlerNames.map((handlerName, i) => {
			let handler = new MenuButtons.prototype[handlerName](menuButtons[i], pauseControl, playerViewControl, chat);
			this.initButton(handler, menuButtons[i], i);
			return handler;
		});

		this.endPosition = this.margin + this.buttonHeight * (1 + handlerNames.length);
		let size = this.menuButtonPanel.size;
		size.top = -this.endPosition;
		size.bottom = 0;
		this.menuButtonPanel.size = size;
	}

	rebuild()
	{
		this.menuButton.tooltip = sprintf(translate("Press %(hotkey)s to toggle this menu."), {
			"hotkey": colorizeHotkey("%(hotkey)s", this.menuButton.hotkey),
		});
	}

	/**
	 * This function may be overwritten to change the button order.
	 */
	getHandlerNames()
	{
		return Object.keys(MenuButtons.prototype);
	}

	toggle()
	{
		this.isOpen = !this.isOpen;
		this.startAnimation();
	}

	close()
	{
		this.isOpen = false;
		this.startAnimation();
	}

	initButton(handler, button, i)
	{
		button.onPress = () => {
			this.close();
			handler.onPress();
		};

		let size = button.size;
		size.top = this.buttonHeight * (i + 1) + this.margin;
		size.bottom = this.buttonHeight * (i + 2);
		button.size = size;

		button.hidden = false;
	}

	startAnimation()
	{
		this.lastTick = Date.now();
		this.menuButtonPanel.onTick = this.onTick.bind(this);
	}

	/**
	 * Animate menu panel.
	 */
	onTick()
	{
		let tickLength = Date.now() - this.lastTick;
		this.lastTick = Date.now();

		let maxOffset =
			this.endPosition + (
			this.isOpen ?
				-this.menuButtonPanel.size.bottom :
				+this.menuButtonPanel.size.top);


		if (maxOffset <= 0)
		{
			delete this.menuButtonPanel.onTick;
			return;
		}

		let offset = Math.min(this.Speed * tickLength, maxOffset) * (this.isOpen ? +1 : -1);
		let size = this.menuButtonPanel.size;
		size.top += offset;
		size.bottom += offset;
		this.menuButtonPanel.size = size;
	}
}

/**
 * Number of pixels per millisecond to move.
 */
Menu.prototype.Speed = 1.2;
