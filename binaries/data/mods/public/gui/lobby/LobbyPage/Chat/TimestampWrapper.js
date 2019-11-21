/**
 * This class wraps a string with a timestamp dating to when the message was sent.
 */
class TimestampWrapper
{
	constructor()
	{
		this.timeArgs = {};
		this.timestampArgs = {};
	}

	format(timestamp, text)
	{
		this.timeArgs.time =
			Engine.FormatMillisecondsIntoDateStringLocal(timestamp * 1000, this.TimeFormat);

		this.timestampArgs.time = sprintf(this.TimestampFormat, this.timeArgs);
		this.timestampArgs.message = text;

		return sprintf(this.TimestampedMessageFormat, this.timestampArgs);
	}
}

// Translation: Chat message format when there is a time prefix.
TimestampWrapper.prototype.TimestampedMessageFormat = translate("%(time)s %(message)s");

// Translation: Time prefix as shown in the multiplayer lobby (when you enable it in the options page).
TimestampWrapper.prototype.TimestampFormat = translate("\\[%(time)s]");

// Translation: Time as shown in the multiplayer lobby (when you enable it in the options page).
// For a list of symbols that you can use, see:
// https://sites.google.com/site/icuprojectuserguide/formatparse/datetime?pli=1#TOC-Date-Field-Symbol-Table
TimestampWrapper.prototype.TimeFormat = translate("HH:mm");
