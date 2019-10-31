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
		this.lineHeight = dataCounter.size.bottom - dataCounter.size.top;
		this.counters = [];
		this.enabledCounters = [];
		this.lastTick = undefined;
		this.lastLineCount = undefined;
		this.resizeHandlers = [];

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
	 * Handlers subscribed here will be informed then the dimension of the overlay changed.
	 * This allows placing the buttons below the counter.
	 */
	registerResizeHandler(handler)
	{
		this.resizeHandlers.push(handler);
	}

	onTick()
	{
		// Don't rebuild the caption every frame
		let now = Date.now();
		if (now < this.lastTick + this.Delay)
			return;

		this.lastTick = now;

		let lineCount = 0;
		let txt = "";

		for (let counter of this.enabledCounters)
		{
			let newTxt = counter.get();
			if (!newTxt)
				continue;

			++lineCount;
			txt += newTxt + "\n";
		}

		if (lineCount)
			this.dataCounter.caption = txt;

		// The caption changes more often than not,
		// but adding or removing lines happens rarely.
		if (this.lastLineCount == lineCount)
			return;

		this.lastLineCount = lineCount;

		let offset = this.lineHeight * lineCount;

		if (lineCount)
		{
			let size = this.dataCounter.size;
			size.bottom = size.top + offset;
			this.dataCounter.size = size;
		}

		this.dataCounter.hidden = !lineCount;

		for (let handler of this.resizeHandlers)
			handler(offset);
	}
}

/**
 * To minimize the computation performed every frame, this duration
 * in milliseconds determines how often the caption is rebuilt.
 */
OverlayCounterManager.prototype.Delay = 250;
