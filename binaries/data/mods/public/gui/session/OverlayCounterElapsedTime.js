/**
 * This class shows the simulated match time below the FPS counter.
 */
OverlayCounterTypes.prototype.ElapsedTime = class extends OverlayCounter
{
	constructor(overlayCounterManager)
	{
		super(overlayCounterManager);

		// Performance optimization
		this.caption = translate(this.Caption);
		this.sprintfData = {};
	}

	get()
	{
		if (!g_SimState)
			return "";

		let time = timeToString(g_SimState.timeElapsed);

		let speed = Engine.GetSimRate();
		if (speed == 1)
			return time;

		this.sprintfData.time = time;
		this.sprintfData.speed = Engine.FormatDecimalNumberIntoString(speed);
		return sprintf(this.caption, this.sprintfData);
	}
};

// Translation: The "x" means "times", with the mathematical meaning of multiplication.
OverlayCounterTypes.prototype.ElapsedTime.prototype.Caption = markForTranslation("%(time)s (%(speed)sx)");

OverlayCounterTypes.prototype.ElapsedTime.prototype.Config = "gui.session.timeelapsedcounter";

OverlayCounterTypes.prototype.ElapsedTime.prototype.Hotkey = "timeelapsedcounter.toggle";
