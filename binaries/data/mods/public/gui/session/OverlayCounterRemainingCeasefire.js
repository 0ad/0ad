/**
 * Adds the ceasefire counter to the global FPS and
 * realtime counters shown in the top right corner.
 */
OverlayCounterTypes.prototype.RemainingCeasefire = class extends OverlayCounter
{
	constructor(overlayCounterManager)
	{
		super(overlayCounterManager);
		registerCeasefireEndedHandler(this.onCeasefireEnded.bind(this));
	}

	onCeasefireEnded()
	{
		this.overlayCounterManager.deleteCounter(this);
	}

	get()
	{
		return timeToString(g_SimState.ceasefireTimeRemaining);
	}
};

OverlayCounterTypes.prototype.RemainingCeasefire.prototype.Config = "gui.session.ceasefirecounter";

OverlayCounterTypes.prototype.RemainingCeasefire.prototype.Hotkey = "ceasefirecounter.toggle";

OverlayCounterTypes.prototype.RemainingCeasefire.IsAvailable = () => GetSimState().ceasefireActive;
