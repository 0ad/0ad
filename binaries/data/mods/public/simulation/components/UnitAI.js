function UnitAI() {}

UnitAI.prototype.Schema =
	"<a:help>Controls the unit's movement, attacks, etc, in response to commands from the player.</a:help>" +
	"<a:example/>" +
	"<empty/>";

var UnitFsmSpec = {

	"INDIVIDUAL": {

		"MoveStopped": function() {
			// ignore spurious movement messages
			// (these can happen when stopping moving at the same time
			// as switching states)
		},

		"ConstructionFinished": function(msg) {
			// ignore uninteresting construction messages
		},

		"LosRangeUpdate": function(msg) {
			// ignore newly-seen units by default
		},

		"Attacked": function(msg) {
			// Default behaviour: attack back at our attacker
			if (this.CanAttack(msg.data.attacker))
			{
				this.PushOrderFront("Attack", { "target": msg.data.attacker });
			}
		},


		"IDLE": {
			"enter": function() {
				// If we entered the idle state we must have nothing better to do,
				// so immediately check whether there's anybody nearby to attack.
				// (If anyone approaches later, it'll be handled via LosRangeUpdate.)
				if (this.losRangeQuery)
				{
					var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
					var ents = rangeMan.ResetActiveQuery(this.losRangeQuery);
					if (this.AttackVisibleEntity(ents))
						return true;
				}

				// Nobody to attack - switch to idle
				this.SelectAnimation("idle");
				return false;
			},

			"leave": function() {
				var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
				rangeMan.DisableActiveQuery(this.losRangeQuery);
			},

			"LosRangeUpdate": function(msg) {
				// TODO: implement stances (ignore this message if hold-fire stance)

				// Start attacking one of the newly-seen enemy (if any)
				this.AttackVisibleEntity(msg.data.added);
			},
		},

		"Order.Walk": function(msg) {
			var ok;
			if (this.order.data.target)
				ok = this.MoveToTarget(this.order.data.target);
			else
				ok = this.MoveToPoint(this.order.data.x, this.order.data.z);

			if (ok)
			{
				// We've started walking to the given point
				this.SetNextState("WALKING");
			}
			else
			{
				// We are already at the target, or can't move at all
				this.FinishOrder();
			}
		},

		"WALKING": {
			"enter": function() {
				this.SelectAnimation("walk", false, this.GetWalkSpeed());
				this.PlaySound("walk");
			},

			"MoveStopped": function() {
				this.FinishOrder();
			},
		},


		"Order.Attack": function(msg) {
			// Work out how to attack the given target
			var type = this.GetBestAttack();
			if (!type)
			{
				// Oops, we can't attack at all
				this.FinishOrder();
				return;
			}
			this.attackType = type;

			// Try to move within attack range
			if (this.MoveToTargetRange(this.order.data.target, IID_Attack, this.attackType))
			{
				// We've started walking to the given point
				this.SetNextState("COMBAT.APPROACHING");
			}
			else
			{
				// We are already at the target, or can't move at all,
				// so try attacking it from here.
				// TODO: need better handling of the can't-reach-target case
				this.SetNextState("COMBAT.ATTACKING");
			}
		},

		"COMBAT": {

			"Attacked": function(msg) {
				// If we're already in combat mode, ignore anyone else
				// who's attacking us
			},

			"APPROACHING": {
				"enter": function() {
					this.SelectAnimation("walk", false, this.GetWalkSpeed());
					this.PlaySound("walk");
				},

				"MoveStopped": function() {
					this.SetNextState("ATTACKING");
				},
			},

			"ATTACKING": {
				"enter": function() {
					var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
					this.attackTimers = cmpAttack.GetTimers(this.attackType);

					this.SelectAnimation("melee", false, 1.0, "attack");
					this.SetAnimationSync(this.attackTimers.prepare, this.attackTimers.repeat);
					this.StartTimer(this.attackTimers.prepare, this.attackTimers.repeat);
					// TODO: we should probably only bother syncing projectile attacks, not melee

					// TODO: if .prepare is short, players can cheat by cycling attack/stop/attack
					// to beat the .repeat time; should enforce a minimum time
				},

				"leave": function() {
					this.StopTimer();
				},

				"Timer": function(msg) {
					// Check we can still reach the target
					if (this.CheckTargetRange(this.order.data.target, IID_Attack, this.attackType))
					{
						var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
						cmpAttack.PerformAttack(this.attackType, this.order.data.target);
					}
					else
					{
						// Try to chase after it
						if (this.MoveToTargetRange(this.order.data.target, IID_Attack, this.attackType))
						{
							this.SetNextState("COMBAT.CHASING");
						}
						else
						{
							// Can't reach it, or it doesn't exist any more - give up
							this.FinishOrder();
							
							// TODO: see if we can switch to a new nearby enemy
						}
					}
				},

				// TODO: respond to target deaths immediately, rather than waiting
				// until the next Timer event
			},

			"CHASING": {
				"enter": function() {
					this.SelectAnimation("walk", false, this.GetWalkSpeed());
					this.PlaySound("walk");
				},
			
				"MoveStopped": function() {
					this.SetNextState("ATTACKING");
				},
			},
		},


		"Order.Gather": function(msg) {
			// Try to move within range
			if (this.MoveToTargetRange(this.order.data.target, IID_ResourceGatherer))
			{
				// We've started walking to the given point
				this.SetNextState("GATHER.APPROACHING");
			}
			else
			{
				// We are already at the target, or can't move at all,
				// so try gathering it from here.
				// TODO: need better handling of the can't-reach-target case
				this.SetNextState("GATHER.GATHERING");
			}
		},

		"GATHER": {
			"APPROACHING": {
				"enter": function() {
					this.SelectAnimation("walk", false, this.GetWalkSpeed());
					this.PlaySound("walk");
				},
			
				"MoveStopped": function() {
					this.SetNextState("GATHERING");
				},
			},

			"GATHERING": {
				"enter": function() {
					var typename = "gather_" + this.order.data.type.specific;
					this.SelectAnimation(typename, false, 1.0, typename);
					this.StartTimer(1000, 1000);
				},

				"leave": function() {
					this.StopTimer();
				},

				"Timer": function(msg) {
					// Check we can still reach the target
					if (this.CheckTargetRange(this.order.data.target, IID_ResourceGatherer))
					{
						var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
						var status = cmpResourceGatherer.PerformGather(this.order.data.target);
					}
					else
					{
						// Try to follow it
						if (this.MoveToTargetRange(this.order.data.target, IID_ResourceGatherer))
						{
							this.SetNextState("APPROACHING");
						}
						else
						{
							// Save the current order's type in case we need it later
							var oldType = this.order.data.type;

							// Can't reach it, or it doesn't exist any more - give up on this order
							if (this.FinishOrder())
								return;

							// No remaining orders - pick a useful default behaviour

							// Try to find a nearby target of the same type

							var range = 32; // TODO: what's a sensible number?
							var players = [0]; // owned by Gaia (TODO: is this what we want?)
							var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
							var nearby = rangeMan.ExecuteQuery(this.entity, range, players, IID_ResourceSupply);
							for each (var ent in nearby)
							{
								var cmpResourceSupply = Engine.QueryInterface(ent, IID_ResourceSupply);
								var type = cmpResourceSupply.GetType();
								if (type.specific == oldType.specific)
								{
									this.Gather(ent, true);
									return;
								}
							}

							// Nothing else to gather - just give up
						}
					}
				},
			},
		},


		"Order.Repair": function(msg) {
			// Try to move within range
			if (this.MoveToTargetRange(this.order.data.target, IID_Builder))
			{
				// We've started walking to the given point
				this.SetNextState("REPAIR.APPROACHING");
			}
			else
			{
				// We are already at the target, or can't move at all,
				// so try repairing it from here.
				// TODO: need better handling of the can't-reach-target case
				this.SetNextState("REPAIR.REPAIRING");
			}
		},

		"REPAIR": {
			"APPROACHING": {
				"enter": function() {
					this.SelectAnimation("walk", false, this.GetWalkSpeed());
					this.PlaySound("walk");
				},
			
				"MoveStopped": function() {
					this.SetNextState("REPAIRING");
				},
			},

			"REPAIRING": {
				"enter": function() {
					this.SelectAnimation("build", false, 1.0, "build");
					this.StartTimer(1000, 1000);
				},

				"leave": function() {
					this.StopTimer();
				},

				"Timer": function(msg) {
					var target = this.order.data.target;
					// Check we can still reach the target
					if (!this.CheckTargetRange(target, IID_Builder))
					{
						// Can't reach it, or it doesn't exist any more
						this.FinishOrder();
						return;
					}
					
					var cmpBuilder = Engine.QueryInterface(this.entity, IID_Builder);
					var status = cmpBuilder.PerformBuilding(target);
					if (!status.finished)
						return; // continue repairing it
				},
			},

			"ConstructionFinished": function(msg) {
				if (msg.data.entity != this.order.data.target)
					return; // ignore other buildings

				// We finished building it.
				// Switch to the next order (if any)
				if (this.FinishOrder())
					return;

				// No remaining orders - pick a useful default behaviour

				// If this building was e.g. a farm, we should start gathering from it
				// if we are capable of doing so
				if (this.CanGather(msg.data.newentity))
				{
					this.Gather(msg.data.newentity, true);
				}
				else
				{
					// TODO: look for a nearby foundation to help with
				}
			},
		},
	},
};

var UnitFsm = new FSM(UnitFsmSpec);

UnitAI.prototype.Init = function()
{
	this.orderQueue = []; // current order is at the front of the list
	this.order = undefined; // always == this.orderQueue[0]
};

UnitAI.prototype.OnCreate = function()
{
	UnitFsm.Init(this, "INDIVIDUAL.IDLE");
};

UnitAI.prototype.OnOwnershipChanged = function(msg)
{
	this.SetupRangeQuery(msg.to);
};

UnitAI.prototype.OnDestroy = function()
{
	// Clean up any timers that are now obsolete
	this.StopTimer();

	// Clean up range queries
	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (this.losRangeQuery)
		rangeMan.DestroyActiveQuery(this.losRangeQuery);
};

// Set up a range query for all enemy units within LOS range
// which can be attacked.
// This should be called whenever our ownership changes.
UnitAI.prototype.SetupRangeQuery = function(owner)
{
	var cmpVision = Engine.QueryInterface(this.entity, IID_Vision);
	if (!cmpVision)
		return;

	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var playerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);

	if (this.losRangeQuery)
		rangeMan.DestroyActiveQuery(this.losRangeQuery);

	var range = cmpVision.GetRange();

	// Find all enemy players (i.e. exclude Gaia and ourselves)
	var players = [];
	for (var i = 1; i < playerMan.GetNumPlayers(); ++i)
		if (i != owner)
			players.push(i);

	this.losRangeQuery = rangeMan.CreateActiveQuery(this.entity, range, players, IID_DamageReceiver);
	rangeMan.EnableActiveQuery(this.losRangeQuery);
};

//// FSM linkage functions ////

UnitAI.prototype.SetNextState = function(state)
{
	UnitFsm.SetNextState(this, state);
};

UnitAI.prototype.DeferMessage = function(msg)
{
	UnitFsm.DeferMessage(this, msg);
};

/**
 * Call when the current order has been completed (or failed).
 * Removes the current order from the queue, and processes the
 * next one (if any). Returns false and defaults to IDLE
 * if there are no remaining orders.
 */
UnitAI.prototype.FinishOrder = function()
{
	if (!this.orderQueue.length)
		error("FinishOrder called when order queue is empty");

	this.orderQueue.shift();
	this.order = this.orderQueue[0];

	if (this.orderQueue.length)
	{
		UnitFsm.ProcessMessage(this, {"type": "Order."+this.order.type, "data": this.order.data});
		return true;
	}
	else
	{
		this.SetNextState("IDLE");
		return false;
	}
};

/**
 * Add an order onto the back of the queue,
 * and execute it if we didn't already have an order.
 */
UnitAI.prototype.PushOrder = function(type, data)
{
	var order = { "type": type, "data": data };
	this.orderQueue.push(order);

	// If we didn't already have an order, then process this new one
	if (this.orderQueue.length == 1)
	{
		this.order = order;
		UnitFsm.ProcessMessage(this, {"type": "Order."+this.order.type, "data": this.order.data});
	}
};

/**
 * Add an order onto the front of the queue,
 * and execute it immediately.
 */
UnitAI.prototype.PushOrderFront = function(type, data)
{
	var order = { "type": type, "data": data };
	this.orderQueue.unshift(order);

	this.order = order;
	UnitFsm.ProcessMessage(this, {"type": "Order."+this.order.type, "data": this.order.data});
};

UnitAI.prototype.ReplaceOrder = function(type, data)
{
	this.orderQueue = [];
	this.PushOrder(type, data);
};

UnitAI.prototype.TimerHandler = function(data, lateness)
{
	// Reset the timer
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "TimerHandler", data.timerRepeat - lateness, data);

	UnitFsm.ProcessMessage(this, {"type": "Timer", "data": data, "lateness": lateness});
};

UnitAI.prototype.StartTimer = function(offset, repeat)
{
	if (this.timer)
		error("Called StartTimer when there's already an active timer");

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "TimerHandler", offset, { "timerRepeat": repeat });
};

UnitAI.prototype.StopTimer = function()
{
	if (!this.timer)
		return;

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.timer);
	this.timer = undefined;
};

//// Message handlers /////

UnitAI.prototype.OnMotionChanged = function(msg)
{
	if (!msg.speed)
		UnitFsm.ProcessMessage(this, {"type": "MoveStopped"});
};

UnitAI.prototype.OnGlobalConstructionFinished = function(msg)
{
	// TODO: This is a bit inefficient since every unit listens to every
	// construction message - ideally we could scope it to only the one we're building

	UnitFsm.ProcessMessage(this, {"type": "ConstructionFinished", "data": msg});
};

UnitAI.prototype.OnAttacked = function(msg)
{
	UnitFsm.ProcessMessage(this, {"type": "Attacked", "data": msg});
};

UnitAI.prototype.OnRangeUpdate = function(msg)
{
	if (msg.tag == this.losRangeQuery)
		UnitFsm.ProcessMessage(this, {"type": "LosRangeUpdate", "data": msg});
};

//// Helper functions to be called by the FSM ////

UnitAI.prototype.GetWalkSpeed = function()
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpMotion.GetWalkSpeed();
};

UnitAI.prototype.GetRunSpeed = function()
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpMotion.GetRunSpeed();
};

UnitAI.prototype.PlaySound = function(name)
{
	PlaySound(name, this.entity);
};

UnitAI.prototype.SelectAnimation = function(name, once, speed, sound)
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

UnitAI.prototype.SetAnimationSync = function(actiontime, repeattime)
{
	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpVisual)
		return;

	cmpVisual.SetAnimationSyncRepeat(repeattime);
	cmpVisual.SetAnimationSyncOffset(actiontime);
};

UnitAI.prototype.MoveToPoint = function(x, z)
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpMotion.MoveToPoint(x, z);
};

UnitAI.prototype.MoveToTarget = function(target)
{
	var cmpPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpPosition)
		return false;

	if (!cmpPosition.IsInWorld())
		return false;

	var pos = cmpPosition.GetPosition();
	return this.MoveToPoint(pos.x, pos.z);
};

UnitAI.prototype.MoveToTargetRange = function(target, iid, type)
{
	var cmpRanged = Engine.QueryInterface(this.entity, iid);
	var range = cmpRanged.GetRange(type);

	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpMotion.MoveToAttackRange(target, range.min, range.max);
};

UnitAI.prototype.CheckTargetRange = function(target, iid, type)
{
	var cmpRanged = Engine.QueryInterface(this.entity, iid);
	var range = cmpRanged.GetRange(type);

	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpMotion.IsInAttackRange(target, range.min, range.max);
};

UnitAI.prototype.GetBestAttack = function()
{
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return undefined;
	return cmpAttack.GetBestAttack();
};

/**
 * Try to find one of the given entities which can be attacked,
 * and start attacking it.
 * Returns true if it found something to attack.
 */
UnitAI.prototype.AttackVisibleEntity = function(ents)
{
	for each (var target in ents)
	{
		if (this.CanAttack(target))
		{
			this.PushOrderFront("Attack", { "target": target });
			return true;
		}
	}
	return false;
};

//// External interface functions ////

UnitAI.prototype.AddOrder = function(type, data, queued)
{
	if (queued)
		this.PushOrder(type, data);
	else
		this.ReplaceOrder(type, data);
};

UnitAI.prototype.Walk = function(x, z, queued)
{
	this.AddOrder("Walk", { "x": x, "z": z }, queued);
};

UnitAI.prototype.WalkToTarget = function(target, queued)
{
	this.AddOrder("Walk", { "target": target }, queued);
};

UnitAI.prototype.Attack = function(target, queued)
{
	if (!this.CanAttack(target))
	{
		this.WalkToTarget(target, queued);
		return;
	}

	this.AddOrder("Attack", { "target": target }, queued);
};

UnitAI.prototype.Gather = function(target, queued)
{
	if (!this.CanGather(target))
	{
		this.WalkToTarget(target, queued);
		return;
	}

	// Save the resource type now, so if the resource gets destroyed
	// before we process the order then we still know what resource
	// type to look for more of
	var cmpResourceSupply = Engine.QueryInterface(target, IID_ResourceSupply);
	var type = cmpResourceSupply.GetType();

	this.AddOrder("Gather", { "target": target, "type": type }, queued);
};

UnitAI.prototype.Repair = function(target, queued)
{
	// Verify that we're able to respond to Repair commands
	var cmpBuilder = Engine.QueryInterface(this.entity, IID_Builder);
	if (!cmpBuilder)
	{
		this.WalkToTarget(target, queued);
		return;
	}

	// TODO: verify that this is a valid target

	this.AddOrder("Repair", { "target": target }, queued);
};

//// Helper functions ////

UnitAI.prototype.CanAttack = function(target)
{
	// Verify that we're able to respond to Attack commands
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return false;

	// TODO: verify that this is a valid target

	return true;
};

UnitAI.prototype.CanGather = function(target)
{
	// Verify that we're able to respond to Gather commands
	var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
	if (!cmpResourceGatherer)
		return false;

	// Verify that we can gather from this target
	if (!cmpResourceGatherer.GetTargetGatherRate(target))
		return false;

	// TODO: should verify it's owned by the correct player, etc

	return true;
};


Engine.RegisterComponentType(IID_UnitAI, "UnitAI", UnitAI);
