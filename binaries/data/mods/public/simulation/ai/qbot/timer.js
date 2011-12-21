//The Timer class // The instance of this class is created in the qBot object under the name 'timer'
//The methods that are available to call from this instance are:
//timer.setTimer				: Creates a new timer with the given interval (miliseconds).
//												Optional set dalay or a limited repeat value.
//timer.checkTimer			: Gives true if called at the time of the interval.
//timer.clearTimer			: Deletes the timer permanently. No way to get the same timer back.
//timer.activateTimer		: Sets the status of a deactivated timer to active.
//timer.deactivateTimer	: Deactivates a timer. Deactivated timers will never give true.


//-EmjeR-// Timer class //
function Timer()
{
	var alarmList = new Array();
	this.setTimer = set_timer;
	this.checkTimer = check_timer;
	this.clearTimer = clear_timer;
	this.activateTimer = activate_timer;
	this.deactivateTimer = deactivate_timer;
	
	// Get an alarm with 'id'
	function get_alarm(id)
	{
		return this.alarmList[id];
	}
}

	// Add an new alarm to the list
	function set_alarm(interval, delay = 0, repeat = -1)
	{
		var index = this.alarmList.length;
		
		// Check for an empty place in the alarmList
		// !Uncomment this if the empty spaces in alarmList should be filled with new alarms!
		/*for (i = 0; i < this.alarmList.length; i++)
		{
			if (this.alarmList[i] == undefined)
			{
				index = i;
				break;
			}
		}*/
		
		//Add a new alarm to the list
		this.alarmList[index] = new alarm(index, interval, delay, repeat);
		return index;
	}
	
	
	// Check if a alarm has reached its interval.
	function check_timer(id)
	{
		var alarm = get_alarm(id);
		if (alarm == undefined)
			return false;
		if (!alarm.active)
			return false;
		var time = gameState.getTimeElapsed();
		var alarmState = false;
		
		// If repeat forever (repeat is -1).
		if (alarm.repeat < 0)
		{
			var time_diffrence = time - alarm.start_time - alarm.delay - alarm.inteval * alarm.counter;
			if (time_diffrence > alarm.interval)
			{
				alarmState = true;
				alarm.counter++;
			}
		}
		// If the alarm has rung less times than repeat.
		else if (alarm.counter < alarm.repeat)
		{
			var time_diffrence = time - alarm.start_time - alarm.delay - alarm.inteval * alarm.counter;
			if (time_diffrence > alarm.interval)
			{
				alarmState = true;
				alarm.counter++;
			}
		}
		
		// Check if the alarm has rung 'alarm.repeat' times ifso, delete the alarm.
		if (alarm.counter == alarm.repeat)
		{
			this.clear_timer(id);
		}
		
		return alarmState;
	}
	
	
	// Remove an alarm from the list.
	function clear_timer(id)
	{
		delete this.alarmList[id];
	}
	
	
	// Activate a deactivated alarm.
	function activate_timer(id)
	{
		var alarm = get_alarm(id);
		alarm.active = true;
	}
	
	
	// Deactivate an active alarm but don't delete it.
	function deactivate_timer(id)
	{
		var alarm = get_alarm(id);
		alarm.active = false;
	}



//-EmjeR-// Alarm class //
function alarm(id, interval, delay, repeat)
{
	this.id = id;
	this.interval = interval;
	this.delay = delay;
	this.repeat = repeat;
	
	this.start_time = gameState.getTimeElapsed();
	this.active = true;
	this.counter = 0;
}