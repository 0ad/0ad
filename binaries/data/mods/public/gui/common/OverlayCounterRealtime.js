/**
 * Shows the current time to the player in their current timezone.
 */
OverlayCounterTypes.prototype.Realtime = class extends OverlayCounter
{
	constructor(overlayCounterManager)
	{
		super(overlayCounterManager);
		this.date = new Date();
	}

	get()
	{
		this.date.setTime(Date.now());
		return this.date.toLocaleTimeString();
	}
};

OverlayCounterTypes.prototype.Realtime.prototype.Config = "overlay.realtime";

OverlayCounterTypes.prototype.Realtime.prototype.Hotkey = "realtime.toggle";
