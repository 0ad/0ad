/**
 * Since every GUI page can display the FPS or realtime counter,
 * this manager is initialized for every GUI page.
 */
var g_OverlayCounterManager;

class OverlayCounterManager
{
	constructor(dataCounter)
	{
		this.dataCounter = dataCounter;
		this.counters = [];
		this.enabledCounters = [];
		this.lastTick = undefined;
		this.resizeHandlers = [];
		this.lastHeight = undefined;

		for (let name of this.availableCounterNames())
		{
			let counterType = OverlayCounterTypes.prototype[name];
			if (counterType.IsAvailable && !counterType.IsAvailable())
				continue;

			let counter = new counterType(this);
			this.counters.push(counter);
			counter.updateEnabled();
		}

		this.dataCounter.onTick = this.onTick.bind(this);
	}

	/**
	 * Mods may overwrite this to change the order of the counters shown.
	 */
	availableCounterNames()
	{
		return Object.keys(OverlayCounterTypes.prototype);
	}

	deleteCounter(counter)
	{
		let filter = count => count != counter;
		this.counters = this.counters.filter(filter);
		this.enabledCounters = this.enabledCounters.filter(filter);
	}

	/**
	 * This function allows enabling and disabling of timers while preserving the counter order.
	 */
	setCounterEnabled(counter, enabled)
	{
		if (enabled)
			this.enabledCounters = this.counters.filter(count =>
				this.enabledCounters.indexOf(count) != -1 || count == counter);
		else
			this.enabledCounters = this.enabledCounters.filter(count => count != counter);

		// Update instantly
		this.lastTick = undefined;
		this.onTick();
	}

	/**
	 * Handlers subscribed here will be informed when the dimension of the overlay changed.
	 * This allows placing the buttons away from the counter.
	 */
	registerResizeHandler(handler)
	{
		this.resizeHandlers.push(handler);
	}

	onTick()
	{
		// Don't rebuild the caption every frame.
		const now = Date.now();
		if (now < this.lastTick + this.Delay)
			return;

		this.lastTick = now;

		let txt = "";

		for (let counter of this.enabledCounters)
		{
			const newTxt = counter.get();
			if (newTxt)
				txt += newTxt + "\n";
		}

		let height;
		if (txt)
		{
			this.dataCounter.caption = txt;
			const size = resizeGUIObjectToCaption(this.dataCounter, this.Alignment, this.Margins);
			height = size.bottom - size.top;
		}
		else
			height = 0;

		this.dataCounter.hidden = !txt;

		if (this.lastHeight != height)
		{
			this.lastHeight = height;
			for (let handler of this.resizeHandlers)
				handler(height);
		}
	}
}

/**
 * To minimize the computation performed every frame, this duration
 * in milliseconds determines how often the caption is rebuilt.
 */
OverlayCounterManager.prototype.Delay = 250;

/**
 * The parameters to resize the data counter to.
 */
OverlayCounterManager.prototype.Alignment = {
	"horizontal": "left",
	"vertical": "bottom"
};

OverlayCounterManager.prototype.Margins = {
	"vertical": -2
};
