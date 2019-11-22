/**
 * This class deals with the button that opens the profile view page.
 */
class ProfileButton
{
	constructor(xmppMessages, profilePage)
	{
		this.profileButton = Engine.GetGUIObjectByName("profileButton");
		this.profileButton.caption = translate("Player Profile Lookup");
		this.profileButton.onPress = profilePage.openPage.bind(profilePage, false);

		let onConnectionStatusChange = this.onConnectionStatusChange.bind(this);
		xmppMessages.registerXmppMessageHandler("system", "connected", onConnectionStatusChange);
		xmppMessages.registerXmppMessageHandler("system", "disconnected", onConnectionStatusChange);
		this.onConnectionStatusChange();
	}

	onConnectionStatusChange()
	{
		this.profileButton.enabled = Engine.IsXmppClientConnected();
	}
}
