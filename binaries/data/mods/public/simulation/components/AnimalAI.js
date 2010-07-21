function AnimalAI() {}

AnimalAI.prototype.Schema =
	"<a:example/>" +
	"<element name='NaturalBehaviour' a:help='Behaviour of the unit in the absence of player commands (intended for animals)'>" +
		"<choice>" +
			"<value a:help='Will actively attack any unit it encounters, even if not threatened'>violent</value>" +
			"<value a:help='Will attack nearby units if it feels threatened (if they linger within LOS for too long)'>aggressive</value>" +
			"<value a:help='Will attack nearby units if attacked'>defensive</value>" +
			"<value a:help='Will never attack units'>passive</value>" +
			"<value a:help='Will never attack units. Will typically attempt to flee for short distances when units approach'>skittish</value>" +
		"</choice>" +
	"</element>" +
	"<element name='RoamDistance'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='FleeDistance'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='RoamTimeMin'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='RoamTimeMax'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='FeedTimeMin'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='FeedTimeMax'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>";

var AnimalFsmSpec = {

	"SKITTISH": {

		"ResourceGather": function(msg) {
			// If someone's carving chunks of meat off us, then run away
			this.MoveAwayFrom(msg.gatherer, +this.template.FleeDistance);
			this.SetNextState("FLEEING");
			this.PlaySound("panic");
		},

		"ROAMING": {
			"enter": function() {
				// Walk in a random direction
				this.SelectAnimation("walk", false, this.GetWalkSpeed());
				this.MoveRandomly(+this.template.RoamDistance);
				// Set a random timer to switch to feeding state
				this.StartTimer(RandomInt(+this.template.RoamTimeMin, +this.template.RoamTimeMax));
			},

			"leave": function() {
				this.StopTimer();
			},

			"Timer": function(msg) {
				this.SetNextState("FEEDING");
			},

			"MoveStopped": function() {
				this.MoveRandomly(+this.template.RoamDistance);
			},
		},

		"FEEDING": {
			"enter": function() {
				// Stop and eat for a while
				this.SelectAnimation("feeding");
				this.StopMoving();
				this.StartTimer(RandomInt(+this.template.FeedTimeMin, +this.template.FeedTimeMax));
			},
			
			"leave": function() {
				this.StopTimer();
			},

			"MoveStopped": function() { },

			"Timer": function(msg) {
				this.SetNextState("ROAMING");
			},
		},

		"FLEEING": {
			"enter": function() {
				// Run quickly
				var speed = this.GetRunSpeed();
				this.SelectAnimation("run", false, speed);
				this.SetMoveSpeed(speed);
			},

			"leave": function() {
				// Reset normal speed
				this.SetMoveSpeed(this.GetWalkSpeed());
			},

			"MoveStopped": function() {
				// When we've run far enough, go back to the roaming state
				this.SetNextState("ROAMING");
			},
		},
	},
};

var AnimalFsm = new FSM(AnimalFsmSpec);

AnimalAI.prototype.Init = function()
{
	this.messageQueue = [];
};

// FSM linkage functions:

AnimalAI.prototype.OnCreate = function()
{
	AnimalFsm.Init(this, "SKITTISH.FEEDING");
};

AnimalAI.prototype.SetNextState = function(state)
{
	AnimalFsm.SetNextState(this, state);
};

AnimalAI.prototype.DeferMessage = function(msg)
{
	AnimalFsm.DeferMessage(this, msg);
};

AnimalAI.prototype.PushMessage = function(msg)
{
	this.messageQueue.push(msg);
};

AnimalAI.prototype.OnUpdate = function()
{
	var mq = this.messageQueue;
	if (mq.length)
	{
		this.messageQueue = [];
		for each (var msg in mq)
			AnimalFsm.ProcessMessage(this, msg);
	}
};

AnimalAI.prototype.OnMotionChanged = function(msg)
{
	if (!msg.speed)
		this.PushMessage({"type": "MoveStopped"});
};

AnimalAI.prototype.OnResourceGather = function(msg)
{
	this.PushMessage({"type": "ResourceGather", "gatherer": msg.gatherer});
};

AnimalAI.prototype.TimerHandler = function(data, lateness)
{
	this.PushMessage({"type": "Timer", "data": data, "lateness": lateness});
};

// Functions to be called by the FSM:

AnimalAI.prototype.GetWalkSpeed = function()
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpMotion.GetWalkSpeed();
};

AnimalAI.prototype.GetRunSpeed = function()
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpMotion.GetRunSpeed();
};

AnimalAI.prototype.PlaySound = function(name)
{
	PlaySound(name, this.entity);
};

AnimalAI.prototype.SelectAnimation = function(name, once, speed, sound)
{
	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpVisual)
		return;

	var soundgroup;
	if (sound)
	{
		var cmpSound = Engine.QueryInterface(this.entity, IID_Sound);
		if (cmpSound)
			soundgroup = cmpSound.GetSoundGroup(sound);
	}

	// Set default values if unspecified
	if (typeof once == "undefined")
		once = false;
	if (typeof speed == "undefined")
		speed = 1.0;
	if (typeof soundgroup == "undefined")
		soundgroup = "";

	cmpVisual.SelectAnimation(name, once, speed, soundgroup);
};

AnimalAI.prototype.MoveRandomly = function(distance)
{
	// We want to walk in a random direction, but avoid getting stuck
	// in obstacles or narrow spaces.
	// So pick a circular range from approximately our current position,
	// and move outwards to the nearest point on that circle, which will
	// lead to us avoiding obstacles and moving towards free space.

	// TODO: we probably ought to have a 'home' point, and drift towards
	// that, so we don't spread out all across the whole map

	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition)
		return;

	if (!cmpPosition.IsInWorld())
		return;

	var pos = cmpPosition.GetPosition();

	var jitter = 0.5;

	// Randomly adjust the range's center a bit, so we tend to prefer
	// moving in random directions (if there's nothing in the way)
	var tx = pos.x + (2*Math.random()-1)*jitter;
	var tz = pos.z + (2*Math.random()-1)*jitter;

	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	cmpMotion.MoveToPointRange(tx, tz, distance, distance);
};

AnimalAI.prototype.MoveAwayFrom = function(ent, distance)
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	cmpMotion.MoveToAttackRange(ent, distance, distance);
};

AnimalAI.prototype.StopMoving = function()
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	cmpMotion.StopMoving();
};

AnimalAI.prototype.SetMoveSpeed = function(speed)
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	cmpMotion.SetSpeed(speed);
};

AnimalAI.prototype.StartTimer = function(interval, data)
{
	if (this.timer)
		error("Called StartTimer when there's already an active timer");

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetTimeout(this.entity, IID_AnimalAI, "TimerHandler", interval, data);
};

AnimalAI.prototype.StopTimer = function()
{
	if (!this.timer)
		return;

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.timer);
	this.timer = undefined;
};

Engine.RegisterComponentType(IID_AnimalAI, "AnimalAI", AnimalAI);
