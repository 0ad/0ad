var g_LobbyMessages = {
	"error": message => {
		setFeedback(message.text ||
			translate("Unknown error. This usually occurs because the same IP address is not allowed to register more than one account within one hour."));
		Engine.StopXmppClient();
	},
	"disconnected": message => {
		setFeedback(message.reason + message.certificate_status);
		Engine.StopXmppClient();
	}
};

/**
 * Other message types (such as gamelists) may be received in case of the current player being logged in and
 * logging in in a second program instance with the same account name.
 * Therefore messages without handlers are ignored without reporting them here.
 */
function onTick()
{
	let messages = Engine.LobbyGuiPollNewMessages();
	if (!messages)
		return;

	for (let message of messages)
	{
		if (message.type == "system" && message.level)
			g_LobbyMessages[message.level](message);

		if (!Engine.HasXmppClient())
			break;
	}
}

function setFeedback(feedbackText)
{
	Engine.GetGUIObjectByName("feedback").caption = feedbackText;
	Engine.GetGUIObjectByName("continue").enabled = !feedbackText;
}

function cancelButton()
{
	if (Engine.HasXmppClient())
		Engine.StopXmppClient();
	Engine.PopGuiPage();
}
