/**
 * The purpose of this page is to display a placeholder in multiplayer until the settings from the server have been received.
 * This is not technically necessary, but only performed to avoid confusion or irritation when showing the clients first the
 * default settings and then switching to the server settings quickly thereafter.
 */
class LoadingWindow
{
	constructor(netMessages)
	{
		if (g_IsNetworked)
			netMessages.registerNetMessageHandler("gamesetup", this.hideLoadingWindow.bind(this));
		else
			this.hideLoadingWindow();
	}

	hideLoadingWindow()
	{
		let loadingWindow = Engine.GetGUIObjectByName("loadingWindow");
		if (loadingWindow.hidden)
			return;

		loadingWindow.hidden = true;
		Engine.GetGUIObjectByName("setupWindow").hidden = false;
	}
}
