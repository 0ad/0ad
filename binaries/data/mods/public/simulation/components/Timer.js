function Timer() {}

Timer.prototype.Schema =
	"<a:component type='system'/><empty/>";

Timer.prototype.Init = function()
{
	this.id = 0;
	this.time = 0;
	this.timers = new Map();
	this.turnLength = 0;
};

/**
 * Returns time since the start of the game in milliseconds.
 */
Timer.prototype.GetTime = function()
{
	return this.time;
};

/**
 * Returns the duration of the latest turn in milliseconds.
 */
Timer.prototype.GetLatestTurnLength = function()
{
	return this.turnLength;
};

/**
 * Create a new timer, which will call the 'funcname' method with arguments (data, lateness)
 * on the 'iid' component of the 'ent' entity, after at least 'time' milliseconds.
 * 'lateness' is how late the timer is executed after the specified time (in milliseconds).
 * Returns a non-zero id that can be passed to CancelTimer.
 */
Timer.prototype.SetTimeout = function(ent, iid, funcname, time, data)
{
	let id = ++this.id;

	this.timers.set(id, {
		"entity": ent,
		"iid": iid,
		"functionName": funcname,
		"time": this.time + time,
		"repeatTime": 0,
		"data": data
	});

	return id;
};

/**
 * Create a new repeating timer, which will call the 'funcname' method with arguments (data, lateness)
 * on the 'iid' component of the 'ent' entity, after at least 'time' milliseconds
 * and then every 'repeattime' milliseconds thereafter.
 * It will run multiple times per simulation turn if necessary.
 * 'repeattime' must be non-zero.
 * 'lateness' is how late the timer is executed after the specified time (in milliseconds).
 * Returns a non-zero id that can be passed to CancelTimer.
 */
Timer.prototype.SetInterval = function(ent, iid, funcname, time, repeattime, data)
{
	if (typeof repeattime != "number" || !(repeattime > 0))
		error("Invalid repeattime to SetInterval of "+funcname);

	let id = ++this.id;

	this.timers.set(id, {
		"entity": ent,
		"iid": iid,
		"functionName": funcname,
		"time": this.time + time,
		"repeatTime": repeattime,
		"data": data
	});

	return id;
};

/**
 * Cancels an existing timer that was created with SetTimeout/SetInterval.
 */
Timer.prototype.CancelTimer = function(id)
{
	this.timers.delete(id);
};


Timer.prototype.OnUpdate = function(msg)
{
	this.turnLength = Math.round(msg.turnLength * 1000);
	this.time += this.turnLength;

	// Collect the timers that need to run
	// (We do this in two stages to avoid deleting from the timer list while
	// we're in the middle of iterating through it)
	let run = [];
	for (let [id, timer] of this.timers)
		if (timer.time <= this.time)
			run.push(id);

	for (let id of run)
	{
		let timer = this.timers.get(id);

		// An earlier timer might have cancelled this one, so skip it
		if (!timer)
			continue;

		// The entity was probably destroyed; clean up the timer
		let cmpTimer = Engine.QueryInterface(timer.entity, timer.iid);
		if (!cmpTimer)
		{
			this.timers.delete(id);
			continue;
		}

		try
		{
			cmpTimer[timer.functionName](timer.data, this.time - timer.time);
		}
		catch (e)
		{
			error(
				"Error in timer on entity " + timer.entity + ", " +
				"IID" + timer.iid + ", " +
				"function " + timer.functionName + ": " +
				e + "\n" +
				// Indent the stack trace
				e.stack.trimRight().replace(/^/mg, '  ') + "\n");
		}

		if (!timer.repeatTime)
		{
			this.timers.delete(id);
			continue;
		}

		timer.time += timer.repeatTime;

		// Add it to the list to get re-executed if it's soon enough
		if (timer.time <= this.time)
			run.push(id);
	}
};

Engine.RegisterSystemComponentType(IID_Timer, "Timer", Timer);
