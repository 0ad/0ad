var g_TimerID = 0;
var g_Timers = {};
var g_Time = Date.now();

/**
 * Set a timeout to call func() after 'delay' msecs.
 * func: function to call
 * delay: delay in ms
 * Returns an id that can be passed to clearTimeout.
 */
function setTimeout(func, delay)
{
	var id = ++g_TimerID;
	g_Timers[id] = [g_Time + delay, func];
	return id;
}

/**
 * deletes a timer
 * id: of the timer
 */
function clearTimeout(id)
{
	delete g_Timers[id];
}

/**
* alters an function call
* id: of the timer
* func: function to call
*/
function setNewTimerFunction(id, func)
{
	if (id in g_Timers)
		g_Timers[id][1] = func;
}

/**
 * If you want to use timers, then you must call this function regularly
 * (e.g. in a Tick handler)
 */
function updateTimers()
{
	g_Time = Date.now();

	// Collect the timers that need to run
	// (We do this in two stages to avoid deleting from the timer list while
	// we're in the middle of iterating through it)
	var run = [];
	for (var id in g_Timers)
	{
		if (g_Timers[id][0] <= g_Time)
			run.push(id);
	}
	for each (var id in run)
	{
		var t = g_Timers[id];
		if (!t)
			continue; // an earlier timer might have cancelled this one, so skip it

		try {
			t[1]();
		} catch (e) {
			var stack = e.stack.trimRight().replace(/^/mg, '  '); // indent the stack trace
			error(sprintf("Error in timer: %(error)s", { error: e })+"\n"+stack+"\n");
		}
		delete g_Timers[id];
	}
}
