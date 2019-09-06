class MainMenuItemHandler
{
	constructor(menuItems, menuSpeed = 1.2, margin = 4, buttonHeight = 28)
	{
		this.menuItems = menuItems;
		this.menuSpeed = menuSpeed;
		this.margin = margin;
		this.buttonHeight = buttonHeight;
		this.lastTickTime = Date.now();

		this.mainMenu = Engine.GetGUIObjectByName("mainMenu");
		this.mainMenuButtons = Engine.GetGUIObjectByName("mainMenuButtons");
		this.submenu = Engine.GetGUIObjectByName("submenu");
		this.submenuButtons = Engine.GetGUIObjectByName("submenuButtons");
		this.MainMenuPanelRightBorderTop = Engine.GetGUIObjectByName("MainMenuPanelRightBorderTop");
		this.MainMenuPanelRightBorderBottom = Engine.GetGUIObjectByName("MainMenuPanelRightBorderBottom");

		this.setupMenuButtons(this.mainMenuButtons.children, this.menuItems);
		this.setupHotkeys(this.menuItems);
		Engine.GetGUIObjectByName("closeMenuButton").onPress = () => { this.closeSubmenu(); };
	}

	setupMenuButtons(buttons, menuItems)
	{
		buttons.forEach((button, i) => {
			let item = menuItems[i];
			button.hidden = !item;
			if (button.hidden)
				return;

			button.size = new GUISize(
				0, (this.buttonHeight + this.margin) * i,
				0, (this.buttonHeight + this.margin) * i + this.buttonHeight,
				0, 0, 100, 0);
			button.caption = item.caption;
			button.tooltip = item.tooltip;
			button.enabled = item.enabled === undefined || item.enabled;
			button.onPress = () => {
				this.closeSubmenu();

				if (item.onPress)
					item.onPress();
				else
					this.openSubmenu(i);
			};
			button.hidden = false;
		});

		if (buttons.length < menuItems.length)
			error("GUI page has space for " + buttons.length + " menu buttons, but " + menuItems.length + " items are provided!");
	}

	setupHotkeys(menuItems)
	{
		for (let name in menuItems)
		{
			let item = menuItems[name];

			if (item.onPress && item.hotkey)
			{
				Engine.SetGlobalHotkey(item.hotkey, () => {
					this.closeSubmenu();
					item.onPress();
				});
			}

			if (item.submenu)
				this.setupHotkeys(item.submenu);
		}
	}

	openSubmenu(i)
	{
		this.setupMenuButtons(this.submenuButtons.children, this.menuItems[i].submenu);
		let top = this.mainMenuButtons.size.top + this.mainMenuButtons.children[i].size.top;
		this.submenu.size = new GUISize(
			this.submenu.size.left, top - this.margin,
			this.submenu.size.right, top + ((this.buttonHeight + this.margin) * this.menuItems[i].submenu.length));
		this.submenu.hidden = false;
		this.MainMenuPanelRightBorderTop.size = "100%-2 0 100% " + (this.submenu.size.top + this.margin);
		this.MainMenuPanelRightBorderBottom.size = "100%-2 " + this.submenu.size.bottom + " 100% 100%";
	}

	closeSubmenu()
	{
		this.submenu.hidden = true;
		this.submenu.size = this.mainMenu.size;
		this.MainMenuPanelRightBorderTop.size = "100%-2 0 100% 100%";
	}

	onTick()
	{
		let now = Date.now();

		let maxOffset = this.mainMenu.size.right - this.submenu.size.left;
		let offset = Math.min(this.menuSpeed * (now - this.lastTickTime), maxOffset);

		this.lastTickTime = now;

		if (this.submenu.hidden || offset <= 0)
			return;

		let size = this.submenu.size;
		size.left += offset;
		size.right += offset;
		this.submenu.size = size;
	}
}
