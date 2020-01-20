class SoundNotification
{
	constructor(setupWindow)
	{
		setupWindow.controls.netMessages.registerNetMessageHandler("chat", this.onClientChat.bind(this));
		setupWindow.controls.playerAssignmentsControl.registerClientJoinHandler(this.onClientJoin.bind(this));
	}

	onClientJoin(guid)
	{
		if (guid != Engine.GetPlayerGUID())
			soundNotification(this.ConfigJoinNotification);
	}

	onClientChat(message)
	{
		if (message.guid != Engine.GetPlayerGUID() &&
			message.text.toLowerCase().indexOf(
				splitRatingFromNick(g_PlayerAssignments[Engine.GetPlayerGUID()].name).nick.toLowerCase()) != -1)
			soundNotification(this.ConfigNickNotification);
	}
}

SoundNotification.prototype.ConfigJoinNotification =
	"gamesetup.join";

SoundNotification.prototype.ConfigNickNotification =
	"nick";
