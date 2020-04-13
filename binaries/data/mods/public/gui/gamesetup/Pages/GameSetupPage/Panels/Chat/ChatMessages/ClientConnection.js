ChatMessageEvents.ClientConnection = class
{
	constructor(setupWindow, chatMessagesPanel)
	{
		this.chatMessagesPanel = chatMessagesPanel;

		setupWindow.controls.playerAssignmentsControl.registerClientJoinHandler(this.onClientJoin.bind(this));
		setupWindow.controls.playerAssignmentsControl.registerClientLeaveHandler(this.onClientLeave.bind(this));

		this.args = {};
	}

	onClientJoin(newGUID, newAssignments)
	{
		this.args.username = newAssignments[newGUID].name;
		this.chatMessagesPanel.addStatusMessage(sprintf(this.JoinText, this.args));
	}

	onClientLeave(guid)
	{
		this.args.username = colorizePlayernameByGUID(guid);
		this.chatMessagesPanel.addStatusMessage(sprintf(this.LeaveText, this.args));
	}
};

ChatMessageEvents.ClientConnection.prototype.JoinText =
	translate("%(username)s has joined");

ChatMessageEvents.ClientConnection.prototype.LeaveText =
	translate("%(username)s has left");
