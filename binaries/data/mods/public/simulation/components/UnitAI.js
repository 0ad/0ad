function UnitAI() {}

UnitAI.prototype.Schema =
	"<a:help>Controls the unit's movement, attacks, etc, in response to commands from the player.</a:help>" +
	"<a:example/>" +
	"<element name='DefaultStance'>" +
		"<choice>" +
			"<value>violent</value>" +
			"<value>aggressive</value>" +
			"<value>defensive</value>" +
			"<value>passive</value>" +
			"<value>standground</value>" +
		"</choice>" +
	"</element>" +
	"<element name='FormationController'>" +
		"<data type='boolean'/>" +
	"</element>" +
	"<element name='FleeDistance'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<optional>" +
		"<interleave>" +
			"<element name='NaturalBehaviour' a:help='Behaviour of the unit in the absence of player commands (intended for animals)'>" +
				"<choice>" +
					"<value a:help='Will actively attack any unit it encounters, even if not threatened'>violent</value>" +
					"<value a:help='Will attack nearby units if it feels threatened (if they linger within LOS for too long)'>aggressive</value>" +
					"<value a:help='Will attack nearby units if attacked'>defensive</value>" +
					"<value a:help='Will never attack units but will attempt to flee when attacked'>passive</value>" +
					"<value a:help='Will never attack units. Will typically attempt to flee for short distances when units approach'>skittish</value>" +
					"<value a:help='Will never attack units and will not attempt to flee when attacked'>domestic</value>" +
				"</choice>" +
			"</element>" +
			"<element name='RoamDistance'>" +
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
			"</element>"+
		"</interleave>" +
	"</optional>";

// Unit stances.
// There some targeting options:
//   targetVisibleEnemies: anything in vision range is a viable target
//   targetAttackers: anything that hurts us is a viable target
// There are some response options, triggered when targets are detected:
//   respondFlee: run away
//   respondChase: start chasing after the enemy
//   respondChaseBeyondVision: start chasing, and don't stop even if it's out
//     of this unit's vision range (though still visible to the player)
//   respondStandGround: attack enemy but don't move at all
//   respondHoldGround: attack enemy but don't move far from current position
// TODO: maybe add targetAggressiveEnemies (don't worry about lone scouts,
// do worry around armies slaughtering the guy standing next to you), etc.
var g_Stances = {
	"violent": {
		targetVisibleEnemies: true,
		targetAttackers: true,
		respondFlee: false,
		respondChase: true,
		respondChaseBeyondVision: true,
		respondStandGround: false,
		respondHoldGround: false,
	},
	"aggressive": {
		targetVisibleEnemies: true,
		targetAttackers: true,
		respondFlee: false,
		respondChase: true,
		respondChaseBeyondVision: false,
		respondStandGround: false,
		respondHoldGround: false,
	},
	"defensive": {
		targetVisibleEnemies: true,
		targetAttackers: true,
		respondFlee: false,
		respondChase: false,
		respondChaseBeyondVision: false,
		respondStandGround: false,
		respondHoldGround: true,
	},
	"passive": {
		targetVisibleEnemies: false,
		targetAttackers: true,
		respondFlee: true,
		respondChase: false,
		respondChaseBeyondVision: false,
		respondStandGround: false,
		respondHoldGround: false,
	},
	"standground": {
		targetVisibleEnemies: true,
		targetAttackers: true,
		respondFlee: false,
		respondChase: false,
		respondChaseBeyondVision: false,
		respondStandGround: true,
		respondHoldGround: false,
	},
};

// See ../helpers/FSM.js for some documentation of this FSM specification syntax
var UnitFsmSpec = {

	// Default event handlers:

	"MoveCompleted": function() {
		// ignore spurious movement messages
		// (these can happen when stopping moving at the same time
		// as switching states)
	},

	"MoveStarted": function() {
		// ignore spurious movement messages
	},

	"ConstructionFinished": function(msg) {
		// ignore uninteresting construction messages
	},

	"LosRangeUpdate": function(msg) {
		// ignore newly-seen units by default
	},
	
	"LosHealRangeUpdate": function(msg) {
		// ignore newly-seen injured units by default
	},

	"Attacked": function(msg) {
		// ignore attacker
	},

	"HealthChanged": function(msg) {
		// ignore
	},

	"EntityRenamed": function(msg) {
		// ignore
	},

	// Formation handlers:

	"FormationLeave": function(msg) {
		// ignore when we're not in FORMATIONMEMBER
	},

	// Called when being told to walk as part of a formation
	"Order.FormationWalk": function(msg) {
		// Let players move captured domestic animals around
		if (this.IsAnimal() && !this.IsDomestic())
		{
			this.FinishOrder();
			return;
		}

		var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
		cmpUnitMotion.MoveToFormationOffset(msg.data.target, msg.data.x, msg.data.z);

		this.SetNextState("FORMATIONMEMBER.WALKING");
	},

	// Special orders:
	// (these will be overridden by various states)

	"Order.LeaveFoundation": function(msg) {
		// Default behaviour is to ignore the order since we're busy
		return { "discardOrder": true };
	},

	// Individual orders:
	// (these will switch the unit out of formation mode)

	"Order.Walk": function(msg) {
		// Let players move captured domestic animals around
		if (this.IsAnimal() && !this.IsDomestic())
		{
			this.FinishOrder();
			return;
		}

		this.SetHeldPosition(this.order.data.x, this.order.data.z);
		this.MoveToPoint(this.order.data.x, this.order.data.z);
		if (this.IsAnimal())
			this.SetNextState("ANIMAL.WALKING");
		else
			this.SetNextState("INDIVIDUAL.WALKING");
	},

	"Order.WalkToTarget": function(msg) {
		// Let players move captured domestic animals around
		if (this.IsAnimal() && !this.IsDomestic())
		{
			this.FinishOrder();
			return;
		}

		var ok = this.MoveToTarget(this.order.data.target);
		if (ok)
		{
			// We've started walking to the given point
			if (this.IsAnimal())
				this.SetNextState("ANIMAL.WALKING");
			else
				this.SetNextState("INDIVIDUAL.WALKING");
		}
		else
		{
			// We are already at the target, or can't move at all
			this.StopMoving();
			this.FinishOrder();
		}
	},

	"Order.Flee": function(msg) {
		// TODO: if we were attacked by a ranged unit, we need to flee much further away
		var ok = this.MoveToTargetRangeExplicit(this.order.data.target, +this.template.FleeDistance, -1);
		if (ok)
		{
			// We've started fleeing from the given target
			if (this.IsAnimal())
				this.SetNextState("ANIMAL.FLEEING");
			else
				this.SetNextState("INDIVIDUAL.FLEEING");
		}
		else
		{
			// We are already at the target, or can't move at all
			this.StopMoving();
			this.FinishOrder();
		}
	},

	"Order.Attack": function(msg) {
		// Check the target is alive
		if (!this.TargetIsAlive(this.order.data.target))
		{
			this.FinishOrder();
			return;
		}

		// Work out how to attack the given target
		var type = this.GetBestAttack();
		if (!type)
		{
			// Oops, we can't attack at all
			this.FinishOrder();
			return;
		}
		this.attackType = type;

		// If we are already at the target, try attacking it from here
		if (this.CheckTargetRange(this.order.data.target, IID_Attack, this.attackType))
		{
			this.StopMoving();
			if (this.IsAnimal())
				this.SetNextState("ANIMAL.COMBAT.ATTACKING");
			else
				this.SetNextState("INDIVIDUAL.COMBAT.ATTACKING");
			return;
		}

		// If we can't reach the target, but are standing ground,
		// then abandon this attack order
		if (this.GetStance().respondStandGround && !this.order.data.force)
		{
			this.FinishOrder();
			return;
		}

		// Try to move within attack range
		if (this.MoveToTargetRange(this.order.data.target, IID_Attack, this.attackType))
		{
			// We've started walking to the given point
			if (this.IsAnimal())
				this.SetNextState("ANIMAL.COMBAT.APPROACHING");
			else
				this.SetNextState("INDIVIDUAL.COMBAT.APPROACHING");
			return;
		}

		// We can't reach the target, and can't move towards it,
		// so abandon this attack order
		this.FinishOrder();
	},

	"Order.Heal": function(msg) {
		// Check the target is alive
		if (!this.TargetIsAlive(this.order.data.target))
		{
			this.FinishOrder();
			return;
		}

		// Check if the target is in range
		if (this.CheckTargetRange(this.order.data.target, IID_Heal))
		{
			this.StopMoving();
			this.SetNextState("INDIVIDUAL.HEAL.HEALING");
			return;
		}

		// If we can't reach the target, but are standing ground,
		// then abandon this heal order
		if (this.GetStance().respondStandGround && !this.order.data.force)
		{
			this.FinishOrder();
			return;
		}

		// Try to move within heal range
		if (this.MoveToTargetRange(this.order.data.target, IID_Heal))
		{
			// We've started walking to the given point
			this.SetNextState("INDIVIDUAL.HEAL.APPROACHING");
			return;
		}

		// We can't reach the target, and can't move towards it,
		// so abandon this heal order
		this.FinishOrder();
	},

	"Order.Gather": function(msg) {
		
		// If the target is still alive, we need to kill it first
		if (this.MustKillGatherTarget(this.order.data.target) && this.CheckTargetVisible(this.order.data.target))
		{
			// Make sure we can attack the target, else we'll get very stuck
			if (!this.GetBestAttack())
			{
				// Oops, we can't attack at all - give up
				// TODO: should do something so the player knows why this failed
				this.FinishOrder();
				return;
			}

			this.PushOrderFront("Attack", { "target": this.order.data.target, "force": true });
			return;
		}

		// Try to move within range
		if (this.MoveToTargetRange(this.order.data.target, IID_ResourceGatherer))
		{
			// We've started walking to the given point
			this.SetNextState("INDIVIDUAL.GATHER.APPROACHING");
		}
		else
		{
			// We are already at the target, or can't move at all,
			// so try gathering it from here.
			// TODO: need better handling of the can't-reach-target case
			this.StopMoving();
			this.SetNextState("INDIVIDUAL.GATHER.GATHERING");
		}
	},
	
	"Order.GatherNearPosition": function(msg) {
		// Move the unit to the position to gather from.
		this.MoveToPoint(this.order.data.x, this.order.data.z);
		this.SetNextState("INDIVIDUAL.GATHER.WALKING");
	},

	"Order.ReturnResource": function(msg) {
		// Try to move to the dropsite
		if (this.MoveToTarget(this.order.data.target))
		{
			// We've started walking to the target
			this.SetNextState("INDIVIDUAL.RETURNRESOURCE.APPROACHING");
		}
		else
		{
			// Oops, we can't reach the dropsite.
			// Maybe we should try to pick another dropsite, to find an
			// accessible one?
			// For now, just give up.
			this.StopMoving();
			this.FinishOrder();
			return;
		}
	},

	"Order.Trade": function(msg) {
		if (this.MoveToMarket(this.order.data.firstMarket))
		{
			// We've started walking to the first market
			this.SetNextState("INDIVIDUAL.TRADE.APPROACHINGFIRSTMARKET");
		}
	},

	"Order.Repair": function(msg) {
		// Try to move within range
		if (this.MoveToTargetRange(this.order.data.target, IID_Builder))
		{
			// We've started walking to the given point
			this.SetNextState("INDIVIDUAL.REPAIR.APPROACHING");
		}
		else
		{
			// We are already at the target, or can't move at all,
			// so try repairing it from here.
			// TODO: need better handling of the can't-reach-target case
			this.StopMoving();
			this.SetNextState("INDIVIDUAL.REPAIR.REPAIRING");
		}
	},
	
	"Order.Garrison": function(msg) {
		if (this.MoveToTarget(this.order.data.target))
		{
			this.SetNextState("INDIVIDUAL.GARRISON.APPROACHING");
		}
		else
		{
			// We do a range check before actually garrisoning
			this.StopMoving();
			this.SetNextState("INDIVIDUAL.GARRISON.GARRISONED");
		}
	},
	
	"Order.Cheering": function(msg) {
		this.SetNextState("INDIVIDUAL.CHEERING");
	},

	// States for the special entity representing a group of units moving in formation:
	"FORMATIONCONTROLLER": {

		"Order.Walk": function(msg) {
			var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
			cmpFormation.CallMemberFunction("SetHeldPosition", [msg.data.x, msg.data.z]);

			this.MoveToPoint(this.order.data.x, this.order.data.z);
			this.SetNextState("WALKING");
		},

		"Order.Attack": function(msg) {
			// TODO: we should move in formation towards the target,
			// then break up into individuals when close enough to it

			var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
			cmpFormation.CallMemberFunction("Attack", [msg.data.target, false]);

			// TODO: we should wait until the target is killed, then
			// move on to the next queued order.
			// Don't bother now, just disband the formation immediately.
			cmpFormation.Disband();
		},

		"Order.Heal": function(msg) {
			// TODO: see notes in Order.Attack
			var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
			cmpFormation.CallMemberFunction("Heal", [msg.data.target, false]);
			cmpFormation.Disband();
		},

		"Order.Repair": function(msg) {
			// TODO: see notes in Order.Attack
			var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
			cmpFormation.CallMemberFunction("Repair", [msg.data.target, msg.data.autocontinue, false]);
			cmpFormation.Disband();
		},

		"Order.Gather": function(msg) {
			// TODO: see notes in Order.Attack
			var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
			cmpFormation.CallMemberFunction("Gather", [msg.data.target, false]);
			cmpFormation.Disband();
		},
		
		"Order.GatherNearPosition": function(msg) {
			var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
			cmpFormation.CallMemberFunction("GatherNearPosition", [msg.data.x, msg.data.z, msg.data.type, false]);
			cmpFormation.Disband();
		},

		"Order.ReturnResource": function(msg) {
			// TODO: see notes in Order.Attack
			var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
			cmpFormation.CallMemberFunction("ReturnResource", [msg.data.target, false]);
			cmpFormation.Disband();
		},

		"Order.Garrison": function(msg) {
			var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
			cmpFormation.CallMemberFunction("Garrison", [msg.data.target, false]);
			cmpFormation.Disband();
		},
		
		"IDLE": {
		},

		"WALKING": {
			"MoveStarted": function(msg) {
				var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.MoveMembersIntoFormation(true);
			},

			"MoveCompleted": function(msg) {
				if (this.FinishOrder())
					return;

				var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.Disband();
			},
		},
	},


	// States for entities moving as part of a formation:
	"FORMATIONMEMBER": {

		"FormationLeave": function(msg) {
			// Stop moving as soon as the formation disbands
			this.StopMoving();

			// We're leaving the formation, so stop our FormationWalk order
			if (this.FinishOrder())
				return;

			// No orders left, we're an individual now
			if (this.IsAnimal())
				this.SetNextState("ANIMAL.IDLE");
			else
				this.SetNextState("INDIVIDUAL.IDLE");
		},

		// Override the LeaveFoundation order since we're not doing
		// anything more important (and we might be stuck in the WALKING
		// state forever and need to get out of foundations in that case)
		"Order.LeaveFoundation": function(msg) {
			if (!IsOwnedByAllyOfEntity(this.entity, msg.data.target))
			{
				this.FinishOrder();
				return;
			}
			// Move a tile outside the building
			var range = 4;
			var ok = this.MoveToTargetRangeExplicit(msg.data.target, range, range);
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

		"IDLE": {
			"enter": function() {
				this.SelectAnimation("idle");
			},
		},

		"WALKING": {
			"enter": function () {
				this.SelectAnimation("move");
			},

			// (We stay in this state even if we're already in position
			// and no longer moving, because the formation controller might
			// move and we'll automatically start chasing after it again)
		},
	},


	// States for entities not part of a formation:
	"INDIVIDUAL": {

		"enter": function() {
			// Sanity-checking
			if (this.IsAnimal())
				error("Animal got moved into INDIVIDUAL.* state");
		},

		"Attacked": function(msg) {
			if (this.GetStance().targetAttackers)
			{
				this.RespondToTargetedEntities([msg.data.attacker]);
			}
		},

		"IDLE": {
			"enter": function() {
				// Switch back to idle animation to guarantee we won't
				// get stuck with an incorrect animation
				this.SelectAnimation("idle");

				// The GUI and AI want to know when a unit is idle, but we don't
				// want to send frequent spurious messages if the unit's only
				// idle for an instant and will quickly go off and do something else.
				// So we'll set a timer here and only report the idle event if we
				// remain idle
				this.StartTimer(1000);

				// If a unit can heal and attack we first want to heal wounded units,
				// so check if we are a healer and find whether there's anybody nearby to heal.
				// (If anyone approaches later it'll be handled via LosHealRangeUpdate.)
				// If anyone in sight gets hurt that will be handled via LosHealRangeUpdate.
				if (this.IsHealer() && this.FindNewHealTargets())
					return true; // (abort the FSM transition since we may have already switched state)

				// If we entered the idle state we must have nothing better to do,
				// so immediately check whether there's anybody nearby to attack.
				// (If anyone approaches later, it'll be handled via LosRangeUpdate.)
				if (this.FindNewTargets())
					return true; // (abort the FSM transition since we may have already switched state)

				// Nobody to attack - stay in idle
				return false;
			},

			"leave": function() {
				var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
				rangeMan.DisableActiveQuery(this.losRangeQuery);
				if (this.losHealRangeQuery)
					rangeMan.DisableActiveQuery(this.losHealRangeQuery);

				this.StopTimer();

				if (this.isIdle)
				{
					this.isIdle = false;
					Engine.PostMessage(this.entity, MT_UnitIdleChanged, { "idle": this.isIdle });
				}
			},

			"LosRangeUpdate": function(msg) {
				if (this.GetStance().targetVisibleEnemies)
				{
					// Start attacking one of the newly-seen enemy (if any)
					this.RespondToTargetedEntities(msg.data.added);
				}
			},
			
			"LosHealRangeUpdate": function(msg) {
				this.RespondToHealableEntities(msg.data.added);
			},

			"Timer": function(msg) {
				if (!this.isIdle)
				{
					this.isIdle = true;
					Engine.PostMessage(this.entity, MT_UnitIdleChanged, { "idle": this.isIdle });
				}
			},

			// Override the LeaveFoundation order since we're not doing
			// anything more important
			"Order.LeaveFoundation": function(msg) {
				if (!IsOwnedByAllyOfEntity(this.entity, msg.data.target))
				{
					this.FinishOrder();
					return;
				}
				// Move a tile outside the building
				var range = 4;
				var ok = this.MoveToTargetRangeExplicit(msg.data.target, range, range);
				if (ok)
				{
					// We've started walking to the given point
					this.SetNextState("INDIVIDUAL.WALKING");
				}
				else
				{
					// We are already at the target, or can't move at all
					this.FinishOrder();
				}
			},
		},

		"WALKING": {
			"enter": function () {
				this.SelectAnimation("move");
			},

			"MoveCompleted": function() {
				this.FinishOrder();
			},
		},

		"FLEEING": {
			"enter": function() {
				this.PlaySound("panic");

				// Run quickly
				var speed = this.GetRunSpeed();
				this.SelectAnimation("move");
				this.SetMoveSpeed(speed);
			},

			"leave": function() {
				// Reset normal speed
				this.SetMoveSpeed(this.GetWalkSpeed());
			},

			"MoveCompleted": function() {
				// When we've run far enough, stop fleeing
				this.FinishOrder();
			},

			// TODO: what if we run into more enemies while fleeing?
		},

		"COMBAT": {
			"EntityRenamed": function(msg) {
				if (this.order.data.target == msg.entity)
					this.order.data.target = msg.newentity;
			},

			"Attacked": function(msg) {
				// If we're already in combat mode, ignore anyone else
				// who's attacking us
			},

			"APPROACHING": {
				"enter": function () {
					this.SelectAnimation("move");
					this.StartTimer(1000, 1000);
				},

				"leave": function() {
					this.StopTimer();
				},

				"Timer": function(msg) {
					if (this.ShouldAbandonChase(this.order.data.target, this.order.data.force, IID_Attack))
					{
						this.StopMoving();
						this.FinishOrder();

						// Return to our original position
						if (this.GetStance().respondHoldGround)
							this.WalkToHeldPosition();
					}
				},

				"MoveCompleted": function() {
					this.SetNextState("ATTACKING");
				},

				"Attacked": function(msg) {
					// If we're attacked by a close enemy, we should try to defend ourself
					if (this.GetStance().targetAttackers && msg.data.type == "Melee")
					{
						this.RespondToTargetedEntities([msg.data.attacker]);
					}
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

					this.FaceTowardsTarget(this.order.data.target);
				},

				"leave": function() {
					this.StopTimer();
				},

				"Timer": function(msg) {
					var target = this.order.data.target;
					// Check the target is still alive and attackable
					if (this.TargetIsAlive(target) && this.CanAttack(target))
					{
						// Check we can still reach the target
						if (this.CheckTargetRange(target, IID_Attack, this.attackType))
						{
							this.FaceTowardsTarget(target);
							var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
							cmpAttack.PerformAttack(this.attackType, target);
							return;
						}

						// Can't reach it - try to chase after it
						if (this.ShouldChaseTargetedEntity(target, this.order.data.force))
						{
							if (this.MoveToTargetRange(target, IID_Attack, this.attackType))
							{
								this.SetNextState("COMBAT.CHASING");
								return;
							}
						}
					}

					// Can't reach it, no longer owned by enemy, or it doesn't exist any more - give up
					if (this.FinishOrder())
						return;

					// See if we can switch to a new nearby enemy
					if (this.FindNewTargets())
						return;

					// Return to our original position
					if (this.GetStance().respondHoldGround)
						this.WalkToHeldPosition();
				},

				// TODO: respond to target deaths immediately, rather than waiting
				// until the next Timer event

				"Attacked": function(msg) {
					if (this.order.data.target != msg.data.attacker)
					{
						// If we're attacked by a close enemy stronger than our current target, we choose to attack him
						if (this.GetStance().targetAttackers && msg.data.type == "Melee")
						{
							var ents = [this.order.data.target, msg.data.attacker];
							SortEntitiesByPriority(ents);
							if (ents[0] != this.order.data.target)
							{
								this.RespondToTargetedEntities(ents);
							}
						}
					}
				},
			},

			"CHASING": {
				"enter": function () {
					this.SelectAnimation("move");
					this.StartTimer(1000, 1000);
				},

				"leave": function() {
					this.StopTimer();
				},

				"Timer": function(msg) {
					if (this.ShouldAbandonChase(this.order.data.target, this.order.data.force, IID_Attack))
					{
						this.StopMoving();
						this.FinishOrder();

						// Return to our original position
						if (this.GetStance().respondHoldGround)
							this.WalkToHeldPosition();
					}
				},

				"MoveCompleted": function() {
					this.SetNextState("ATTACKING");
				},
			},
		},

		"GATHER": {
			"APPROACHING": {
				"enter": function() {
					this.SelectAnimation("move");
				},

				"MoveCompleted": function(msg) {
					if (msg.data.error)
					{
						// We failed to reach the target

						// Save the current order's data in case we need it later
						var oldType = this.order.data.type;
						var oldTarget = this.order.data.target;
						var oldTemplate = this.order.data.template;

						// Try the next queued order if there is any
						if (this.FinishOrder())
							return;

						// Try to find another nearby target of the same specific type
						// Also don't switch to a different type of huntable animal
						var nearby = this.FindNearbyResource(function (ent, type, template) {
							return (
								ent != oldTarget
								&& type.specific == oldType.specific
								&& (type.specific != "meat" || oldTemplate == template)
							);
						});
						if (nearby)
						{
							this.Gather(nearby, true);
							return;
						}

						// Couldn't find anything else. Just try this one again,
						// maybe we'll succeed next time
						this.Gather(oldTarget, true);
						return;
					}

					// We reached the target - start gathering from it now
					this.SetNextState("GATHERING");
				},
			},
			
			// Walking to a good place to gather resources near, used by GatherNearPosition
			"WALKING": {
				"enter": function() {
					this.SelectAnimation("move");
				},

				"MoveCompleted": function(msg) {
					var resourceType = this.order.data.type;
					var resourceTemplate = this.order.data.template;

					// Try to find another nearby target of the same specific type
					// Also don't switch to a different type of huntable animal
					var nearby = this.FindNearbyResource(function (ent, type, template) {
						return (
							type.specific == resourceType
							&& (type.specific != "meat" || resourceTemplate == template)
						);
					});

					// If there is a nearby resource start gathering
					if (nearby)
					{
						this.Gather(nearby, false);
						return;
					}

					// Couldn't find nearby resources, so give up
					this.FinishOrder();
				},
			},

			"GATHERING": {
				"enter": function() {
					this.StartTimer(1000, 1000);

					// We want to start the gather animation as soon as possible,
					// but only if we're actually at the target and it's still alive
					// (else it'll look like we're chopping empty air).
					// (If it's not alive, the Timer handler will deal with sending us
					// off to a different target.)
					if (this.CheckTargetRange(this.order.data.target, IID_ResourceGatherer))
					{
						var typename = "gather_" + this.order.data.type.specific;
						this.SelectAnimation(typename, false, 1.0, typename);
					}
				},

				"leave": function() {
					this.StopTimer();
				},

				"Timer": function(msg) {
					var target = this.order.data.target;
					// Check we can still reach and gather from the target
					if (this.CheckTargetRange(target, IID_ResourceGatherer) && this.CanGather(target))
					{
						// Gather the resources:

						var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);

						// Try to gather treasure
						if (cmpResourceGatherer.TryInstantGather(target)) 
							return;
						
						// If we've already got some resources but they're the wrong type,
						// drop them first to ensure we're only ever carrying one type
						if (cmpResourceGatherer.IsCarryingAnythingExcept(this.order.data.type.generic))
							cmpResourceGatherer.DropResources();

						// Collect from the target
						var status = cmpResourceGatherer.PerformGather(target);

						// TODO: if exhausted, we should probably stop immediately
						// and choose a new target

						// If we've collected as many resources as possible,
						// return to the nearest dropsite
						if (status.filled)
						{
							var nearby = this.FindNearestDropsite(this.order.data.type.generic);
							if (nearby)
							{
								// (Keep this Gather order on the stack so we'll
								// continue gathering after returning)
								this.PushOrderFront("ReturnResource", { "target": nearby });
								return;
							}

							// Oh no, couldn't find any drop sites. Give up on gathering.
							this.FinishOrder();
						}
					}
					else
					{
						// Try to follow the target
						if (this.MoveToTargetRange(target, IID_ResourceGatherer))
						{
							this.SetNextState("APPROACHING");
							return;
						}

						// Can't reach the target, or it doesn't exist any more

						// We want to carry on gathering resources in the same area as
						// the old one. So try to get close to the old resource's
						// last known position

						var maxRange = 8; // get close but not too close
						if (this.order.data.lastPos &&
							this.MoveToPointRange(this.order.data.lastPos.x, this.order.data.lastPos.z,
								0, maxRange))
						{
							this.SetNextState("APPROACHING");
							return;
						}

						// We're already in range, or can't get anywhere near it.

						// Save the current order's type in case we need it later
						var oldType = this.order.data.type;
						var oldTemplate = this.order.data.template;

						// Give up on this order and try our next queued order
						if (this.FinishOrder())
							return;

						// No remaining orders - pick a useful default behaviour

						// Try to find a new resource of the same specific type near our current position:
						// Also don't switch to a different type of huntable animal
						var nearby = this.FindNearbyResource(function (ent, type, template) {
							return (
								type.specific == oldType.specific
								&& (type.specific != "meat" || oldTemplate == template)
							);
						});
						if (nearby)
						{
							this.Gather(nearby, true);
							return;
						}

						// Nothing else to gather - if we're carrying anything then we should
						// drop it off, and if not then we might as well head to the dropsite
						// anyway because that's a nice enough place to congregate and idle

						var nearby = this.FindNearestDropsite(oldType.generic);
						if (nearby)
						{
							this.PushOrderFront("ReturnResource", { "target": nearby });
							return;
						}

						// No dropsites - just give up
					}
				},
			},
		},

		"HEAL": {
			"EntityRenamed": function(msg) {
				if (this.order.data.target == msg.entity)
					this.order.data.target = msg.newentity;
			},

			"Attacked": function(msg) {
				// If we stand ground we will rather die than flee
				if (!this.GetStance().respondStandGround)
					this.Flee(msg.data.attacker, false);
			},

			"APPROACHING": {
				"enter": function () {
					this.SelectAnimation("move");
					this.StartTimer(1000, 1000);
				},

				"leave": function() {
					this.StopTimer();
				},

				"Timer": function(msg) {
					if (this.ShouldAbandonChase(this.order.data.target, this.order.data.force, IID_Heal))
					{
						this.StopMoving();
						this.FinishOrder();

						// Return to our original position
						if (this.GetStance().respondHoldGround)
							this.WalkToHeldPosition();
					}
				},

				"MoveCompleted": function() {
					this.SetNextState("HEALING");
				},
			},

			"HEALING": {
				"enter": function() {
					var cmpHeal = Engine.QueryInterface(this.entity, IID_Heal);
					this.healTimers = cmpHeal.GetTimers();
					this.SelectAnimation("heal", false, 1.0, "heal");
					this.SetAnimationSync(this.healTimers.prepare, this.healTimers.repeat);
					this.StartTimer(this.healTimers.prepare, this.healTimers.repeat);
					// TODO if .prepare is short, players can cheat by cycling heal/stop/heal
					// to beat the .repeat time; should enforce a minimum time
					// see comment in ATTACKING.enter
					this.FaceTowardsTarget(this.order.data.target);
				},

				"leave": function() {
					this.StopTimer();
				},

				"Timer": function(msg) {
					var target = this.order.data.target;
					// Check the target is still alive and healable
					if (this.TargetIsAlive(target) && this.CanHeal(target))
					{
						// Check if we can still reach the target
						if (this.CheckTargetRange(target, IID_Heal))
						{
							this.FaceTowardsTarget(target);
							var cmpHeal = Engine.QueryInterface(this.entity, IID_Heal);
							cmpHeal.PerformHeal(target);
							return;
						}
						// Can't reach it - try to chase after it
						if (this.ShouldChaseTargetedEntity(target, this.order.data.force))
						{
							if (this.MoveToTargetRange(target, IID_Heal))
							{
								this.SetNextState("HEAL.CHASING");
								return;
							}
						}
					}
					// Can't reach it, healed to max hp or doesn't exist any more - give up
					if (this.FinishOrder())
						return;

					// Heal another one
					if (this.FindNewHealTargets())
						return;
					
					// Return to our original position
					if (this.GetStance().respondHoldGround)
						this.WalkToHeldPosition();
				},
			},
			"CHASING": {
				"enter": function () {
					this.SelectAnimation("move");
					this.StartTimer(1000, 1000);
				},

				"leave": function () {
					this.StopTimer();
				},
				"Timer": function(msg) {
					if (this.ShouldAbandonChase(this.order.data.target, this.order.data.force, IID_Heal))
					{
						this.StopMoving();
						this.FinishOrder();

						// Return to our original position
						if (this.GetStance().respondHoldGround)
							this.WalkToHeldPosition();
					}
				},
				"MoveCompleted": function () {
					this.SetNextState("HEALING");
				},
			},  
		},

		// Returning to dropsite
		"RETURNRESOURCE": {
			"APPROACHING": {
				"enter": function () {
					// Work out what we're carrying, in order to select an appropriate animation
					var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
					var type = cmpResourceGatherer.GetLastCarriedType();
					if (type)
					{
						var typename = "carry_" + type.generic;

						// Special case for meat
						if (type.specific == "meat")
							typename = "carry_" + type.specific;

						this.SelectAnimation(typename, false, this.GetWalkSpeed());
					}
					else
					{
						// We're returning empty-handed
						this.SelectAnimation("move");
					}
				},

				"MoveCompleted": function() {
					// Switch back to idle animation to guarantee we won't
					// get stuck with the carry animation after stopping moving
					this.SelectAnimation("idle");

					// Check the dropsite is in range and we can return our resource there
					// (we didn't get stopped before reaching it)
					if (this.CheckTargetRange(this.order.data.target, IID_ResourceGatherer) && this.CanReturnResource(this.order.data.target, true))
					{
						var cmpResourceDropsite = Engine.QueryInterface(this.order.data.target, IID_ResourceDropsite);
						if (cmpResourceDropsite)
						{
							// Dump any resources we can
							var dropsiteTypes = cmpResourceDropsite.GetTypes();

							var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
							cmpResourceGatherer.CommitResources(dropsiteTypes);

							// Our next order should always be a Gather,
							// so just switch back to that order
							this.FinishOrder();
							return;
						}
					}

					// The dropsite was destroyed, or we couldn't reach it, or ownership changed
					// Look for a new one.

					var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
					var genericType = cmpResourceGatherer.GetMainCarryingType();
					var nearby = this.FindNearestDropsite(genericType);
					if (nearby)
					{
						this.FinishOrder();
						this.PushOrderFront("ReturnResource", { "target": nearby });
						return;
					}

					// Oh no, couldn't find any drop sites. Give up on returning.
					this.FinishOrder();
				},
			},
		},

		"TRADE": {
			"Attacked": function(msg) {
				// Ignore attack
				// TODO: Inform player
			},

			"APPROACHINGFIRSTMARKET": {
				"enter": function () {
					this.SelectAnimation("move");
				},

				"MoveCompleted": function() {
					this.PerformTradeAndMoveToNextMarket(this.order.data.firstMarket, this.order.data.secondMarket, "INDIVIDUAL.TRADE.APPROACHINGSECONDMARKET");
				},
			},

			"APPROACHINGSECONDMARKET": {
				"enter": function () {
					this.SelectAnimation("move");
				},

				"MoveCompleted": function() {
					this.order.data.firstPass = false;
					this.PerformTradeAndMoveToNextMarket(this.order.data.secondMarket, this.order.data.firstMarket, "INDIVIDUAL.TRADE.APPROACHINGFIRSTMARKET");
				},
			},
		},

		"REPAIR": {
			"APPROACHING": {
				"enter": function () {
					this.SelectAnimation("move");
				},
			
				"MoveCompleted": function() {
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
					// Check we can still reach and repair the target
					if (!this.CheckTargetRange(target, IID_Builder) || !this.CanRepair(target))
					{
						// Can't reach it, no longer owned by ally, or it doesn't exist any more
						this.FinishOrder();
						return;
					}
					
					var cmpBuilder = Engine.QueryInterface(this.entity, IID_Builder);
					cmpBuilder.PerformBuilding(target);
				},
			},

			"ConstructionFinished": function(msg) {
				if (msg.data.entity != this.order.data.target)
					return; // ignore other buildings

				// Save the current order's data in case we need it later
				var oldAutocontinue = this.order.data.autocontinue;

				// We finished building it.
				// Switch to the next order (if any)
				if (this.FinishOrder())
					return;

				// No remaining orders - pick a useful default behaviour

				// If autocontinue explicitly disabled (e.g. by AI) then
				// do nothing automatically
				if (!oldAutocontinue)
					return;

				// If this building was e.g. a farm, we should start gathering from it
				// if we own the building
				if (this.CanGather(msg.data.newentity))
				{
					this.Gather(msg.data.newentity, true);
					return;
				}

				// If this building was e.g. a farmstead, we should look for nearby
				// resources we can gather, if we own the building
				if (this.CanReturnResource(msg.data.newentity, false))
				{
					var cmpResourceDropsite = Engine.QueryInterface(msg.data.newentity, IID_ResourceDropsite);
					var types = cmpResourceDropsite.GetTypes();
					// TODO: Slightly undefined behavior here, we don't know what type of resource will be collected,
					//   may cause problems for AIs (especially hunting fast animals), but avoid ugly hacks to fix that!
					var nearby = this.FindNearbyResource(function (ent, type, template) {
						return (types.indexOf(type.generic) != -1);
					});
					if (nearby)
					{
						this.Gather(nearby, true);
						return;
					}
				}

				// Look for a nearby foundation to help with
				var nearbyFoundation = this.FindNearbyFoundation();
				if (nearbyFoundation)
				{
					this.Repair(nearbyFoundation, oldAutocontinue, true);
					return;
				}
			},

			// Override the LeaveFoundation order since we don't want to be
			// accidentally blocking our own building
			"Order.LeaveFoundation": function(msg) {
				if (!IsOwnedByAllyOfEntity(this.entity, msg.data.target))
				{
					this.FinishOrder();
					return;
				}
				// Move a tile outside the building
				var range = 4;
				var ok = this.MoveToTargetRangeExplicit(msg.data.target, range, range);
				if (ok)
				{
					// We've started walking to the given point
					this.SetNextState("INDIVIDUAL.WALKING");
				}
				else
				{
					// We are already at the target, or can't move at all
					this.FinishOrder();
				}
			},

		},

		"GARRISON": {
			"APPROACHING": {
				"enter": function() {
					this.SelectAnimation("walk", false, this.GetWalkSpeed());
					this.PlaySound("walk");
				},

				"MoveCompleted": function() {
					this.SetNextState("GARRISONED");
				},
				
				"leave": function() {
					this.StopTimer();
				}
			},

			"GARRISONED": {
				"enter": function() {
					var target = this.order.data.target;
					var cmpGarrisonHolder = Engine.QueryInterface(target, IID_GarrisonHolder);

					// Check that we can garrison here
					if (this.CanGarrison(target))
					{
						// Check that we're in range of the garrison target
						if (this.CheckGarrisonRange(target))
						{
							// Check that garrisoning succeeds
							if (cmpGarrisonHolder.Garrison(this.entity))
							{
								this.isGarrisoned = true;
								
								// Check if we are garrisoned in a dropsite
								var cmpResourceDropsite = Engine.QueryInterface(target, IID_ResourceDropsite);
								if (cmpResourceDropsite)
								{
									// Dump any resources we can
									var dropsiteTypes = cmpResourceDropsite.GetTypes();
									var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
									if (cmpResourceGatherer)
										cmpResourceGatherer.CommitResources(dropsiteTypes);
								}
								
								return;
							}
						}
						else
						{
							// Unable to reach the target, try again
							// (or follow if it's a moving target)
							if (this.MoveToTarget(target))
							{
								this.SetNextState("APPROACHING");
								return;
							}
						}
					}

					// Garrisoning failed for some reason, so finish the order
					this.FinishOrder();
					return;
				},
				
				"Order.Ungarrison": function() {
					if (this.FinishOrder())
						return;
				},

				"leave": function() {
					this.isGarrisoned = false;
				}
			},
		},

		"CHEERING": {
			"enter": function() {
				// Unit is invulnerable while cheering
				var cmpDamageReceiver = Engine.QueryInterface(this.entity, IID_DamageReceiver);
				cmpDamageReceiver.SetInvulnerability(true); 
				this.SelectAnimation("promotion");
				this.StartTimer(4000, 4000);
				return false;
			},

			"leave": function() {
				this.StopTimer();
				var cmpDamageReceiver = Engine.QueryInterface(this.entity, IID_DamageReceiver);
				cmpDamageReceiver.SetInvulnerability(false);
			},

			"Timer": function(msg) {
				this.FinishOrder();
			},
		},
	},

	"ANIMAL": {

		"HealthChanged": function(msg) {
			// If we died (got reduced to 0 hitpoints), stop the AI and act like a corpse
			if (msg.to == 0)
				this.SetNextState("CORPSE");
		},

		"Attacked": function(msg) {
			if (this.template.NaturalBehaviour == "skittish" ||
			    this.template.NaturalBehaviour == "passive")
			{
				this.Flee(msg.data.attacker, false);
			}
			else if (this.template.NaturalBehaviour == "violent" ||
			         this.template.NaturalBehaviour == "aggressive" ||
			         this.template.NaturalBehaviour == "defensive")
			{
				if (this.CanAttack(msg.data.attacker))
					this.Attack(msg.data.attacker, false);
			}
			else if (this.template.NaturalBehaviour == "domestic")
			{
				// Never flee, stop what we were doing
				this.SetNextState("IDLE");
			}
		},

		"Order.LeaveFoundation": function(msg) {
			// Run away from the foundation
			this.MoveToTargetRangeExplicit(msg.data.target, +this.template.FleeDistance, +this.template.FleeDistance);
			this.SetNextState("FLEEING");
		},

		"IDLE": {
			// (We need an IDLE state so that FinishOrder works)

			"enter": function() {
				// Start feeding immediately
				this.SetNextState("FEEDING");
				return true;
			},
		},

		"CORPSE": {
			"enter": function() {
				this.StopMoving();
			},

			// Ignore all orders that animals might otherwise respond to
			"Order.FormationWalk": function() { },
			"Order.Walk": function() { },
			"Order.WalkToTarget": function() { },
			"Order.Attack": function() { },

			"Attacked": function(msg) {
				// Do nothing, because we're dead already
			},

			"Order.LeaveFoundation": function(msg) {
				// We can't walk away from the foundation (since we're dead),
				// but we mustn't block its construction (since the builders would get stuck),
				// and we don't want to trick gatherers into trying to reach us when
				// we're stuck in the middle of a building, so just delete our corpse.
				Engine.DestroyEntity(this.entity);
			},
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

			"LosRangeUpdate": function(msg) {
				if (this.template.NaturalBehaviour == "skittish")
				{
					if (msg.data.added.length > 0)
					{
						this.Flee(msg.data.added[0], false);
						return;
					}
				}
				// Start attacking one of the newly-seen enemy (if any)
				else if (this.template.NaturalBehaviour == "violent" ||
				         this.template.NaturalBehaviour == "aggressive")
				{
					this.AttackVisibleEntity(msg.data.added);
				}

				// TODO: if two units enter our range together, we'll attack the
				// first and then the second won't trigger another LosRangeUpdate
				// so we won't notice it. Probably we should do something with
				// ResetActiveQuery in ROAMING.enter/FEEDING.enter in order to
				// find any units that are already in range.
			},

			"Timer": function(msg) {
				this.SetNextState("FEEDING");
			},

			"MoveCompleted": function() {
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

			"LosRangeUpdate": function(msg) {
				if (this.template.NaturalBehaviour == "skittish")
				{
					if (msg.data.added.length > 0)
					{
						this.Flee(msg.data.added[0], false);
						return;
					}
				}
				// Start attacking one of the newly-seen enemy (if any)
				else if (this.template.NaturalBehaviour == "violent")
				{
					this.AttackVisibleEntity(msg.data.added);
				}
			},

			"MoveCompleted": function() { },

			"Timer": function(msg) {
				this.SetNextState("ROAMING");
			},
		},

		"FLEEING": "INDIVIDUAL.FLEEING", // reuse the same fleeing behaviour for animals

		"COMBAT": "INDIVIDUAL.COMBAT", // reuse the same combat behaviour for animals
		
		"WALKING": "INDIVIDUAL.WALKING",	// reuse the same walking behaviour for animals
							// only used for domestic animals
	},
};

var UnitFsm = new FSM(UnitFsmSpec);

UnitAI.prototype.Init = function()
{
	this.orderQueue = []; // current order is at the front of the list
	this.order = undefined; // always == this.orderQueue[0]
	this.formationController = INVALID_ENTITY; // entity with IID_Formation that we belong to
	this.isGarrisoned = false;
	this.isIdle = false;
	this.lastFormationName = "Line Closed";

	this.SetStance(this.template.DefaultStance);
};

UnitAI.prototype.IsFormationController = function()
{
	return (this.template.FormationController == "true");
};

UnitAI.prototype.IsAnimal = function()
{
	return (this.template.NaturalBehaviour ? true : false);
};

UnitAI.prototype.IsDomestic = function()
{
	var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	if (!cmpIdentity)
		return false;
	return cmpIdentity.HasClass("Domestic");
};

UnitAI.prototype.IsHealer = function()
{
	return Engine.QueryInterface(this.entity, IID_Heal);
};

UnitAI.prototype.IsIdle = function()
{
	return this.isIdle;
};

UnitAI.prototype.IsGarrisoned = function()
{
	return this.isGarrisoned;
};

UnitAI.prototype.OnCreate = function()
{
	if (this.IsAnimal())
		UnitFsm.Init(this, "ANIMAL.FEEDING", this.StateChanged);
	else if (this.IsFormationController())
		UnitFsm.Init(this, "FORMATIONCONTROLLER.IDLE", this.StateChanged);
	else
		UnitFsm.Init(this, "INDIVIDUAL.IDLE", this.StateChanged);
};

UnitAI.prototype.StateChanged = function()
{
	Engine.PostMessage(this.entity, MT_UnitAIStateChanged, { "to": this.GetCurrentState()});
}

UnitAI.prototype.OnOwnershipChanged = function(msg)
{
	this.SetupRangeQuery();
	if (this.IsHealer())
		this.SetupHealRangeQuery();
};

UnitAI.prototype.OnDestroy = function()
{
	// Clean up any timers that are now obsolete
	this.StopTimer();

	// Clean up range queries
	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (this.losRangeQuery)
		rangeMan.DestroyActiveQuery(this.losRangeQuery);
	if (this.losHealRangeQuery)
		rangeMan.DestroyActiveQuery(this.losHealRangeQuery);
};

// Set up a range query for all enemy units within LOS range
// which can be attacked.
// This should be called whenever our ownership changes.
UnitAI.prototype.SetupRangeQuery = function()
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var owner = cmpOwnership.GetOwner();

	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var playerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);

	if (this.losRangeQuery)
		rangeMan.DestroyActiveQuery(this.losRangeQuery);

	var players = [];

	if (owner != -1)
	{
		// If unit not just killed, get enemy players via diplomacy
		var cmpPlayer = Engine.QueryInterface(playerMan.GetPlayerByID(owner), IID_Player);
		var numPlayers = playerMan.GetNumPlayers();
		for (var i = 1; i < numPlayers; ++i)
		{
			// Exclude gaia, allies, and self
			// TODO: How to handle neutral players - Special query to attack military only?
			if (cmpPlayer.IsEnemy(i))
				players.push(i);
		}
	}

	var range = this.GetQueryRange(IID_Attack);

	this.losRangeQuery = rangeMan.CreateActiveQuery(this.entity, range.min, range.max, players, IID_DamageReceiver, rangeMan.GetEntityFlagMask("normal"));
	rangeMan.EnableActiveQuery(this.losRangeQuery);
};

// Set up a range query for all own or ally units within LOS range
// which can be healed.
// This should be called whenever our ownership changes.
UnitAI.prototype.SetupHealRangeQuery = function()
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var owner = cmpOwnership.GetOwner();

	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var playerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);

	if (this.losHealRangeQuery)
		rangeMan.DestroyActiveQuery(this.losHealRangeQuery);

	var players = [owner];

	if (owner != -1)
	{
		// If unit not just killed, get ally players via diplomacy
		var cmpPlayer = Engine.QueryInterface(playerMan.GetPlayerByID(owner), IID_Player);
		var numPlayers = playerMan.GetNumPlayers();
		for (var i = 1; i < numPlayers; ++i)
		{
			// Exclude gaia and enemies
			if (cmpPlayer.IsAlly(i))
				players.push(i);
		}
	}

	var range = this.GetQueryRange(IID_Heal);

	this.losHealRangeQuery = rangeMan.CreateActiveQuery(this.entity, range.min, range.max, players, IID_Health, rangeMan.GetEntityFlagMask("injured"));
	rangeMan.EnableActiveQuery(this.losHealRangeQuery);
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

UnitAI.prototype.GetCurrentState = function()
{
	return UnitFsm.GetCurrentState(this);
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
		var ret = UnitFsm.ProcessMessage(this,
			{"type": "Order."+this.order.type, "data": this.order.data}
		);
		
		Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData()});

		// If the order was rejected then immediately take it off
		// and process the remaining queue
		if (ret && ret.discardOrder)
		{
			return this.FinishOrder();
		}

		// Otherwise we've successfully processed a new order
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
		var ret = UnitFsm.ProcessMessage(this,
			{"type": "Order."+this.order.type, "data": this.order.data}
		);
		
		Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData()});

		// If the order was rejected then immediately take it off
		// and process the remaining queue
		if (ret && ret.discardOrder)
		{
			this.FinishOrder();
		}
	}
};

/**
 * Add an order onto the front of the queue,
 * and execute it immediately.
 */
UnitAI.prototype.PushOrderFront = function(type, data)
{
	var order = { "type": type, "data": data };
	// If current order is cheering then add new order after it 
	if (this.order && this.order.type == "Cheering")
	{
		var cheeringOrder = this.orderQueue.shift();
		this.orderQueue.unshift(cheeringOrder, order);
	}
	else
	{
		this.orderQueue.unshift(order);
		this.order = order;
		var ret = UnitFsm.ProcessMessage(this,
			{"type": "Order."+this.order.type, "data": this.order.data}
		);
		
		Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData()});

		// If the order was rejected then immediately take it off again;
		// assume the previous active order is still valid (the short-lived
		// new order hasn't changed state or anything) so we can carry on
		// as if nothing had happened
		if (ret && ret.discardOrder)
		{
			this.orderQueue.shift();
			this.order = this.orderQueue[0];
		}
	}
};

UnitAI.prototype.ReplaceOrder = function(type, data)
{
	// If current order is cheering then add new order after it
	if (this.order && this.order.type == "Cheering")
	{
		var order = { "type": type, "data": data };
		var cheeringOrder = this.orderQueue.shift();
		this.orderQueue = [ cheeringOrder, order ];
	}
	else
	{
		this.orderQueue = [];
		this.PushOrder(type, data);
	}
};

UnitAI.prototype.GetOrders = function()
{
	return this.orderQueue.slice();
}

UnitAI.prototype.AddOrders = function(orders)
{
	for each (var order in orders)
	{
		this.PushOrder(order.type, order.data);
	}
}

UnitAI.prototype.GetOrderData = function()
{
	if (this.order && this.order.data)
	{
		return eval(uneval(this.order.data)); // return a copy
	}
	else
		return undefined;
}

UnitAI.prototype.TimerHandler = function(data, lateness)
{
	// Reset the timer
	if (data.timerRepeat === undefined)
	{
		this.timer = undefined;
	}
	else
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "TimerHandler", data.timerRepeat - lateness, data);
	}

	UnitFsm.ProcessMessage(this, {"type": "Timer", "data": data, "lateness": lateness});
};

/**
 * Set up the UnitAI timer to run after 'offset' msecs, and then
 * every 'repeat' msecs until StopTimer is called. A "Timer" message
 * will be sent each time the timer runs.
 */
UnitAI.prototype.StartTimer = function(offset, repeat)
{
	if (this.timer)
		error("Called StartTimer when there's already an active timer");

	var data = { "timerRepeat": repeat };

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "TimerHandler", offset, data);
};

/**
 * Stop the current UnitAI timer.
 */
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
	if (msg.starting && !msg.error)
	{
		UnitFsm.ProcessMessage(this, {"type": "MoveStarted", "data": msg});
	}
	else if (!msg.starting || msg.error)
	{
		UnitFsm.ProcessMessage(this, {"type": "MoveCompleted", "data": msg});
	}
};

UnitAI.prototype.OnGlobalConstructionFinished = function(msg)
{
	// TODO: This is a bit inefficient since every unit listens to every
	// construction message - ideally we could scope it to only the one we're building

	UnitFsm.ProcessMessage(this, {"type": "ConstructionFinished", "data": msg});
};

UnitAI.prototype.OnGlobalEntityRenamed = function(msg)
{
	UnitFsm.ProcessMessage(this, {"type": "EntityRenamed", "entity": msg.entity, "newentity": msg.newentity});
}

UnitAI.prototype.OnAttacked = function(msg)
{
	UnitFsm.ProcessMessage(this, {"type": "Attacked", "data": msg});
};

UnitAI.prototype.OnHealthChanged = function(msg)
{
	UnitFsm.ProcessMessage(this, {"type": "HealthChanged", "from": msg.from, "to": msg.to});
};

UnitAI.prototype.OnRangeUpdate = function(msg)
{
	if (msg.tag == this.losRangeQuery)
		UnitFsm.ProcessMessage(this, {"type": "LosRangeUpdate", "data": msg});
	else if (msg.tag == this.losHealRangeQuery)
		UnitFsm.ProcessMessage(this, {"type": "LosHealRangeUpdate", "data": msg});
};

//// Helper functions to be called by the FSM ////

UnitAI.prototype.GetWalkSpeed = function()
{
	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.GetWalkSpeed();
};

UnitAI.prototype.GetRunSpeed = function()
{
	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.GetRunSpeed();
};

/**
 * Returns true if the target exists and has non-zero hitpoints.
 */
UnitAI.prototype.TargetIsAlive = function(ent)
{
	var cmpHealth = Engine.QueryInterface(ent, IID_Health);
	if (!cmpHealth)
		return false;

	return (cmpHealth.GetHitpoints() != 0);
};

/**
 * Returns true if the target exists and needs to be killed before
 * beginning to gather resources from it.
 */
UnitAI.prototype.MustKillGatherTarget = function(ent)
{
	var cmpResourceSupply = Engine.QueryInterface(ent, IID_ResourceSupply);
	if (!cmpResourceSupply)
		return false;

	if (!cmpResourceSupply.GetKillBeforeGather())
		return false;

	return this.TargetIsAlive(ent);
};

/**
 * Returns the entity ID of the nearest resource supply where the given
 * filter returns true, or undefined if none can be found.
 * TODO: extend this to exclude resources that already have lots of
 * gatherers.
 */
UnitAI.prototype.FindNearbyResource = function(filter)
{
	var range = 64; // TODO: what's a sensible number?

	var playerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	// We accept resources owned by Gaia or any player
	var players = [0];
	for (var i = 1; i < playerMan.GetNumPlayers(); ++i)
		players.push(i);

	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var nearby = cmpRangeManager.ExecuteQuery(this.entity, 0, range, players, IID_ResourceSupply);
	for each (var ent in nearby)
	{
		var cmpResourceSupply = Engine.QueryInterface(ent, IID_ResourceSupply);
		var type = cmpResourceSupply.GetType();
		var template = cmpTemplateManager.GetCurrentTemplateName(ent);
		if (filter(ent, type, template))
			return ent;
	}

	return undefined;
};

/**
 * Returns the entity ID of the nearest resource dropsite that accepts
 * the given type, or undefined if none can be found.
 */
UnitAI.prototype.FindNearestDropsite = function(genericType)
{
	// Find dropsites owned by this unit's player
	var players = [];
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpOwnership)
		players.push(cmpOwnership.GetOwner());

	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var nearby = rangeMan.ExecuteQuery(this.entity, 0, -1, players, IID_ResourceDropsite);
	for each (var ent in nearby)
	{
		var cmpDropsite = Engine.QueryInterface(ent, IID_ResourceDropsite);
		if (!cmpDropsite.AcceptsType(genericType))
			continue;

		return ent;
	}

	return undefined;
};

/**
 * Returns the entity ID of the nearest building that needs to be constructed,
 * or undefined if none can be found close enough.
 */
UnitAI.prototype.FindNearbyFoundation = function()
{
	var range = 64; // TODO: what's a sensible number?

	// Find buildings owned by this unit's player
	var players = [];
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpOwnership)
		players.push(cmpOwnership.GetOwner());

	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var nearby = rangeMan.ExecuteQuery(this.entity, 0, range, players, IID_Foundation);
	for each (var ent in nearby)
	{
		// Skip foundations that are already complete. (This matters since
		// we process the ConstructionFinished message before the foundation
		// we're working on has been deleted.)
		var cmpFoundation = Engine.QueryInterface(ent, IID_Foundation);
		if (cmpFoundation.IsFinished())
			continue;

		return ent;
	}

	return undefined;
};

/**
 * Play a sound appropriate to the current entity.
 */
UnitAI.prototype.PlaySound = function(name)
{
	// If we're a formation controller, use the sounds from our first member
	if (this.IsFormationController())
	{
		var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
		var member = cmpFormation.GetPrimaryMember();
		if (member)
			PlaySound(name, member);
	}
	else
	{
		// Otherwise use our own sounds
		PlaySound(name, this.entity);
	}
};

UnitAI.prototype.SelectAnimation = function(name, once, speed, sound)
{
	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpVisual)
		return;

	// Special case: the "move" animation gets turned into a special
	// movement mode that deals with speeds and walk/run automatically
	if (name == "move")
	{
		// Speed to switch from walking to running animations
		var runThreshold = (this.GetWalkSpeed() + this.GetRunSpeed()) / 2;

		cmpVisual.SelectMovementAnimation(runThreshold);
		return;
	}

	var soundgroup;
	if (sound)
	{
		var cmpSound = Engine.QueryInterface(this.entity, IID_Sound);
		if (cmpSound)
			soundgroup = cmpSound.GetSoundGroup(sound);
	}

	// Set default values if unspecified
	if (once === undefined)
		once = false;
	if (speed === undefined)
		speed = 1.0;
	if (soundgroup === undefined)
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

UnitAI.prototype.StopMoving = function()
{
	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	cmpUnitMotion.StopMoving();
};

UnitAI.prototype.MoveToPoint = function(x, z)
{
	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.MoveToPointRange(x, z, 0, 0);
};

UnitAI.prototype.MoveToPointRange = function(x, z, rangeMin, rangeMax)
{
	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.MoveToPointRange(x, z, rangeMin, rangeMax);
};

UnitAI.prototype.MoveToTarget = function(target)
{
	if (!this.CheckTargetVisible(target))
		return false;

	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.MoveToTargetRange(target, 0, 0);
};

UnitAI.prototype.MoveToTargetRange = function(target, iid, type)
{
	if (!this.CheckTargetVisible(target))
		return false;

	var cmpRanged = Engine.QueryInterface(this.entity, iid);
	var range = cmpRanged.GetRange(type);

	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.MoveToTargetRange(target, range.min, range.max);
};

UnitAI.prototype.MoveToTargetRangeExplicit = function(target, min, max)
{
	if (!this.CheckTargetVisible(target))
		return false;

	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.MoveToTargetRange(target, min, max);
};

UnitAI.prototype.CheckTargetRange = function(target, iid, type)
{
	var cmpRanged = Engine.QueryInterface(this.entity, iid);
	var range = cmpRanged.GetRange(type);

	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.IsInTargetRange(target, range.min, range.max);
};

UnitAI.prototype.CheckGarrisonRange = function(target)
{
	var cmpGarrisonHolder = Engine.QueryInterface(target, IID_GarrisonHolder);
	var range = cmpGarrisonHolder.GetLoadingRange();

	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.IsInTargetRange(target, range.min, range.max);
}

/**
 * Returns true if the target entity is visible through the FoW/SoD.
 */
UnitAI.prototype.CheckTargetVisible = function(target)
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
		return false;

	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (!cmpRangeManager)
		return false;

	if (cmpRangeManager.GetLosVisibility(target, cmpOwnership.GetOwner(), false) == "hidden")
		return false;

	// Either visible directly, or visible in fog
	return true;
};

UnitAI.prototype.FaceTowardsTarget = function(target)
{
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
			return;
	var cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld())
			return;
	var pos = cmpPosition.GetPosition();
	var targetpos = cmpTargetPosition.GetPosition();
	var angle = Math.atan2(targetpos.x - pos.x, targetpos.z - pos.z);
	var rot = cmpPosition.GetRotation();
	var delta = (rot.y - angle + Math.PI) % (2 * Math.PI) - Math.PI;
	if (Math.abs(delta) > 0.2)
	{
		var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
		if (cmpUnitMotion)
			cmpUnitMotion.FaceTowardsPoint(targetpos.x, targetpos.z);
	}
}

UnitAI.prototype.CheckTargetDistanceFromHeldPosition = function(target, iid, type)
{
	var cmpRanged = Engine.QueryInterface(this.entity, iid);
	var range = iid !== IID_Attack ? cmpRanged.GetRange() : cmpRanged.GetRange(type);

	var cmpPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return false;

	var cmpVision = Engine.QueryInterface(this.entity, IID_Vision);
	if (!cmpVision)
		return false;
	var halfvision = cmpVision.GetRange() / 2;

	var pos = cmpPosition.GetPosition();
	var dx = this.heldPosition.x - pos.x;
	var dz = this.heldPosition.z - pos.z;
	var dist = Math.sqrt(dx*dx + dz*dz);

	return dist < halfvision + range.max;
};

UnitAI.prototype.CheckTargetIsInVisionRange = function(target)
{
	var cmpVision = Engine.QueryInterface(this.entity, IID_Vision);
	if (!cmpVision)
		return false;
	var range = cmpVision.GetRange();

	var distance = DistanceBetweenEntities(this.entity,target);

	return distance < range;
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
			this.PushOrderFront("Attack", { "target": target, "force": false });
			return true;
		}
	}
	return false;
};

/**
 * Try to find one of the given entities which can be attacked
 * and which is close to the hold position, and start attacking it.
 * Returns true if it found something to attack.
 */
UnitAI.prototype.AttackEntityInZone = function(ents)
{
	var type = this.GetBestAttack();
	for each (var target in ents)
	{
		if (this.CanAttack(target) && this.CheckTargetDistanceFromHeldPosition(target, IID_Attack, type))
		{
			this.PushOrderFront("Attack", { "target": target, "force": false });
			return true;
		}
	}
	return false;
};

/**
 * Try to respond appropriately given our current stance,
 * given a list of entities that match our stance's target criteria.
 * Returns true if it responded.
 */
UnitAI.prototype.RespondToTargetedEntities = function(ents)
{
	if (!ents.length)
		return false;

	if (this.GetStance().respondChase)
		return this.AttackVisibleEntity(ents);

	if (this.GetStance().respondStandGround)
		return this.AttackVisibleEntity(ents);

	if (this.GetStance().respondHoldGround)
		return this.AttackEntityInZone(ents);

	if (this.GetStance().respondFlee)
	{
		this.PushOrderFront("Flee", { "target": ents[0] });
		return true;
	}

	return false;
};

/**
 * Try to respond to healable entities.
 * Returns true if it responded.
 */
UnitAI.prototype.RespondToHealableEntities = function(ents)
{
	if (!ents.length)
		return false;

	for each (var ent in ents)
	{
		if (this.CanHeal(ent))
		{
			this.PushOrderFront("Heal", { "target": ent, "force": false });
			return true;
		}
	}

	return false;
};

/**
 * Returns true if we should stop following the target entity.
 */
UnitAI.prototype.ShouldAbandonChase = function(target, force, iid)
{
	// Stop if we're in hold-ground mode and it's too far from the holding point
	if (this.GetStance().respondHoldGround)
	{
		if (!this.CheckTargetDistanceFromHeldPosition(target, iid, this.attackType))
			return true;
	}

	// Stop if it's left our vision range, unless we're especially persistent
	if (!force && !this.GetStance().respondChaseBeyondVision)
	{
		if (!this.CheckTargetIsInVisionRange(target))
			return true;
	}

	// (Note that CCmpUnitMotion will detect if the target is lost in FoW,
	// and will continue moving to its last seen position and then stop)

	return false;
};

/*
 * Returns whether we should chase the targeted entity,
 * given our current stance.
 */
UnitAI.prototype.ShouldChaseTargetedEntity = function(target, force)
{
	if (this.GetStance().respondChase)
		return true;

	if (force)
		return true;

	return false;
};

//// External interface functions ////

UnitAI.prototype.SetFormationController = function(ent)
{
	this.formationController = ent;

	// Set obstruction group, so we can walk through members
	// of our own formation (or ourself if not in formation)
	var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (cmpObstruction)
	{
		if (ent == INVALID_ENTITY)
			cmpObstruction.SetControlGroup(this.entity);
		else
			cmpObstruction.SetControlGroup(ent);
	}

	// If we were removed from a formation, let the FSM switch back to INDIVIDUAL
	if (ent == INVALID_ENTITY)
		UnitFsm.ProcessMessage(this, { "type": "FormationLeave" });
};

UnitAI.prototype.GetFormationController = function()
{
	return this.formationController;
};

UnitAI.prototype.SetLastFormationName = function(name)
{
	this.lastFormationName = name;
};

UnitAI.prototype.GetLastFormationName = function()
{
	return this.lastFormationName;
};

/**
 * Returns the estimated distance that this unit will travel before either
 * finishing all of its orders, or reaching a non-walk target (attack, gather, etc).
 * Intended for Formation to switch to column layout on long walks.
 */
UnitAI.prototype.ComputeWalkingDistance = function()
{
	var distance = 0;

	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return 0;

	// Keep track of the position at the start of each order
	var pos = cmpPosition.GetPosition();

	for (var i = 0; i < this.orderQueue.length; ++i)
	{
		var order = this.orderQueue[i];
		switch (order.type)
		{
		case "Walk":
		case "GatherNearPosition":
			// Add the distance to the target point
			var dx = order.data.x - pos.x;
			var dz = order.data.z - pos.z;
			var d = Math.sqrt(dx*dx + dz*dz);
			distance += d;
			
			// Remember this as the start position for the next order
			pos = order.data;

			break; // and continue the loop

		case "WalkToTarget":
		case "Flee":
		case "LeaveFoundation":
		case "Attack":
		case "Heal":
		case "Gather":
		case "ReturnResource":
		case "Repair":
		case "Garrison":
			// Find the target unit's position
			var cmpTargetPosition = Engine.QueryInterface(order.data.target, IID_Position);
			if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld())
				return distance;
			var targetPos = cmpTargetPosition.GetPosition();

			// Add the distance to the target unit
			var dx = targetPos.x - pos.x;
			var dz = targetPos.z - pos.z;
			var d = Math.sqrt(dx*dx + dz*dz);
			distance += d;

			// Return the total distance to the target
			return distance;

		default:
			error("ComputeWalkingDistance: Unrecognised order type '"+order.type+"'");
			return distance;
		}
	}

	// Return the total distance to the end of the order queue
	return distance;
};

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
	this.AddOrder("WalkToTarget", { "target": target }, queued);
};

UnitAI.prototype.LeaveFoundation = function(target)
{
	// If we're already being told to leave a foundation, then
	// ignore this new request so we don't end up being too indecisive
	// to ever actually move anywhere
	if (this.order && this.order.type == "LeaveFoundation")
		return;

	this.PushOrderFront("LeaveFoundation", { "target": target });
};

UnitAI.prototype.Attack = function(target, queued)
{
	if (!this.CanAttack(target))
	{
		// We don't want to let healers walk to the target unit so they can be easily killed.
		// Instead we just let them get into healing range.
		if (this.IsHealer())
			this.MoveToTargetRange(target, IID_Heal);
		else
			this.WalkToTarget(target, queued);
		return;
	}

	this.AddOrder("Attack", { "target": target, "force": true }, queued);
};

UnitAI.prototype.Garrison = function(target, queued)
{
	if (!this.CanGarrison(target))
	{
		this.WalkToTarget(target, queued);
		return;
	}
	this.AddOrder("Garrison", { "target": target }, queued);
};

UnitAI.prototype.Ungarrison = function()
{
	if (this.IsGarrisoned())
	{
		this.AddOrder("Ungarrison", null, false);
	}
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

	// Also save the target entity's template, so that if it's an animal,
	// we won't go from hunting slow safe animals to dangerous fast ones
	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var template = cmpTemplateManager.GetCurrentTemplateName(target);

	// Remember the position of our target, if any, in case it disappears
	// later and we want to head to its last known position
	// (TODO: if the target moves a lot (e.g. it's an animal), maybe we
	// need to update this lastPos regularly rather than just here?)
	var lastPos = undefined;
	var cmpPosition = Engine.QueryInterface(target, IID_Position);
	if (cmpPosition && cmpPosition.IsInWorld())
		lastPos = cmpPosition.GetPosition();

	this.AddOrder("Gather", { "target": target, "type": type, "template": template, "lastPos": lastPos }, queued);
};

UnitAI.prototype.GatherNearPosition = function(x, z, type, queued)
{
	this.AddOrder("GatherNearPosition", { "type": type, "x": x, "z": z }, queued);
}

UnitAI.prototype.Heal = function(target, queued)
{
	if (!this.CanHeal(target))
	{
		this.WalkToTarget(target, queued);
		return;
	}
	
	this.AddOrder("Heal", { "target": target, "force": true }, queued);
};

UnitAI.prototype.ReturnResource = function(target, queued)
{
	if (!this.CanReturnResource(target, true))
	{
		this.WalkToTarget(target, queued);
		return;
	}

	this.AddOrder("ReturnResource", { "target": target }, queued);
};

UnitAI.prototype.SetupTradeRoute = function(target, queued)
{
	if (!this.CanTrade(target))
	{
		this.WalkToTarget(target, queued);
		return;
	}

	var cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
	var marketsChanged = cmpTrader.SetTargetMarket(target);
	if (marketsChanged)
	{
		if (cmpTrader.HasBothMarkets())
			this.AddOrder("Trade", { "firstMarket": cmpTrader.GetFirstMarket(), "secondMarket": cmpTrader.GetSecondMarket() }, queued);
		else
			this.WalkToTarget(cmpTrader.GetFirstMarket(), queued);
	}
}

UnitAI.prototype.MoveToMarket = function(targetMarket)
{
	if (this.MoveToTarget(targetMarket))
	{
		// We've started walking to the market
		return true;
	}
	else
	{
		// We can't reach the market.
		// Give up.
		this.StopMoving();
		this.StopTrading();
		return false;
	}
}

UnitAI.prototype.PerformTradeAndMoveToNextMarket = function(currentMarket, nextMarket, nextFsmStateName)
{
	if (!this.CanTrade(currentMarket))
	{
		this.StopTrading();
		return;
	}

	if (this.CheckTargetRange(currentMarket, IID_Trader))
	{
		this.PerformTrade();
		if (this.MoveToMarket(nextMarket))
		{
			// We've started walking to the next market
			this.SetNextState(nextFsmStateName);
		}
	}
	else
	{
		// If the current market is not reached try again
		this.MoveToMarket(currentMarket);
	}
}

UnitAI.prototype.PerformTrade = function()
{
	var cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
	cmpTrader.PerformTrade();
}

UnitAI.prototype.StopTrading = function()
{
	this.FinishOrder();
	var cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
	cmpTrader.StopTrading();
}

UnitAI.prototype.Repair = function(target, autocontinue, queued)
{
	if (!this.CanRepair(target))
	{
		this.WalkToTarget(target, queued);
		return;
	}

	this.AddOrder("Repair", { "target": target, "autocontinue": autocontinue }, queued);
};

UnitAI.prototype.Flee = function(target, queued)
{
	this.AddOrder("Flee", { "target": target }, queued);
};

UnitAI.prototype.Cheer = function()
{
	this.AddOrder("Cheering", null, false);
};

UnitAI.prototype.SetStance = function(stance)
{
	if (g_Stances[stance])
		this.stance = stance;
	else
		error("UnitAI: Setting to invalid stance '"+stance+"'");
};

UnitAI.prototype.SwitchToStance = function(stance)
{
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return;
	var pos = cmpPosition.GetPosition();
	this.SetHeldPosition(pos.x, pos.z);

	this.SetStance(stance);
	// Stop moving if switching to stand ground
	// TODO: Also stop existing orders in a sensible way
	if (stance == "standground")
		this.StopMoving();

	// Reset the range query, since the range depends on stance
	this.SetupRangeQuery();
	// Just if we are a healer
	// TODO maybe move those two to a SetupRangeQuerys()
	if (this.IsHealer())
		this.SetupHealRangeQuery();
};

/**
 * Resets losRangeQuery, and if there are some targets in range that we can
 * attack then we start attacking and this returns true; otherwise, returns false.
 */
UnitAI.prototype.FindNewTargets = function()
{
	if (!this.losRangeQuery)
		return false;

	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var ents = rangeMan.ResetActiveQuery(this.losRangeQuery);

	if (!this.GetStance().targetVisibleEnemies)
		return false;

	SortEntitiesByPriority(ents);
	return this.RespondToTargetedEntities(ents);
};

/**
 * Resets losHealRangeQuery, and if there are some targets in range that we can heal
 * then we start healing and this returns true; otherwise, returns false.
 */
UnitAI.prototype.FindNewHealTargets = function()
{
	if (!this.losHealRangeQuery)
		return false;
	
	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var ents = rangeMan.ResetActiveQuery(this.losHealRangeQuery);
	
	for each (var ent in ents)
	{
		if (this.CanHeal(ent))
		{
			this.PushOrderFront("Heal", { "target": ent, "force": false });
			return true;
		}
	}
	// We haven't found any target to heal
	return false;
};

UnitAI.prototype.GetQueryRange = function(iid)
{
	var ret = { "min": 0, "max": 0 };
	if (this.GetStance().respondStandGround)
	{
		var cmpRanged = Engine.QueryInterface(this.entity, iid);
		if (!cmpRanged)
			return ret;
		var range = iid !== IID_Attack ? cmpRanged.GetRange() : cmpRanged.GetRange(cmpRanged.GetBestAttack());
		ret.min = range.min;
		ret.max = range.max;
	}
	else if (this.GetStance().respondChase)
	{
		var cmpVision = Engine.QueryInterface(this.entity, IID_Vision);
		if (!cmpVision)
			return ret;
		var range = cmpVision.GetRange();
		ret.max = range;
	}
	else if (this.GetStance().respondHoldGround)
	{
		var cmpRanged = Engine.QueryInterface(this.entity, iid);
		if (!cmpRanged)
			return ret;
		var range = iid !== IID_Attack ? cmpRanged.GetRange() : cmpRanged.GetRange(cmpRanged.GetBestAttack());
		var cmpVision = Engine.QueryInterface(this.entity, IID_Vision);
		if (!cmpVision)
			return ret;
		var halfvision = cmpVision.GetRange() / 2;
		ret.max = range.max + halfvision;
	}
	// We probably have stance 'passive' and we wouldn't have a range,
	// but as it is the default for healers we need to set it to something sane.
	else if (iid === IID_Heal)
	{
		var cmpVision = Engine.QueryInterface(this.entity, IID_Vision);
		if (!cmpVision)
			return ret;
		var range = cmpVision.GetRange();
		ret.max = range;
	}
	return ret;
};

UnitAI.prototype.GetStance = function()
{
	return g_Stances[this.stance];
};

UnitAI.prototype.GetStanceName = function()
{
	return this.stance;
};

//// Helper functions ////

UnitAI.prototype.CanAttack = function(target)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds)
	if (this.IsFormationController())
		return true;

	// Verify that we're able to respond to Attack commands
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return false;

	// Verify that the target is alive
	if (!this.TargetIsAlive(target))
		return false;

	// Verify that the target is owned by an enemy of this entity's player,
	//	or that it's an attackable resource supply like a domestic animal
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || (!this.MustKillGatherTarget(target) && !IsOwnedByEnemyOfPlayer(cmpOwnership.GetOwner(), target)))
		return false;

	return true;
};

UnitAI.prototype.CanGarrison = function(target)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds)
	if (this.IsFormationController())
		return true;

	var cmpGarrisonHolder = Engine.QueryInterface(target, IID_GarrisonHolder);
	if (!cmpGarrisonHolder)
		return false;

	// Verify that the target is owned by this entity's player
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || !IsOwnedByPlayer(cmpOwnership.GetOwner(), target))
		return false;

	// Don't let animals garrison for now
	// (If we want to support that, we'll need to change Order.Garrison so it
	// doesn't move the animal into an INVIDIDUAL.* state)
	if (this.IsAnimal())
		return false;

	return true;
};

UnitAI.prototype.CanGather = function(target)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds)
	if (this.IsFormationController())
		return true;

	// Verify that we're able to respond to Gather commands
	var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
	if (!cmpResourceGatherer)
		return false;

	// Verify that we can gather from this target
	if (!cmpResourceGatherer.GetTargetGatherRate(target))
		return false;

	// No need to verify ownership as we should be able to gather from
	// a target regardless of ownership.

	return true;
};

UnitAI.prototype.CanHeal = function(target)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds)
	if (this.IsFormationController())
		return true;

	// Verify that we're able to respond to Heal commands
	var cmpHeal = Engine.QueryInterface(this.entity, IID_Heal);
	if (!cmpHeal)
		return false;

	// Verify that the target is alive
	if (!this.TargetIsAlive(target))
		return false;

	// Verify that the target is owned by the same player as the entity or of an ally
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || !(IsOwnedByPlayer(cmpOwnership.GetOwner(), target) || IsOwnedByAllyOfPlayer(cmpOwnership.GetOwner(), target)))
		return false;

	// Verify that the target is not unhealable (or at max health)
	var cmpHealth = Engine.QueryInterface(target, IID_Health);
	if (!cmpHealth || cmpHealth.IsUnhealable())
		return false;

	// Verify that the target has no unhealable class
	var cmpIdentity = Engine.QueryInterface(target, IID_Identity);
	if (!cmpIdentity)
		return false;
	for each (var unhealableClass in cmpHeal.GetUnhealableClasses())
	{
		if (cmpIdentity.HasClass(unhealableClass) != -1)
		{
			return false;
		}
	}

	// Verify that the target is a healable class
	var healable = false;
	for each (var healableClass in cmpHeal.GetHealableClasses())
	{
		if (cmpIdentity.HasClass(healableClass) != -1)
		{
			healable = true;
		}
	}
	if (!healable)
		return false;

	return true;
};

UnitAI.prototype.CanReturnResource = function(target, checkCarriedResource)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds)
	if (this.IsFormationController())
		return true;

	// Verify that we're able to respond to ReturnResource commands
	var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
	if (!cmpResourceGatherer)
		return false;

	// Verify that the target is a dropsite
	var cmpResourceDropsite = Engine.QueryInterface(target, IID_ResourceDropsite);
	if (!cmpResourceDropsite)
		return false;

	if (checkCarriedResource)
	{
		// Verify that we are carrying some resources,
		// and can return our current resource to this target
		var type = cmpResourceGatherer.GetMainCarryingType();
		if (!type || !cmpResourceDropsite.AcceptsType(type))
			return false;
	}

	// Verify that the dropsite is owned by this entity's player
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || !IsOwnedByPlayer(cmpOwnership.GetOwner(), target))
		return false;

	return true;
};

UnitAI.prototype.CanTrade = function(target)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds)
	if (this.IsFormationController())
		return true;

	// Verify that we're able to respond to Trade commands
	var cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
	if (!cmpTrader || !cmpTrader.CanTrade(target))
		return false;

	return true;
}

UnitAI.prototype.CanRepair = function(target)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds)
	if (this.IsFormationController())
		return true;

	// Verify that we're able to respond to Repair (Builder) commands
	var cmpBuilder = Engine.QueryInterface(this.entity, IID_Builder);
	if (!cmpBuilder)
		return false;

	// Verify that the target is owned by an ally of this entity's player
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || !IsOwnedByAllyOfPlayer(cmpOwnership.GetOwner(), target))
		return false;

	return true;
};

//// Animal specific functions ////

UnitAI.prototype.MoveRandomly = function(distance)
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

UnitAI.prototype.SetMoveSpeed = function(speed)
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	cmpMotion.SetSpeed(speed);
};

UnitAI.prototype.SetHeldPosition = function(x, z)
{
	this.heldPosition = {"x": x, "z": z};
};

UnitAI.prototype.GetHeldPosition = function(pos)
{
	return this.heldPosition;
};

UnitAI.prototype.WalkToHeldPosition = function()
{
	if (this.heldPosition)
	{
		this.Walk(this.heldPosition.x, this.heldPosition.z, false);
		return true;
	}
	return false;
};

Engine.RegisterComponentType(IID_UnitAI, "UnitAI", UnitAI);
