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
			"<value>skittish</value>" +
			"<value>passive-defensive</value>" +
		"</choice>" +
	"</element>" +
	"<element name='FormationController'>" +
		"<data type='boolean'/>" +
	"</element>" +
	"<element name='FleeDistance'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<optional>" +
		"<element name='Formations' a:help='Optional list of space-separated formations this unit is allowed to use. Choices include: Scatter, Box, ColumnClosed, LineClosed, ColumnOpen, LineOpen, Flank, Skirmish, Wedge, Testudo, Phalanx, Syntagma, BattleLine.'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<element name='CanGuard'>" +
		"<data type='boolean'/>" +
	"</element>" +
	"<element name='CanPatrol'>" +
		"<data type='boolean'/>" +
	"</element>" +
	"<element name='PatrolWaitTime' a:help='Number of seconds to wait in between patrol waypoints.'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>" +
	"<optional>" +
		"<element name='CheeringTime'>" +
			"<data type='nonNegativeInteger'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<interleave>" +
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
//   targetAttackersAlways: anything that hurts us is a viable target,
//     possibly overriding user orders!
// There are some response options, triggered when targets are detected:
//   respondFlee: run away
//   respondFleeOnSight: run away when an enemy is sighted
//   respondChase: start chasing after the enemy
//   respondChaseBeyondVision: start chasing, and don't stop even if it's out
//     of this unit's vision range (though still visible to the player)
//   respondStandGround: attack enemy but don't move at all
//   respondHoldGround: attack enemy but don't move far from current position
// TODO: maybe add targetAggressiveEnemies (don't worry about lone scouts,
// do worry around armies slaughtering the guy standing next to you), etc.
var g_Stances = {
	"violent": {
		"targetVisibleEnemies": true,
		"targetAttackersAlways": true,
		"respondFlee": false,
		"respondFleeOnSight": false,
		"respondChase": true,
		"respondChaseBeyondVision": true,
		"respondStandGround": false,
		"respondHoldGround": false,
		"selectable": true
	},
	"aggressive": {
		"targetVisibleEnemies": true,
		"targetAttackersAlways": false,
		"respondFlee": false,
		"respondFleeOnSight": false,
		"respondChase": true,
		"respondChaseBeyondVision": false,
		"respondStandGround": false,
		"respondHoldGround": false,
		"selectable": true
	},
	"defensive": {
		"targetVisibleEnemies": true,
		"targetAttackersAlways": false,
		"respondFlee": false,
		"respondFleeOnSight": false,
		"respondChase": false,
		"respondChaseBeyondVision": false,
		"respondStandGround": false,
		"respondHoldGround": true,
		"selectable": true
	},
	"passive": {
		"targetVisibleEnemies": false,
		"targetAttackersAlways": false,
		"respondFlee": true,
		"respondFleeOnSight": false,
		"respondChase": false,
		"respondChaseBeyondVision": false,
		"respondStandGround": false,
		"respondHoldGround": false,
		"selectable": true
	},
	"standground": {
		"targetVisibleEnemies": true,
		"targetAttackersAlways": false,
		"respondFlee": false,
		"respondFleeOnSight": false,
		"respondChase": false,
		"respondChaseBeyondVision": false,
		"respondStandGround": true,
		"respondHoldGround": false,
		"selectable": true
	},
	"skittish": {
		"targetVisibleEnemies": false,
		"targetAttackersAlways": false,
		"respondFlee": true,
		"respondFleeOnSight": true,
		"respondChase": false,
		"respondChaseBeyondVision": false,
		"respondStandGround": false,
		"respondHoldGround": false,
		"selectable": false
	},
	"passive-defensive": {
		"targetVisibleEnemies": false,
		"targetAttackersAlways": false,
		"respondFlee": false,
		"respondFleeOnSight": false,
		"respondChase": false,
		"respondChaseBeyondVision": false,
		"respondStandGround": false,
		"respondHoldGround": true,
		"selectable": false
	},
	"none": {
		// Only to be used by AI or trigger scripts
		"targetVisibleEnemies": false,
		"targetAttackersAlways": false,
		"respondFlee": false,
		"respondFleeOnSight": false,
		"respondChase": false,
		"respondChaseBeyondVision": false,
		"respondStandGround": false,
		"respondHoldGround": false,
		"selectable": false
	}
};

// These orders always require a packed unit, so if a unit that is unpacking is given one of these orders,
// it will immediately cancel unpacking.
var g_OrdersCancelUnpacking = new Set([
	"FormationWalk",
	"Walk",
	"WalkAndFight",
	"WalkToTarget",
	"Patrol",
	"Garrison"
]);

// When leaving a foundation, we want to be clear of it by this distance.
var g_LeaveFoundationRange = 4;

UnitAI.prototype.notifyToCheerInRange = 30;

UnitAI.prototype.DEFAULT_CAPTURE = false;

// To reject an order, use 'return this.FinishOrder();'
const ACCEPT_ORDER = true;

// See ../helpers/FSM.js for some documentation of this FSM specification syntax
UnitAI.prototype.UnitFsmSpec = {

	// Default event handlers:

	"MovementUpdate": function(msg) {
		// ignore spurious movement messages
		// (these can happen when stopping moving at the same time
		// as switching states)
	},

	"ConstructionFinished": function(msg) {
		// ignore uninteresting construction messages
	},

	"LosRangeUpdate": function(msg) {
		// Ignore newly-seen units by default.
	},

	"LosHealRangeUpdate": function(msg) {
		// Ignore newly-seen injured units by default.
	},

	"LosAttackRangeUpdate": function(msg) {
		// Ignore newly-seen enemy units by default.
	},

	"Attacked": function(msg) {
		// ignore attacker
	},

	"PackFinished": function(msg) {
		// ignore
	},

	"PickupCanceled": function(msg) {
		// ignore
	},

	"TradingCanceled": function(msg) {
		// ignore
	},

	"GuardedAttacked": function(msg) {
		// ignore
	},

	"OrderTargetRenamed": function() {
		// By default, trigger an exit-reenter
		// so that state preconditions are checked against the new entity
		// (there is no reason to assume the target is still valid).
		this.SetNextState(this.GetCurrentState());
	},

	// Formation handlers:

	"FormationLeave": function(msg) {
		// Overloaded by FORMATIONMEMBER
		// We end up here if LeaveFormation was called when the entity
		// was executing an order in an individual state, so we must
		// discard the order now that it has been executed.
		if (this.order && this.order.type === "LeaveFormation")
			this.FinishOrder();
	},

	// Called when being told to walk as part of a formation
	"Order.FormationWalk": function(msg) {
		if (!this.IsFormationMember() || !this.AbleToMove())
			return this.FinishOrder();

		if (this.CanPack())
		{
			// If the controller is IDLE, this is just the regular reformation timer.
			// In that case we don't actually want to move, as that would unpack us.
			let cmpControllerAI = Engine.QueryInterface(this.GetFormationController(), IID_UnitAI);
			if (cmpControllerAI.IsIdle())
				return this.FinishOrder();
			this.PushOrderFront("Pack", { "force": true });
		}
		else
			this.SetNextState("FORMATIONMEMBER.WALKING");
		return ACCEPT_ORDER;
	},

	// Special orders:
	// (these will be overridden by various states)

	"Order.LeaveFoundation": function(msg) {
		if (!this.WillMoveFromFoundation(msg.data.target))
			return this.FinishOrder();
		msg.data.min = g_LeaveFoundationRange;
		this.SetNextState("INDIVIDUAL.WALKING");
		return ACCEPT_ORDER;
	},

	// Individual orders:

	"Order.LeaveFormation": function() {
		if (!this.IsFormationMember())
			return this.FinishOrder();

		let cmpFormation = Engine.QueryInterface(this.formationController, IID_Formation);
		if (cmpFormation)
		{
			cmpFormation.SetRearrange(false);
			// Triggers FormationLeave, which ultimately will FinishOrder,
			// discarding this order.
			cmpFormation.RemoveMembers([this.entity]);
			cmpFormation.SetRearrange(true);
		}
		return ACCEPT_ORDER;
	},

	"Order.Stop": function(msg) {
		this.FinishOrder();
		return ACCEPT_ORDER;
	},

	"Order.Walk": function(msg) {
		if (!this.AbleToMove())
			return this.FinishOrder();

		if (this.CanPack())
		{
			this.PushOrderFront("Pack", { "force": true });
			return ACCEPT_ORDER;
		}

		this.SetHeldPosition(msg.data.x, msg.data.z);
		msg.data.relaxed = true;
		this.SetNextState("INDIVIDUAL.WALKING");
		return ACCEPT_ORDER;
	},

	"Order.WalkAndFight": function(msg) {
		if (!this.AbleToMove())
			return this.FinishOrder();

		if (this.CanPack())
		{
			this.PushOrderFront("Pack", { "force": true });
			return ACCEPT_ORDER;
		}

		this.SetHeldPosition(msg.data.x, msg.data.z);
		msg.data.relaxed = true;
		this.SetNextState("INDIVIDUAL.WALKINGANDFIGHTING");
		return ACCEPT_ORDER;
	},


	"Order.WalkToTarget": function(msg) {
		if (!this.AbleToMove())
			return this.FinishOrder();

		if (this.CanPack())
		{
			this.PushOrderFront("Pack", { "force": true });
			return ACCEPT_ORDER;
		}


		if (this.CheckRange(msg.data))
			return this.FinishOrder();

		msg.data.relaxed = true;
		this.SetNextState("INDIVIDUAL.WALKING");
		return ACCEPT_ORDER;
	},

	"Order.PickupUnit": function(msg) {
		let cmpHolder = Engine.QueryInterface(this.entity, msg.data.iid);
		if (!cmpHolder || cmpHolder.IsFull())
			return this.FinishOrder();

		let range = cmpHolder.LoadingRange();
		msg.data.min = range.min;
		msg.data.max = range.max;
		if (this.CheckRange(msg.data))
			return this.FinishOrder();

		// Check if we need to move
		// If the target can reach us and we are reasonably close, don't move.
		// TODO: it would be slightly more optimal to check for real, not bird-flight distance.
		let cmpPassengerMotion = Engine.QueryInterface(msg.data.target, IID_UnitMotion);
		if (cmpPassengerMotion &&
		        cmpPassengerMotion.IsTargetRangeReachable(this.entity, range.min, range.max) &&
		        PositionHelper.DistanceBetweenEntities(this.entity, msg.data.target) < 200)
			this.SetNextState("INDIVIDUAL.PICKUP.LOADING");
		else if (this.AbleToMove())
			this.SetNextState("INDIVIDUAL.PICKUP.APPROACHING");
		else
			return this.FinishOrder();
		return ACCEPT_ORDER;
	},

	"Order.Guard": function(msg) {
		if (!this.AddGuard(msg.data.target))
			return this.FinishOrder();

		if (this.CheckTargetRangeExplicit(this.isGuardOf, 0, this.guardRange))
			this.SetNextState("INDIVIDUAL.GUARD.GUARDING");
		else if (this.AbleToMove())
			this.SetNextState("INDIVIDUAL.GUARD.ESCORTING");
		else
			return this.FinishOrder();
		return ACCEPT_ORDER;
	},

	"Order.Flee": function(msg) {
		if (!this.AbleToMove())
			return this.FinishOrder();
		this.SetNextState("INDIVIDUAL.FLEEING");
		return ACCEPT_ORDER;
	},

	"Order.Attack": function(msg) {
		let type = this.GetBestAttackAgainst(msg.data.target, msg.data.allowCapture);
		if (!type)
			return this.FinishOrder();

		msg.data.attackType = type;

		this.RememberTargetPosition();
		if (msg.data.hunting && this.orderQueue.length > 1 && this.orderQueue[1].type === "Gather")
			this.RememberTargetPosition(this.orderQueue[1].data);

		if (this.CheckTargetAttackRange(msg.data.target, msg.data.attackType))
		{
			if (this.CanUnpack())
			{
				this.PushOrderFront("Unpack", { "force": true });
				return ACCEPT_ORDER;
			}

			// Cancel any current packing order.
			if (this.EnsureCorrectPackStateForAttack(false))
				this.SetNextState("INDIVIDUAL.COMBAT.ATTACKING");

			return ACCEPT_ORDER;
		}

		// If we're hunting, that's a special case where we should continue attacking our target.
		if (this.GetStance().respondStandGround && !msg.data.force && !msg.data.hunting || !this.AbleToMove())
			return this.FinishOrder();

		if (this.CanPack())
		{
			this.PushOrderFront("Pack", { "force": true });
			return ACCEPT_ORDER;
		}

		// If we're currently packing/unpacking, make sure we are packed, so we can move.
		if (this.EnsureCorrectPackStateForAttack(true))
			this.SetNextState("INDIVIDUAL.COMBAT.APPROACHING");
		return ACCEPT_ORDER;
	},

	"Order.Patrol": function(msg) {
		if (!this.AbleToMove())
			return this.FinishOrder();

		if (this.CanPack())
		{
			this.PushOrderFront("Pack", { "force": true });
			return ACCEPT_ORDER;
		}

		msg.data.relaxed = true;

		this.SetNextState("INDIVIDUAL.PATROL.PATROLLING");
		return ACCEPT_ORDER;
	},

	"Order.Heal": function(msg) {
		if (!this.TargetIsAlive(msg.data.target))
			return this.FinishOrder();

		// Healers can't heal themselves.
		if (msg.data.target == this.entity)
			return this.FinishOrder();

		if (this.CheckTargetRange(msg.data.target, IID_Heal))
		{
			this.SetNextState("INDIVIDUAL.HEAL.HEALING");
			return ACCEPT_ORDER;
		}
		if (!this.AbleToMove())
			return this.FinishOrder();

		if (this.GetStance().respondStandGround && !msg.data.force)
			return this.FinishOrder();

		this.SetNextState("INDIVIDUAL.HEAL.APPROACHING");
		return ACCEPT_ORDER;
	},

	"Order.Gather": function(msg) {
		let cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
		if (!cmpResourceGatherer)
			return this.FinishOrder();

		// We were given the order to gather while we were still gathering.
		// This is needed because we don't re-enter the GATHER-state.
		const taskedResourceType = cmpResourceGatherer.GetTaskedResourceType();
		if (taskedResourceType && msg.data.type.generic != taskedResourceType)
			this.UnitFsm.SwitchToNextState(this, "INDIVIDUAL.GATHER");

		if (!this.CanGather(msg.data.target))
		{
			this.SetNextState("INDIVIDUAL.GATHER.FINDINGNEWTARGET");
			return ACCEPT_ORDER;
		}

		if (this.MustKillGatherTarget(msg.data.target))
		{
			const bestAttack = Engine.QueryInterface(this.entity, IID_Attack)?.GetBestAttackAgainst(msg.data.target, false);
			// Make sure we can attack the target, else we'll get very stuck
			if (!bestAttack)
			{
				// Oops, we can't attack at all - give up
				// TODO: should do something so the player knows why this failed
				return this.FinishOrder();
			}
			// The target was visible when this order was issued,
			// but could now be invisible again.
			if (!this.CheckTargetVisible(msg.data.target))
			{
				if (msg.data.secondTry === undefined)
				{
					msg.data.secondTry = true;
					this.PushOrderFront("Walk", msg.data.lastPos);
				}
				// We couldn't move there, or the target moved away
				else if (!this.FinishOrder())
					this.PushOrderFront("GatherNearPosition", {
						"x": msg.data.lastPos.x,
						"z": msg.data.lastPos.z,
						"type": msg.data.type,
						"template": msg.data.template
					});
				return ACCEPT_ORDER;
			}

			if (!this.AbleToMove() && !this.CheckTargetRange(msg.data.target, IID_Attack, bestAttack))
				return this.FinishOrder();

			this.PushOrderFront("Attack", { "target": msg.data.target, "force": !!msg.data.force, "hunting": true });
			return ACCEPT_ORDER;
		}

		// If the unit is full go to the nearest dropsite instead of trying to gather.
		if (!cmpResourceGatherer.CanCarryMore(msg.data.type.generic))
		{
			this.SetNextState("INDIVIDUAL.GATHER.RETURNINGRESOURCE");
			return ACCEPT_ORDER;
		}

		this.RememberTargetPosition();
		if (!msg.data.initPos)
			msg.data.initPos = msg.data.lastPos;

		if (this.CheckTargetRange(msg.data.target, IID_ResourceGatherer))
			this.SetNextState("INDIVIDUAL.GATHER.GATHERING");
		else if (this.AbleToMove())
			this.SetNextState("INDIVIDUAL.GATHER.APPROACHING");
		else
			return this.FinishOrder();
		return ACCEPT_ORDER;
	},

	"Order.GatherNearPosition": function(msg) {
		if (!this.AbleToMove())
			return this.FinishOrder();
		this.SetNextState("INDIVIDUAL.GATHER.WALKING");
		msg.data.initPos = { 'x': msg.data.x, 'z': msg.data.z };
		msg.data.relaxed = true;
		return ACCEPT_ORDER;
	},

	"Order.DropAtNearestDropSite": function(msg) {
		const cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
		if (!cmpResourceGatherer)
			return this.FinishOrder();
		const nearby = this.FindNearestDropsite(cmpResourceGatherer.GetMainCarryingType());
		if (!nearby)
			return this.FinishOrder();
		this.ReturnResource(nearby, false, true);
		return ACCEPT_ORDER;
	},

	"Order.ReturnResource": function(msg) {
		if (this.CheckTargetRange(msg.data.target, IID_ResourceGatherer))
			this.SetNextState("INDIVIDUAL.RETURNRESOURCE.DROPPINGRESOURCES");
		else if (this.AbleToMove())
			this.SetNextState("INDIVIDUAL.RETURNRESOURCE.APPROACHING");
		else
			return this.FinishOrder();
		return ACCEPT_ORDER;
	},

	"Order.Trade": function(msg) {
		if (!this.AbleToMove())
			return this.FinishOrder();
		// We must check if this trader has both markets in case it was a back-to-work order.
		let cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
		if (!cmpTrader || !cmpTrader.HasBothMarkets())
			return this.FinishOrder();

		this.waypoints = [];
		this.SetNextState("TRADE.APPROACHINGMARKET");
		return ACCEPT_ORDER;
	},

	"Order.Repair": function(msg) {
		if (this.CheckTargetRange(msg.data.target, IID_Builder))
			this.SetNextState("INDIVIDUAL.REPAIR.REPAIRING");
		else if (this.AbleToMove())
			this.SetNextState("INDIVIDUAL.REPAIR.APPROACHING");
		else
			return this.FinishOrder();
		return ACCEPT_ORDER;
	},

	"Order.Garrison": function(msg) {
		if (!this.AbleToMove())
			return this.FinishOrder();

		// Also pack when we are in range.
		if (this.CanPack())
		{
			this.PushOrderFront("Pack", { "force": true });
			return ACCEPT_ORDER;
		}

		if (this.CheckTargetRange(msg.data.target, msg.data.garrison ? IID_Garrisonable : IID_Turretable))
			this.SetNextState("INDIVIDUAL.GARRISON.GARRISONING");
		else
			this.SetNextState("INDIVIDUAL.GARRISON.APPROACHING");
		return ACCEPT_ORDER;
	},

	"Order.Ungarrison": function(msg) {
		// Note that this order MUST succeed, or we break
		// the assumptions done in garrisonable/garrisonHolder,
		// especially in Unloading in the latter. (For user feedback.)
		// ToDo: This can be fixed by not making that assumption :)
		this.FinishOrder();
		return ACCEPT_ORDER;
	},

	"Order.Cheer": function(msg) {
		return this.FinishOrder();
	},

	"Order.Pack": function(msg) {
		if (!this.CanPack())
			return this.FinishOrder();
		this.SetNextState("INDIVIDUAL.PACKING");
		return ACCEPT_ORDER;
	},

	"Order.Unpack": function(msg) {
		if (!this.CanUnpack())
			return this.FinishOrder();
		this.SetNextState("INDIVIDUAL.UNPACKING");
		return ACCEPT_ORDER;
	},

	"Order.MoveToChasingPoint": function(msg) {
		// Overriden by the CHASING state.
		// Can however happen outside of it when renaming...
		// TODO: don't use an order for that behaviour.
		return this.FinishOrder();
	},

	"Order.CollectTreasure": function(msg) {
		if (this.CheckTargetRange(msg.data.target, IID_TreasureCollector))
			this.SetNextState("INDIVIDUAL.COLLECTTREASURE.COLLECTING");
		else if (this.AbleToMove())
			this.SetNextState("INDIVIDUAL.COLLECTTREASURE.APPROACHING");
		else
			return this.FinishOrder();

		return ACCEPT_ORDER;
	},

	"Order.CollectTreasureNearPosition": function(msg) {
		if (!this.AbleToMove())
			return this.FinishOrder();
		this.SetNextState("INDIVIDUAL.COLLECTTREASURE.WALKING");
		msg.data.initPos = { 'x': msg.data.x, 'z': msg.data.z };
		msg.data.relaxed = true;
		return ACCEPT_ORDER;
	},

	// States for the special entity representing a group of units moving in formation:
	"FORMATIONCONTROLLER": {

		"Order.Walk": function(msg) {
			if (!this.AbleToMove())
				return this.FinishOrder();
			this.CallMemberFunction("SetHeldPosition", [msg.data.x, msg.data.z]);
			this.SetNextState("WALKING");
			return ACCEPT_ORDER;
		},

		"Order.WalkAndFight": function(msg) {
			if (!this.AbleToMove())
				return this.FinishOrder();
			this.CallMemberFunction("SetHeldPosition", [msg.data.x, msg.data.z]);
			this.SetNextState("WALKINGANDFIGHTING");
			return ACCEPT_ORDER;
		},

		"Order.MoveIntoFormation": function(msg) {
			if (!this.AbleToMove())
				return this.FinishOrder();
			this.CallMemberFunction("SetHeldPosition", [msg.data.x, msg.data.z]);
			this.SetNextState("FORMING");
			return ACCEPT_ORDER;
		},

		// Only used by other orders to walk there in formation.
		"Order.WalkToTargetRange": function(msg) {
			if (this.CheckRange(msg.data))
				return this.FinishOrder();
			if (!this.AbleToMove())
				return this.FinishOrder();
			this.SetNextState("WALKING");
			return ACCEPT_ORDER;
		},

		"Order.WalkToTarget": function(msg) {
			if (this.CheckRange(msg.data))
				return this.FinishOrder();
			if (!this.AbleToMove())
				return this.FinishOrder();
			this.SetNextState("WALKING");
			return ACCEPT_ORDER;
		},

		"Order.WalkToPointRange": function(msg) {
			if (this.CheckRange(msg.data))
				return this.FinishOrder();
			if (!this.AbleToMove())
				return this.FinishOrder();
			this.SetNextState("WALKING");
			return ACCEPT_ORDER;
		},

		"Order.Patrol": function(msg) {
			if (!this.AbleToMove())
				return this.FinishOrder();
			this.CallMemberFunction("SetHeldPosition", [msg.data.x, msg.data.z]);
			this.SetNextState("PATROL.PATROLLING");
			return ACCEPT_ORDER;
		},

		"Order.Guard": function(msg) {
			this.CallMemberFunction("Guard", [msg.data.target, false]);
			Engine.QueryInterface(this.entity, IID_Formation).Disband();
			return ACCEPT_ORDER;
		},

		"Order.Stop": function(msg) {
			let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
			cmpFormation.ResetOrderVariant();
			if (!this.IsAttackingAsFormation())
				this.CallMemberFunction("Stop", [false]);
			this.FinishOrder();
			return ACCEPT_ORDER;
			// Don't move the members back into formation,
			// as the formation then resets and it looks odd when walk-stopping.
			// TODO: this should be improved in the formation reshaping code.
		},

		"Order.Attack": function(msg) {
			let target = msg.data.target;
			let cmpTargetUnitAI = Engine.QueryInterface(target, IID_UnitAI);
			if (cmpTargetUnitAI && cmpTargetUnitAI.IsFormationMember())
				target = cmpTargetUnitAI.GetFormationController();

			if (!this.CheckFormationTargetAttackRange(target))
			{
				if (this.AbleToMove() && this.CheckTargetVisible(target))
				{
					this.SetNextState("COMBAT.APPROACHING");
					return ACCEPT_ORDER;
				}
				return this.FinishOrder();
			}
			this.CallMemberFunction("Attack", [target, msg.data.allowCapture, false]);
			let cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
			if (cmpAttack && cmpAttack.CanAttackAsFormation())
				this.SetNextState("COMBAT.ATTACKING");
			else
				this.SetNextState("MEMBER");
			return ACCEPT_ORDER;
		},

		"Order.Garrison": function(msg) {
			if (!Engine.QueryInterface(msg.data.target,
				msg.data.garrison ? IID_GarrisonHolder : IID_TurretHolder))
				return this.FinishOrder();
			if (this.CheckTargetRange(msg.data.target, msg.data.garrison ? IID_Garrisonable : IID_Turretable))
			{
				if (!this.AbleToMove() || !this.CheckTargetVisible(msg.data.target))
					return this.FinishOrder();

				this.SetNextState("GARRISON.APPROACHING");
			}
			else
				this.SetNextState("GARRISON.GARRISONING");
			return ACCEPT_ORDER;
		},

		"Order.Gather": function(msg) {
			if (this.MustKillGatherTarget(msg.data.target))
			{
				// The target was visible when this order was given,
				// but could now be invisible.
				if (!this.CheckTargetVisible(msg.data.target))
				{
					if (msg.data.secondTry === undefined)
					{
						msg.data.secondTry = true;
						this.PushOrderFront("Walk", msg.data.lastPos);
					}
					// We couldn't move there, or the target moved away
					else
					{
						let data = msg.data;
						if (!this.FinishOrder())
							this.PushOrderFront("GatherNearPosition", {
								"x": data.lastPos.x,
								"z": data.lastPos.z,
								"type": data.type,
								"template": data.template
							});
					}
					return ACCEPT_ORDER;
				}
				this.PushOrderFront("Attack", { "target": msg.data.target, "force": !!msg.data.force, "hunting": true, "min": 0, "max": 10 });
				return ACCEPT_ORDER;
			}

			// TODO: on what should we base this range?
			if (!this.CheckTargetRangeExplicit(msg.data.target, 0, 10))
			{
				if (!this.CanGather(msg.data.target) || !this.CheckTargetVisible(msg.data.target))
					return this.FinishOrder();
				// TODO: Should we issue a gather-near-position order
				// if the target isn't gatherable/doesn't exist anymore?
				if (!msg.data.secondTry)
				{
					msg.data.secondTry = true;
					this.PushOrderFront("WalkToTargetRange", { "target": msg.data.target, "min": 0, "max": 10 });
					return ACCEPT_ORDER;
				}
				return this.FinishOrder();
			}

			this.CallMemberFunction("Gather", [msg.data.target, false]);

			this.SetNextState("MEMBER");
			return ACCEPT_ORDER;
		},

		"Order.GatherNearPosition": function(msg) {
			// TODO: on what should we base this range?
			if (!this.CheckPointRangeExplicit(msg.data.x, msg.data.z, 0, 20))
			{
				// Out of range; move there in formation
				this.PushOrderFront("WalkToPointRange", { "x": msg.data.x, "z": msg.data.z, "min": 0, "max": 20 });
				return ACCEPT_ORDER;
			}

			this.CallMemberFunction("GatherNearPosition", [msg.data.x, msg.data.z, msg.data.type, msg.data.template, false]);

			this.SetNextState("MEMBER");
			return ACCEPT_ORDER;
		},

		"Order.Heal": function(msg) {
			// TODO: on what should we base this range?
			if (!this.CheckTargetRangeExplicit(msg.data.target, 0, 10))
			{
				if (!this.TargetIsAlive(msg.data.target) || !this.CheckTargetVisible(msg.data.target))
					return this.FinishOrder();

				if (!msg.data.secondTry)
				{
					msg.data.secondTry = true;
					this.PushOrderFront("WalkToTargetRange", { "target": msg.data.target, "min": 0, "max": 10 });
					return ACCEPT_ORDER;
				}
				return this.FinishOrder();
			}

			this.CallMemberFunction("Heal", [msg.data.target, false]);

			this.SetNextState("MEMBER");
			return ACCEPT_ORDER;
		},

		"Order.CollectTreasure": function(msg) {
			// TODO: on what should we base this range?
			if (this.CheckTargetRangeExplicit(msg.data.target, 0, 20))
			{
				this.CallMemberFunction("CollectTreasure", [msg.data.target, false, false]);
				this.SetNextState("MEMBER");

				return ACCEPT_ORDER;
			}
			if (msg.data.secondTry || !this.CheckTargetVisible(msg.data.target))
				return this.FinishOrder();

			msg.data.secondTry = true;
			this.PushOrderFront("WalkToTargetRange", { "target": msg.data.target, "min": 0, "max": 20 });
			return ACCEPT_ORDER;
		},

		"Order.CollectTreasureNearPosition": function(msg) {
			// TODO: on what should we base this range?
			if (!this.CheckPointRangeExplicit(msg.data.x, msg.data.z, 0, 20))
			{
				this.PushOrderFront("WalkToPointRange", { "x": msg.data.x, "z": msg.data.z, "min": 0, "max": 20 });
				return ACCEPT_ORDER;
			}

			this.CallMemberFunction("CollectTreasureNearPosition", [msg.data.x, msg.data.z, false, false]);
			this.SetNextState("MEMBER");
			return ACCEPT_ORDER;
		},

		"Order.Repair": function(msg) {
			// TODO: on what should we base this range?
			if (!this.CheckTargetRangeExplicit(msg.data.target, 0, 10))
			{
				if (!this.TargetIsAlive(msg.data.target) || !this.CheckTargetVisible(msg.data.target))
					return this.FinishOrder();

				if (!msg.data.secondTry)
				{
					msg.data.secondTry = true;
					this.PushOrderFront("WalkToTargetRange", { "target": msg.data.target, "min": 0, "max": 10 });
					return ACCEPT_ORDER;
				}
				return this.FinishOrder();
			}

			this.CallMemberFunction("Repair", [msg.data.target, msg.data.autocontinue, false]);

			this.SetNextState("MEMBER");
			return ACCEPT_ORDER;
		},

		"Order.ReturnResource": function(msg) {
			// TODO: on what should we base this range?
			if (!this.CheckTargetRangeExplicit(msg.data.target, 0, 10))
			{
				if (!this.CheckTargetVisible(msg.data.target))
					return this.FinishOrder();

				if (!msg.data.secondTry)
				{
					msg.data.secondTry = true;
					this.PushOrderFront("WalkToTargetRange", { "target": msg.data.target, "min": 0, "max": 10 });
					return ACCEPT_ORDER;
				}
				return this.FinishOrder();
			}

			this.CallMemberFunction("ReturnResource", [msg.data.target, false]);

			this.SetNextState("MEMBER");
			return ACCEPT_ORDER;
		},

		"Order.Pack": function(msg) {
			this.CallMemberFunction("Pack", [false]);

			this.SetNextState("MEMBER");
			return ACCEPT_ORDER;
		},

		"Order.Unpack": function(msg) {
			this.CallMemberFunction("Unpack", [false]);

			this.SetNextState("MEMBER");
			return ACCEPT_ORDER;
		},

		"Order.DropAtNearestDropSite": function(msg) {
			this.CallMemberFunction("DropAtNearestDropSite", [false, false]);

			this.SetNextState("MEMBER");
			return ACCEPT_ORDER;
		},

		"IDLE": {
			"enter": function(msg) {
				// Turn rearrange off. Otherwise, if the formation is idle
				// but individual units go off to fight,
				// any death will rearrange the formation, which looks odd.
				// Instead, move idle units in formation on a timer.
				let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.SetRearrange(false);
				// Start the timer on the next turn to catch up with potential stragglers.
				this.StartTimer(100, 2000);
				this.isIdle = true;
				this.CallMemberFunction("ResetIdle");
				return false;
			},

			"leave": function() {
				this.isIdle = false;
				this.StopTimer();
			},

			"Timer": function(msg) {
				let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				if (!cmpFormation)
					return;

				if (this.TestAllMemberFunction("IsIdle"))
					cmpFormation.MoveMembersIntoFormation(false, false);
			},

		},

		"WALKING": {
			"enter": function() {
				let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.SetRearrange(true);
				cmpFormation.MoveMembersIntoFormation(true, true);
				if (!this.MoveTo(this.order.data))
				{
					this.FinishOrder();
					return true;
				}
				return false;
			},

			"leave": function() {
				this.StopTimer();
				this.StopMoving();
			},

			"MovementUpdate": function(msg) {
				if (msg.veryObstructed && !this.timer)
				{
					// It's possible that the controller (with large clearance)
					// is stuck, but not the individual units.
					// Ask them to move individually for a little while.
					this.CallMemberFunction("MoveTo", [this.order.data]);
					this.StartTimer(3000);
					return;
				}
				else if (this.timer)
					return;
				if (msg.likelyFailure || this.CheckRange(this.order.data))
					this.FinishOrder();
			},

			"Timer": function() {
				// Reenter to reset the pathfinder state.
				this.SetNextState("WALKING");
			}
		},

		"WALKINGANDFIGHTING": {
			"enter": function(msg) {
				let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.SetRearrange(true);
				cmpFormation.MoveMembersIntoFormation(true, true, "combat");
				if (!this.MoveTo(this.order.data))
				{
					this.FinishOrder();
					return true;
				}
				this.StartTimer(0, 1000);
				this.order.data.returningState = "WALKINGANDFIGHTING";
				return false;
			},

			"leave": function() {
				this.StopMoving();
				this.StopTimer();
			},

			"Timer": function(msg) {
				Engine.ProfileStart("FindWalkAndFightTargets");
				if (this.FindWalkAndFightTargets())
					this.SetNextState("MEMBER");

				Engine.ProfileStop();
			},

			"MovementUpdate": function(msg) {
				if (msg.likelyFailure || this.CheckRange(this.order.data))
					this.FinishOrder();
			},
		},

		"PATROL": {
			"enter": function() {
				let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
				if (!cmpPosition || !cmpPosition.IsInWorld())
				{
					this.FinishOrder();
					return true;
				}
				// Memorize the origin position in case that we want to go back.
				if (!this.patrolStartPosOrder)
				{
					this.patrolStartPosOrder = cmpPosition.GetPosition();
					this.patrolStartPosOrder.targetClasses = this.order.data.targetClasses;
					this.patrolStartPosOrder.allowCapture = this.order.data.allowCapture;
				}

				this.SetAnimationVariant("combat");

				return false;
			},

			"leave": function() {
				delete this.patrolStartPosOrder;
				this.SetDefaultAnimationVariant();
			},

			"PATROLLING": {
				"enter": function() {
					let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
					cmpFormation.SetRearrange(true);
					cmpFormation.MoveMembersIntoFormation(true, true, "combat");

					let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
					if (!cmpPosition || !cmpPosition.IsInWorld() ||
					    !this.MoveTo(this.order.data))
					{
						this.FinishOrder();
						return true;
					}

					this.StartTimer(0, 1000);
					this.order.data.returningState = "PATROL.PATROLLING";
					return false;
				},

				"leave": function() {
					this.StopMoving();
					this.StopTimer();
				},

				"Timer": function(msg) {
					if (this.FindWalkAndFightTargets())
						this.SetNextState("MEMBER");
				},

				"MovementUpdate": function(msg) {
					if (!msg.likelyFailure && !msg.likelySuccess && !this.RelaxedMaxRangeCheck(this.order.data, this.DefaultRelaxedMaxRange))
						return;

					if (this.orderQueue.length == 1)
						this.PushOrder("Patrol", this.patrolStartPosOrder);

					this.PushOrder(this.order.type, this.order.data);
					this.SetNextState("CHECKINGWAYPOINT");
				},
			},

			"CHECKINGWAYPOINT": {
				"enter": function() {
					this.StartTimer(0, 1000);
					this.stopSurveying = 0;
					// TODO: pick a proper animation
					return false;
				},

				"leave": function() {
					this.StopTimer();
					delete this.stopSurveying;
				},

				"Timer": function(msg) {
					if (this.stopSurveying >= +this.template.PatrolWaitTime)
					{
						this.FinishOrder();
						return;
					}
					if (this.FindWalkAndFightTargets())
						this.SetNextState("MEMBER");
					else
						++this.stopSurveying;
				}
			}
		},

		"GARRISON": {
			"APPROACHING": {
				"enter": function() {
					if (!this.MoveToTargetRange(this.order.data.target, this.order.data.garrison ? IID_Garrisonable : IID_Turretable))
					{
						this.FinishOrder();
						return true;
					}

					let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
					cmpFormation.SetRearrange(true);
					cmpFormation.MoveMembersIntoFormation(true, true);

					// If the holder should pickup, warn it so it can take needed action.
					let cmpHolder = Engine.QueryInterface(this.order.data.target, this.order.data.garrison ? IID_GarrisonHolder : IID_TurretHolder);
					if (cmpHolder && cmpHolder.CanPickup(this.entity))
					{
						this.pickup = this.order.data.target;       // temporary, deleted in "leave"
						Engine.PostMessage(this.pickup, MT_PickupRequested, { "entity": this.entity, "iid": this.order.data.garrison ? IID_GarrisonHolder : IID_TurretHolder });
					}
					return false;
				},

				"leave": function() {
					this.StopMoving();
					if (this.pickup)
					{
						Engine.PostMessage(this.pickup, MT_PickupCanceled, { "entity": this.entity });
						delete this.pickup;
					}
				},

				"MovementUpdate": function(msg) {
					if (msg.likelyFailure || msg.likelySuccess)
						this.SetNextState("GARRISONING");
				},
			},

			"GARRISONING": {
				"enter": function() {
					this.CallMemberFunction(this.order.data.garrison ? "Garrison" : "OccupyTurret", [this.order.data.target, false]);
					// We might have been disbanded due to the lack of members.
					if (Engine.QueryInterface(this.entity, IID_Formation).GetMemberCount())
						this.SetNextState("MEMBER");
					return true;
				},
			},
		},

		"FORMING": {
			"enter": function() {
				let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.SetRearrange(true);
				cmpFormation.MoveMembersIntoFormation(true, true);

				if (!this.MoveTo(this.order.data))
				{
					this.FinishOrder();
					return true;
				}
				return false;
			},

			"leave": function() {
				this.StopMoving();
			},

			"MovementUpdate": function(msg) {
				if (!msg.likelyFailure && !this.CheckRange(this.order.data))
					return;

				this.FinishOrder();
			}
		},

		"COMBAT": {
			"APPROACHING": {
				"enter": function() {
					let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
					cmpFormation.SetRearrange(true);
					cmpFormation.MoveMembersIntoFormation(true, true, "combat");

					if (!this.MoveFormationToTargetAttackRange(this.order.data.target))
					{
						this.FinishOrder();
						return true;
					}
					return false;
				},

				"leave": function() {
					this.StopMoving();
				},

				"MovementUpdate": function(msg) {
					let target = this.order.data.target;
					let cmpTargetUnitAI = Engine.QueryInterface(target, IID_UnitAI);
					if (cmpTargetUnitAI && cmpTargetUnitAI.IsFormationMember())
						target = cmpTargetUnitAI.GetFormationController();
					let cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
					this.CallMemberFunction("Attack", [target, this.order.data.allowCapture, false]);
					if (cmpAttack.CanAttackAsFormation())
						this.SetNextState("COMBAT.ATTACKING");
					else
						this.SetNextState("MEMBER");
				},
			},

			"ATTACKING": {
				// Wait for individual members to finish
				"enter": function(msg) {
					const target = this.order.data.target;
					if (!this.CheckFormationTargetAttackRange(target))
					{
						if (this.CanAttack(target) && this.CheckTargetVisible(target))
						{
							this.SetNextState("COMBAT.APPROACHING");
							return true;
						}
						this.FinishOrder();
						return true;
					}

					let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
					// TODO fix the rearranging while attacking as formation
					cmpFormation.SetRearrange(!this.IsAttackingAsFormation());
					cmpFormation.MoveMembersIntoFormation(false, false, "combat");
					this.StartTimer(200, 200);
					return false;
				},

				"Timer": function(msg) {
					const target = this.order.data.target;
					if (!this.CheckFormationTargetAttackRange(target))
					{
						if (this.CanAttack(target) && this.CheckTargetVisible(target))
						{
							this.SetNextState("COMBAT.APPROACHING");
							return;
						}
						this.FinishOrder();
						return;
					}
				},

				"leave": function(msg) {
					this.StopTimer();
					var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
					if (cmpFormation)
						cmpFormation.SetRearrange(true);
				},
			},
		},

		// Wait for individual members to finish
		"MEMBER": {
			"OrderTargetRenamed": function(msg) {
				// In general, don't react - we don't want to send spurious messages to members.
				// This looks odd for hunting however because we wait for all
				// entities to have clumped around the dead resource before proceeding
				// so explicitly handle this case.
				if (this.order && this.order.data && this.order.data.hunting &&
				     this.order.data.target == msg.data.newentity &&
				     this.orderQueue.length > 1)
					this.FinishOrder();
			},

			"enter": function(msg) {
				// Don't rearrange the formation, as that forces all units to stop
				// what they're doing.
				let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				if (cmpFormation)
					cmpFormation.SetRearrange(false);
				// While waiting on members, the formation is more like
				// a group of unit and does not have a well-defined position,
				// so move the controller out of the world to enforce that.
				let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
				if (cmpPosition && cmpPosition.IsInWorld())
					cmpPosition.MoveOutOfWorld();

				this.StartTimer(1000, 1000);
				return false;
			},

			"Timer": function(msg) {
				let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				if (cmpFormation && !cmpFormation.AreAllMembersFinished())
					return;

				if (this.order?.data?.returningState)
					this.SetNextState(this.order.data.returningState);
				else
					this.FinishOrder();
			},

			"leave": function(msg) {
				this.StopTimer();
				// Reform entirely as members might be all over the place now.
				let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				if (cmpFormation && (cmpFormation.AreAllMembersIdle() || this.orderQueue.length))
					cmpFormation.MoveMembersIntoFormation(true);

				// Update the held position so entities respond to orders.
				let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
				if (cmpPosition && cmpPosition.IsInWorld())
				{
					let pos = cmpPosition.GetPosition2D();
					this.CallMemberFunction("SetHeldPosition", [pos.x, pos.y]);
				}
			},
		},
	},


	// States for entities moving as part of a formation:
	"FORMATIONMEMBER": {
		"FormationLeave": function(msg) {
			// Stop moving as soon as the formation disbands
			// Keep current rotation
			let facePointAfterMove = this.GetFacePointAfterMove();
			this.SetFacePointAfterMove(false);
			this.StopMoving();
			this.SetFacePointAfterMove(facePointAfterMove);

			// If the controller handled an order but some members rejected it,
			// they will have no orders and be in the FORMATIONMEMBER.IDLE state.
			if (this.orderQueue.length)
			{
				// We're leaving the formation, so stop our FormationWalk order
				if (this.FinishOrder())
					return;
			}

			this.formationAnimationVariant = undefined;
			this.SetNextState("INDIVIDUAL.IDLE");
		},

		// Override the LeaveFoundation order since we're not doing
		// anything more important (and we might be stuck in the WALKING
		// state forever and need to get out of foundations in that case)
		"Order.LeaveFoundation": function(msg) {
			if (!this.WillMoveFromFoundation(msg.data.target))
				return this.FinishOrder();
			msg.data.min = g_LeaveFoundationRange;
			this.SetNextState("WALKINGTOPOINT");
			return ACCEPT_ORDER;
		},

		"enter": function() {
			let cmpFormation = Engine.QueryInterface(this.formationController, IID_Formation);
			if (cmpFormation)
			{
				this.formationAnimationVariant = cmpFormation.GetFormationAnimationVariant(this.entity);
				if (this.formationAnimationVariant)
					this.SetAnimationVariant(this.formationAnimationVariant);
				else
					this.SetDefaultAnimationVariant();
			}
			return false;
		},

		"leave": function() {
			this.SetDefaultAnimationVariant();
			this.formationAnimationVariant = undefined;
		},

		"IDLE": "INDIVIDUAL.IDLE",

		"CHEERING": "INDIVIDUAL.CHEERING",

		"WALKING": {
			"enter": function() {
				let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
				cmpUnitMotion.MoveToFormationOffset(this.order.data.target, this.order.data.x, this.order.data.z);
				if (this.order.data.offsetsChanged)
				{
					let cmpFormation = Engine.QueryInterface(this.formationController, IID_Formation);
					if (cmpFormation)
						this.formationAnimationVariant = cmpFormation.GetFormationAnimationVariant(this.entity);
				}
				if (this.formationAnimationVariant)
					this.SetAnimationVariant(this.formationAnimationVariant);
				else if (this.order.data.variant)
					this.SetAnimationVariant(this.order.data.variant);
				else
					this.SetDefaultAnimationVariant();
				return false;
			},

			"leave": function() {
				// Don't use the logic from unitMotion, as SetInPosition
				// has already given us a custom rotation
				// (or we failed to move and thus don't care.)
				let facePointAfterMove = this.GetFacePointAfterMove();
				this.SetFacePointAfterMove(false);
				this.StopMoving();
				this.SetFacePointAfterMove(facePointAfterMove);
			},

			// Occurs when the unit has reached its destination and the controller
			// is done moving. The controller is notified.
			"MovementUpdate": function(msg) {
				// When walking in formation, we'll only get notified in case of failure
				// if the formation controller has stopped walking.
				// Formations can start lagging a lot if many entities request short path
				// so prefer to finish order early than retry pathing.
				// (see https://code.wildfiregames.com/rP23806)
				// (if the message is likelyFailure of likelySuccess, we also want to stop).
				this.FinishOrder();
			},
		},

		// Special case used by Order.LeaveFoundation
		"WALKINGTOPOINT": {
			"enter": function() {
				if (!this.MoveTo(this.order.data))
				{
					this.FinishOrder();
					return true;
				}
				return false;
			},

			"leave": function() {
				this.StopMoving();
			},

			"MovementUpdate": function() {
				if (!this.CheckRange(this.order.data))
					return;
				this.FinishOrder();
			},
		},
	},


	// States for entities not part of a formation:
	"INDIVIDUAL": {
		"Attacked": function(msg) {
			if (this.GetStance().targetAttackersAlways || !this.order || !this.order.data || !this.order.data.force)
				this.RespondToTargetedEntities([msg.data.attacker]);
		},

		"GuardedAttacked": function(msg) {
			// do nothing if we have a forced order in queue before the guard order
			for (var i = 0; i < this.orderQueue.length; ++i)
			{
				if (this.orderQueue[i].type == "Guard")
					break;
				if (this.orderQueue[i].data && this.orderQueue[i].data.force)
					return;
			}
			// if we already are targeting another unit still alive, finish with it first
			if (this.order && (this.order.type == "WalkAndFight" || this.order.type == "Attack"))
				if (this.order.data.target != msg.data.attacker && this.CanAttack(msg.data.attacker))
					return;

			var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
			var cmpHealth = Engine.QueryInterface(this.isGuardOf, IID_Health);
			if (cmpIdentity && cmpIdentity.HasClass("Support") &&
			    cmpHealth && cmpHealth.IsInjured())
			{
				if (this.CanHeal(this.isGuardOf))
					this.PushOrderFront("Heal", { "target": this.isGuardOf, "force": false });
				else if (this.CanRepair(this.isGuardOf))
					this.PushOrderFront("Repair", { "target": this.isGuardOf, "autocontinue": false, "force": false });
				return;
			}

			var cmpBuildingAI = Engine.QueryInterface(msg.data.attacker, IID_BuildingAI);
			if (cmpBuildingAI && this.CanRepair(this.isGuardOf))
			{
				this.PushOrderFront("Repair", { "target": this.isGuardOf, "autocontinue": false, "force": false });
				return;
			}

			if (this.CheckTargetVisible(msg.data.attacker))
				this.PushOrderFront("Attack", { "target": msg.data.attacker, "force": false });
			else
			{
				var cmpPosition = Engine.QueryInterface(msg.data.attacker, IID_Position);
				if (!cmpPosition || !cmpPosition.IsInWorld())
					return;
				var pos = cmpPosition.GetPosition();
				this.PushOrderFront("WalkAndFight", { "x": pos.x, "z": pos.z, "target": msg.data.attacker, "force": false });
				// if we already had a WalkAndFight, keep only the most recent one in case the target has moved
				if (this.orderQueue[1] && this.orderQueue[1].type == "WalkAndFight")
				{
					this.orderQueue.splice(1, 1);
					Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
				}
			}
		},

		"IDLE": {
			"Order.Cheer": function() {
				// Do not cheer if there is no cheering time and we are not idle yet.
				if (!this.cheeringTime || !this.isIdle)
					return this.FinishOrder();

				this.SetNextState("CHEERING");
				return ACCEPT_ORDER;
			},

			"enter": function() {
				// Switch back to idle animation to guarantee we won't
				// get stuck with an incorrect animation
				this.SelectAnimation("idle");

				// Idle is the default state. If units try, from the IDLE.enter sub-state, to
				// begin another order, and that order fails (calling FinishOrder), they might
				// end up in an infinite loop. To avoid this, all methods that could put the unit in
				// a new state are done on the next turn.
				// This wastes a turn but avoids infinite loops.
				// Further, the GUI and AI want to know when a unit is idle,
				// but sending this info in Idle.enter will send spurious messages.
				// Pick 100 to execute on the next turn in SP and MP.
				this.StartTimer(100);
				return false;
			},

			"leave": function() {
				let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
				if (this.losRangeQuery)
					cmpRangeManager.DisableActiveQuery(this.losRangeQuery);
				if (this.losHealRangeQuery)
					cmpRangeManager.DisableActiveQuery(this.losHealRangeQuery);
				if (this.losAttackRangeQuery)
					cmpRangeManager.DisableActiveQuery(this.losAttackRangeQuery);

				this.StopTimer();

				if (this.isIdle)
				{
					if (this.IsFormationMember())
						Engine.QueryInterface(this.formationController, IID_Formation).UnsetIdleEntity(this.entity);
					this.isIdle = false;
					Engine.PostMessage(this.entity, MT_UnitIdleChanged, { "idle": this.isIdle });
				}
			},

			"Attacked": function(msg) {
				if (this.isIdle && (this.GetStance().targetAttackersAlways || !this.order || !this.order.data || !this.order.data.force))
					this.RespondToTargetedEntities([msg.data.attacker]);
			},

			// On the range updates:
			// We check for idleness to prevent an entity to react only to newly seen entities
			// when receiving a Los*RangeUpdate on the same turn as the entity becomes idle
			// since this.FindNew*Targets is called in the timer.

			"LosRangeUpdate": function(msg) {
				if (this.isIdle && msg && msg.data && msg.data.added && msg.data.added.length)
					this.RespondToSightedEntities(msg.data.added);
			},

			"LosHealRangeUpdate": function(msg) {
				if (this.isIdle && msg && msg.data && msg.data.added && msg.data.added.length)
					this.RespondToHealableEntities(msg.data.added);
			},

			"LosAttackRangeUpdate": function(msg) {
				if (this.isIdle && msg && msg.data && msg.data.added && msg.data.added.length && this.GetStance().targetVisibleEnemies)
					this.AttackEntitiesByPreference(msg.data.added);
			},

			"Timer": function(msg) {
				if (this.isGuardOf)
				{
					this.Guard(this.isGuardOf, false);
					return;
				}

				// If a unit can heal and attack we first want to heal wounded units,
				// so check if we are a healer and find whether there's anybody nearby to heal.
				// (If anyone approaches later it'll be handled via LosHealRangeUpdate.)
				// If anyone in sight gets hurt that will be handled via LosHealRangeUpdate.
				if (this.IsHealer() && this.FindNewHealTargets())
					return;

				// If we entered the idle state we must have nothing better to do,
				// so immediately check whether there's anybody nearby to attack.
				// (If anyone approaches later, it'll be handled via LosAttackRangeUpdate.)
				if (this.FindNewTargets())
					return;

				if (this.FindSightedEnemies())
					return;

				if (!this.isIdle)
				{
					// Move back to the held position if we drifted away.
					// (only if not a formation member).
					if (!this.IsFormationMember() &&
					     this.GetStance().respondHoldGround && this.heldPosition &&
					     !this.CheckPointRangeExplicit(this.heldPosition.x, this.heldPosition.z, 0, 10) &&
					     this.WalkToHeldPosition())
						return;

					if (this.IsFormationMember())
					{
						let cmpFormationAI = Engine.QueryInterface(this.formationController, IID_UnitAI);
						if (!cmpFormationAI || !cmpFormationAI.IsIdle())
							return;
						Engine.QueryInterface(this.formationController, IID_Formation).SetIdleEntity(this.entity);
					}

					this.isIdle = true;
					Engine.PostMessage(this.entity, MT_UnitIdleChanged, { "idle": this.isIdle });
				}

				// Go linger first to prevent all roaming entities
				// to move all at the same time on map init.
				if (this.template.RoamDistance)
					this.SetNextState("LINGERING");
			},

			"ROAMING": {
				"enter": function() {
					this.SetFacePointAfterMove(false);
					this.MoveRandomly(+this.template.RoamDistance);
					this.StartTimer(randIntInclusive(+this.template.RoamTimeMin, +this.template.RoamTimeMax));
					return false;
				},

				"leave": function() {
					this.StopMoving();
					this.StopTimer();
					this.SetFacePointAfterMove(true);
				},

				"Timer": function(msg) {
					this.SetNextState("LINGERING");
				},

				"MovementUpdate": function() {
					this.MoveRandomly(+this.template.RoamDistance);
				},
			},

			"LINGERING": {
				"enter": function() {
					// ToDo: rename animations?
					this.SelectAnimation("feeding");
					this.StartTimer(randIntInclusive(+this.template.FeedTimeMin, +this.template.FeedTimeMax));
					return false;
				},

				"leave": function() {
					this.ResetAnimation();
					this.StopTimer();
				},

				"Timer": function(msg) {
					this.SetNextState("ROAMING");
				},
			},
		},

		"WALKING": {
			"enter": function() {
				if (!this.MoveTo(this.order.data))
				{
					this.FinishOrder();
					return true;
				}
				return false;
			},

			"leave": function() {
				this.StopMoving();
			},

			"MovementUpdate": function(msg) {
				// If it looks like the path is failing, and we are close enough stop anyways.
				// This avoids pathing for an unreachable goal and reduces lag considerably.
				if (msg.likelyFailure || msg.obstructed && this.RelaxedMaxRangeCheck(this.order.data, this.DefaultRelaxedMaxRange) ||
					 this.CheckRange(this.order.data))
					this.FinishOrder();
			},
		},

		"WALKINGANDFIGHTING": {
			"enter": function() {
				if (!this.MoveTo(this.order.data))
				{
					this.FinishOrder();
					return true;
				}
				// Show weapons rather than carried resources.
				this.SetAnimationVariant("combat");

				this.StartTimer(0, 1000);
				return false;
			},

			"Timer": function(msg) {
				this.FindWalkAndFightTargets();
			},

			"leave": function(msg) {
				this.StopMoving();
				this.StopTimer();
				this.SetDefaultAnimationVariant();
			},

			"MovementUpdate": function(msg) {
				// If it looks like the path is failing, and we are close enough stop anyways.
				// This avoids pathing for an unreachable goal and reduces lag considerably.
				if (msg.likelyFailure || msg.obstructed && this.RelaxedMaxRangeCheck(this.order.data, this.DefaultRelaxedMaxRange) ||
					 this.CheckRange(this.order.data))
					this.FinishOrder();
			},
		},

		"PATROL": {
			"enter": function() {
				let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
				if (!cmpPosition || !cmpPosition.IsInWorld())
				{
					this.FinishOrder();
					return true;
				}

				// Memorize the origin position in case that we want to go back.
				if (!this.patrolStartPosOrder)
				{
					this.patrolStartPosOrder = cmpPosition.GetPosition();
					this.patrolStartPosOrder.targetClasses = this.order.data.targetClasses;
					this.patrolStartPosOrder.allowCapture = this.order.data.allowCapture;
				}

				this.SetAnimationVariant("combat");

				return false;
			},

			"leave": function() {
				delete this.patrolStartPosOrder;
				this.SetDefaultAnimationVariant();
			},

			"PATROLLING": {
				"enter": function() {
					let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
					if (!cmpPosition || !cmpPosition.IsInWorld() ||
					    !this.MoveTo(this.order.data))
					{
						this.FinishOrder();
						return true;
					}
					this.StartTimer(0, 1000);
					return false;
				},

				"leave": function() {
					this.StopMoving();
					this.StopTimer();
				},

				"Timer": function(msg) {
					this.FindWalkAndFightTargets();
				},

				"MovementUpdate": function(msg) {
					if (!msg.likelyFailure && !msg.likelySuccess && !this.RelaxedMaxRangeCheck(this.order.data, this.DefaultRelaxedMaxRange))
						return;

					if (this.orderQueue.length == 1)
						this.PushOrder("Patrol", this.patrolStartPosOrder);

					this.PushOrder(this.order.type, this.order.data);
					this.SetNextState("CHECKINGWAYPOINT");
				},
			},

			"CHECKINGWAYPOINT": {
				"enter": function() {
					this.StartTimer(0, 1000);
					this.stopSurveying = 0;
					// TODO: pick a proper animation
					return false;
				},

				"leave": function() {
					this.StopTimer();
					delete this.stopSurveying;
				},

				"Timer": function(msg) {
					if (this.stopSurveying >= +this.template.PatrolWaitTime)
					{
						this.FinishOrder();
						return;
					}
					if (!this.FindWalkAndFightTargets())
						++this.stopSurveying;
				}
			}
		},

		"GUARD": {
			"RemoveGuard": function() {
				this.FinishOrder();
			},

			"ESCORTING": {
				"enter": function() {
					if (!this.MoveToTargetRangeExplicit(this.isGuardOf, 0, this.guardRange))
					{
						this.FinishOrder();
						return true;
					}

					// Show weapons rather than carried resources.
					this.SetAnimationVariant("combat");

					this.StartTimer(0, 1000);
					this.SetHeldPositionOnEntity(this.isGuardOf);
					return false;
				},

				"Timer": function(msg) {
					if (!this.ShouldGuard(this.isGuardOf))
					{
						this.FinishOrder();
						return;
					}

					let cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);
					if (cmpObstructionManager.IsInTargetRange(this.entity, this.isGuardOf, 0, 3 * this.guardRange, false))
						this.TryMatchTargetSpeed(this.isGuardOf, false);

					this.SetHeldPositionOnEntity(this.isGuardOf);
				},

				"leave": function(msg) {
					this.StopMoving();
					this.ResetSpeedMultiplier();
					this.StopTimer();
					this.SetDefaultAnimationVariant();
				},

				"MovementUpdate": function(msg) {
					if (msg.likelyFailure || this.CheckTargetRangeExplicit(this.isGuardOf, 0, this.guardRange))
						this.SetNextState("GUARDING");
				},
			},

			"GUARDING": {
				"enter": function() {
					this.StartTimer(1000, 1000);
					this.SetHeldPositionOnEntity(this.entity);
					this.SetAnimationVariant("combat");
					this.FaceTowardsTarget(this.order.data.target);
					return false;
				},

				"LosAttackRangeUpdate": function(msg) {
					if (this.GetStance().targetVisibleEnemies)
						this.AttackEntitiesByPreference(msg.data.added);
				},

				"Timer": function(msg) {
					if (!this.ShouldGuard(this.isGuardOf))
					{
						this.FinishOrder();
						return;
					}
					// TODO: find out what to do if we cannot move.
					if (!this.CheckTargetRangeExplicit(this.isGuardOf, 0, this.guardRange) &&
					    this.MoveToTargetRangeExplicit(this.isGuardOf, 0, this.guardRange))
						this.SetNextState("ESCORTING");
					else
					{
						this.FaceTowardsTarget(this.order.data.target);
						var cmpHealth = Engine.QueryInterface(this.isGuardOf, IID_Health);
						if (cmpHealth && cmpHealth.IsInjured())
						{
							if (this.CanHeal(this.isGuardOf))
								this.PushOrderFront("Heal", { "target": this.isGuardOf, "force": false });
							else if (this.CanRepair(this.isGuardOf))
								this.PushOrderFront("Repair", { "target": this.isGuardOf, "autocontinue": false, "force": false });
						}
					}
				},

				"leave": function(msg) {
					this.StopTimer();
					this.SetDefaultAnimationVariant();
				},
			},
		},

		"FLEEING": {
			"enter": function() {
				// We use the distance between the entities to account for ranged attacks
				this.order.data.distanceToFlee = PositionHelper.DistanceBetweenEntities(this.entity, this.order.data.target) + (+this.template.FleeDistance);
				let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
				// Use unit motion directly to ignore the visibility check. TODO: change this if we add LOS to fauna.
				if (this.CheckTargetRangeExplicit(this.order.data.target, this.order.data.distanceToFlee, -1) ||
				    !cmpUnitMotion || !cmpUnitMotion.MoveToTargetRange(this.order.data.target, this.order.data.distanceToFlee, -1))
				{
					this.FinishOrder();
					return true;
				}

				this.PlaySound("panic");

				this.SetSpeedMultiplier(this.GetRunMultiplier());
				return false;
			},

			"OrderTargetRenamed": function(msg) {
				// To avoid replaying the panic sound, handle this explicitly.
				let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
				if (this.CheckTargetRangeExplicit(this.order.data.target, this.order.data.distanceToFlee, -1) ||
				    !cmpUnitMotion || !cmpUnitMotion.MoveToTargetRange(this.order.data.target, this.order.data.distanceToFlee, -1))
					this.FinishOrder();
			},

			"Attacked": function(msg) {
				if (msg.data.attacker == this.order.data.target)
					return;

				let cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);
				if (cmpObstructionManager.DistanceToTarget(this.entity, msg.data.target) > cmpObstructionManager.DistanceToTarget(this.entity, this.order.data.target))
					return;

				if (this.GetStance().targetAttackersAlways || !this.order || !this.order.data || !this.order.data.force)
					this.RespondToTargetedEntities([msg.data.attacker]);
			},

			"leave": function() {
				this.ResetSpeedMultiplier();
				this.StopMoving();
			},

			"MovementUpdate": function(msg) {
				if (msg.likelyFailure || this.CheckTargetRangeExplicit(this.order.data.target, this.order.data.distanceToFlee, -1))
					this.FinishOrder();
			},
		},

		"COMBAT": {
			"Order.LeaveFoundation": function(msg) {
				// Ignore the order as we're busy.
				return this.FinishOrder();
			},

			"Attacked": function(msg) {
				// If we're already in combat mode, ignore anyone else who's attacking us
				// unless it's a melee attack since they may be blocking our way to the target
				if (msg.data.type == "Melee" && (this.GetStance().targetAttackersAlways || !this.order.data.force))
					this.RespondToTargetedEntities([msg.data.attacker]);
			},

			"leave": function() {
				if (!this.formationAnimationVariant)
					this.SetDefaultAnimationVariant();
			},

			"APPROACHING": {
				"enter": function() {
					if (!this.MoveToTargetAttackRange(this.order.data.target, this.order.data.attackType))
					{
						this.FinishOrder();
						return true;
					}

					if (!this.formationAnimationVariant)
						this.SetAnimationVariant("combat");

					this.StartTimer(1000, 1000);
					return false;
				},

				"leave": function() {
					this.StopMoving();
					this.StopTimer();
				},

				"Timer": function(msg) {
					if (this.ShouldAbandonChase(this.order.data.target, this.order.data.force, IID_Attack, this.order.data.attackType))
					{
						this.FinishOrder();

						if (this.GetStance().respondHoldGround)
							this.WalkToHeldPosition();
					}
					else
					{
						this.RememberTargetPosition();
						if (this.order.data.hunting && this.orderQueue.length > 1 &&
						     this.orderQueue[1].type === "Gather")
							this.RememberTargetPosition(this.orderQueue[1].data);
					}
				},

				"MovementUpdate": function(msg) {
					if (msg.likelyFailure)
					{
						// This also handles hunting.
						if (this.orderQueue.length > 1)
						{
							this.FinishOrder();
							return;
						}
						else if (!this.order.data.force || !this.order.data.lastPos)
						{
							this.SetNextState("COMBAT.FINDINGNEWTARGET");
							return;
						}
						// If the order was forced, try moving to the target position,
						// under the assumption that this is desirable if the target
						// was somewhat far away - we'll likely end up closer to where
						// the player hoped we would.
						let lastPos = this.order.data.lastPos;
						this.PushOrder("WalkAndFight", {
							"x": lastPos.x, "z": lastPos.z,
							"force": false,
						});
						return;
					}

					if (this.CheckTargetAttackRange(this.order.data.target, this.order.data.attackType))
					{
						if (this.CanUnpack())
						{
							this.PushOrderFront("Unpack", { "force": true });
							return;
						}
						this.SetNextState("ATTACKING");
					}
					else if (msg.likelySuccess)
						// Try moving again,
						// attack range uses a height-related formula and our actual max range might have changed.
						if (!this.MoveToTargetAttackRange(this.order.data.target, this.order.data.attackType))
							this.FinishOrder();
				},
			},

			"ATTACKING": {
				"enter": function() {
					let target = this.order.data.target;
					let cmpFormation = Engine.QueryInterface(target, IID_Formation);
					if (cmpFormation)
					{
						this.order.data.formationTarget = target;
						target = cmpFormation.GetClosestMember(this.entity);
						this.order.data.target = target;
					}

					this.shouldCheer = false;

					let cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
					if (!cmpAttack)
					{
						this.FinishOrder();
						return true;
					}

					if (!this.CheckTargetAttackRange(target, this.order.data.attackType))
					{
						if (this.CanPack())
						{
							this.PushOrderFront("Pack", { "force": true });
							return true;
						}

						this.ProcessMessage("OutOfRange");
						return true;
					}

					if (!this.formationAnimationVariant)
						this.SetAnimationVariant("combat");

					this.FaceTowardsTarget(this.order.data.target);

					this.RememberTargetPosition();
					if (this.order.data.hunting && this.orderQueue.length > 1 && this.orderQueue[1].type === "Gather")
						this.RememberTargetPosition(this.orderQueue[1].data);

					if (!cmpAttack.StartAttacking(this.order.data.target, this.order.data.attackType, IID_UnitAI))
					{
						this.ProcessMessage("TargetInvalidated");
						return true;
					}

					let cmpBuildingAI = Engine.QueryInterface(this.entity, IID_BuildingAI);
					if (cmpBuildingAI)
					{
						cmpBuildingAI.SetUnitAITarget(this.order.data.target);
						return false;
					}

					let cmpUnitAI = Engine.QueryInterface(this.order.data.target, IID_UnitAI);

					// Units with no cheering time do not cheer.
					this.shouldCheer = cmpUnitAI && (!cmpUnitAI.IsAnimal() || cmpUnitAI.IsDangerousAnimal()) && this.cheeringTime > 0;

					return false;
				},

				"leave": function() {
					let cmpBuildingAI = Engine.QueryInterface(this.entity, IID_BuildingAI);
					if (cmpBuildingAI)
						cmpBuildingAI.SetUnitAITarget(0);
					let cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
					if (cmpAttack)
						cmpAttack.StopAttacking();
				},

				"OutOfRange": function() {
					if (this.ShouldChaseTargetedEntity(this.order.data.target, this.order.data.force))
					{
						if (this.CanPack())
						{
							this.PushOrderFront("Pack", { "force": true });
							return;
						}
						this.SetNextState("CHASING");
						return;
					}
					this.SetNextState("FINDINGNEWTARGET");
				},

				"TargetInvalidated": function() {
					this.SetNextState("FINDINGNEWTARGET");
				},

				"Attacked": function(msg) {
					if (this.order.data.attackType == "Capture" && (this.GetStance().targetAttackersAlways || !this.order.data.force) &&
						this.order.data.target != msg.data.attacker && this.GetBestAttackAgainst(msg.data.attacker, true) != "Capture")
						this.RespondToTargetedEntities([msg.data.attacker]);
				},
			},

			"FINDINGNEWTARGET": {
				"Order.Cheer": function() {
					if (!this.cheeringTime)
						return this.FinishOrder();

					this.SetNextState("CHEERING");
					return ACCEPT_ORDER;
				},

				"enter": function() {
					// Try to find the formation the target was a part of.
					let cmpFormation = Engine.QueryInterface(this.order.data.target, IID_Formation);
					if (!cmpFormation)
						cmpFormation = Engine.QueryInterface(this.order.data.formationTarget || INVALID_ENTITY, IID_Formation);

					// If the target is a formation, pick closest member.
					if (cmpFormation)
					{
						let filter = (t) => this.CanAttack(t);
						this.order.data.formationTarget = this.order.data.target;
						let target = cmpFormation.GetClosestMember(this.entity, filter);
						this.order.data.target = target;
						this.SetNextState("COMBAT.ATTACKING");
						return true;
					}

					// Can't reach it, no longer owned by enemy, or it doesn't exist any more - give up
					// except if in WalkAndFight mode where we look for more enemies around before moving again.
					if (this.FinishOrder())
					{
						if (this.IsWalkingAndFighting())
						{
							Engine.ProfileStart("FindWalkAndFightTargets");
							this.FindWalkAndFightTargets();
							Engine.ProfileStop();
						}
						return true;
					}

					if (this.FindNewTargets())
						return true;

					if (this.GetStance().respondHoldGround)
						this.WalkToHeldPosition();

					if (this.shouldCheer)
					{
						this.Cheer();
						this.CallPlayerOwnedEntitiesFunctionInRange("Cheer", [], this.notifyToCheerInRange);
					}

					return true;
				},
			},

			"CHASING": {
				"Order.MoveToChasingPoint": function(msg) {
					if (this.CheckPointRangeExplicit(msg.data.x, msg.data.z, 0, msg.data.max) || !this.AbleToMove())
						return this.FinishOrder();
					msg.data.relaxed = true;
					this.StopTimer();
					this.SetNextState("MOVINGTOPOINT");
					return ACCEPT_ORDER;
				},
				"enter": function() {
					if (!this.MoveToTargetAttackRange(this.order.data.target, this.order.data.attackType))
					{
						this.FinishOrder();
						return true;
					}

					if (!this.formationAnimationVariant)
						this.SetAnimationVariant("combat");

					var cmpUnitAI = Engine.QueryInterface(this.order.data.target, IID_UnitAI);
					if (cmpUnitAI && cmpUnitAI.IsFleeing())
						this.SetSpeedMultiplier(this.GetRunMultiplier());

					this.StartTimer(1000, 1000);
					return false;
				},

				"leave": function() {
					this.ResetSpeedMultiplier();
					this.StopMoving();
					this.StopTimer();
				},

				"Timer": function(msg) {
					if (this.ShouldAbandonChase(this.order.data.target, this.order.data.force, IID_Attack, this.order.data.attackType))
					{
						this.FinishOrder();

						if (this.GetStance().respondHoldGround)
							this.WalkToHeldPosition();
					}
					else
					{
						this.RememberTargetPosition();
						if (this.order.data.hunting && this.orderQueue.length > 1 &&
						     this.orderQueue[1].type === "Gather")
							this.RememberTargetPosition(this.orderQueue[1].data);
					}
				},

				"MovementUpdate": function(msg) {
					if (msg.likelyFailure)
					{
						// This also handles hunting.
						if (this.orderQueue.length > 1)
						{
							this.FinishOrder();
							return;
						}
						else if (!this.order.data.force)
						{
							this.SetNextState("COMBAT.FINDINGNEWTARGET");
							return;
						}
						else if (this.order.data.lastPos)
						{
							let lastPos = this.order.data.lastPos;
							let cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
							this.PushOrder("MoveToChasingPoint", {
								"x": lastPos.x,
								"z": lastPos.z,
								"max": cmpAttack.GetRange(this.order.data.attackType).max,
								"force": true
							});
							return;
						}
					}
					if (this.CheckTargetAttackRange(this.order.data.target, this.order.data.attackType))
					{
						if (this.CanUnpack())
						{
							this.PushOrderFront("Unpack", { "force": true });
							return;
						}
						this.SetNextState("ATTACKING");
					}
					else if (msg.likelySuccess)
						// Try moving again,
						// attack range uses a height-related formula and our actual max range might have changed.
						if (!this.MoveToTargetAttackRange(this.order.data.target, this.order.data.attackType))
							this.FinishOrder();
				},
				"MOVINGTOPOINT": {
					"enter": function() {
						if (!this.MoveTo(this.order.data))
						{
							this.FinishOrder();
							return true;
						}
						return false;
					},
					"leave": function() {
						this.StopMoving();
					},
					"MovementUpdate": function(msg) {
						// If it looks like the path is failing, and we are close enough from wanted range
						// stop anyways. This avoids pathing for an unreachable goal and reduces lag considerably.
						if (msg.likelyFailure ||
							msg.obstructed && this.RelaxedMaxRangeCheck(this.order.data, this.order.data.max + this.DefaultRelaxedMaxRange) ||
							!msg.obstructed && this.CheckRange(this.order.data))
							this.FinishOrder();
					},
				},
			},
		},

		"GATHER": {
			"enter": function() {
				let cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
				if (cmpResourceGatherer)
					cmpResourceGatherer.AddToPlayerCounter(this.order.data.type.generic);
				return false;
			},

			"leave": function() {
				let cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
				if (cmpResourceGatherer)
					cmpResourceGatherer.RemoveFromPlayerCounter();

				// Show the carried resource, if we've gathered anything.
				this.SetDefaultAnimationVariant();
			},

			"APPROACHING": {
				"enter": function() {
					this.gatheringTarget = this.order.data.target;	// temporary, deleted in "leave".
					if (this.CheckRange(this.order.data, IID_ResourceGatherer))
					{
						this.SetNextState("GATHERING");
						return true;
					}

					// If we can't move, assume we'll fail any subsequent order
					// and finish the order entirely to avoid an infinite loop.
					if (!this.AbleToMove())
					{
						this.FinishOrder();
						return true;
					}

					let cmpSupply = Engine.QueryInterface(this.gatheringTarget, IID_ResourceSupply);
					let cmpMirage = Engine.QueryInterface(this.gatheringTarget, IID_Mirage);
					if ((!cmpMirage || !cmpMirage.Mirages(IID_ResourceSupply)) &&
					    (!cmpSupply || !cmpSupply.AddGatherer(this.entity)) ||
					    !this.MoveTo(this.order.data, IID_ResourceGatherer))
					{
						// If the target's last known position is in FOW, try going there
						// and hope that we might find it then.
						let lastPos = this.order.data.lastPos;
						if (this.gatheringTarget != INVALID_ENTITY &&
						    lastPos && !this.CheckPositionVisible(lastPos.x, lastPos.z))
						{
							this.PushOrderFront("Walk", {
								"x": lastPos.x, "z": lastPos.z,
								"force": this.order.data.force
							});
							return true;
						}
						this.SetNextState("FINDINGNEWTARGET");
						return true;
					}
					this.SetAnimationVariant("approach_" + this.order.data.type.specific);
					return false;
				},

				"MovementUpdate": function(msg) {
					// The GATHERING timer will handle finding a valid resource.
					if (msg.likelyFailure)
						this.SetNextState("FINDINGNEWTARGET");
					else if (this.CheckRange(this.order.data, IID_ResourceGatherer))
						this.SetNextState("GATHERING");
				},

				"leave": function() {
					this.StopMoving();
					this.SetDefaultAnimationVariant();

					if (!this.gatheringTarget)
						return;

					let cmpSupply = Engine.QueryInterface(this.gatheringTarget, IID_ResourceSupply);
					if (cmpSupply)
						cmpSupply.RemoveGatherer(this.entity);

					delete this.gatheringTarget;
				},
			},

			// Walking to a good place to gather resources near, used by GatherNearPosition
			"WALKING": {
				"enter": function() {
					if (!this.MoveTo(this.order.data))
					{
						this.FinishOrder();
						return true;
					}
					this.SetAnimationVariant("approach_" + this.order.data.type.specific);
					return false;
				},

				"leave": function() {
					this.StopMoving();
					this.SetDefaultAnimationVariant();
				},

				"MovementUpdate": function(msg) {
					if (msg.likelyFailure || msg.obstructed && this.RelaxedMaxRangeCheck(this.order.data, this.DefaultRelaxedMaxRange) ||
						 this.CheckRange(this.order.data))
						this.SetNextState("FINDINGNEWTARGET");
				},
			},

			"GATHERING": {
				"enter": function() {
					let cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
					if (!cmpResourceGatherer)
					{
						this.FinishOrder();
						return true;
					}

					if (!this.CheckTargetRange(this.order.data.target, IID_ResourceGatherer))
					{
						this.ProcessMessage("OutOfRange");
						return true;
					}

					// If this order was forced, the player probably gave it, but now we've reached the target
					//	switch to an unforced order (can be interrupted by attacks)
					this.order.data.force = false;
					this.order.data.autoharvest = true;

					this.FaceTowardsTarget(this.order.data.target);
					if (!cmpResourceGatherer.StartGathering(this.order.data.target, IID_UnitAI))
					{
						this.ProcessMessage("TargetInvalidated");
						return true;
					}

					return false;
				},

				"leave": function() {
					let cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
					if (cmpResourceGatherer)
						cmpResourceGatherer.StopGathering();
				},

				"InventoryFilled": function(msg) {
					this.SetNextState("RETURNINGRESOURCE");
				},

				"OutOfRange": function(msg) {
					if (this.MoveToTargetRange(this.order.data.target, IID_ResourceGatherer))
						this.SetNextState("APPROACHING");
					// Our target is no longer visible - go to its last known position first
					// and then hopefully it will become visible.
					else if (!this.CheckTargetVisible(this.order.data.target) && this.order.data.lastPos)
						this.PushOrderFront("Walk", {
							"x": this.order.data.lastPos.x,
							"z": this.order.data.lastPos.z,
							"force": this.order.data.force
						});
					else
						this.SetNextState("FINDINGNEWTARGET");
				},

				"TargetInvalidated": function(msg) {
					this.SetNextState("FINDINGNEWTARGET");
				},
			},

			"FINDINGNEWTARGET": {
				"enter": function() {
					const previousForced = this.order.data.force;
					let previousTarget = this.order.data.target;
					let resourceTemplate = this.order.data.template;
					let resourceType = this.order.data.type;

					// Give up on this order and try our next queued order
					// but first check what is our next order and, if needed, insert a returnResource order
					let cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
					if (cmpResourceGatherer.IsCarrying(resourceType.generic) &&
						this.orderQueue.length > 1 && this.orderQueue[1] !== "ReturnResource" &&
						(this.orderQueue[1].type !== "Gather" || this.orderQueue[1].data.type.generic !== resourceType.generic))
					{
						let nearestDropsite = this.FindNearestDropsite(resourceType.generic);
						if (nearestDropsite)
							this.orderQueue.splice(1, 0, { "type": "ReturnResource", "data": { "target": nearestDropsite, "force": false } });
					}

					// Must go before FinishOrder or this.order will be undefined.
					let initPos = this.order.data.initPos;

					if (this.FinishOrder())
						return true;

					// No remaining orders - pick a useful default behaviour

					let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
					if (!cmpPosition || !cmpPosition.IsInWorld())
						return true;

					let filter = (ent, type, template) => {
						if (previousTarget == ent)
							return false;

						// Don't switch to a different type of huntable animal.
						return type.specific == resourceType.specific &&
						    (type.specific != "meat" || resourceTemplate == template);
					};

					// Current position is often next to a dropsite.
					// But don't use that on forced orders, as the order may want us to go
					// to the other side of the map on purpose.
					let pos = cmpPosition.GetPosition();
					let nearbyResource;
					if (!previousForced)
						nearbyResource = this.FindNearbyResource(Vector2D.from3D(pos), filter);

					// If there is an initPos, search there as well when we haven't found anything.
					// Otherwise set initPos to our current pos.
					if (!initPos)
						initPos = { 'x': pos.X, 'z': pos.Z };
					else if (!nearbyResource || previousForced)
						nearbyResource = this.FindNearbyResource(new Vector2D(initPos.x, initPos.z), filter);

					if (nearbyResource)
					{
						this.PerformGather(nearbyResource, false, false);
						return true;
					}

					// Failing that, try to move there and se if we are more lucky: maybe there are resources in FOW.
					// Only move if we are some distance away (TODO: pick the distance better?).
					// Using the default relaxed range check since that is used in the WALKING-state.
					if (!this.CheckPointRangeExplicit(initPos.x, initPos.z, 0, this.DefaultRelaxedMaxRange))
					{
						this.GatherNearPosition(initPos.x, initPos.z, resourceType, resourceTemplate);
						return true;
					}

					// Nothing else to gather - if we're carrying anything then we should
					// drop it off, and if not then we might as well head to the dropsite
					// anyway because that's a nice enough place to congregate and idle

					let nearestDropsite = this.FindNearestDropsite(resourceType.generic);
					if (nearestDropsite)
					{
						this.PushOrderFront("ReturnResource", { "target": nearestDropsite, "force": false });
						return true;
					}
					// No dropsites - just give up.
					return true;
				},
			},

			"RETURNINGRESOURCE": {
				"enter": function() {
					let nearestDropsite = this.FindNearestDropsite(this.order.data.type.generic);
					if (!nearestDropsite)
					{
						// The player expects the unit to move upon failure.
						let formerTarget = this.order.data.target;
						if (!this.FinishOrder())
							this.WalkToTarget(formerTarget);
						return true;
					}
					this.order.data.formerTarget = this.order.data.target;
					this.order.data.target = nearestDropsite;
					if (this.CheckTargetRange(this.order.data.target, IID_ResourceGatherer))
					{
						this.SetNextState("DROPPINGRESOURCES");
						return true;
					}
					this.SetNextState("APPROACHING");
					return true;
				},

				"leave": function() {
				},

				"APPROACHING": "INDIVIDUAL.RETURNRESOURCE.APPROACHING",

				"DROPPINGRESOURCES": {
					"enter": function() {
						let cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
						if (this.CanReturnResource(this.order.data.target, true, cmpResourceGatherer) &&
							cmpResourceGatherer.IsTargetInRange(this.order.data.target))
						{
							cmpResourceGatherer.CommitResources(this.order.data.target);
							// Stop showing the carried resource animation.
							this.SetDefaultAnimationVariant();
							this.SetNextState("GATHER.APPROACHING");
						}
						else
							this.SetNextState("RETURNINGRESOURCE");
						this.order.data.target = this.order.data.formerTarget;

						return true;
					},

					"leave": function() {
					},
				},
			},
		},

		"HEAL": {
			"APPROACHING": {
				"enter": function() {
					if (this.CheckRange(this.order.data, IID_Heal))
					{
						this.SetNextState("HEALING");
						return true;
					}

					if (!this.MoveTo(this.order.data, IID_Heal))
					{
						this.FinishOrder();
						return true;
					}

					this.StartTimer(1000, 1000);
					return false;
				},

				"leave": function() {
					this.StopMoving();
					this.StopTimer();
				},

				"Timer": function(msg) {
					if (this.ShouldAbandonChase(this.order.data.target, this.order.data.force, IID_Heal, null))
						this.SetNextState("FINDINGNEWTARGET");
				},

				"MovementUpdate": function(msg) {
					if (msg.likelyFailure || this.CheckRange(this.order.data, IID_Heal))
						this.SetNextState("HEALING");
				},
			},

			"HEALING": {
				"enter": function() {
					let cmpHeal = Engine.QueryInterface(this.entity, IID_Heal);
					if (!cmpHeal)
					{
						this.FinishOrder();
						return true;
					}

					if (!this.CheckRange(this.order.data, IID_Heal))
					{
						this.ProcessMessage("OutOfRange");
						return true;
					}

					if (!cmpHeal.StartHealing(this.order.data.target, IID_UnitAI))
					{
						this.ProcessMessage("TargetInvalidated");
						return true;
					}

					this.FaceTowardsTarget(this.order.data.target);
					return false;
				},

				"leave": function() {
					let cmpHeal = Engine.QueryInterface(this.entity, IID_Heal);
					if (cmpHeal)
						cmpHeal.StopHealing();
				},

				"OutOfRange": function(msg) {
					if (this.ShouldChaseTargetedEntity(this.order.data.target, this.order.data.force))
					{
						if (this.CanPack())
							this.PushOrderFront("Pack", { "force": true });
						else
							this.SetNextState("APPROACHING");
					}
					else
						this.SetNextState("FINDINGNEWTARGET");
				},

				"TargetInvalidated": function(msg) {
					this.SetNextState("FINDINGNEWTARGET");
				},
			},

			"FINDINGNEWTARGET": {
				"enter": function() {
					// If we have another order, do that instead.
					if (this.FinishOrder())
						return true;

					if (this.FindNewHealTargets())
						return true;

					if (this.GetStance().respondHoldGround)
						this.WalkToHeldPosition();

					// We quit this state right away.
					return true;
				},
			},
		},

		// Returning to dropsite
		"RETURNRESOURCE": {
			"APPROACHING": {
				"enter": function() {
					if (this.CheckTargetRange(this.order.data.target, IID_ResourceGatherer))
					{
						this.SetNextState("DROPPINGRESOURCES");
						return true;
					}

					if (!this.MoveTo(this.order.data, IID_ResourceGatherer))
					{
						this.FinishOrder();
						return true;
					}

					this.SetDefaultAnimationVariant();
					return false;
				},

				"leave": function() {
					this.StopMoving();
				},

				"MovementUpdate": function(msg) {
					if (msg.likelyFailure || this.CheckTargetRange(this.order.data.target, IID_ResourceGatherer))
						this.SetNextState("DROPPINGRESOURCES");
				},
			},

			"DROPPINGRESOURCES": {
				"enter": function() {
					let cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
					if (this.CanReturnResource(this.order.data.target, true, cmpResourceGatherer) &&
						cmpResourceGatherer.IsTargetInRange(this.order.data.target))
					{
						cmpResourceGatherer.CommitResources(this.order.data.target);

						// Stop showing the carried resource animation.
						this.SetDefaultAnimationVariant();

						this.FinishOrder();
						return true;
					}
					let nearby = this.FindNearestDropsite(cmpResourceGatherer.GetMainCarryingType());
					this.FinishOrder();
					if (nearby)
						this.PushOrderFront("ReturnResource", { "target": nearby, "force": false });

					return true;
				},

				"leave": function() {
				},
			},
		},

		"COLLECTTREASURE": {
			"leave": function() {
			},

			"APPROACHING": {
				"enter": function() {
					// If we can't move, assume we'll fail any subsequent order
					// and finish the order entirely to avoid an infinite loop.
					if (!this.AbleToMove())
					{
						this.FinishOrder();
						return true;
					}
					if (!this.MoveToTargetRange(this.order.data.target, IID_TreasureCollector))
					{
						this.SetNextState("FINDINGNEWTARGET");
						return true;
					}
					return false;
				},

				"leave": function() {
					this.StopMoving();
				},

				"MovementUpdate": function(msg) {
					if (this.CheckTargetRange(this.order.data.target, IID_TreasureCollector))
						this.SetNextState("COLLECTING");
					else if (msg.likelyFailure)
						this.SetNextState("FINDINGNEWTARGET");
				},
			},

			"COLLECTING": {
				"enter": function() {
					let cmpTreasureCollector = Engine.QueryInterface(this.entity, IID_TreasureCollector);
					if (!cmpTreasureCollector.StartCollecting(this.order.data.target, IID_UnitAI))
					{
						this.ProcessMessage("TargetInvalidated");
						return true;
					}
					this.FaceTowardsTarget(this.order.data.target);
					return false;
				},

				"leave": function() {
					let cmpTreasureCollector = Engine.QueryInterface(this.entity, IID_TreasureCollector);
					if (cmpTreasureCollector)
						cmpTreasureCollector.StopCollecting();
				},

				"OutOfRange": function(msg) {
					this.SetNextState("APPROACHING");
				},

				"TargetInvalidated": function(msg) {
					this.SetNextState("FINDINGNEWTARGET");
				},
			},

			"FINDINGNEWTARGET": {
				"enter": function() {
					let oldTarget = this.order.data.target || INVALID_ENTITY;

					// Switch to the next order (if any).
					if (this.FinishOrder())
						return true;

					let nearbyTreasure = this.FindNearbyTreasure(this.TargetPosOrEntPos(oldTarget));
					if (nearbyTreasure)
						this.CollectTreasure(nearbyTreasure, true);

					return true;
				},
			},

			// Walking to a good place to collect treasures near, used by CollectTreasureNearPosition.
			"WALKING": {
				"enter": function() {
					if (!this.MoveTo(this.order.data))
					{
						this.FinishOrder();
						return true;
					}
					return false;
				},

				"leave": function() {
					this.StopMoving();
				},

				"MovementUpdate": function(msg) {
					if (msg.likelyFailure || msg.obstructed && this.RelaxedMaxRangeCheck(this.order.data, this.DefaultRelaxedMaxRange) ||
						 this.CheckRange(this.order.data))
						this.SetNextState("FINDINGNEWTARGET");
				},
			},
		},

		"TRADE": {
			"Attacked": function(msg) {
				// Ignore attack
				// TODO: Inform player
			},

			"leave": function() {
			},

			"APPROACHINGMARKET": {
				"enter": function() {
					if (!this.MoveToMarket(this.order.data.target))
					{
						this.FinishOrder();
						return true;
					}
					return false;
				},

				"leave": function() {
					this.StopMoving();
				},

				"MovementUpdate": function(msg) {
					if (!msg.likelyFailure && !this.CheckRange(this.order.data.nextTarget, IID_Trader))
						return;
					if (this.waypoints && this.waypoints.length)
					{
						if (!this.MoveToMarket(this.order.data.target))
							this.FinishOrder();
					}
					else
						this.SetNextState("TRADING");
				},
			},

			"TRADING": {
				"enter": function() {
					if (!this.CanTrade(this.order.data.target))
					{
						this.FinishOrder();
						return true;
					}

					if (!this.CheckTargetRange(this.order.data.target, IID_Trader))
					{
						this.SetNextState("APPROACHINGMARKET");
						return true;
					}

					let cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
					let nextMarket = cmpTrader.PerformTrade(this.order.data.target);
					let amount = cmpTrader.GetGoods().amount;
					if (!nextMarket || !amount || !amount.traderGain)
					{
						this.FinishOrder();
						return true;
					}

					this.order.data.target = nextMarket;

					if (this.order.data.route && this.order.data.route.length)
					{
						this.waypoints = this.order.data.route.slice();
						if (this.order.data.target == cmpTrader.GetSecondMarket())
							this.waypoints.reverse();
					}

					this.SetNextState("APPROACHINGMARKET");
					return true;
				},

				"leave": function() {
				},
			},

			"TradingCanceled": function(msg) {
				if (msg.market != this.order.data.target)
					return;
				let cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
				let otherMarket = cmpTrader && cmpTrader.GetFirstMarket();
				if (otherMarket)
					this.WalkToTarget(otherMarket);
				else
					this.FinishOrder();
			},
		},

		"REPAIR": {
			"APPROACHING": {
				"enter": function() {
					if (!this.MoveTo(this.order.data, IID_Builder))
					{
						this.FinishOrder();
						return true;
					}
					return false;
				},

				"leave": function() {
					this.StopMoving();
				},

				"MovementUpdate": function(msg) {
					if (msg.likelyFailure || msg.likelySuccess)
						this.SetNextState("REPAIRING");
				},
			},

			"REPAIRING": {
				"enter": function() {
					let cmpBuilder = Engine.QueryInterface(this.entity, IID_Builder);
					if (!cmpBuilder)
					{
						this.FinishOrder();
						return true;
					}

					// If this order was forced, the player probably gave it, but now we've reached the target
					//	switch to an unforced order (can be interrupted by attacks)
					if (this.order.data.force)
						this.order.data.autoharvest = true;

					this.order.data.force = false;

					if (!this.CheckTargetRange(this.order.data.target, IID_Builder))
					{
						this.ProcessMessage("OutOfRange");
						return true;
					}

					let cmpHealth = Engine.QueryInterface(this.order.data.target, IID_Health);
					if (cmpHealth && cmpHealth.GetHitpoints() >= cmpHealth.GetMaxHitpoints())
					{
						// The building was already finished/fully repaired before we arrived;
						// let the ConstructionFinished handler handle this.
						this.ConstructionFinished({ "entity": this.order.data.target, "newentity": this.order.data.target });
						return true;
					}

					if (!cmpBuilder.StartRepairing(this.order.data.target, IID_UnitAI))
					{
						this.ProcessMessage("TargetInvalidated");
						return true;
					}

					this.FaceTowardsTarget(this.order.data.target);
					return false;
				},

				"leave": function() {
					let cmpBuilder = Engine.QueryInterface(this.entity, IID_Builder);
					if (cmpBuilder)
						cmpBuilder.StopRepairing();
				},

				"OutOfRange": function(msg) {
					this.SetNextState("APPROACHING");
				},

				"TargetInvalidated": function(msg) {
					this.FinishOrder();
				},
			},

			"ConstructionFinished": function(msg) {
				if (msg.data.entity != this.order.data.target)
					return; // ignore other buildings

				let oldData = this.order.data;

				// Save the current state so we can continue walking if necessary
				// FinishOrder() below will switch to IDLE if there's no order, which sets the idle animation.
				// Idle animation while moving towards finished construction looks weird (ghosty).
				let oldState = this.GetCurrentState();

				let cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
				let canReturnResources = this.CanReturnResource(msg.data.newentity, true, cmpResourceGatherer);
				if (this.CheckTargetRange(msg.data.newentity, IID_Builder) && canReturnResources)
				{
					cmpResourceGatherer.CommitResources(msg.data.newentity);
					this.SetDefaultAnimationVariant();
				}

				// Switch to the next order (if any)
				if (this.FinishOrder())
				{
					if (canReturnResources)
					{
						// We aren't in range, but we can still return resources there: always do so.
						this.SetDefaultAnimationVariant();
						this.PushOrderFront("ReturnResource", { "target": msg.data.newentity, "force": false });
					}
					return;
				}

				if (canReturnResources)
				{
					// We aren't in range, but we can still return resources there: always do so.
					this.SetDefaultAnimationVariant();
					this.PushOrderFront("ReturnResource", { "target": msg.data.newentity, "force": false });
				}

				// No remaining orders - pick a useful default behaviour

				// If autocontinue explicitly disabled (e.g. by AI) then
				// do nothing automatically
				if (!oldData.autocontinue)
					return;

				// If this building was e.g. a farm of ours, the entities that received
				// the build command should start gathering from it
				if ((oldData.force || oldData.autoharvest) && this.CanGather(msg.data.newentity))
				{
					this.PerformGather(msg.data.newentity, true, false);
					return;
				}

				// If this building was e.g. a farmstead of ours, entities that received
				// the build command should look for nearby resources to gather
				if ((oldData.force || oldData.autoharvest) &&
				    this.CanReturnResource(msg.data.newentity, false, cmpResourceGatherer))
				{
					let cmpResourceDropsite = Engine.QueryInterface(msg.data.newentity, IID_ResourceDropsite);
					let types = cmpResourceDropsite.GetTypes();
					// TODO: Slightly undefined behavior here, we don't know what type of resource will be collected,
					//   may cause problems for AIs (especially hunting fast animals), but avoid ugly hacks to fix that!
					let nearby = this.FindNearbyResource(this.TargetPosOrEntPos(msg.data.newentity),
						(ent, type, template) => types.indexOf(type.generic) != -1);

					if (nearby)
					{
						this.PerformGather(nearby, true, false);
						return;
					}
				}

				let nearbyFoundation = this.FindNearbyFoundation(this.TargetPosOrEntPos(msg.data.newentity));
				if (nearbyFoundation)
				{
					this.AddOrder("Repair", { "target": nearbyFoundation, "autocontinue": oldData.autocontinue, "force": false }, true);
					return;
				}

				// Unit was approaching and there's nothing to do now, so switch to walking
				if (oldState.endsWith("REPAIR.APPROACHING"))
					// We're already walking to the given point, so add this as a order.
					this.WalkToTarget(msg.data.newentity, true);
			},
		},

		"GARRISON": {
			"APPROACHING": {
				"enter": function() {
					if (this.order.data.garrison ? !this.CanGarrison(this.order.data.target) :
						!this.CanOccupyTurret(this.order.data.target))
					{
						this.FinishOrder();
						return true;
					}

					if (!this.MoveToTargetRange(this.order.data.target, this.order.data.garrison ? IID_Garrisonable : IID_Turretable))
					{
						this.FinishOrder();
						return true;
					}

					if (this.pickup)
						Engine.PostMessage(this.pickup, MT_PickupCanceled, { "entity": this.entity });

					let cmpHolder = Engine.QueryInterface(this.order.data.target, this.order.data.garrison ? IID_GarrisonHolder : IID_TurretHolder);
					if (cmpHolder && cmpHolder.CanPickup(this.entity))
					{
						this.pickup = this.order.data.target;
						Engine.PostMessage(this.pickup, MT_PickupRequested, { "entity": this.entity, "iid": this.order.data.garrison ? IID_GarrisonHolder : IID_TurretHolder });
					}
					return false;
				},

				"leave": function() {
					if (this.pickup)
					{
						Engine.PostMessage(this.pickup, MT_PickupCanceled, { "entity": this.entity });
						delete this.pickup;
					}
					this.StopMoving();
				},

				"MovementUpdate": function(msg) {
					if (!msg.likelyFailure && !msg.likelySuccess)
						return;

					if (this.CheckTargetRange(this.order.data.target, this.order.data.garrison ? IID_Garrisonable : IID_Turretable))
						this.SetNextState("GARRISONING");
					else
					{
						// Unable to reach the target, try again (or follow if it is a moving target)
						// except if the target does not exist anymore or its orders have changed.
						if (this.pickup)
						{
							let cmpUnitAI = Engine.QueryInterface(this.pickup, IID_UnitAI);
							if (!cmpUnitAI || (!cmpUnitAI.HasPickupOrder(this.entity) && !cmpUnitAI.IsIdle()))
								this.FinishOrder();
						}
					}
				},
			},

			"GARRISONING": {
				"enter": function() {
					let target = this.order.data.target;
					if (this.order.data.garrison)
					{
						let cmpGarrisonable = Engine.QueryInterface(this.entity, IID_Garrisonable);
						if (!cmpGarrisonable || !cmpGarrisonable.Garrison(target))
						{
							this.FinishOrder();
							return true;
						}
					}
					else
					{
						let cmpTurretable = Engine.QueryInterface(this.entity, IID_Turretable);
						if (!cmpTurretable || !cmpTurretable.OccupyTurret(target))
						{
							this.FinishOrder();
							return true;
						}
					}

					if (this.formationController)
					{
						let cmpFormation = Engine.QueryInterface(this.formationController, IID_Formation);
						if (cmpFormation)
						{
							let rearrange = cmpFormation.rearrange;
							cmpFormation.SetRearrange(false);
							cmpFormation.RemoveMembers([this.entity]);
							cmpFormation.SetRearrange(rearrange);
						}
					}

					let cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
					if (this.CanReturnResource(target, true, cmpResourceGatherer))
					{
						cmpResourceGatherer.CommitResources(target);
						this.SetDefaultAnimationVariant();
					}

					this.FinishOrder();
					return true;
				},

				"leave": function() {
				},
			},
		},

		"CHEERING": {
			"enter": function() {
				this.SelectAnimation("promotion");
				this.StartTimer(this.cheeringTime);
				return false;
			},

			"leave": function() {
				// PushOrderFront preserves the cheering order,
				// which can lead to very bad behaviour, so make
				// sure to delete any queued ones.
				for (let i = 1; i < this.orderQueue.length; ++i)
					if (this.orderQueue[i].type == "Cheer")
						this.orderQueue.splice(i--, 1);
				this.StopTimer();
				this.ResetAnimation();
			},

			"LosRangeUpdate": function(msg) {
				if (msg && msg.data && msg.data.added && msg.data.added.length)
					this.RespondToSightedEntities(msg.data.added);
			},

			"LosHealRangeUpdate": function(msg) {
				if (msg && msg.data && msg.data.added && msg.data.added.length)
					this.RespondToHealableEntities(msg.data.added);
			},

			"LosAttackRangeUpdate": function(msg) {
				if (msg && msg.data && msg.data.added && msg.data.added.length && this.GetStance().targetVisibleEnemies)
					this.AttackEntitiesByPreference(msg.data.added);
			},

			"Timer": function(msg) {
				this.FinishOrder();
			},
		},

		"PACKING": {
			"enter": function() {
				let cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
				cmpPack.Pack();
				return false;
			},

			"Order.CancelPack": function(msg) {
				this.FinishOrder();
				return ACCEPT_ORDER;
			},

			"PackFinished": function(msg) {
				this.FinishOrder();
			},

			"leave": function() {
				let cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
				cmpPack.CancelPack();
			},

			"Attacked": function(msg) {
				// Ignore attacks while packing
			},
		},

		"UNPACKING": {
			"enter": function() {
				let cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
				cmpPack.Unpack();
				return false;
			},

			"Order.CancelUnpack": function(msg) {
				this.FinishOrder();
				return ACCEPT_ORDER;
			},

			"PackFinished": function(msg) {
				this.FinishOrder();
			},

			"leave": function() {
				let cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
				cmpPack.CancelPack();
			},

			"Attacked": function(msg) {
				// Ignore attacks while unpacking
			},
		},

		"PICKUP": {
			"APPROACHING": {
				"enter": function() {
					if (!this.MoveTo(this.order.data))
					{
						this.FinishOrder();
						return true;
					}
					return false;
				},

				"leave": function() {
					this.StopMoving();
				},

				"MovementUpdate": function(msg) {
					if (msg.likelyFailure || msg.likelySuccess)
						this.SetNextState("LOADING");
				},

				"PickupCanceled": function() {
					this.FinishOrder();
				},
			},

			"LOADING": {
				"enter": function() {
					let cmpHolder = Engine.QueryInterface(this.entity, this.order.data.iid);
					if (!cmpHolder || cmpHolder.IsFull())
					{
						this.FinishOrder();
						return true;
					}
					return false;
				},

				"PickupCanceled": function() {
					this.FinishOrder();
				},
			},
		},
	},
};

UnitAI.prototype.Init = function()
{
	this.orderQueue = []; // current order is at the front of the list
	this.order = undefined; // always == this.orderQueue[0]
	this.formationController = INVALID_ENTITY; // entity with IID_Formation that we belong to
	this.isIdle = false;

	this.heldPosition = undefined;

	// Queue of remembered works
	this.workOrders = [];

	this.isGuardOf = undefined;

	this.formationAnimationVariant = undefined;
	this.cheeringTime = +(this.template.CheeringTime || 0);
	this.SetStance(this.template.DefaultStance);
};

/**
 * @param {cmpTurretable} cmpTurretable - Optionally the component to save a query here.
 * @return {boolean} - Whether we are occupying a turret point.
 */
UnitAI.prototype.IsTurret = function(cmpTurretable)
{
	if (!cmpTurretable)
		cmpTurretable = Engine.QueryInterface(this.entity, IID_Turretable);
	return cmpTurretable && cmpTurretable.HolderID() != INVALID_ENTITY;
};

UnitAI.prototype.IsFormationController = function()
{
	return (this.template.FormationController == "true");
};

UnitAI.prototype.IsFormationMember = function()
{
	return (this.formationController != INVALID_ENTITY);
};

UnitAI.prototype.GetFormationsList = function()
{
	return this.template.Formations?._string?.split(/\s+/) || [];
};

UnitAI.prototype.CanUseFormation = function(formation)
{
	return this.GetFormationsList().includes(formation);
};

/**
 * For now, entities with a RoamDistance are animals.
 */
UnitAI.prototype.IsAnimal = function()
{
	return !!this.template.RoamDistance;
};

/**
 * ToDo: Make this not needed by fixing gaia
 * range queries in BuildingAI and UnitAI regarding
 * animals and other gaia entities.
 */
UnitAI.prototype.IsDangerousAnimal = function()
{
	return this.IsAnimal() && this.GetStance().targetVisibleEnemies && !!Engine.QueryInterface(this.entity, IID_Attack);
};

UnitAI.prototype.IsHealer = function()
{
	return Engine.QueryInterface(this.entity, IID_Heal);
};

UnitAI.prototype.IsIdle = function()
{
	return this.isIdle;
};

/**
 * Used by formation controllers to toggle the idleness of their members.
 */
UnitAI.prototype.ResetIdle = function()
{
	let shouldBeIdle = this.GetCurrentState().endsWith(".IDLE");
	if (this.isIdle == shouldBeIdle)
		return;
	this.isIdle = shouldBeIdle;
	Engine.PostMessage(this.entity, MT_UnitIdleChanged, { "idle": this.isIdle });
};

UnitAI.prototype.SetGarrisoned = function()
{
	// UnitAI caches its own garrisoned state for performance.
	this.isGarrisoned = true;
	this.SetImmobile();
};

UnitAI.prototype.UnsetGarrisoned = function()
{
	delete this.isGarrisoned;
	this.SetMobile();
};

UnitAI.prototype.ShouldRespondToEndOfAlert = function()
{
	return !this.orderQueue.length || this.orderQueue[0].type == "Garrison";
};

UnitAI.prototype.SetImmobile = function()
{
	if (this.isImmobile)
		return;

	this.isImmobile = true;
	Engine.PostMessage(this.entity, MT_UnitAbleToMoveChanged, {
		"entity": this.entity,
		"ableToMove": this.AbleToMove()
	});
};

UnitAI.prototype.SetMobile = function()
{
	if (!this.isImmobile)
		return;

	delete this.isImmobile;
	Engine.PostMessage(this.entity, MT_UnitAbleToMoveChanged, {
		"entity": this.entity,
		"ableToMove": this.AbleToMove()
	});
};

/**
 * @param cmpUnitMotion - optionally pass unitMotion to avoid querying it here
 * @returns true if the entity can move, i.e. has UnitMotion and isn't immobile.
 */
UnitAI.prototype.AbleToMove = function(cmpUnitMotion)
{
	if (this.isImmobile)
		return false;

	if (!cmpUnitMotion)
		cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);

	return !!cmpUnitMotion;
};

UnitAI.prototype.IsFleeing = function()
{
	var state = this.GetCurrentState().split(".").pop();
	return (state == "FLEEING");
};

UnitAI.prototype.IsWalking = function()
{
	var state = this.GetCurrentState().split(".").pop();
	return (state == "WALKING");
};

/**
 * Return true if the current order is WalkAndFight or Patrol.
 */
UnitAI.prototype.IsWalkingAndFighting = function()
{
	if (this.IsFormationMember())
		return Engine.QueryInterface(this.formationController, IID_UnitAI).IsWalkingAndFighting();

	return this.orderQueue.length > 0 && (this.orderQueue[0].type == "WalkAndFight" || this.orderQueue[0].type == "Patrol");
};

UnitAI.prototype.OnCreate = function()
{
	if (this.IsFormationController())
		this.UnitFsm.Init(this, "FORMATIONCONTROLLER.IDLE");
	else
		this.UnitFsm.Init(this, "INDIVIDUAL.IDLE");
	this.isIdle = true;
};

UnitAI.prototype.OnDiplomacyChanged = function(msg)
{
	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpOwnership && cmpOwnership.GetOwner() == msg.player)
		this.SetupRangeQueries();

	if (this.isGuardOf && !IsOwnedByMutualAllyOfEntity(this.entity, this.isGuardOf))
		this.RemoveGuard();
};

UnitAI.prototype.OnOwnershipChanged = function(msg)
{
	this.SetupRangeQueries();

	if (this.isGuardOf && (msg.to == INVALID_PLAYER || !IsOwnedByMutualAllyOfEntity(this.entity, this.isGuardOf)))
		this.RemoveGuard();

	// If the unit isn't being created or dying, reset stance and clear orders
	if (msg.to != INVALID_PLAYER && msg.from != INVALID_PLAYER)
	{
		// Switch to a virgin state to let states execute their leave handlers.
		// Except if (un)packing, in which case we only clear the order queue.
		if (this.IsPacking())
		{
			this.orderQueue.length = Math.min(this.orderQueue.length, 1);
			Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
		}
		else
		{
			const state = this.GetCurrentState();
			// Special "will be destroyed soon" mode - do nothing.
			if (state === "")
				return;
			const index = state.indexOf(".");
			if (index != -1)
				this.UnitFsm.SwitchToNextState(this, this.GetCurrentState().slice(0, index));
			this.Stop(false);
		}

		this.workOrders = [];
		let cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
		if (cmpTrader)
			cmpTrader.StopTrading();

		this.SetStance(this.template.DefaultStance);
		if (this.IsTurret())
			this.SetTurretStance();
	}
};

UnitAI.prototype.OnDestroy = function()
{
	// Switch to an empty state to let states execute their leave handlers.
	this.UnitFsm.SwitchToNextState(this, "");

	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (this.losRangeQuery)
		cmpRangeManager.DestroyActiveQuery(this.losRangeQuery);
	if (this.losHealRangeQuery)
		cmpRangeManager.DestroyActiveQuery(this.losHealRangeQuery);
	if (this.losAttackRangeQuery)
		cmpRangeManager.DestroyActiveQuery(this.losAttackRangeQuery);
};

UnitAI.prototype.OnVisionRangeChanged = function(msg)
{
	if (this.entity == msg.entity)
		this.SetupRangeQueries();
};

UnitAI.prototype.HasPickupOrder = function(entity)
{
	return this.orderQueue.some(order => order.type == "PickupUnit" && order.data.target == entity);
};

UnitAI.prototype.OnPickupRequested = function(msg)
{
	if (this.HasPickupOrder(msg.entity))
		return;
	this.PushOrderAfterForced("PickupUnit", { "target": msg.entity, "iid": msg.iid });
};

UnitAI.prototype.OnPickupCanceled = function(msg)
{
	for (let i = 0; i < this.orderQueue.length; ++i)
	{
		if (this.orderQueue[i].type != "PickupUnit" || this.orderQueue[i].data.target != msg.entity)
			continue;
		if (i == 0)
			this.UnitFsm.ProcessMessage(this, { "type": "PickupCanceled", "data": msg });
		else
			this.orderQueue.splice(i, 1);
		Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
		break;
	}
};

/**
 * Wrapper function that sets up the LOS, healer and attack range queries.
 * This should be called whenever our ownership changes.
 */
UnitAI.prototype.SetupRangeQueries = function()
{
	if (this.GetStance().respondFleeOnSight)
		this.SetupLOSRangeQuery();

	if (this.IsHealer())
		this.SetupHealRangeQuery();

	if (Engine.QueryInterface(this.entity, IID_Attack))
		this.SetupAttackRangeQuery();
};

UnitAI.prototype.UpdateRangeQueries = function()
{
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (this.losRangeQuery)
		this.SetupLOSRangeQuery(cmpRangeManager.IsActiveQueryEnabled(this.losRangeQuery));

	if (this.losHealRangeQuery)
		this.SetupHealRangeQuery(cmpRangeManager.IsActiveQueryEnabled(this.losHealRangeQuery));

	if (this.losAttackRangeQuery)
		this.SetupAttackRangeQuery(cmpRangeManager.IsActiveQueryEnabled(this.losAttackRangeQuery));
};

/**
 * Set up a range query for all enemy units within LOS range.
 * @param {boolean} enable - Optional parameter whether to enable the query.
 */
UnitAI.prototype.SetupLOSRangeQuery = function(enable = true)
{
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (this.losRangeQuery)
	{
		cmpRangeManager.DestroyActiveQuery(this.losRangeQuery);
		this.losRangeQuery = undefined;
	}

	let cmpPlayer = QueryOwnerInterface(this.entity);
	// If we are being destructed (owner == -1), creating a range query is pointless.
	if (!cmpPlayer)
		return;

	let players = cmpPlayer.GetEnemies();
	if (!players.length)
		return;

	let range = this.GetQueryRange(IID_Vision);
	// Do not compensate for entity sizes: LOS doesn't, and UnitAI relies on that.
	this.losRangeQuery = cmpRangeManager.CreateActiveQuery(this.entity,
		range.min, range.max, players, IID_Identity,
		cmpRangeManager.GetEntityFlagMask("normal"), false);

	if (enable)
		cmpRangeManager.EnableActiveQuery(this.losRangeQuery);
};

/**
 * Set up a range query for all own or ally units within LOS range
 * which can be healed.
 * @param {boolean} enable - Optional parameter whether to enable the query.
 */
UnitAI.prototype.SetupHealRangeQuery = function(enable = true)
{
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);

	if (this.losHealRangeQuery)
	{
		cmpRangeManager.DestroyActiveQuery(this.losHealRangeQuery);
		this.losHealRangeQuery = undefined;
	}

	let cmpPlayer = QueryOwnerInterface(this.entity);
	// If we are being destructed (owner == -1), creating a range query is pointless.
	if (!cmpPlayer)
		return;

	let players = cmpPlayer.GetAllies();
	let range = this.GetQueryRange(IID_Heal);

	// Do not compensate for entity sizes: LOS doesn't, and UnitAI relies on that.
	this.losHealRangeQuery = cmpRangeManager.CreateActiveQuery(this.entity,
		range.min, range.max, players, IID_Health,
		cmpRangeManager.GetEntityFlagMask("injured"), false);

	if (enable)
		cmpRangeManager.EnableActiveQuery(this.losHealRangeQuery);
};

/**
 * Set up a range query for all enemy and gaia units within range
 * which can be attacked.
 * @param {boolean} enable - Optional parameter whether to enable the query.
 */
UnitAI.prototype.SetupAttackRangeQuery = function(enable = true)
{
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);

	if (this.losAttackRangeQuery)
	{
		cmpRangeManager.DestroyActiveQuery(this.losAttackRangeQuery);
		this.losAttackRangeQuery = undefined;
	}

	let cmpPlayer = QueryOwnerInterface(this.entity);
	// If we are being destructed (owner == -1), creating a range query is pointless.
	if (!cmpPlayer)
		return;

	// TODO: How to handle neutral players - Special query to attack military only?
	let players = cmpPlayer.GetEnemies();
	if (!players.length)
		return;

	let range = this.GetQueryRange(IID_Attack);
	// Do not compensate for entity sizes: LOS doesn't, and UnitAI relies on that.
	this.losAttackRangeQuery = cmpRangeManager.CreateActiveQuery(this.entity,
		range.min, range.max, players, IID_Resistance,
		cmpRangeManager.GetEntityFlagMask("normal"), false);

	if (enable)
		cmpRangeManager.EnableActiveQuery(this.losAttackRangeQuery);
};


// FSM linkage functions

// Setting the next state to the current state will leave/re-enter the top-most substate.
// Must be called from inside the FSM.
UnitAI.prototype.SetNextState = function(state)
{
	this.UnitFsm.SetNextState(this, state);
};

// Must be called from inside the FSM.
UnitAI.prototype.DeferMessage = function(msg)
{
	this.UnitFsm.DeferMessage(this, msg);
};

UnitAI.prototype.GetCurrentState = function()
{
	return this.UnitFsm.GetCurrentState(this);
};

UnitAI.prototype.FsmStateNameChanged = function(state)
{
	Engine.PostMessage(this.entity, MT_UnitAIStateChanged, { "to": state });
};

/**
 * Call when the current order has been completed (or failed).
 * Removes the current order from the queue, and processes the
 * next one (if any). Returns false and defaults to IDLE
 * if there are no remaining orders or if the unit is not
 * inWorld and not garrisoned (thus usually waiting to be destroyed).
 * Must be called from inside the FSM.
 */
UnitAI.prototype.FinishOrder = function()
{
	if (!this.orderQueue.length)
	{
		let stack = new Error().stack.trimRight().replace(/^/mg, '  '); // indent each line
		let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
		let template = cmpTemplateManager.GetCurrentTemplateName(this.entity);
		error("FinishOrder called for entity " + this.entity + " (" + template + ") when order queue is empty\n" + stack);
	}

	this.orderQueue.shift();
	this.order = this.orderQueue[0];

	if (this.orderQueue.length && (this.isGarrisoned || this.IsFormationController() ||
	        Engine.QueryInterface(this.entity, IID_Position)?.IsInWorld()))
	{
		let ret = this.UnitFsm.ProcessMessage(this, {
			"type": "Order."+this.order.type,
			"data": this.order.data
		});

		Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });

		return ret;
	}

	this.orderQueue = [];
	this.order = undefined;

	// Switch to IDLE as a default state.
	this.SetNextState("IDLE");

	Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });

	// Check if there are queued formation orders
	if (this.IsFormationMember())
	{
		this.SetNextState("FORMATIONMEMBER.IDLE");
		let cmpUnitAI = Engine.QueryInterface(this.formationController, IID_UnitAI);
		if (cmpUnitAI)
		{
			// Inform the formation controller that we finished this task
			Engine.QueryInterface(this.formationController, IID_Formation).
				SetFinishedEntity(this.entity);
			// We don't want to carry out the default order
			// if there are still queued formation orders left
			if (cmpUnitAI.GetOrders().length > 1)
				return true;
		}
	}
	return false;
};

/**
 * Add an order onto the back of the queue,
 * and execute it if we didn't already have an order.
 */
UnitAI.prototype.PushOrder = function(type, data)
{
	var order = { "type": type, "data": data };
	this.orderQueue.push(order);

	if (this.orderQueue.length == 1)
	{
		this.order = order;
		this.UnitFsm.ProcessMessage(this, {
			"type": "Order."+this.order.type,
			"data": this.order.data
		});
	}

	Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
};

/**
 * Add an order onto the front of the queue,
 * and execute it immediately.
 */
UnitAI.prototype.PushOrderFront = function(type, data, ignorePacking = false)
{
	var order = { "type": type, "data": data };
	// If current order is packing/unpacking then add new order after it.
	if (!ignorePacking && this.order && this.IsPacking())
	{
		var packingOrder = this.orderQueue.shift();
		this.orderQueue.unshift(packingOrder, order);
	}
	else
	{
		this.orderQueue.unshift(order);
		this.order = order;
		this.UnitFsm.ProcessMessage(this, {
			"type": "Order."+this.order.type,
			"data": this.order.data
		});
	}

	Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });

};

/**
 * Insert an order after the last forced order onto the queue
 * and after the other orders of the same type
 */
UnitAI.prototype.PushOrderAfterForced = function(type, data)
{
	if (!this.order || ((!this.order.data || !this.order.data.force) && this.order.type != type))
		this.PushOrderFront(type, data);
	else
	{
		for (let i = 1; i < this.orderQueue.length; ++i)
		{
			if (this.orderQueue[i].data && this.orderQueue[i].data.force)
				continue;
			if (this.orderQueue[i].type == type)
				continue;
			this.orderQueue.splice(i, 0, { "type": type, "data": data });
			Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
			return;
		}
		this.PushOrder(type, data);
	}

	Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
};

/**
 * For a unit that is packing and trying to attack something,
 * either cancel packing or continue with packing, as appropriate.
 * Precondition:  if the unit is packing/unpacking, then orderQueue
 * should have the Attack order at index 0,
 * and the Pack/Unpack order at index 1.
 * This precondition holds because if we are packing while processing "Order.Attack",
 * then we must have come from ReplaceOrder, which guarantees it.
 *
 * @param {boolean} requirePacked - true if the unit needs to be packed to continue attacking,
 *   false if it needs to be unpacked.
 * @return {boolean} true if the unit can attack now, false if it must continue packing (or unpacking) first.
 */
UnitAI.prototype.EnsureCorrectPackStateForAttack = function(requirePacked)
{
	let cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	if (!cmpPack ||
	  !cmpPack.IsPacking() ||
	  this.orderQueue.length != 2 ||
	  this.orderQueue[0].type != "Attack" ||
	  this.orderQueue[1].type != "Pack" &&
	  this.orderQueue[1].type != "Unpack")
		return true;

	if (cmpPack.IsPacked() == requirePacked)
	{
		// The unit is already in the packed/unpacked state we want.
		// Delete the packing order.
		this.orderQueue.splice(1, 1);
		cmpPack.CancelPack();
		Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
		// Continue with the attack order.
		return true;
	}
	// Move the attack order behind the unpacking order, to continue unpacking.
	let tmp = this.orderQueue[0];
	this.orderQueue[0] = this.orderQueue[1];
	this.orderQueue[1] = tmp;
	Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
	return false;
};

UnitAI.prototype.WillMoveFromFoundation = function(target, checkPacking = true)
{
	let cmpUnitAI = Engine.QueryInterface(target, IID_UnitAI);
	if (!IsOwnedByAllyOfEntity(this.entity, target) && cmpUnitAI && !cmpUnitAI.IsAnimal() &&
	    !Engine.QueryInterface(SYSTEM_ENTITY, IID_CeasefireManager).IsCeasefireActive() ||
	    checkPacking && this.IsPacking() || this.CanPack() || !this.AbleToMove())
		return false;

	return !this.CheckTargetRangeExplicit(target, g_LeaveFoundationRange, -1);
};

UnitAI.prototype.ReplaceOrder = function(type, data)
{
	// Remember the previous work orders to be able to go back to them later if required
	if (data && data.force)
	{
		if (this.IsFormationController())
			this.CallMemberFunction("UpdateWorkOrders", [type]);
		else
			this.UpdateWorkOrders(type);
	}

	// Do not replace packing/unpacking unless it is cancel order.
	// TODO: maybe a better way of doing this would be to use priority levels
	if (this.IsPacking() && type != "CancelPack" && type != "CancelUnpack" && type != "Stop")
	{
		var order = { "type": type, "data": data };
		var packingOrder = this.orderQueue.shift();
		if (type == "Attack")
		{
			// The Attack order is able to handle a packing unit, while other orders can't.
			this.orderQueue = [packingOrder];
			this.PushOrderFront(type, data, true);
		}
		else if (packingOrder.type == "Unpack" && g_OrdersCancelUnpacking.has(type))
		{
			// Immediately cancel unpacking before processing an order that demands a packed unit.
			let cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
			cmpPack.CancelPack();
			this.orderQueue = [];
			this.PushOrder(type, data);
		}
		else
			this.orderQueue = [packingOrder, order];
	}
	else if (this.IsFormationMember())
	{
		// Don't replace orders after a LeaveFormation order
		// (this is needed to support queued no-formation orders).
		let idx = this.orderQueue.findIndex(o => o.type == "LeaveFormation");
		if (idx === -1)
		{
			this.orderQueue = [];
			this.order = undefined;
		}
		else
			this.orderQueue.splice(0, idx);
		this.PushOrderFront(type, data);
	}
	else
	{
		this.orderQueue = [];
		this.PushOrder(type, data);
	}

	Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
};

UnitAI.prototype.GetOrders = function()
{
	return this.orderQueue.slice();
};

UnitAI.prototype.AddOrders = function(orders)
{
	orders.forEach(order => this.PushOrder(order.type, order.data));
};

UnitAI.prototype.GetOrderData = function()
{
	var orders = [];
	for (let order of this.orderQueue)
		if (order.data)
			orders.push(clone(order.data));

	return orders;
};

UnitAI.prototype.UpdateWorkOrders = function(type)
{
	var isWorkType = type => type == "Gather" || type == "Trade" || type == "Repair" || type == "ReturnResource";
	if (isWorkType(type))
	{
		this.workOrders = [];
		return;
	}

	if (this.workOrders.length)
		return;

	if (this.IsFormationMember())
	{
		var cmpUnitAI = Engine.QueryInterface(this.formationController, IID_UnitAI);
		if (cmpUnitAI)
		{
			for (var i = 0; i < cmpUnitAI.orderQueue.length; ++i)
			{
				if (isWorkType(cmpUnitAI.orderQueue[i].type))
				{
					this.workOrders = cmpUnitAI.orderQueue.slice(i);
					return;
				}
			}
		}
	}

	// If nothing found, take the unit orders
	for (var i = 0; i < this.orderQueue.length; ++i)
	{
		if (isWorkType(this.orderQueue[i].type))
		{
			this.workOrders = this.orderQueue.slice(i);
			return;
		}
	}
};

UnitAI.prototype.BackToWork = function()
{
	if (this.workOrders.length == 0)
		return false;

	if (this.isGarrisoned && !Engine.QueryInterface(this.entity, IID_Garrisonable)?.UnGarrison(false))
		return false;

	const cmpTurretable = Engine.QueryInterface(this.entity, IID_Turretable);
	if (this.IsTurret(cmpTurretable) && !cmpTurretable.LeaveTurret())
		return false;

	this.orderQueue = [];

	this.AddOrders(this.workOrders);
	Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });

	if (this.IsFormationMember())
	{
		var cmpFormation = Engine.QueryInterface(this.formationController, IID_Formation);
		if (cmpFormation)
			cmpFormation.RemoveMembers([this.entity]);
	}

	this.workOrders = [];
	return true;
};

UnitAI.prototype.HasWorkOrders = function()
{
	return this.workOrders.length > 0;
};

UnitAI.prototype.GetWorkOrders = function()
{
	return this.workOrders;
};

UnitAI.prototype.SetWorkOrders = function(orders)
{
	this.workOrders = orders;
};

UnitAI.prototype.TimerHandler = function(data, lateness)
{
	// Reset the timer
	if (data.timerRepeat === undefined)
		this.timer = undefined;

	this.UnitFsm.ProcessMessage(this, { "type": "Timer", "data": data, "lateness": lateness });
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
	if (repeat === undefined)
		this.timer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "TimerHandler", offset, data);
	else
		this.timer = cmpTimer.SetInterval(this.entity, IID_UnitAI, "TimerHandler", offset, repeat, data);
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

UnitAI.prototype.OnMotionUpdate = function(msg)
{
	if (msg.veryObstructed)
		msg.obstructed = true;
	this.UnitFsm.ProcessMessage(this, Object.assign({ "type": "MovementUpdate" }, msg));
};

/**
 * Called directly by cmpFoundation and cmpRepairable to
 * inform builders that repairing has finished.
 * This not done by listening to a global message due to performance.
 */
UnitAI.prototype.ConstructionFinished = function(msg)
{
	this.UnitFsm.ProcessMessage(this, { "type": "ConstructionFinished", "data": msg });
};

UnitAI.prototype.OnGlobalEntityRenamed = function(msg)
{
	let changed = false;
	let currentOrderChanged = false;
	for (let i = 0; i < this.orderQueue.length; ++i)
	{
		let order = this.orderQueue[i];
		if (order.data && order.data.target && order.data.target == msg.entity)
		{
			changed = true;
			if (i == 0)
				currentOrderChanged = true;
			order.data.target = msg.newentity;
		}
		if (order.data && order.data.formationTarget && order.data.formationTarget == msg.entity)
		{
			changed = true;
			if (i == 0)
				currentOrderChanged = true;
			order.data.formationTarget = msg.newentity;
		}
	}
	if (!changed)
		return;

	if (currentOrderChanged)
		this.UnitFsm.ProcessMessage(this, { "type": "OrderTargetRenamed", "data": msg });

	Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
};

UnitAI.prototype.OnAttacked = function(msg)
{
	if (msg.fromStatusEffect)
		return;

	this.UnitFsm.ProcessMessage(this, { "type": "Attacked", "data": msg });
};

UnitAI.prototype.OnGuardedAttacked = function(msg)
{
	this.UnitFsm.ProcessMessage(this, { "type": "GuardedAttacked", "data": msg.data });
};

UnitAI.prototype.OnRangeUpdate = function(msg)
{
	if (msg.tag == this.losRangeQuery)
		this.UnitFsm.ProcessMessage(this, { "type": "LosRangeUpdate", "data": msg });
	else if (msg.tag == this.losHealRangeQuery)
		this.UnitFsm.ProcessMessage(this, { "type": "LosHealRangeUpdate", "data": msg });
	else if (msg.tag == this.losAttackRangeQuery)
		this.UnitFsm.ProcessMessage(this, { "type": "LosAttackRangeUpdate", "data": msg });
};

UnitAI.prototype.OnPackFinished = function(msg)
{
	this.UnitFsm.ProcessMessage(this, { "type": "PackFinished", "packed": msg.packed });
};

/**
 * A general function to process messages sent from components.
 * @param {string} type - The type of message to process.
 * @param {Object} msg - Optionally extra data to use.
 */
UnitAI.prototype.ProcessMessage = function(type, msg)
{
	this.UnitFsm.ProcessMessage(this, { "type": type, "data": msg });
};

// Helper functions to be called by the FSM

UnitAI.prototype.GetWalkSpeed = function()
{
	let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (!cmpUnitMotion)
		return 0;
	return cmpUnitMotion.GetWalkSpeed();
};

UnitAI.prototype.GetRunMultiplier = function()
{
	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (!cmpUnitMotion)
		return 0;
	return cmpUnitMotion.GetRunMultiplier();
};

/**
 * Returns true if the target exists and has non-zero hitpoints.
 */
UnitAI.prototype.TargetIsAlive = function(ent)
{
	var cmpFormation = Engine.QueryInterface(ent, IID_Formation);
	if (cmpFormation)
		return true;

	var cmpHealth = QueryMiragedInterface(ent, IID_Health);
	return cmpHealth && cmpHealth.GetHitpoints() != 0;
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
 * Returns the position of target or, if there is none,
 * the entity's position, or undefined.
 */
UnitAI.prototype.TargetPosOrEntPos = function(target)
{
	let cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
	if (cmpTargetPosition && cmpTargetPosition.IsInWorld())
		return cmpTargetPosition.GetPosition2D();

	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (cmpPosition && cmpPosition.IsInWorld())
		return cmpPosition.GetPosition2D();

	return undefined;
};


/**
 * Returns the entity ID of the nearest resource supply where the given
 * filter returns true, or undefined if none can be found.
 * "Nearest" is nearest from @param position.
 * TODO: extend this to exclude resources that already have lots of gatherers.
 */
UnitAI.prototype.FindNearbyResource = function(position, filter)
{
	if (!position)
		return undefined;

	// We accept resources owned by Gaia or any player
	let players = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetAllPlayers();

	let range = 64; // TODO: what's a sensible number?

	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	// Don't account for entity size, we need to match LOS visibility.
	let nearby = cmpRangeManager.ExecuteQueryAroundPos(position, 0, range, players, IID_ResourceSupply, false);
	return nearby.find(ent => {
		if (!this.CanGather(ent) || !this.CheckTargetVisible(ent))
			return false;

		let template = cmpTemplateManager.GetCurrentTemplateName(ent);
		if (template.indexOf("resource|") != -1)
			template = template.slice(9);

		let cmpResourceSupply = Engine.QueryInterface(ent, IID_ResourceSupply);
		let type = cmpResourceSupply.GetType();
		return cmpResourceSupply.IsAvailableTo(this.entity) && filter(ent, type, template);
	});
};

/**
 * Returns the entity ID of the nearest resource dropsite that accepts
 * the given type, or undefined if none can be found.
 */
UnitAI.prototype.FindNearestDropsite = function(genericType)
{
	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() == INVALID_PLAYER)
		return undefined;

	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return undefined;

	let pos = cmpPosition.GetPosition2D();
	let bestDropsite;
	let bestDist = Infinity;
	// Maximum distance a point on an obstruction can be from the center of the obstruction.
	let maxDifference = 40;

	let owner = cmpOwnership.GetOwner();
	let cmpPlayer = QueryOwnerInterface(this.entity);
	let players = cmpPlayer && cmpPlayer.HasSharedDropsites() ? cmpPlayer.GetMutualAllies() : [owner];
	let nearestDropsites = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).ExecuteQuery(this.entity, 0, -1, players, IID_ResourceDropsite, false);

	let isShip = Engine.QueryInterface(this.entity, IID_Identity).HasClass("Ship");
	let cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);
	for (let dropsite of nearestDropsites)
	{
		// Ships are unable to reach land dropsites and shouldn't attempt to do so.
		if (isShip && !Engine.QueryInterface(dropsite, IID_Identity).HasClass("Naval"))
			continue;

		let cmpResourceDropsite = Engine.QueryInterface(dropsite, IID_ResourceDropsite);
		if (!cmpResourceDropsite.AcceptsType(genericType) || !this.CheckTargetVisible(dropsite))
			continue;
		if (Engine.QueryInterface(dropsite, IID_Ownership).GetOwner() != owner && !cmpResourceDropsite.IsShared())
			continue;

		// The range manager sorts entities by the distance to their center,
		// but we want the distance to the point where resources will be dropped off.
		let dist = cmpObstructionManager.DistanceToPoint(dropsite, pos.x, pos.y);
		if (dist == -1)
			continue;

		if (dist < bestDist)
		{
			bestDropsite = dropsite;
			bestDist = dist;
		}
		else if (dist > bestDist + maxDifference)
			break;
	}
	return bestDropsite;
};

/**
 * Returns the entity ID of the nearest building that needs to be constructed.
 * "Nearest" is nearest from @param position.
 */
UnitAI.prototype.FindNearbyFoundation = function(position)
{
	if (!position)
		return undefined;

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() == INVALID_PLAYER)
		return undefined;

	let players = [cmpOwnership.GetOwner()];

	let range = 64; // TODO: what's a sensible number?
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	// Don't account for entity size, we need to match LOS visibility.
	let nearby = cmpRangeManager.ExecuteQueryAroundPos(position, 0, range, players, IID_Foundation, false);

	// Skip foundations that are already complete. (This matters since
	// we process the ConstructionFinished message before the foundation
	// we're working on has been deleted.)
	return nearby.find(ent => !Engine.QueryInterface(ent, IID_Foundation).IsFinished() && this.CheckTargetVisible(ent));
};

/**
 * Returns the entity ID of the nearest treasure.
 * "Nearest" is nearest from @param position.
 */
UnitAI.prototype.FindNearbyTreasure = function(position)
{
	if (!position)
		return undefined;

	let cmpTreasureCollector = Engine.QueryInterface(this.entity, IID_TreasureCollector);
	if (!cmpTreasureCollector)
		return undefined;

	let players = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetAllPlayers();

	let range = 64; // TODO: what's a sensible number?
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	// Don't account for entity size, we need to match LOS visibility.
	let nearby = cmpRangeManager.ExecuteQueryAroundPos(position, 0, range, players, IID_Treasure, false);
	return nearby.find(ent => cmpTreasureCollector.CanCollect(ent) && this.CheckTargetVisible(ent));
};

/**
 * Play a sound appropriate to the current entity.
 */
UnitAI.prototype.PlaySound = function(name)
{
	if (this.IsFormationController())
	{
		var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
		var member = cmpFormation.GetPrimaryMember();
		if (member)
			PlaySound(name, member);
	}
	else
	{
		PlaySound(name, this.entity);
	}
};

/*
 * Set a visualActor animation variant.
 * By changing the animation variant, you can change animations based on unitAI state.
 * If there are no specific variants or the variant doesn't exist in the actor,
 * the actor fallbacks to any existing animation.
 * @param type if present, switch to a specific animation variant.
 */
UnitAI.prototype.SetAnimationVariant = function(type)
{
	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpVisual)
		return;

	cmpVisual.SetVariant("animationVariant", type);
};

/*
 * Reset the animation variant to default behavior.
 * Default behavior is to pick a resource-carrying variant if resources are being carried.
 * Otherwise pick nothing in particular.
 */
UnitAI.prototype.SetDefaultAnimationVariant = function()
{
	let cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
	if (cmpResourceGatherer)
	{
		let type = cmpResourceGatherer.GetLastCarriedType();
		if (type)
		{
			let typename = "carry_" + type.generic;

			if (type.specific == "meat")
				typename = "carry_" + type.specific;

			this.SetAnimationVariant(typename);
			return;
		}
	}

	this.SetAnimationVariant("");
};

UnitAI.prototype.ResetAnimation = function()
{
	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpVisual)
		return;

	cmpVisual.SelectAnimation("idle", false, 1.0);
};

UnitAI.prototype.SelectAnimation = function(name, once = false, speed = 1.0)
{
	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpVisual)
		return;

	cmpVisual.SelectAnimation(name, once, speed);
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
	let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (cmpUnitMotion)
		cmpUnitMotion.StopMoving();
};

/**
 * Generic dispatcher for other MoveTo functions.
 * @param iid - Interface ID (optional) implementing GetRange
 * @param type - Range type for the interface call
 * @returns whether the move succeeded or failed.
 */
UnitAI.prototype.MoveTo = function(data, iid, type)
{
	if (data.target)
	{
		if (data.min || data.max)
			return this.MoveToTargetRangeExplicit(data.target, data.min || -1, data.max || -1);
		else if (!iid)
			return this.MoveToTarget(data.target);

		return this.MoveToTargetRange(data.target, iid, type);
	}
	else if (data.min || data.max)
		return this.MoveToPointRange(data.x, data.z, data.min || -1, data.max || -1);

	return this.MoveToPoint(data.x, data.z);
};

UnitAI.prototype.MoveToPoint = function(x, z)
{
	let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return this.AbleToMove(cmpUnitMotion) && cmpUnitMotion.MoveToPointRange(x, z, 0, 0); // For point goals, allow a max range of 0.
};

UnitAI.prototype.MoveToPointRange = function(x, z, rangeMin, rangeMax)
{
	let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return this.AbleToMove(cmpUnitMotion) && cmpUnitMotion.MoveToPointRange(x, z, rangeMin, rangeMax);
};

UnitAI.prototype.MoveToTarget = function(target)
{
	if (!this.CheckTargetVisible(target))
		return false;

	let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return this.AbleToMove(cmpUnitMotion) && cmpUnitMotion.MoveToTargetRange(target, 0, 1);
};

UnitAI.prototype.MoveToTargetRange = function(target, iid, type)
{
	if (!this.CheckTargetVisible(target))
		return false;

	let range = this.GetRange(iid, type, target);
	if (!range)
		return false;

	let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return this.AbleToMove(cmpUnitMotion) && cmpUnitMotion.MoveToTargetRange(target, range.min, range.max);
};

/**
 * Move unit so we hope the target is in the attack range
 * for melee attacks, this goes straight to the default range checks
 * for ranged attacks, the parabolic range is used
 */
UnitAI.prototype.MoveToTargetAttackRange = function(target, type)
{
	// for formation members, the formation will take care of the range check
	if (this.IsFormationMember())
	{
		let cmpFormationUnitAI = Engine.QueryInterface(this.formationController, IID_UnitAI);
		if (cmpFormationUnitAI && cmpFormationUnitAI.IsAttackingAsFormation())
			return false;
	}

	const cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (!this.AbleToMove(cmpUnitMotion))
		return false;

	const cmpFormation = Engine.QueryInterface(target, IID_Formation);
	if (cmpFormation)
		target = cmpFormation.GetClosestMember(this.entity);

	if (type != "Ranged")
		return this.MoveToTargetRange(target, IID_Attack, type);

	if (!this.CheckTargetVisible(target))
		return false;

	const cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return false;
	const range = cmpAttack.GetRange(type);

	// In case the range returns negative, we are probably too high compared to the target. Hope we come close enough.
	const parabolicMaxRange = Math.max(0, Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).GetEffectiveParabolicRange(this.entity, target, range.max, cmpAttack.GetAttackYOrigin(type)));

	// The parabole changes while walking so be cautious:
	const guessedMaxRange = parabolicMaxRange > range.max ? (range.max + parabolicMaxRange) / 2 : parabolicMaxRange;

	return cmpUnitMotion && cmpUnitMotion.MoveToTargetRange(target, range.min, guessedMaxRange);
};

UnitAI.prototype.MoveToTargetRangeExplicit = function(target, min, max)
{
	if (!this.CheckTargetVisible(target))
		return false;

	let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return this.AbleToMove(cmpUnitMotion) && cmpUnitMotion.MoveToTargetRange(target, min, max);
};

/**
 * Move unit so we hope the target is in the attack range of the formation.
 *
 * @param {number} target - The target entity ID to attack.
 * @return {boolean} - Whether the order to move has succeeded.
 */
UnitAI.prototype.MoveFormationToTargetAttackRange = function(target)
{
	let cmpTargetFormation = Engine.QueryInterface(target, IID_Formation);
	if (cmpTargetFormation)
		target = cmpTargetFormation.GetClosestMember(this.entity);

	if (!this.CheckTargetVisible(target))
		return false;

	let cmpFormationAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpFormationAttack)
		return false;
	let range = cmpFormationAttack.GetRange(target);

	let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return this.AbleToMove(cmpUnitMotion) && cmpUnitMotion.MoveToTargetRange(target, range.min, range.max);
};

/**
 * Generic dispatcher for other Check...Range functions.
 * @param iid - Interface ID (optional) implementing GetRange
 * @param type - Range type for the interface call
 */
UnitAI.prototype.CheckRange = function(data, iid, type)
{
	if (data.target)
	{
		if (data.min || data.max)
			return this.CheckTargetRangeExplicit(data.target, data.min || -1, data.max || -1);
		else if (!iid)
			return this.CheckTargetRangeExplicit(data.target, 0, 1);

		return this.CheckTargetRange(data.target, iid, type);
	}
	else if (data.min || data.max)
		return this.CheckPointRangeExplicit(data.x, data.z, data.min || -1, data.max || -1);

	return this.CheckPointRangeExplicit(data.x, data.z, 0, 0);
};

UnitAI.prototype.CheckPointRangeExplicit = function(x, z, min, max)
{
	let cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);
	return cmpObstructionManager.IsInPointRange(this.entity, x, z, min, max, false);
};

UnitAI.prototype.CheckTargetRange = function(target, iid, type)
{
	let range = this.GetRange(iid, type, target);
	if (!range)
		return false;

	let cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);
	return cmpObstructionManager.IsInTargetRange(this.entity, target, range.min, range.max, false);
};

/**
 * Check if the target is inside the attack range
 * For melee attacks, this goes straigt to the regular range calculation
 * For ranged attacks, the parabolic formula is used to accout for bigger ranges
 * when the target is lower, and smaller ranges when the target is higher
 */
UnitAI.prototype.CheckTargetAttackRange = function(target, type)
{
	// for formation members, the formation will take care of the range check
	if (this.IsFormationMember())
	{
		let cmpFormationUnitAI = Engine.QueryInterface(this.formationController, IID_UnitAI);
		if (cmpFormationUnitAI && cmpFormationUnitAI.IsAttackingAsFormation() &&
		    cmpFormationUnitAI.order.data.target == target)
			return true;
	}

	let cmpFormation = Engine.QueryInterface(target, IID_Formation);
	if (cmpFormation)
		target = cmpFormation.GetClosestMember(this.entity);

	let cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	return cmpAttack && cmpAttack.IsTargetInRange(target, type);
};

UnitAI.prototype.CheckTargetRangeExplicit = function(target, min, max)
{
	let cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);
	return cmpObstructionManager.IsInTargetRange(this.entity, target, min, max, false);
};

/**
 * Check if the target is inside the attack range of the formation.
 *
 * @param {number} target - The target entity ID to attack.
 * @return {boolean} - Whether the entity is within attacking distance.
 */
UnitAI.prototype.CheckFormationTargetAttackRange = function(target)
{
	let cmpTargetFormation = Engine.QueryInterface(target, IID_Formation);
	if (cmpTargetFormation)
		target = cmpTargetFormation.GetClosestMember(this.entity);

	let cmpFormationAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpFormationAttack)
		return false;
	let range = cmpFormationAttack.GetRange(target);

	let cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);
	return cmpObstructionManager.IsInTargetRange(this.entity, target, range.min, range.max, false);
};

/**
 * Returns true if the target entity is visible through the FoW/SoD.
 */
UnitAI.prototype.CheckTargetVisible = function(target)
{
	if (this.isGarrisoned)
		return false;

	const cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
		return false;

	const cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (!cmpRangeManager)
		return false;

	// Entities that are hidden and miraged are considered visible
	const cmpFogging = Engine.QueryInterface(target, IID_Fogging);
	if (cmpFogging && cmpFogging.IsMiraged(cmpOwnership.GetOwner()))
		return true;

	if (cmpRangeManager.GetLosVisibility(target, cmpOwnership.GetOwner()) == "hidden")
		return false;

	// Either visible directly, or visible in fog
	return true;
};

/**
 * Returns true if the given position is currentl visible (not in FoW/SoD).
 */
UnitAI.prototype.CheckPositionVisible = function(x, z)
{
	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
		return false;

	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (!cmpRangeManager)
		return false;

	return cmpRangeManager.GetLosVisibilityPosition(x, z, cmpOwnership.GetOwner()) == "visible";
};

/**
 * How close to our goal do we consider it's OK to stop if the goal appears unreachable.
 * Currently 3 terrain tiles as that's relatively close but helps pathfinding.
 */
UnitAI.prototype.DefaultRelaxedMaxRange = 12;

/**
 * @returns true if the unit is in the relaxed-range from the target.
 */
UnitAI.prototype.RelaxedMaxRangeCheck = function(data, relaxedRange)
{
	if (!data.relaxed)
		return false;

	let ndata = data;
	ndata.min = 0;
	ndata.max = relaxedRange;
	return this.CheckRange(ndata);
};

/**
 * Let an entity face its target.
 * @param {number} target - The entity-ID of the target.
 */
UnitAI.prototype.FaceTowardsTarget = function(target)
{
	let cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld())
		return;

	let targetPosition = cmpTargetPosition.GetPosition2D();

	// Use cmpUnitMotion for units that support that, otherwise try cmpPosition (e.g. turrets)
	let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (cmpUnitMotion)
	{
		cmpUnitMotion.FaceTowardsPoint(targetPosition.x, targetPosition.y);
		return;
	}

	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (cmpPosition && cmpPosition.IsInWorld())
		cmpPosition.TurnTo(cmpPosition.GetPosition2D().angleTo(targetPosition));
};

UnitAI.prototype.CheckTargetDistanceFromHeldPosition = function(target, iid, type)
{
	let range = this.GetRange(iid, type, target);
	if (!range)
		return false;

	let cmpPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return false;

	let cmpVision = Engine.QueryInterface(this.entity, IID_Vision);
	if (!cmpVision)
		return false;
	let halfvision = cmpVision.GetRange() / 2;

	let pos = cmpPosition.GetPosition();
	let heldPosition = this.heldPosition;
	if (heldPosition === undefined)
		heldPosition = { "x": pos.x, "z": pos.z };

	return Math.euclidDistance2D(pos.x, pos.z, heldPosition.x, heldPosition.z) < halfvision + range.max;
};

UnitAI.prototype.CheckTargetIsInVisionRange = function(target)
{
	let cmpVision = Engine.QueryInterface(this.entity, IID_Vision);
	if (!cmpVision)
		return false;

	let range = cmpVision.GetRange();
	let distance = PositionHelper.DistanceBetweenEntities(this.entity, target);

	return distance < range;
};

UnitAI.prototype.GetBestAttackAgainst = function(target, allowCapture = this.DEFAULT_CAPTURE)
{
	return Engine.QueryInterface(this.entity, IID_Attack)?.GetBestAttackAgainst(target, allowCapture);
};

/**
 * Try to find one of the given entities which can be attacked,
 * and start attacking it.
 * Returns true if it found something to attack.
 */
UnitAI.prototype.AttackVisibleEntity = function(ents)
{
	var target = ents.find(target => this.CanAttack(target));
	if (!target)
		return false;

	this.PushOrderFront("Attack", { "target": target, "force": false });
	return true;
};

/**
 * Try to find one of the given entities which can be attacked
 * and which is close to the hold position, and start attacking it.
 * Returns true if it found something to attack.
 */
UnitAI.prototype.AttackEntityInZone = function(ents)
{
	var target = ents.find(target =>
		this.CanAttack(target) &&
		this.CheckTargetDistanceFromHeldPosition(target, IID_Attack, this.GetBestAttackAgainst(target, true)) &&
		(this.GetStance().respondChaseBeyondVision || this.CheckTargetIsInVisionRange(target))
	);
	if (!target)
		return false;

	this.PushOrderFront("Attack", { "target": target, "force": false });
	return true;
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
		if (this.order && this.order.type == "Flee")
			this.orderQueue.shift();
		this.PushOrderFront("Flee", { "target": ents[0], "force": false });
		return true;
	}

	return false;
};

/**
 * @param {number} ents - An array of the IDs of the spotted entities.
 * @return {boolean} - Whether we responded.
 */
UnitAI.prototype.RespondToSightedEntities = function(ents)
{
	if (!ents || !ents.length)
		return false;

	if (this.GetStance().respondFleeOnSight)
	{
		this.Flee(ents[0], false);
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
	let ent = ents.find(ent => this.CanHeal(ent));
	if (!ent)
		return false;

	this.PushOrderFront("Heal", { "target": ent, "force": false });
	return true;
};

/**
 * Returns true if we should stop following the target entity.
 */
UnitAI.prototype.ShouldAbandonChase = function(target, force, iid, type)
{
	if (!this.CheckTargetVisible(target))
		return true;

	// Forced orders shouldn't be interrupted.
	if (force)
		return false;

	// If we are guarding/escorting, don't abandon as long as the guarded unit is in target range of the attacker
	if (this.isGuardOf)
	{
		let cmpUnitAI = Engine.QueryInterface(target, IID_UnitAI);
		let cmpAttack = Engine.QueryInterface(target, IID_Attack);
		if (cmpUnitAI && cmpAttack &&
		    cmpAttack.GetAttackTypes().some(type => cmpUnitAI.CheckTargetAttackRange(this.isGuardOf, type)))
			return false;
	}

	if (this.GetStance().respondHoldGround)
		if (!this.CheckTargetDistanceFromHeldPosition(target, iid, type))
			return true;

	// Stop if it's left our vision range, unless we're especially persistent.
	if (!this.GetStance().respondChaseBeyondVision)
		if (!this.CheckTargetIsInVisionRange(target))
			return true;

	return false;
};

/*
 * Returns whether we should chase the targeted entity,
 * given our current stance.
 */
UnitAI.prototype.ShouldChaseTargetedEntity = function(target, force)
{
	if (!this.AbleToMove())
		return false;

	if (this.GetStance().respondChase)
		return true;

	// If we are guarding/escorting, chase at least as long as the guarded unit is in target range of the attacker
	if (this.isGuardOf)
	{
		let cmpUnitAI = Engine.QueryInterface(target, IID_UnitAI);
		let cmpAttack = Engine.QueryInterface(target, IID_Attack);
		if (cmpUnitAI && cmpAttack &&
		    cmpAttack.GetAttackTypes().some(type => cmpUnitAI.CheckTargetAttackRange(this.isGuardOf, type)))
			return true;
	}

	return force;
};

// External interface functions

/**
 * Order a unit to leave the formation it is in.
 * Used to handle queued no-formation orders for units in formation.
 */
UnitAI.prototype.LeaveFormation = function(queued = true)
{
	// If queued, add the order even if we're not in formation,
	// maybe we will be later.
	if (!queued && !this.IsFormationMember())
		return;

	if (queued)
		this.AddOrder("LeaveFormation", { "force": true }, queued);
	else
		this.PushOrderFront("LeaveFormation", { "force": true });
};

UnitAI.prototype.SetFormationController = function(ent)
{
	this.formationController = ent;

	// Set obstruction group, so we can walk through members of our own formation.
	Engine.QueryInterface(this.entity, IID_Obstruction)?.SetControlGroup(ent);
	Engine.QueryInterface(this.entity, IID_UnitMotion)?.SetMemberOfFormation(ent);
};

UnitAI.prototype.UnsetFormationController = function()
{
	this.formationController = INVALID_ENTITY;
	Engine.QueryInterface(this.entity, IID_Obstruction)?.SetControlGroup(this.entity);
	Engine.QueryInterface(this.entity, IID_UnitMotion)?.SetMemberOfFormation(this.formationController);
	this.UnitFsm.ProcessMessage(this, { "type": "FormationLeave" });
};

UnitAI.prototype.GetFormationController = function()
{
	return this.formationController;
};

UnitAI.prototype.GetFormationTemplate = function()
{
	return Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager).GetCurrentTemplateName(this.formationController) || NULL_FORMATION;
};

UnitAI.prototype.MoveIntoFormation = function(cmd)
{
	var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
	if (!cmpFormation)
		return;

	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return;

	var pos = cmpPosition.GetPosition();
	this.PushOrderFront("MoveIntoFormation", { "x": pos.x, "z": pos.z, "force": true });
};

UnitAI.prototype.GetTargetPositions = function()
{
	var targetPositions = [];
	for (var i = 0; i < this.orderQueue.length; ++i)
	{
		var order = this.orderQueue[i];
		switch (order.type)
		{
		case "Walk":
		case "WalkAndFight":
		case "WalkToPointRange":
		case "MoveIntoFormation":
		case "GatherNearPosition":
		case "Patrol":
			targetPositions.push(new Vector2D(order.data.x, order.data.z));
			break; // and continue the loop

		case "WalkToTarget":
		case "WalkToTargetRange": // This doesn't move to the target (just into range), but a later order will.
		case "Guard":
		case "Flee":
		case "LeaveFoundation":
		case "Attack":
		case "Heal":
		case "Gather":
		case "ReturnResource":
		case "Repair":
		case "Garrison":
		case "CollectTreasure":
			var cmpTargetPosition = Engine.QueryInterface(order.data.target, IID_Position);
			if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld())
				return targetPositions;
			targetPositions.push(cmpTargetPosition.GetPosition2D());
			return targetPositions;

		case "Stop":
			return [];

		case "DropAtNearestDropSite":
			break;

		default:
			error("GetTargetPositions: Unrecognised order type '"+order.type+"'");
			return [];
		}
	}
	return targetPositions;
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
	var pos = cmpPosition.GetPosition2D();
	var targetPositions = this.GetTargetPositions();
	for (var i = 0; i < targetPositions.length; ++i)
	{
		distance += pos.distanceTo(targetPositions[i]);

		// Remember this as the start position for the next order
		pos = targetPositions[i];
	}

	return distance;
};

UnitAI.prototype.AddOrder = function(type, data, queued, pushFront)
{
	if (this.expectedRoute)
		this.expectedRoute = undefined;

	if (pushFront)
		this.PushOrderFront(type, data);
	else if (queued)
		this.PushOrder(type, data);
	else
		this.ReplaceOrder(type, data);
};

/**
 * Adds guard/escort order to the queue, forced by the player.
 */
UnitAI.prototype.Guard = function(target, queued, pushFront)
{
	if (!this.CanGuard())
	{
		this.WalkToTarget(target, queued);
		return;
	}

	if (target === this.entity)
		return;

	if (this.isGuardOf)
	{
		if (this.isGuardOf == target && this.order && this.order.type == "Guard")
			return;
		this.RemoveGuard();
	}

	this.AddOrder("Guard", { "target": target, "force": false }, queued, pushFront);
};

/**
 * @return {boolean} - Whether it makes sense to guard the given entity.
 */
UnitAI.prototype.ShouldGuard = function(target)
{
	return this.TargetIsAlive(target) ||
		Engine.QueryInterface(target, IID_Capturable) ||
		Engine.QueryInterface(target, IID_StatusEffectsReceiver);
};

UnitAI.prototype.AddGuard = function(target)
{
	if (!this.CanGuard())
		return false;

	var cmpGuard = Engine.QueryInterface(target, IID_Guard);
	if (!cmpGuard)
		return false;

	this.isGuardOf = target;
	this.guardRange = cmpGuard.GetRange(this.entity);
	cmpGuard.AddGuard(this.entity);
	return true;
};

UnitAI.prototype.RemoveGuard = function()
{
	if (!this.isGuardOf)
		return;

	let cmpGuard = Engine.QueryInterface(this.isGuardOf, IID_Guard);
	if (cmpGuard)
		cmpGuard.RemoveGuard(this.entity);
	this.guardRange = undefined;
	this.isGuardOf = undefined;

	if (!this.order)
		return;

	if (this.order.type == "Guard")
		this.UnitFsm.ProcessMessage(this, { "type": "RemoveGuard" });
	else
		for (let i = 1; i < this.orderQueue.length; ++i)
			if (this.orderQueue[i].type == "Guard")
				this.orderQueue.splice(i, 1);
	Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
};

UnitAI.prototype.IsGuardOf = function()
{
	return this.isGuardOf;
};

UnitAI.prototype.SetGuardOf = function(entity)
{
	// entity may be undefined
	this.isGuardOf = entity;
};

UnitAI.prototype.CanGuard = function()
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds)
	if (this.IsFormationController())
		return true;

	return this.template.CanGuard == "true";
};

UnitAI.prototype.CanPatrol = function()
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds)
	return this.IsFormationController() || this.template.CanPatrol == "true";
};

/**
 * Adds walk order to queue, forced by the player.
 */
UnitAI.prototype.Walk = function(x, z, queued, pushFront)
{
	if (!pushFront && this.expectedRoute && queued)
		this.expectedRoute.push({ "x": x, "z": z });
	else
		this.AddOrder("Walk", { "x": x, "z": z, "force": true }, queued, pushFront);
};

/**
 * Adds walk to point range order to queue, forced by the player.
 */
UnitAI.prototype.WalkToPointRange = function(x, z, min, max, queued, pushFront)
{
	this.AddOrder("Walk", { "x": x, "z": z, "min": min, "max": max, "force": true }, queued, pushFront);
};

/**
 * Adds stop order to queue, forced by the player.
 */
UnitAI.prototype.Stop = function(queued, pushFront)
{
	this.AddOrder("Stop", { "force": true }, queued, pushFront);
};

/**
 * The unit will drop all resources at the closest dropsite. If this unit is no gatherer or
 * no dropsite is available, it will do nothing.
 */
UnitAI.prototype.DropAtNearestDropSite = function(queued, pushFront)
{
	this.AddOrder("DropAtNearestDropSite", { "force": true }, queued, pushFront);
};

/**
 * Adds walk-to-target order to queue, this only occurs in response
 * to a player order, and so is forced.
 */
UnitAI.prototype.WalkToTarget = function(target, queued, pushFront)
{
	this.AddOrder("WalkToTarget", { "target": target, "force": true }, queued, pushFront);
};

/**
 * Adds walk-and-fight order to queue, this only occurs in response
 * to a player order, and so is forced.
 * If targetClasses is given, only entities matching the targetClasses can be attacked.
 */
UnitAI.prototype.WalkAndFight = function(x, z, targetClasses, allowCapture = this.DEFAULT_CAPTURE, queued = false, pushFront = false)
{
	this.AddOrder("WalkAndFight", { "x": x, "z": z, "targetClasses": targetClasses, "allowCapture": allowCapture, "force": true }, queued, pushFront);
};

UnitAI.prototype.Patrol = function(x, z, targetClasses, allowCapture = this.DEFAULT_CAPTURE, queued = false, pushFront = false)
{
	if (!this.CanPatrol())
	{
		this.Walk(x, z, queued);
		return;
	}

	this.AddOrder("Patrol", { "x": x, "z": z, "targetClasses": targetClasses, "allowCapture": allowCapture, "force": true }, queued, pushFront);
};

/**
 * Adds leave foundation order to queue, treated as forced.
 */
UnitAI.prototype.LeaveFoundation = function(target)
{
	// If we're already being told to leave a foundation, then
	// ignore this new request so we don't end up being too indecisive
	// to ever actually move anywhere.
	if (this.order && (this.order.type == "LeaveFoundation" || (this.order.type == "Flee" && this.order.data.target == target)))
		return;

	if (this.orderQueue.length && this.orderQueue[0].type == "Unpack" && this.WillMoveFromFoundation(target, false))
	{
		let cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
		if (cmpPack)
			cmpPack.CancelPack();
	}

	if (this.IsPacking())
		return;

	this.PushOrderFront("LeaveFoundation", { "target": target, "force": true });
};

/**
 * Adds attack order to the queue, forced by the player.
 */
UnitAI.prototype.Attack = function(target, allowCapture = this.DEFAULT_CAPTURE, queued = false, pushFront = false)
{
	if (!this.CanAttack(target))
	{
		// We don't want to let healers walk to the target unit so they can be easily killed.
		// Instead we just let them get into healing range.
		if (this.IsHealer())
			this.MoveToTargetRange(target, IID_Heal);
		else
			this.WalkToTarget(target, queued, pushFront);
		return;
	}

	let order = {
		"target": target,
		"force": true,
		"allowCapture": allowCapture,
	};

	this.RememberTargetPosition(order);

	if (this.order && this.order.type == "Attack" &&
		this.order.data &&
		this.order.data.target === order.target &&
		this.order.data.allowCapture === order.allowCapture)
	{
		this.order.data.lastPos = order.lastPos;
		this.order.data.force = order.force;
		if (order.force)
			this.orderQueue = [this.order];
		return;
	}

	this.AddOrder("Attack", order, queued, pushFront);
};

/**
 * Adds garrison order to the queue, forced by the player.
 */
UnitAI.prototype.Garrison = function(target, queued, pushFront)
{
	// Not allowed to garrison when occupying a turret, at the moment.
	if (this.isGarrisoned || this.IsTurret())
		return;
	if (target == this.entity)
		return;
	if (!this.CanGarrison(target))
	{
		this.WalkToTarget(target, queued);
		return;
	}
	this.AddOrder("Garrison", { "target": target, "force": true, "garrison": true }, queued, pushFront);
};

/**
 * Adds ungarrison order to the queue.
 */
UnitAI.prototype.Ungarrison = function()
{
	if (!this.isGarrisoned && !this.IsTurret())
		return;
	this.AddOrder("Ungarrison", null, false);
};

/**
 * Adds garrison order to the queue, forced by the player.
 */
UnitAI.prototype.OccupyTurret = function(target, queued, pushFront)
{
	if (target == this.entity)
		return;
	if (!this.CanOccupyTurret(target))
	{
		this.WalkToTarget(target, queued);
		return;
	}
	this.AddOrder("Garrison", { "target": target, "force": true, "garrison": false }, queued, pushFront);
};

/**
 * Adds gather order to the queue, forced by the player
 * until the target is reached
 */
UnitAI.prototype.Gather = function(target, queued, pushFront)
{
	this.PerformGather(target, queued, true, pushFront);
};

/**
 * Internal function to abstract the force parameter.
 */
UnitAI.prototype.PerformGather = function(target, queued, force, pushFront = false)
{
	if (!this.CanGather(target))
	{
		this.WalkToTarget(target, queued);
		return;
	}

	// Save the resource type now, so if the resource gets destroyed
	// before we process the order then we still know what resource
	// type to look for more of
	var type;
	var cmpResourceSupply = QueryMiragedInterface(target, IID_ResourceSupply);
	if (cmpResourceSupply)
		type = cmpResourceSupply.GetType();
	else
		error("CanGather allowed gathering from invalid entity");

	// Also save the target entity's template, so that if it's an animal,
	// we won't go from hunting slow safe animals to dangerous fast ones
	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var template = cmpTemplateManager.GetCurrentTemplateName(target);
	if (template.indexOf("resource|") != -1)
		template = template.slice(9);

	let order = {
		"target": target,
		"type": type,
		"template": template,
		"force": force,
	};

	this.RememberTargetPosition(order);
	order.initPos = order.lastPos;

	if (this.order &&
		(this.order.type == "Gather" || this.order.type == "Attack") &&
		this.order.data &&
		this.order.data.target === order.target)
	{
		this.order.data.lastPos = order.lastPos;
		this.order.data.force = order.force;
		if (order.force)
		{
			if (this.orderQueue[1]?.type === "Gather")
				this.orderQueue = [this.order, this.orderQueue[1]];
			else
				this.orderQueue = [this.order];
		}
		return;
	}

	this.AddOrder("Gather", order, queued, pushFront);
};

/**
 * Adds gather-near-position order to the queue, not forced, so it can be
 * interrupted by attacks.
 */
UnitAI.prototype.GatherNearPosition = function(x, z, type, template, queued, pushFront)
{
	if (template.indexOf("resource|") != -1)
		template = template.slice(9);

	if (this.IsFormationController() || Engine.QueryInterface(this.entity, IID_ResourceGatherer))
		this.AddOrder("GatherNearPosition", { "type": type, "template": template, "x": x, "z": z, "force": false }, queued, pushFront);
	else
		this.AddOrder("Walk", { "x": x, "z": z, "force": false }, queued, pushFront);
};

/**
 * Adds heal order to the queue, forced by the player.
 */
UnitAI.prototype.Heal = function(target, queued, pushFront)
{
	if (!this.CanHeal(target))
	{
		this.WalkToTarget(target, queued);
		return;
	}

	if (this.order && this.order.type == "Heal" &&
		this.order.data &&
		this.order.data.target === target)
	{
		this.order.data.force = true;
		this.orderQueue = [this.order];
		return;
	}

	this.AddOrder("Heal", { "target": target, "force": true }, queued, pushFront);
};

/**
 * Adds return resource order to the queue, forced by the player.
 */
UnitAI.prototype.ReturnResource = function(target, queued, pushFront)
{
	if (!this.CanReturnResource(target, true))
	{
		this.WalkToTarget(target, queued);
		return;
	}

	this.AddOrder("ReturnResource", { "target": target, "force": true }, queued, pushFront);
};

/**
 * Adds order to collect a treasure to queue, forced by the player.
 */
UnitAI.prototype.CollectTreasure = function(target, queued, pushFront)
{
	this.AddOrder("CollectTreasure", {
		"target": target,
		"force": true
	}, queued, pushFront);
};

/**
 * Adds order to collect a treasure to queue, forced by the player.
 */
UnitAI.prototype.CollectTreasureNearPosition = function(posX, posZ, queued, pushFront)
{
	this.AddOrder("CollectTreasureNearPosition", {
		"x": posX,
		"z": posZ,
		"force": true
	}, queued, pushFront);
};

UnitAI.prototype.CancelSetupTradeRoute = function(target)
{
	let cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
	if (!cmpTrader)
		return;
	cmpTrader.RemoveTargetMarket(target);

	if (this.IsFormationController())
		this.CallMemberFunction("CancelSetupTradeRoute", [target]);
};

/**
 * Adds trade order to the queue. Either walk to the first market, or
 * start a new route. Not forced, so it can be interrupted by attacks.
 * The possible route may be given directly as a SetupTradeRoute argument
 * if coming from a RallyPoint, or through this.expectedRoute if a user command.
 */
UnitAI.prototype.SetupTradeRoute = function(target, source, route, queued, pushFront)
{
	if (!this.CanTrade(target))
	{
		this.WalkToTarget(target, queued);
		return;
	}

	// AI has currently no access to BackToWork
	let cmpPlayer = QueryOwnerInterface(this.entity);
	if (cmpPlayer && cmpPlayer.IsAI() && !this.IsFormationController() &&
	    this.workOrders.length && this.workOrders[0].type == "Trade")
	{
		let cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
		if (cmpTrader.HasBothMarkets() &&
		   (cmpTrader.GetFirstMarket() == target && cmpTrader.GetSecondMarket() == source ||
		    cmpTrader.GetFirstMarket() == source && cmpTrader.GetSecondMarket() == target))
		{
			this.BackToWork();
			return;
		}
	}

	var marketsChanged = this.SetTargetMarket(target, source);
	if (!marketsChanged)
		return;

	var cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
	if (cmpTrader.HasBothMarkets())
	{
		let data = {
			"target": cmpTrader.GetFirstMarket(),
			"route": route,
			"force": false
		};

		if (this.expectedRoute)
		{
			if (!route && this.expectedRoute.length)
				data.route = this.expectedRoute.slice();
			this.expectedRoute = undefined;
		}

		if (this.IsFormationController())
		{
			this.CallMemberFunction("AddOrder", ["Trade", data, queued]);
			let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
			if (cmpFormation)
				cmpFormation.Disband();
		}
		else
			this.AddOrder("Trade", data, queued, pushFront);
	}
	else
	{
		if (this.IsFormationController())
			this.CallMemberFunction("WalkToTarget", [cmpTrader.GetFirstMarket(), queued, pushFront]);
		else
			this.WalkToTarget(cmpTrader.GetFirstMarket(), queued, pushFront);
		this.expectedRoute = [];
	}
};

UnitAI.prototype.SetTargetMarket = function(target, source)
{
	var cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
	if (!cmpTrader)
		return false;
	var marketsChanged = cmpTrader.SetTargetMarket(target, source);

	if (this.IsFormationController())
		this.CallMemberFunction("SetTargetMarket", [target, source]);

	return marketsChanged;
};

UnitAI.prototype.SwitchMarketOrder = function(oldMarket, newMarket)
{
	if (this.order && this.order.data && this.order.data.target && this.order.data.target == oldMarket)
		this.order.data.target = newMarket;
};

UnitAI.prototype.MoveToMarket = function(targetMarket)
{
	let nextTarget;
	if (this.waypoints && this.waypoints.length >= 1)
		nextTarget = this.waypoints.pop();
	else
		nextTarget = { "target": targetMarket };
	this.order.data.nextTarget = nextTarget;
	return this.MoveTo(this.order.data.nextTarget, IID_Trader);
};

UnitAI.prototype.MarketRemoved = function(market)
{
	if (this.order && this.order.data && this.order.data.target && this.order.data.target == market)
		this.UnitFsm.ProcessMessage(this, { "type": "TradingCanceled", "market": market });
};

/**
 * Adds repair/build order to the queue, forced by the player
 * until the target is reached
 */
UnitAI.prototype.Repair = function(target, autocontinue, queued, pushFront)
{
	if (!this.CanRepair(target))
	{
		this.WalkToTarget(target, queued);
		return;
	}

	if (this.order && this.order.type == "Repair" &&
		this.order.data &&
		this.order.data.target === target &&
		this.order.data.autocontinue === autocontinue)
	{
		this.order.data.force = true;
		this.orderQueue = [this.order];
		return;
	}

	this.AddOrder("Repair", { "target": target, "autocontinue": autocontinue, "force": true }, queued, pushFront);
};

/**
 * Adds flee order to the queue, not forced, so it can be
 * interrupted by attacks.
 */
UnitAI.prototype.Flee = function(target, queued, pushFront)
{
	this.AddOrder("Flee", { "target": target, "force": false }, queued, pushFront);
};

UnitAI.prototype.Cheer = function()
{
	this.PushOrderFront("Cheer", { "force": false });
};

UnitAI.prototype.Pack = function(queued, pushFront)
{
	if (this.CanPack())
		this.AddOrder("Pack", { "force": true }, queued, pushFront);
};

UnitAI.prototype.Unpack = function(queued, pushFront)
{
	if (this.CanUnpack())
		this.AddOrder("Unpack", { "force": true }, queued, pushFront);
};

UnitAI.prototype.CancelPack = function(queued, pushFront)
{
	var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	if (cmpPack && cmpPack.IsPacking() && !cmpPack.IsPacked())
		this.AddOrder("CancelPack", { "force": true }, queued, pushFront);
};

UnitAI.prototype.CancelUnpack = function(queued, pushFront)
{
	var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	if (cmpPack && cmpPack.IsPacking() && cmpPack.IsPacked())
		this.AddOrder("CancelUnpack", { "force": true }, queued, pushFront);
};

UnitAI.prototype.SetStance = function(stance)
{
	if (g_Stances[stance])
	{
		this.stance = stance;
		Engine.PostMessage(this.entity, MT_UnitStanceChanged, { "to": this.stance });
	}
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

	// Reset the range queries, since the range depends on stance.
	this.SetupRangeQueries();
};

UnitAI.prototype.SetTurretStance = function()
{
	this.SetImmobile();
	this.previousStance = undefined;
	if (this.GetStance().respondStandGround)
		return;
	for (let stance in g_Stances)
	{
		if (!g_Stances[stance].respondStandGround)
			continue;
		this.previousStance = this.GetStanceName();
		this.SwitchToStance(stance);
		return;
	}
};

UnitAI.prototype.ResetTurretStance = function()
{
	this.SetMobile();
	if (!this.previousStance)
		return;
	this.SwitchToStance(this.previousStance);
	this.previousStance = undefined;
};

/**
 * Resets the losRangeQuery.
 * @return {boolean} - Whether there are targets in range that we ought to react upon.
 */
UnitAI.prototype.FindSightedEnemies = function()
{
	if (!this.losRangeQuery)
		return false;

	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	return this.RespondToSightedEntities(cmpRangeManager.ResetActiveQuery(this.losRangeQuery));
};

/**
 * Resets losHealRangeQuery, and if there are some targets in range that we can heal
 * then we start healing and this returns true; otherwise, returns false.
 */
UnitAI.prototype.FindNewHealTargets = function()
{
	if (!this.losHealRangeQuery)
		return false;

	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	return this.RespondToHealableEntities(cmpRangeManager.ResetActiveQuery(this.losHealRangeQuery));
};

/**
 * Resets losAttackRangeQuery, and if there are some targets in range that we can
 * attack then we start attacking and this returns true; otherwise, returns false.
 */
UnitAI.prototype.FindNewTargets = function()
{
	if (!this.losAttackRangeQuery)
		return false;

	if (!this.GetStance().targetVisibleEnemies)
		return false;

	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	return this.AttackEntitiesByPreference(cmpRangeManager.ResetActiveQuery(this.losAttackRangeQuery));
};

UnitAI.prototype.FindWalkAndFightTargets = function()
{
	if (this.IsFormationController())
		return this.CallMemberFunction("FindWalkAndFightTargets", null);

	let cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);

	let entities;
	if (!this.losAttackRangeQuery || !this.GetStance().targetVisibleEnemies || !cmpAttack)
		entities = [];
	else
	{
		let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		entities = cmpRangeManager.ResetActiveQuery(this.losAttackRangeQuery);
	}

	let attackfilter = e => {
		if (this?.order?.data?.targetClasses)
		{
			let cmpIdentity = Engine.QueryInterface(e, IID_Identity);
			let targetClasses = this.order.data.targetClasses;
			if (cmpIdentity && targetClasses.attack &&
				!MatchesClassList(cmpIdentity.GetClassesList(), targetClasses.attack))
				return false;
			if (cmpIdentity && targetClasses.avoid &&
				MatchesClassList(cmpIdentity.GetClassesList(), targetClasses.avoid))
				return false;
			// Only used by the AIs to prevent some choices of targets
			if (targetClasses.vetoEntities && targetClasses.vetoEntities[e])
				return false;
		}
		let cmpOwnership = Engine.QueryInterface(e, IID_Ownership);
		if (cmpOwnership && cmpOwnership.GetOwner() > 0)
			return true;
		let cmpUnitAI = Engine.QueryInterface(e, IID_UnitAI);
		return cmpUnitAI && (!cmpUnitAI.IsAnimal() || cmpUnitAI.IsDangerousAnimal());
	};

	const attack = target => {
		const order = {
			"target": target,
			"force": false,
			"allowCapture": this.order?.data?.allowCapture || this.DEFAULT_CAPTURE
		};
		if (this.IsFormationMember())
			this.ReplaceOrder("Attack", order);
		else
			this.PushOrderFront("Attack", order);
	};

	let prefs = {};
	let bestPref;
	let targets = [];
	let pref;
	for (let v of entities)
	{
		if (this.CanAttack(v) && attackfilter(v))
		{
			pref = cmpAttack.GetPreference(v);
			if (pref === 0)
			{
				attack(v);
				return true;
			}
			targets.push(v);
		}
		prefs[v] = pref;
		if (pref !== undefined && (bestPref === undefined || pref < bestPref))
			bestPref = pref;
	}

	for (let targ of targets)
	{
		if (prefs[targ] !== bestPref)
			continue;
		attack(targ);
		return true;
	}

	// healers on a walk-and-fight order should heal injured units
	if (this.IsHealer())
		return this.FindNewHealTargets();

	return false;
};

UnitAI.prototype.GetQueryRange = function(iid)
{
	let ret = { "min": 0, "max": 0 };

	let cmpVision = Engine.QueryInterface(this.entity, IID_Vision);
	if (!cmpVision)
		return ret;
	let visionRange = cmpVision.GetRange();

	if (iid === IID_Vision)
	{
		ret.max = visionRange;
		return ret;
	}

	if (this.GetStance().respondStandGround)
	{
		let range = this.GetRange(iid);
		if (!range)
			return ret;
		ret.min = range.min;
		ret.max = Math.min(range.max, visionRange);
	}
	else if (this.GetStance().respondChase)
		ret.max = visionRange;
	else if (this.GetStance().respondHoldGround)
	{
		let range = this.GetRange(iid);
		if (!range)
			return ret;
		ret.max = Math.min(range.max + visionRange / 2, visionRange);
	}
	// We probably have stance 'passive' and we wouldn't have a range,
	// but as it is the default for healers we need to set it to something sane.
	else if (iid === IID_Heal)
		ret.max = visionRange;

	return ret;
};

UnitAI.prototype.GetStance = function()
{
	return g_Stances[this.stance];
};

UnitAI.prototype.GetSelectableStances = function()
{
	if (this.IsTurret())
		return [];
	return Object.keys(g_Stances).filter(key => g_Stances[key].selectable);
};

UnitAI.prototype.GetStanceName = function()
{
	return this.stance;
};

/*
 * Make the unit walk at its normal pace.
 */
UnitAI.prototype.ResetSpeedMultiplier = function()
{
	let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (cmpUnitMotion)
		cmpUnitMotion.SetSpeedMultiplier(1);
};

UnitAI.prototype.SetSpeedMultiplier = function(speed)
{
	let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (cmpUnitMotion)
		cmpUnitMotion.SetSpeedMultiplier(speed);
};

/**
 * Try to match the targets current movement speed.
 *
 * @param {number} target - The entity ID of the target to match.
 * @param {boolean} mayRun - Whether the entity is allowed to run to match the speed.
 */
UnitAI.prototype.TryMatchTargetSpeed = function(target, mayRun = true)
{
	let cmpUnitMotionTarget = Engine.QueryInterface(target, IID_UnitMotion);
	if (cmpUnitMotionTarget)
	{
		let targetSpeed = cmpUnitMotionTarget.GetCurrentSpeed();
		if (targetSpeed)
			this.SetSpeedMultiplier(Math.min(mayRun ? this.GetRunMultiplier() : 1, targetSpeed / this.GetWalkSpeed()));
	}
};

/*
 * Remember the position of the target (in lastPos), if any, in case it disappears later
 * and we want to head to its last known position.
 * @param orderData - The order data to set this on. Defaults to this.order.data
 */
UnitAI.prototype.RememberTargetPosition = function(orderData)
{
	if (!orderData)
		orderData = this.order.data;
	let cmpPosition = Engine.QueryInterface(orderData.target, IID_Position);
	if (cmpPosition && cmpPosition.IsInWorld())
		orderData.lastPos = cmpPosition.GetPosition();
};

UnitAI.prototype.SetHeldPosition = function(x, z)
{
	this.heldPosition = { "x": x, "z": z };
};

UnitAI.prototype.SetHeldPositionOnEntity = function(entity)
{
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return;
	var pos = cmpPosition.GetPosition();
	this.SetHeldPosition(pos.x, pos.z);
};

UnitAI.prototype.GetHeldPosition = function()
{
	return this.heldPosition;
};

UnitAI.prototype.WalkToHeldPosition = function()
{
	if (this.heldPosition)
	{
		this.AddOrder("Walk", { "x": this.heldPosition.x, "z": this.heldPosition.z, "force": false }, false, false);
		return true;
	}
	return false;
};

// Helper functions

/**
 * General getter for ranges.
 *
 * @param {number} iid
 * @param {number} target - [Optional]
 * @param {string} type - [Optional]
 * @return {Object | undefined} - The range in the form
 *	{ "min": number, "max": number }
 *	Returns undefined when the entity does not have the requested component.
 */
UnitAI.prototype.GetRange = function(iid, type, target)
{
	let component = Engine.QueryInterface(this.entity, iid);
	if (!component)
		return undefined;

	return component.GetRange(type, target);
};

UnitAI.prototype.CanAttack = function(target)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds)
	if (this.IsFormationController())
		return true;

	let cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	return cmpAttack && cmpAttack.CanAttack(target);
};

UnitAI.prototype.CanGarrison = function(target)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds).
	if (this.IsFormationController())
		return true;

	let cmpGarrisonable = Engine.QueryInterface(this.entity, IID_Garrisonable);
	return cmpGarrisonable && cmpGarrisonable.CanGarrison(target);
};

UnitAI.prototype.CanGather = function(target)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds).
	if (this.IsFormationController())
		return true;

	let cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
	return cmpResourceGatherer && cmpResourceGatherer.CanGather(target);
};

UnitAI.prototype.CanHeal = function(target)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds)
	if (this.IsFormationController())
		return true;

	let cmpHeal = Engine.QueryInterface(this.entity, IID_Heal);
	return cmpHeal && cmpHeal.CanHeal(target);
};

/**
 * Check if the entity can return carried resources at @param target
 * @param checkCarriedResource check we are carrying resources
 * @param cmpResourceGatherer if present, use this directly instead of re-querying.
 */
UnitAI.prototype.CanReturnResource = function(target, checkCarriedResource, cmpResourceGatherer = undefined)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds).
	if (this.IsFormationController())
		return true;

	if (!cmpResourceGatherer)
		cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);

	return cmpResourceGatherer && cmpResourceGatherer.CanReturnResource(target, checkCarriedResource);
};

UnitAI.prototype.CanTrade = function(target)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds).
	if (this.IsFormationController())
		return true;

	let cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
	return cmpTrader && cmpTrader.CanTrade(target);
};

UnitAI.prototype.CanRepair = function(target)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds).
	if (this.IsFormationController())
		return true;

	let cmpBuilder = Engine.QueryInterface(this.entity, IID_Builder);
	return cmpBuilder && cmpBuilder.CanRepair(target);
};

UnitAI.prototype.CanOccupyTurret = function(target)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds).
	if (this.IsFormationController())
		return true;

	let cmpTurretable = Engine.QueryInterface(this.entity, IID_Turretable);
	return cmpTurretable && cmpTurretable.CanOccupy(target);
};

UnitAI.prototype.CanPack = function()
{
	let cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	return cmpPack && cmpPack.CanPack();
};

UnitAI.prototype.CanUnpack = function()
{
	let cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	return cmpPack && cmpPack.CanUnpack();
};

UnitAI.prototype.IsPacking = function()
{
	let cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	return cmpPack && cmpPack.IsPacking();
};

// Formation specific functions

UnitAI.prototype.IsAttackingAsFormation = function()
{
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	return cmpAttack && cmpAttack.CanAttackAsFormation() &&
		this.GetCurrentState() == "FORMATIONCONTROLLER.COMBAT.ATTACKING";
};

UnitAI.prototype.MoveRandomly = function(distance)
{
	// To minimize drift all across the map, describe circles
	// approximated by polygons.
	// And to avoid getting stuck in obstacles or narrow spaces, each side
	// of the polygon is obtained by trying to go away from a point situated
	// half a meter backwards of the current position, after rotation.
	// We also add a fluctuation on the length of each side of the polygon (dist)
	// which, in addition to making the move more random, helps escaping narrow spaces
	// with bigger values of dist.

	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (!cmpPosition || !cmpPosition.IsInWorld() || !cmpUnitMotion)
		return;

	let pos = cmpPosition.GetPosition();
	let ang = cmpPosition.GetRotation().y;

	if (!this.roamAngle)
	{
		this.roamAngle = (randBool() ? 1 : -1) * Math.PI / 6;
		ang -= this.roamAngle / 2;
		this.startAngle = ang;
	}
	else if (Math.abs((ang - this.startAngle + Math.PI) % (2 * Math.PI) - Math.PI) < Math.abs(this.roamAngle / 2))
		this.roamAngle *= randBool() ? 1 : -1;

	let halfDelta = randFloat(this.roamAngle / 4, this.roamAngle * 3 / 4);
	// First half rotation to decrease the impression of immediate rotation
	ang += halfDelta;
	cmpUnitMotion.FaceTowardsPoint(pos.x + 0.5 * Math.sin(ang), pos.z + 0.5 * Math.cos(ang));
	// Then second half of the rotation
	ang += halfDelta;
	let dist = randFloat(0.5, 1.5) * distance;
	cmpUnitMotion.MoveToPointRange(pos.x - 0.5 * Math.sin(ang), pos.z - 0.5 * Math.cos(ang), dist, -1);
};

UnitAI.prototype.SetFacePointAfterMove = function(val)
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (cmpMotion)
		cmpMotion.SetFacePointAfterMove(val);
};

UnitAI.prototype.GetFacePointAfterMove = function()
{
	let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion && cmpUnitMotion.GetFacePointAfterMove();
};

UnitAI.prototype.AttackEntitiesByPreference = function(ents)
{
	if (!ents.length)
		return false;

	let cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return false;

	let attackfilter = function(e) {
		if (!cmpAttack.CanAttack(e))
			return false;

		let cmpOwnership = Engine.QueryInterface(e, IID_Ownership);
		if (cmpOwnership && cmpOwnership.GetOwner() > 0)
			return true;

		let cmpUnitAI = Engine.QueryInterface(e, IID_UnitAI);
		return cmpUnitAI && (!cmpUnitAI.IsAnimal() || cmpUnitAI.IsDangerousAnimal());
	};

	let entsByPreferences = {};
	let preferences = [];
	let entsWithoutPref = [];
	for (let ent of ents)
	{
		if (!attackfilter(ent))
			continue;
		let pref = cmpAttack.GetPreference(ent);
		if (pref === null || pref === undefined)
			entsWithoutPref.push(ent);
		else if (!entsByPreferences[pref])
		{
			preferences.push(pref);
			entsByPreferences[pref] = [ent];
		}
		else
			entsByPreferences[pref].push(ent);
	}

	if (preferences.length)
	{
		preferences.sort((a, b) => a - b);
		for (let pref of preferences)
			if (this.RespondToTargetedEntities(entsByPreferences[pref]))
				return true;
	}

	return this.RespondToTargetedEntities(entsWithoutPref);
};

/**
 * Call UnitAI.funcname(args) on all formation members.
 * @param resetFinishedEntities - If true, call ResetFinishedEntities first.
 *     If the controller wants to wait on its members to finish their order,
 *     this needs to be reset before sending new orders (in case they instafail)
 *     so it makes sense to do it here.
 *     Only set this to false if you're sure it's safe.
 */
UnitAI.prototype.CallMemberFunction = function(funcname, args, resetFinishedEntities = true)
{
	const cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
	if (!cmpFormation)
		return false;

	if (resetFinishedEntities)
		cmpFormation.ResetFinishedEntities();

	let result = false;
	cmpFormation.GetMembers().forEach(ent => {
		const cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		if (cmpUnitAI[funcname].apply(cmpUnitAI, args))
			result = true;
	});
	return result;
};

/**
 * Call obj.funcname(args) on UnitAI components owned by player in given range.
 */
UnitAI.prototype.CallPlayerOwnedEntitiesFunctionInRange = function(funcname, args, range)
{
	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
		return;
	let owner = cmpOwnership.GetOwner();
	if (owner == INVALID_PLAYER)
		return;
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	let nearby = cmpRangeManager.ExecuteQuery(this.entity, 0, range, [owner], IID_UnitAI, true);
	for (let i = 0; i < nearby.length; ++i)
	{
		let cmpUnitAI = Engine.QueryInterface(nearby[i], IID_UnitAI);
		cmpUnitAI[funcname].apply(cmpUnitAI, args);
	}
};

/**
 * Call obj.functname(args) on UnitAI components of all formation members,
 * and return true if all calls return true.
 */
UnitAI.prototype.TestAllMemberFunction = function(funcname, args)
{
	let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
	return cmpFormation && cmpFormation.GetMembers().every(ent => {
		let cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		return cmpUnitAI[funcname].apply(cmpUnitAI, args);
	});
};

UnitAI.prototype.UnitFsm = new FSM(UnitAI.prototype.UnitFsmSpec);

Engine.RegisterComponentType(IID_UnitAI, "UnitAI", UnitAI);
