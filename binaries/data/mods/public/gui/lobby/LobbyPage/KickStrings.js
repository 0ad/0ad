/**
 * This class provides translated kick event notification strings,
 * consumed by chat and notification message box.
 */
class KickStrings
{
	constructor()
	{
		this.nickArgs = {};
		this.reasonArgs = {};
	}

	get(banned, message)
	{
		let level = banned ? 1 : 0;
		let me = message.nick == g_Nickname;

		let txt;
		if (me)
			txt = this.Strings.Local[level];
		else
		{
			this.nickArgs.nick = escapeText(message.nick);
			txt = sprintf(this.Strings.Remote[level], this.nickArgs);
		}

		if (message.reason)
		{
			this.reasonArgs.reason = escapeText(message.reason);
			txt += " " + sprintf(this.Reason, this.reasonArgs);
		}

		return txt;
	}
}

KickStrings.prototype.Strings = {
	"Local": [
		translate("You have been kicked from the lobby!"),
		translate("You have been banned from the lobby!")
	],
	"Remote": [
		translate("%(nick)s has been kicked from the lobby."),
		translate("%(nick)s has been banned from the lobby.")
	]
};

KickStrings.prototype.Reason =
	translateWithContext("lobby kick", "Reason: %(reason)s");
