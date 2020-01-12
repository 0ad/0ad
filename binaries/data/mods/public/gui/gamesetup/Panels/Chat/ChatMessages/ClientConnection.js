ChatMessageEvents.ClientConnection = class
{
	constructor(chatMessagesPanel, netMessages, gameSettingsControl, playerAssignmentsControl)
	{
		this.chatMessagesPanel = chatMessagesPanel;

		playerAssignmentsControl.registerClientJoinHandler(this.onClientJoin.bind(this));
		playerAssignmentsControl.registerClientLeaveHandler(this.onClientLeave.bind(this));

		this.args = {};
	}

	onClientJoin(newGUID, newAssignments)
	{
		this.args.username = newAssignments[newGUID].name;
		this.chatMessagesPanel.addStatusMessage(sprintf(this.JoinText, this.args));
	}

	onClientLeave(guid)
	{
		this.args.username = g_PlayerAssignments[guid].name;
		this.chatMessagesPanel.addStatusMessage(sprintf(this.LeaveText, this.args));
	}
};

ChatMessageEvents.ClientConnection.prototype.JoinText =
	translate("%(username)s has joined");

ChatMessageEvents.ClientConnection.prototype.LeaveText =
	translate("%(username)s has left");
