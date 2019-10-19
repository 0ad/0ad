/**
 * This class manages the timewarp hotkeys that allow jumping back in time and fast forwarding the game.
 */
class TimeWarp
{
	constructor()
	{
		this.enabled = false;

		this.timewarpRewind = Engine.GetGUIObjectByName("timewarpRewind");
		this.timewarpRewind.onPress = this.rewind.bind(this);

		this.timewarpFastForward = Engine.GetGUIObjectByName("timewarpFastForward");
		this.timewarpFastForward.onPress = this.fastForward.bind(this);
		this.timewarpFastForward.onRelease = this.resetSpeed.bind(this);
	}

	setEnabled(enabled)
	{
		if (g_IsNetworked)
			return;

		this.enabled = enabled;

		if (enabled)
			messageBox(
				500, 250,
				translate(this.Description),
				translate(this.Title));

		Engine.EnableTimeWarpRecording(enabled ? this.NumberTurns : 0);
	}

	rewind()
	{
		if (this.enabled)
			Engine.RewindTimeWarp();
	}

	fastForward()
	{
		if (this.enabled)
			Engine.SetSimRate(this.FastForwardSpeed);
	}

	resetSpeed()
	{
		if (this.enabled)
			Engine.SetSimRate(1);
	}
}

/**
 * Number of turns between snapshots.
 */
TimeWarp.prototype.NumberTurns = 10;

/**
 * Gamespeed used while pressing the fast forward hotkey.
 */
TimeWarp.prototype.FastForwardSpeed = 20;

TimeWarp.prototype.Title = markForTranslation("Time warp mode");

TimeWarp.prototype.Description = markForTranslation(
	"Note: time warp mode is a developer option, and not intended for use over long periods of time. Using it incorrectly may cause the game to run out of memory or crash.");
