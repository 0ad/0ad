function Timer() {}

Timer.prototype.Init = function()
{
	this.id = 0;
	this.time = 0;
	this.timers = {};
};

Timer.prototype.GetTime = function()
{
	return this.time;
}

Timer.prototype.OnUpdate = function(msg)
{
	var dt = Math.round(msg.turnLength * 1000);
	this.time += dt;

	// Collect the timers that need to run
	// (We do this in two stages to avoid deleting from the timer list while
	// we're in the middle of iterating through it)
	var run = [];
	for (var id in this.timers)
	{
		if (this.timers[id][3] <= this.time)
			run.push(id);
	}
	for each (var id in run)
	{
		var t = this.timers[id];
		var cmp = Engine.QueryInterface(t[0], t[1]);
		try {
			cmp[t[2]](t[4]);
		} catch (e) {
			print("Error in timer on entity "+t[0]+", IID "+t[1]+", function "+t[2]+": "+e+"\n");
			// TODO: should report in an error log
		}
		delete this.timers[id];
	}
}

/**
 * Create a new timer, which will call the 'funcname' method with argument 'data'
 * on the 'iid' component of the 'ent' entity, after at least 'time' milliseconds.
 * Returns a non-zero id that can be passed to CancelTimer.
 */
Timer.prototype.SetTimeout = function(ent, iid, funcname, time, data)
{
	var id = ++this.id;
	this.timers[id] = [ent, iid, funcname, this.time + time, data];
	return id;
};

Timer.prototype.CancelTimer = function(id)
{
	delete this.timers[id];
};

Engine.RegisterComponentType(IID_Timer, "Timer", Timer);

