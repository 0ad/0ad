/**
 * @file The classes in this file trigger notifications about occurrences in the multi-user
 * chat room that are not chat messages, nor SystemMessages.
 */

ChatMessageEvents.ClientEvents = class
{
	constructor(xmppMessages, chatMessagesPanel, statusMessageFormat)
	{
		this.chatMessagesPanel = chatMessagesPanel;
		this.statusMessageFormat = statusMessageFormat;
		this.kickStrings = new KickStrings();
		this.nickArgs = {};

		xmppMessages.registerXmppMessageHandler("chat", "join", this.onClientJoin.bind(this));
		xmppMessages.registerXmppMessageHandler("chat", "leave", this.onClientLeave.bind(this));
		xmppMessages.registerXmppMessageHandler("chat", "kicked", this.onClientKicked.bind(this, false));
		xmppMessages.registerXmppMessageHandler("chat", "banned", this.onClientKicked.bind(this, true));
	}

	onClientJoin(message)
	{
		this.nickArgs.nick = escapeText(message.nick);
		this.chatMessagesPanel.addText(
			message.time,
			this.statusMessageFormat.format(sprintf(this.FormatJoin, this.nickArgs)));
	}

	onClientLeave(message)
	{
		this.nickArgs.nick = escapeText(message.nick);
		this.chatMessagesPanel.addText(
			message.time,
			this.statusMessageFormat.format(sprintf(this.FormatLeave, this.nickArgs)));
	}

	onClientKicked(banned, message)
	{
		// If the local player had been kicked, that is logged more vividly than a neutral status message
		if (message.nick != g_Nickname)
			this.chatMessagesPanel.addText(
				message.time,
				this.statusMessageFormat.format(this.kickStrings.get(banned, message)));
	}
};

ChatMessageEvents.ClientEvents.prototype.FormatJoin = translate("%(nick)s has joined.");
ChatMessageEvents.ClientEvents.prototype.FormatLeave = translate("%(nick)s has left.");

ChatMessageEvents.Nick = class
{
	constructor(xmppMessages, chatMessagesPanel, statusMessageFormat)
	{
		this.chatMessagesPanel = chatMessagesPanel;
		this.statusMessageFormat = statusMessageFormat;
		this.args = {};
		xmppMessages.registerXmppMessageHandler("chat", "nick", this.onNickChange.bind(this));
	}

	onNickChange(message)
	{
		this.args.oldnick = escapeText(message.oldnick);
		this.args.newnick = escapeText(message.newnick);
		this.chatMessagesPanel.addText(
			message.time,
			this.statusMessageFormat.format(sprintf(this.Format, this.args)));
	}
};

ChatMessageEvents.Nick.prototype.Format = translate("%(oldnick)s is now known as %(newnick)s.");

ChatMessageEvents.Role = class
{
	constructor(xmppMessages, chatMessagesPanel, statusMessageFormat)
	{
		this.chatMessagesPanel = chatMessagesPanel;
		this.statusMessageFormat = statusMessageFormat;
		this.args = {};
		xmppMessages.registerXmppMessageHandler("chat", "role", this.onRoleChange.bind(this));
	}

	onRoleChange(message)
	{
		let roleType = this.RoleStrings.find(type =>
			type.newrole == message.newrole &&
			(!type.oldrole || type.oldrole == message.oldrole));

		let txt;
		if (message.nick == g_Nickname)
			txt = roleType.you;
		else
		{
			this.args.nick = escapeText(message.nick);
			txt = sprintf(roleType.nick, this.args);
		}

		this.chatMessagesPanel.addText(
			message.time,
			this.statusMessageFormat.format(txt));
	}
};

ChatMessageEvents.Role.prototype.RoleStrings =
[
	{
		"newrole": "visitor",
		"you": translate("You have been muted."),
		"nick": translate("%(nick)s has been muted.")
	},
	{
		"newrole": "moderator",
		"you": translate("You are now a moderator."),
		"nick": translate("%(nick)s is now a moderator.")
	},
	{
		"newrole": "participant",
		"oldrole": "visitor",
		"you": translate("You have been unmuted."),
		"nick": translate("%(nick)s has been unmuted.")
	},
	{
		"newrole": "participant",
		"oldrole": "moderator",
		"you": translate("You are not a moderator anymore."),
		"nick": translate("%(nick)s is not a moderator anymore.")
	}
];

ChatMessageEvents.Subject = class
{
	constructor(xmppMessages, chatMessagesPanel, statusMessageFormat)
	{
		this.chatMessagesPanel = chatMessagesPanel;
		this.statusMessageFormat = statusMessageFormat;
		this.args = {};
		xmppMessages.registerXmppMessageHandler("chat", "subject", this.onSubjectChange.bind(this));
	}

	onSubjectChange(message)
	{
		this.args.nick = escapeText(message.nick);
		let subject = message.subject.trim();
		this.chatMessagesPanel.addText(
			message.time,
			this.statusMessageFormat.format(
				subject ?
					sprintf(this.FormatChange, this.args) + "\n" + subject :
					sprintf(this.FormatDelete, this.args)));
	}
};

ChatMessageEvents.Subject.prototype.FormatChange = translate("%(nick)s changed the lobby subject to:");
ChatMessageEvents.Subject.prototype.FormatDelete = translate("%(nick)s deleted the lobby subject.");
