GameSettingControls.MapBrowser = class extends GameSettingControlButton
{
	constructor(...args)
	{
		super(...args);

		this.button.tooltip = colorizeHotkey(this.HotkeyTooltip, this.HotkeyConfig);
		Engine.SetGlobalHotkey(this.HotkeyConfig, "Press", this.onPress.bind(this));
	}

	setControlHidden()
	{
		this.button.hidden = false;
	}

	onPress()
	{
		this.setupWindow.pages.MapBrowserPage.openPage();
	}
};

GameSettingControls.MapBrowser.prototype.HotkeyConfig =
	"gamesetup.mapbrowser.open";

GameSettingControls.MapBrowser.prototype.Caption =
	translate("Browse Maps");

GameSettingControls.MapBrowser.prototype.HotkeyTooltip =
	translate("Press %(hotkey)s to view the list of available maps.");
