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
	"</element>";

var AnimalFsmSpec = {

	"SKITTISH": {

		"ResourceGather": function(msg) {
			// If someone's carving chunks of meat off us, then run away
			this.MoveAwayFrom(msg.gatherer, 12);
			this.SetNextState("FLEEING");
			this.PlaySound("panic");
		},

		"ROAMING": {
			"enter": function() {
				// Walk in a random direction
				this.SelectAnimation("walk", false);
				this.MoveRandomly();
				// Set a random timer to switch to feeding state
				this.StartTimer(RandomInt(2000, 8000));
			},

			"leave": function() {
				this.StopTimer();
			},

			"Timer": function(msg) {
				this.SetNextState("FEEDING");
			},

			"MoveStopped": function() {
				this.MoveRandomly();
			},
		},

		"FEEDING": {
			"enter": function() {
				// Stop and eat for a while
				this.SelectAnimation("idle");
				this.StopMoving();
				this.StartTimer(RandomInt(1000, 4000));
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
				this.SelectAnimation("run", false);
				this.SetMoveSpeedFactor(6.0);
			},

			"leave": function() {
				// Reset normal speed
				this.SetMoveSpeedFactor(1.0);
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
	AnimalFsm.Init(this, "SKITTISH.ROAMING");
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
	this.messageQueue = [];
	for each (var msg in mq)
		AnimalFsm.ProcessMessage(this, msg);
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

AnimalAI.prototype.MoveRandomly = function()
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

	var distance = 4;
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

AnimalAI.prototype.SetMoveSpeedFactor = function(factor)
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	cmpMotion.SetSpeedFactor(factor);
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
