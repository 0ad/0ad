function Timer() {}

Timer.prototype.Schema =
	"<a:component type='system'/><empty/>";

Timer.prototype.Init = function()
{
	this.id = 0;
	this.time = 0;
	this.timers = {};
	this.turnLength = 0;
};

/**
 * Returns time since the start of the game, in integer milliseconds.
 */
Timer.prototype.GetTime = function()
{
	return this.time;
}

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
	var id = ++this.id;
	this.timers[id] = [ent, iid, funcname, this.time + time, 0, data];
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
	var id = ++this.id;
	this.timers[id] = [ent, iid, funcname, this.time + time, repeattime, data];
	return id;
};

/**
 * Cancels an existing timer that was created with SetTimeout/SetInterval.
 */
Timer.prototype.CancelTimer = function(id)
{
	delete this.timers[id];
};


Timer.prototype.OnUpdate = function(msg)
{
	var dt = Math.round(msg.turnLength * 1000);
	this.time += dt;
	this.turnLength = dt;

	// Collect the timers that need to run
	// (We do this in two stages to avoid deleting from the timer list while
	// we're in the middle of iterating through it)
	var run = [];
	for (var id in this.timers)
	{
		if (this.timers[id][3] <= this.time)
			run.push(id);
	}
	for (var i = 0; i < run.length; ++i)
	{
		var id = run[i];

		var t = this.timers[id];
		if (!t)
			continue; // an earlier timer might have cancelled this one, so skip it

		var cmp = Engine.QueryInterface(t[0], t[1]);
		if (!cmp)
		{
			// The entity was probably destroyed; clean up the timer
			delete this.timers[id];
			continue;
		}

		try {
			var lateness = this.time - t[3];
			cmp[t[2]](t[5], lateness);
		} catch (e) {
			var stack = e.stack.trimRight().replace(/^/mg, '  '); // indent the stack trace
			error("Error in timer on entity "+t[0]+", IID "+t[1]+", function "+t[2]+": "+e+"\n"+stack+"\n");
		}

		// Handle repeating timers
		if (t[4])
		{
			// Add the repeat time to the execution time
			t[3] += t[4];
			// Add it to the list to get re-executed if it's soon enough
			if (t[3] <= this.time)
				run.push(id);
		}
		else
		{
			// Non-repeating time - delete it
			delete this.timers[id];
		}
	}
}

Engine.RegisterSystemComponentType(IID_Timer, "Timer", Timer);
