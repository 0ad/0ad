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
 * @returns {number} - The elapsed time in milliseconds since the game was started.
 */
Timer.prototype.GetTime = function()
{
	return this.time;
};

/**
 * @returns {number} - The duration of the latest turn in milliseconds.
 */
Timer.prototype.GetLatestTurnLength = function()
{
	return this.turnLength;
};

/**
 * Create a new timer, which will call the 'funcname' method with arguments (data, lateness)
 * on the 'iid' component of the 'ent' entity, after at least 'time' milliseconds.
 * 'lateness' is how late the timer is executed after the specified time (in milliseconds).
 * @param {number} ent - The entity id to which the timer will be assigned to.
 * @param {number} iid - The component iid of the timer.
 * @param {string} funcname - The name of the function to be called in the component.
 * @param {number} time - The delay before running the function for the first time.
 * @param {any} data - The data to pass to the function.
 * @returns {number} - A non-zero id that can be passed to CancelTimer.
 */
Timer.prototype.SetTimeout = function(ent, iid, funcname, time, data)
{
	return this.SetInterval(ent, iid, funcname, time, 0, data);
};

/**
 * Create a new repeating timer, which will call the 'funcname' method with arguments (data, lateness)
 * on the 'iid' component of the 'ent' entity, after at least 'time' milliseconds.
 * 'lateness' is how late the timer is executed after the specified time (in milliseconds)
 * and then every 'repeattime' milliseconds thereafter.
 * @param {number} ent - The entity the timer will be assigned to.
 * @param {number} iid - The component iid of the timer.
 * @param {string} funcname - The name of the function to be called in the component.
 * @param {number} time - The delay before running the function for the first time.
 * @param {number} repeattime - If non-zero, the interval between each execution of the function.
 * @param {any} data - The data to pass to the function.
 * @returns {number} - A non-zero id that can be passed to CancelTimer.
 */
Timer.prototype.SetInterval = function(ent, iid, funcname, time, repeattime, data)
{
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
 * Updates the repeat time of a timer.
 * Note that this will take only effect after the next update.
 *
 * @param {number} timerID - The timer to update.
 * @param {number} newRepeatTime - The new repeat time to use.
 */
Timer.prototype.UpdateRepeatTime = function(timerID, newRepeatTime)
{
	let timer = this.timers.get(timerID);
	if (timer)
		this.timers.set(timerID, {
			"entity": timer.entity,
			"iid": timer.iid,
			"functionName": timer.functionName,
			"time": timer.time,
			"repeatTime": newRepeatTime,
			"data": timer.data
		});
};

/**
 * Cancels an existing timer that was created with SetTimeout/SetInterval.
 * @param {number} id - The timer's ID returned by either SetTimeout or SetInterval.
 */
Timer.prototype.CancelTimer = function(id)
{
	this.timers.delete(id);
};

/**
 * @param {{ "turnLength": number }} msg - A message containing the turn length in seconds.
 */
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
		let timerTargetComponent = Engine.QueryInterface(timer.entity, timer.iid);
		if (!timerTargetComponent)
		{
			this.timers.delete(id);
			continue;
		}

		try
		{
			timerTargetComponent[timer.functionName](timer.data, this.time - timer.time);
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
