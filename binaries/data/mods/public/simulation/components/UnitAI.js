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
	"<element name='CanGuard'>" +
		"<data type='boolean'/>" +
	"</element>" +
	"<element name='CanPatrol'>" +
		"<data type='boolean'/>" +
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
//   targetAttackersAlways: anything that hurts us is a viable target,
//     possibly overriding user orders!
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
		"targetVisibleEnemies": true,
		"targetAttackersAlways": true,
		"respondFlee": false,
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
		"respondChase": false,
		"respondChaseBeyondVision": false,
		"respondStandGround": true,
		"respondHoldGround": false,
		"selectable": true
	},
	"none": {
		// Only to be used by AI or trigger scripts
		"targetVisibleEnemies": false,
		"targetAttackersAlways": false,
		"respondFlee": false,
		"respondChase": false,
		"respondChaseBeyondVision": false,
		"respondStandGround": false,
		"respondHoldGround": false,
		"selectable": false
	}
};

// See ../helpers/FSM.js for some documentation of this FSM specification syntax
UnitAI.prototype.UnitFsmSpec = {

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

	// Formation handlers:

	"FormationLeave": function(msg) {
		// ignore when we're not in FORMATIONMEMBER
	},

	// Called when being told to walk as part of a formation
	"Order.FormationWalk": function(msg) {
		// Let players move captured domestic animals around
		if (this.IsAnimal() && !this.IsDomestic() || this.IsTurret())
		{
			this.FinishOrder();
			return;
		}

		// For packable units:
		// 1. If packed, we can move.
		// 2. If unpacked, we first need to pack, then follow case 1.
		if (this.CanPack())
		{
			this.PushOrderFront("Pack", { "force": true });
			return;
		}

		var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
		cmpUnitMotion.MoveToFormationOffset(msg.data.target, msg.data.x, msg.data.z);

		this.SetNextState("FORMATIONMEMBER.WALKING");
	},

	// Special orders:
	// (these will be overridden by various states)

	"Order.LeaveFoundation": function(msg) {
		// If foundation is not ally of entity, or if entity is unpacked siege,
		// ignore the order
		if (!IsOwnedByAllyOfEntity(this.entity, msg.data.target) && !Engine.QueryInterface(SYSTEM_ENTITY, IID_CeasefireManager).IsCeasefireActive() ||
			this.IsPacking() || this.CanPack() || this.IsTurret())
		{
			this.FinishOrder();
			return;
		}
		// Move a tile outside the building
		let range = 4;
		if (this.MoveToTargetRangeExplicit(msg.data.target, range, range))
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

	// Individual orders:
	// (these will switch the unit out of formation mode)

	"Order.Stop": function(msg) {
		// We have no control over non-domestic animals.
		if (this.IsAnimal() && !this.IsDomestic())
		{
			this.FinishOrder();
			return;
		}

		// Stop moving immediately.
		this.StopMoving();
		this.FinishOrder();

		// No orders left, we're an individual now
		if (this.IsAnimal())
			this.SetNextState("ANIMAL.IDLE");
		else
			this.SetNextState("INDIVIDUAL.IDLE");

	},

	"Order.Walk": function(msg) {
		// Let players move captured domestic animals around
		if (this.IsAnimal() && !this.IsDomestic() || this.IsTurret())
		{
			this.FinishOrder();
			return;
		}

		// For packable units:
		// 1. If packed, we can move.
		// 2. If unpacked, we first need to pack, then follow case 1.
		if (this.CanPack())
		{
			this.PushOrderFront("Pack", { "force": true });
			return;
		}

		this.SetHeldPosition(this.order.data.x, this.order.data.z);
		if (!this.order.data.max)
			this.MoveToPoint(this.order.data.x, this.order.data.z);
		else
			this.MoveToPointRange(this.order.data.x, this.order.data.z, this.order.data.min, this.order.data.max);
		if (this.IsAnimal())
			this.SetNextState("ANIMAL.WALKING");
		else
			this.SetNextState("INDIVIDUAL.WALKING");
	},

	"Order.WalkAndFight": function(msg) {
		// Let players move captured domestic animals around
		if (this.IsAnimal() && !this.IsDomestic() || this.IsTurret())
		{
			this.FinishOrder();
			return;
		}

		// For packable units:
		// 1. If packed, we can move.
		// 2. If unpacked, we first need to pack, then follow case 1.
		if (this.CanPack())
		{
			this.PushOrderFront("Pack", { "force": true });
			return;
		}

		this.SetHeldPosition(this.order.data.x, this.order.data.z);
		this.MoveToPoint(this.order.data.x, this.order.data.z);
		if (this.IsAnimal())
			this.SetNextState("ANIMAL.WALKING");   // WalkAndFight not applicable for animals
		else
			this.SetNextState("INDIVIDUAL.WALKINGANDFIGHTING");
	},


	"Order.WalkToTarget": function(msg) {
		// Let players move captured domestic animals around
		if (this.IsAnimal() && !this.IsDomestic() || this.IsTurret())
		{
			this.FinishOrder();
			return;
		}

		// For packable units:
		// 1. If packed, we can move.
		// 2. If unpacked, we first need to pack, then follow case 1.
		if (this.CanPack())
		{
			this.PushOrderFront("Pack", { "force": true });
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

	"Order.PickupUnit": function(msg) {
		var cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
		if (!cmpGarrisonHolder || cmpGarrisonHolder.IsFull())
		{
			this.FinishOrder();
			return;
		}

		// Check if we need to move     TODO implement a better way to know if we are on the shoreline
		var needToMove = true;
		var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
		if (this.lastShorelinePosition && cmpPosition && (this.lastShorelinePosition.x == cmpPosition.GetPosition().x)
		    && (this.lastShorelinePosition.z == cmpPosition.GetPosition().z))
		{
			// we were already on the shoreline, and have not moved since
			if (DistanceBetweenEntities(this.entity, this.order.data.target) < 50)
				needToMove = false;
		}

		// TODO: what if the units are on a cliff ? the ship will go below the cliff
		// and the units won't be able to garrison. Should go to the nearest (accessible) shore
		if (needToMove && this.MoveToTarget(this.order.data.target))
		{
			this.SetNextState("INDIVIDUAL.PICKUP.APPROACHING");
		}
		else
		{
			// We are already at the target, or can't move at all
			this.StopMoving();
			this.SetNextState("INDIVIDUAL.PICKUP.LOADING");
		}
	},

	"Order.Guard": function(msg) {
		if (!this.AddGuard(this.order.data.target))
		{
			this.FinishOrder();
			return;
		}

		if (this.MoveToTargetRangeExplicit(this.isGuardOf, 0, this.guardRange))
			this.SetNextState("INDIVIDUAL.GUARD.ESCORTING");
		else
			this.SetNextState("INDIVIDUAL.GUARD.GUARDING");
	},

	"Order.Flee": function(msg) {
		// We use the distance between the entities to account for ranged attacks
		var distance = DistanceBetweenEntities(this.entity, this.order.data.target) + (+this.template.FleeDistance);
		var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
		if (cmpUnitMotion.MoveToTargetRange(this.order.data.target, distance, -1))
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
		var type = this.GetBestAttackAgainst(this.order.data.target, this.order.data.allowCapture);
		if (!type)
		{
			// Oops, we can't attack at all
			this.FinishOrder();
			return;
		}
		this.order.data.attackType = type;

		// If we are already at the target, try attacking it from here
		if (this.CheckTargetAttackRange(this.order.data.target, this.order.data.attackType))
		{
			this.StopMoving();
			// For packable units within attack range:
			// 1. If unpacked, we can attack the target.
			// 2. If packed, we first need to unpack, then follow case 1.
			if (this.CanUnpack())
			{
				this.PushOrderFront("Unpack", { "force": true });
				return;
			}

			if (this.order.data.attackType == this.oldAttackType)
			{
				if (this.IsAnimal())
					this.SetNextState("ANIMAL.COMBAT.ATTACKING");
				else
					this.SetNextState("INDIVIDUAL.COMBAT.ATTACKING");
			}
			else
			{
				if (this.IsAnimal())
					this.SetNextState("ANIMAL.COMBAT.ATTACKING");
				else
					this.SetNextState("INDIVIDUAL.COMBAT.ATTACKING");
			}
			return;
		}

		// If we can't reach the target, but are standing ground, then abandon this attack order.
		// Unless we're hunting, that's a special case where we should continue attacking our target.
		if (this.GetStance().respondStandGround && !this.order.data.force && !this.order.data.hunting || this.IsTurret())
		{
			this.FinishOrder();
			return;
		}

		// For packable units out of attack range:
		// 1. If packed, we need to move to attack range and then unpack.
		// 2. If unpacked, we first need to pack, then follow case 1.
		if (this.CanPack())
		{
			this.PushOrderFront("Pack", { "force": true });
			return;
		}

		// Try to move within attack range
		if (this.MoveToTargetAttackRange(this.order.data.target, this.order.data.attackType))
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

	"Order.Patrol": function(msg) {
		if (this.IsAnimal() || this.IsTurret())
		{
			this.FinishOrder();
			return;
		}

		if (this.CanPack())
		{
			this.PushOrderFront("Pack", { "force": true });
			return;
		}

		this.MoveToPoint(this.order.data.x, this.order.data.z);
		this.SetNextState("INDIVIDUAL.PATROL");
	},

	"Order.Heal": function(msg) {
		// Check the target is alive
		if (!this.TargetIsAlive(this.order.data.target))
		{
			this.FinishOrder();
			return;
		}

		// Healers can't heal themselves.
		if (this.order.data.target == this.entity)
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
		if (this.MustKillGatherTarget(this.order.data.target))
		{
			// Make sure we can attack the target, else we'll get very stuck
			if (!this.GetBestAttackAgainst(this.order.data.target, false))
			{
				// Oops, we can't attack at all - give up
				// TODO: should do something so the player knows why this failed
				this.FinishOrder();
				return;
			}
			// The target was visible when this order was issued,
			// but could now be invisible again.
			if (!this.CheckTargetVisible(this.order.data.target))
			{
				if (this.order.data.secondTry === undefined)
				{
					this.order.data.secondTry = true;
					this.PushOrderFront("Walk", this.order.data.lastPos);
				}
				else
				{
					// We couldn't move there, or the target moved away
					this.FinishOrder();
				}
				return;
			}

			this.PushOrderFront("Attack", { "target": this.order.data.target, "force": !!this.order.data.force, "hunting": true, "allowCapture": false });
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
		// Check if the dropsite is already in range
		if (this.CheckTargetRange(this.order.data.target, IID_ResourceGatherer) && this.CanReturnResource(this.order.data.target, true))
		{
			var cmpResourceDropsite = Engine.QueryInterface(this.order.data.target, IID_ResourceDropsite);
			if (cmpResourceDropsite)
			{
				// Dump any resources we can
				var dropsiteTypes = cmpResourceDropsite.GetTypes();

				Engine.QueryInterface(this.entity, IID_ResourceGatherer).CommitResources(dropsiteTypes);
				// Stop showing the carried resource animation.
				this.SetDefaultAnimationVariant();

				// Our next order should always be a Gather,
				// so just switch back to that order
				this.FinishOrder();
				return;
			}
		}
		// Try to move to the dropsite
		if (this.MoveToTargetRange(this.order.data.target, IID_ResourceGatherer))
		{
			// We've started walking to the target
			this.SetNextState("INDIVIDUAL.RETURNRESOURCE.APPROACHING");
			return;
		}
		// Oops, we can't reach the dropsite.
		// Maybe we should try to pick another dropsite, to find an
		// accessible one?
		// For now, just give up.
		this.StopMoving();
		this.FinishOrder();
		return;
	},

	"Order.Trade": function(msg) {
		// We must check if this trader has both markets in case it was a back-to-work order
		var cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
		if (!cmpTrader || !cmpTrader.HasBothMarkets())
		{
			this.FinishOrder();
			return;
		}

		// TODO find the nearest way-point from our position, and start with it
		this.waypoints = undefined;
		if (this.MoveToMarket(this.order.data.target))
			// We've started walking to the next market
			this.SetNextState("TRADE.APPROACHINGMARKET");
		else
			this.FinishOrder();
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
		if (this.IsTurret())
		{
			this.SetNextState("IDLE");
			return;
		}
		else if (this.IsGarrisoned())
		{
			this.SetNextState("INDIVIDUAL.GARRISON.GARRISONED");
			return;
		}

		// For packable units:
		// 1. If packed, we can move to the garrison target.
		// 2. If unpacked, we first need to pack, then follow case 1.
		if (this.CanPack())
		{
			this.PushOrderFront("Pack", { "force": true });
			return;
		}

		if (this.MoveToGarrisonRange(this.order.data.target))
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

	"Order.Ungarrison": function() {
		this.FinishOrder();
		this.isGarrisoned = false;
	},

	"Order.Cheering": function(msg) {
		this.SetNextState("INDIVIDUAL.CHEERING");
	},

	"Order.Pack": function(msg) {
		if (this.CanPack())
		{
			this.StopMoving();
			this.SetNextState("INDIVIDUAL.PACKING");
		}
	},

	"Order.Unpack": function(msg) {
		if (this.CanUnpack())
		{
			this.StopMoving();
			this.SetNextState("INDIVIDUAL.UNPACKING");
		}
	},

	"Order.CancelPack": function(msg) {
		var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
		if (cmpPack && cmpPack.IsPacking() && !cmpPack.IsPacked())
			cmpPack.CancelPack();
		this.FinishOrder();
	},

	"Order.CancelUnpack": function(msg) {
		var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
		if (cmpPack && cmpPack.IsPacking() && cmpPack.IsPacked())
			cmpPack.CancelPack();
		this.FinishOrder();
	},

	// States for the special entity representing a group of units moving in formation:
	"FORMATIONCONTROLLER": {

		"Order.Walk": function(msg) {
			this.CallMemberFunction("SetHeldPosition", [msg.data.x, msg.data.z]);

			this.MoveToPoint(this.order.data.x, this.order.data.z);
			this.SetNextState("WALKING");
		},

		"Order.WalkAndFight": function(msg) {
			this.CallMemberFunction("SetHeldPosition", [msg.data.x, msg.data.z]);

			this.MoveToPoint(this.order.data.x, this.order.data.z);
			this.SetNextState("WALKINGANDFIGHTING");
		},

		"Order.MoveIntoFormation": function(msg) {
			this.CallMemberFunction("SetHeldPosition", [msg.data.x, msg.data.z]);

			this.MoveToPoint(this.order.data.x, this.order.data.z);
			this.SetNextState("FORMING");
		},

		// Only used by other orders to walk there in formation
		"Order.WalkToTargetRange": function(msg) {
			if (this.MoveToTargetRangeExplicit(this.order.data.target, this.order.data.min, this.order.data.max))
				this.SetNextState("WALKING");
			else
				this.FinishOrder();
		},

		"Order.WalkToTarget": function(msg) {
			if (this.MoveToTarget(this.order.data.target))
				this.SetNextState("WALKING");
			else
				this.FinishOrder();
		},

		"Order.WalkToPointRange": function(msg) {
			if (this.MoveToPointRange(this.order.data.x, this.order.data.z, this.order.data.min, this.order.data.max))
				this.SetNextState("WALKING");
			else
				this.FinishOrder();
		},

		"Order.Patrol": function(msg) {
			this.CallMemberFunction("SetHeldPosition", [msg.data.x, msg.data.z]);

			this.MoveToPoint(this.order.data.x, this.order.data.z);
			this.SetNextState("PATROL");
		},

		"Order.Guard": function(msg) {
			this.CallMemberFunction("Guard", [msg.data.target, false]);
			var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
			cmpFormation.Disband();
		},

		"Order.Stop": function(msg) {
			if (!this.IsAttackingAsFormation())
				this.CallMemberFunction("Stop", [false]);
			this.StopMoving();
			this.FinishOrder();
		},

		"Order.Attack": function(msg) {
			var target = msg.data.target;
			var allowCapture = msg.data.allowCapture;
			var cmpTargetUnitAI = Engine.QueryInterface(target, IID_UnitAI);
			if (cmpTargetUnitAI && cmpTargetUnitAI.IsFormationMember())
				target = cmpTargetUnitAI.GetFormationController();

			var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
			// Check if we are already in range, otherwise walk there
			if (!this.CheckTargetAttackRange(target, target))
			{
				if (this.TargetIsAlive(target) && this.CheckTargetVisible(target))
				{
					if (this.MoveToTargetAttackRange(target, target))
					{
						this.SetNextState("COMBAT.APPROACHING");
						return;
					}
				}
				this.FinishOrder();
				return;
			}
			this.CallMemberFunction("Attack", [target, allowCapture, false]);
			if (cmpAttack.CanAttackAsFormation())
				this.SetNextState("COMBAT.ATTACKING");
			else
				this.SetNextState("MEMBER");
		},

		"Order.Garrison": function(msg) {
			if (!Engine.QueryInterface(msg.data.target, IID_GarrisonHolder))
			{
				this.FinishOrder();
				return;
			}
			// Check if we are already in range, otherwise walk there
			if (!this.CheckGarrisonRange(msg.data.target))
			{
				if (!this.CheckTargetVisible(msg.data.target))
				{
					this.FinishOrder();
					return;
				}
				else
				{
					// Out of range; move there in formation
					if (this.MoveToGarrisonRange(msg.data.target))
					{
						this.SetNextState("GARRISON.APPROACHING");
						return;
					}
				}
			}

			this.SetNextState("GARRISON.GARRISONING");
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
					else
					{
						// We couldn't move there, or the target moved away
						this.FinishOrder();
					}
					return;
				}

				this.PushOrderFront("Attack", { "target": msg.data.target, "force": !!msg.data.force, "hunting": true, "allowCapture": false });
				return;
			}

			// TODO: on what should we base this range?
			// Check if we are already in range, otherwise walk there
			if (!this.CheckTargetRangeExplicit(msg.data.target, 0, 10))
			{
				if (!this.CanGather(msg.data.target) || !this.CheckTargetVisible(msg.data.target))
					// The target isn't gatherable or not visible any more.
					this.FinishOrder();
				// TODO: Should we issue a gather-near-position order
				// if the target isn't gatherable/doesn't exist anymore?
				else
					// Out of range; move there in formation
					this.PushOrderFront("WalkToTargetRange", { "target": msg.data.target, "min": 0, "max": 10 });
				return;
			}

			this.CallMemberFunction("Gather", [msg.data.target, false]);

			this.SetNextState("MEMBER");
		},

		"Order.GatherNearPosition": function(msg) {
			// TODO: on what should we base this range?
			// Check if we are already in range, otherwise walk there
			if (!this.CheckPointRangeExplicit(msg.data.x, msg.data.z, 0, 20))
			{
				// Out of range; move there in formation
				this.PushOrderFront("WalkToPointRange", { "x": msg.data.x, "z": msg.data.z, "min": 0, "max": 20 });
				return;
			}

			this.CallMemberFunction("GatherNearPosition", [msg.data.x, msg.data.z, msg.data.type, msg.data.template, false]);

			this.SetNextState("MEMBER");
		},

		"Order.Heal": function(msg) {
			// TODO: on what should we base this range?
			// Check if we are already in range, otherwise walk there
			if (!this.CheckTargetRangeExplicit(msg.data.target, 0, 10))
			{
				if (!this.TargetIsAlive(msg.data.target) || !this.CheckTargetVisible(msg.data.target))
					// The target was destroyed
					this.FinishOrder();
				else
					// Out of range; move there in formation
					this.PushOrderFront("WalkToTargetRange", { "target": msg.data.target, "min": 0, "max": 10 });
				return;
			}

			this.CallMemberFunction("Heal", [msg.data.target, false]);

			this.SetNextState("MEMBER");
		},

		"Order.Repair": function(msg) {
			// TODO: on what should we base this range?
			// Check if we are already in range, otherwise walk there
			if (!this.CheckTargetRangeExplicit(msg.data.target, 0, 10))
			{
				if (!this.TargetIsAlive(msg.data.target) || !this.CheckTargetVisible(msg.data.target))
					// The building was finished or destroyed
					this.FinishOrder();
				else
					// Out of range move there in formation
					this.PushOrderFront("WalkToTargetRange", { "target": msg.data.target, "min": 0, "max": 10 });
				return;
			}

			this.CallMemberFunction("Repair", [msg.data.target, msg.data.autocontinue, false]);

			this.SetNextState("MEMBER");
		},

		"Order.ReturnResource": function(msg) {
			// TODO: on what should we base this range?
			// Check if we are already in range, otherwise walk there
			if (!this.CheckTargetRangeExplicit(msg.data.target, 0, 10))
			{
				if (!this.TargetIsAlive(msg.data.target) || !this.CheckTargetVisible(msg.data.target))
					// The target was destroyed
					this.FinishOrder();
				else
					// Out of range; move there in formation
					this.PushOrderFront("WalkToTargetRange", { "target": msg.data.target, "min": 0, "max": 10 });
				return;
			}

			this.CallMemberFunction("ReturnResource", [msg.data.target, false]);

			this.SetNextState("MEMBER");
		},

		"Order.Pack": function(msg) {
			this.CallMemberFunction("Pack", [false]);

			this.SetNextState("MEMBER");
		},

		"Order.Unpack": function(msg) {
			this.CallMemberFunction("Unpack", [false]);

			this.SetNextState("MEMBER");
		},

		"IDLE": {
			"enter": function(msg) {
				var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.SetRearrange(false);
			},

			"MoveStarted": function() {
				let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.SetRearrange(true);
				cmpFormation.MoveMembersIntoFormation(true, true);
			}
		},

		"WALKING": {
			"MoveStarted": function(msg) {
				var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.SetRearrange(true);
				cmpFormation.MoveMembersIntoFormation(true, true);
			},

			"MoveCompleted": function(msg) {
				if (this.FinishOrder())
					this.CallMemberFunction("ResetFinishOrder", []);
			},
		},

		"WALKINGANDFIGHTING": {
			"enter": function(msg) {
				this.StartTimer(0, 1000);
			},

			"Timer": function(msg) {
				// check if there are no enemies to attack
				this.FindWalkAndFightTargets();
			},

			"leave": function(msg) {
				this.StopTimer();
			},

			"MoveStarted": function(msg) {
				var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.SetRearrange(true);
				cmpFormation.MoveMembersIntoFormation(true, true);
			},

			"MoveCompleted": function(msg) {
				if (this.FinishOrder())
					this.CallMemberFunction("ResetFinishOrder", []);
			},
		},

		"PATROL": {
			"enter": function(msg) {
				// Memorize the origin position in case that we want to go back
				let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
				if (!cmpPosition || !cmpPosition.IsInWorld())
				{
					this.FinishOrder();
					return;
				}
				if (!this.patrolStartPosOrder)
				{
					this.patrolStartPosOrder = cmpPosition.GetPosition();
					this.patrolStartPosOrder.targetClasses = this.order.data.targetClasses;
					this.patrolStartPosOrder.allowCapture = this.order.data.allowCapture;
				}

				this.StartTimer(0, 1000);
			},

			"Timer": function(msg) {
				// Check if there are no enemies to attack
				this.FindWalkAndFightTargets();
			},

			"leave": function(msg) {
				this.StopTimer();
				delete this.patrolStartPosOrder;
			},

			"MoveStarted": function(msg) {
				let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.SetRearrange(true);
				cmpFormation.MoveMembersIntoFormation(true, true);
			},

			"MoveCompleted": function() {
				/**
				 * A-B-A-B-..:
				 * if the user only commands one patrol order, the patrol will be between
				 * the last position and the defined waypoint
				 * A-B-C-..-A-B-..:
				 * otherwise, the patrol is only between the given patrol commands and the
				 * last position is not included (last position = the position where the unit
				 * is located at the time of the first patrol order)
				 */

				if (this.orderQueue.length == 1)
					this.PushOrder("Patrol", this.patrolStartPosOrder);

				this.PushOrder(this.order.type, this.order.data);
				this.FinishOrder();
			},
		},

		"GARRISON":{
			"enter": function() {
				// If the garrisonholder should pickup, warn it so it can take needed action
				var cmpGarrisonHolder = Engine.QueryInterface(this.order.data.target, IID_GarrisonHolder);
				if (cmpGarrisonHolder && cmpGarrisonHolder.CanPickup(this.entity))
				{
					this.pickup = this.order.data.target;       // temporary, deleted in "leave"
					Engine.PostMessage(this.pickup, MT_PickupRequested, { "entity": this.entity });
				}
			},

			"leave": function() {
				// If a pickup has been requested and not yet canceled, cancel it
				if (this.pickup)
				{
					Engine.PostMessage(this.pickup, MT_PickupCanceled, { "entity": this.entity });
					delete this.pickup;
				}
			},


			"APPROACHING": {
				"MoveStarted": function(msg) {
					var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
					cmpFormation.SetRearrange(true);
					cmpFormation.MoveMembersIntoFormation(true, true);
				},

				"MoveCompleted": function(msg) {
					this.SetNextState("GARRISONING");
				},
			},

			"GARRISONING": {
				"enter": function() {
					// If a pickup has been requested, cancel it as it will be requested by members
					if (this.pickup)
					{
						Engine.PostMessage(this.pickup, MT_PickupCanceled, { "entity": this.entity });
						delete this.pickup;
					}
					this.CallMemberFunction("Garrison", [this.order.data.target, false]);
					this.SetNextState("MEMBER");
				},
			},
		},

		"FORMING": {
			"MoveStarted": function(msg) {
				var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.SetRearrange(true);
				cmpFormation.MoveMembersIntoFormation(true, false);
			},

			"MoveCompleted": function(msg) {

				if (this.FinishOrder())
				{
					this.CallMemberFunction("ResetFinishOrder", []);
					return;
				}
				var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);

				cmpFormation.FindInPosition();
			}
		},

		"COMBAT": {
			"APPROACHING": {
				"MoveStarted": function(msg) {
					var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
					cmpFormation.SetRearrange(true);
					cmpFormation.MoveMembersIntoFormation(true, true);
				},

				"MoveCompleted": function(msg) {
					var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
					this.CallMemberFunction("Attack", [this.order.data.target, this.order.data.allowCapture, false]);
					if (cmpAttack.CanAttackAsFormation())
						this.SetNextState("COMBAT.ATTACKING");
					else
						this.SetNextState("MEMBER");
				},
			},

			"ATTACKING": {
				// Wait for individual members to finish
				"enter": function(msg) {
					var target = this.order.data.target;
					var allowCapture = this.order.data.allowCapture;
					// Check if we are already in range, otherwise walk there
					if (!this.CheckTargetAttackRange(target, target))
					{
						if (this.TargetIsAlive(target) && this.CheckTargetVisible(target))
						{
							this.FinishOrder();
							this.PushOrderFront("Attack", { "target": target, "force": false, "allowCapture": allowCapture });
							return true;
						}
						this.FinishOrder();
						return true;
					}

					var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
					// TODO fix the rearranging while attacking as formation
					cmpFormation.SetRearrange(!this.IsAttackingAsFormation());
					cmpFormation.MoveMembersIntoFormation(false, false);
					this.StartTimer(200, 200);
					return false;
				},

				"Timer": function(msg) {
					var target = this.order.data.target;
					var allowCapture = this.order.data.allowCapture;
					// Check if we are already in range, otherwise walk there
					if (!this.CheckTargetAttackRange(target, target))
					{
						if (this.TargetIsAlive(target) && this.CheckTargetVisible(target))
						{
							this.FinishOrder();
							this.PushOrderFront("Attack", { "target": target, "force": false, "allowCapture": allowCapture });
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

		"MEMBER": {
			// Wait for individual members to finish
			"enter": function(msg) {
				var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.SetRearrange(false);
				this.StopMoving();
				this.StartTimer(1000, 1000);
			},

			"Timer": function(msg) {
				// Have all members finished the task?
				if (!this.TestAllMemberFunction("HasFinishedOrder", []))
					return;

				this.CallMemberFunction("ResetFinishOrder", []);

				// Execute the next order
				if (this.FinishOrder())
				{
					// if WalkAndFight order, look for new target before moving again
					if (this.IsWalkingAndFighting())
						this.FindWalkAndFightTargets();
					return;
				}
			},

			"leave": function(msg) {
				this.StopTimer();
				let cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.MoveToMembersCenter();
			},
		},
	},


	// States for entities moving as part of a formation:
	"FORMATIONMEMBER": {
		"FormationLeave": function(msg) {
			// We're not in a formation anymore, so no need to track this.
			this.finishedOrder = false;

			// Stop moving as soon as the formation disbands
			this.StopMoving();

			// If the controller handled an order but some members rejected it,
			// they will have no orders and be in the FORMATIONMEMBER.IDLE state.
			if (this.orderQueue.length)
			{
				// We're leaving the formation, so stop our FormationWalk order
				if (this.FinishOrder())
					return;
			}

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
			// If foundation is not ally of entity, or if entity is unpacked siege,
			// ignore the order
			if (!IsOwnedByAllyOfEntity(this.entity, msg.data.target) && !Engine.QueryInterface(SYSTEM_ENTITY, IID_CeasefireManager).IsCeasefireActive() ||
				this.IsPacking() || this.CanPack() || this.IsTurret())
			{
				this.FinishOrder();
				return;
			}
			// Move a tile outside the building
			let range = 4;
			if (this.MoveToTargetRangeExplicit(msg.data.target, range, range))
			{
				// We've started walking to the given point
				this.SetNextState("WALKINGTOPOINT");
			}
			else
			{
				// We are already at the target, or can't move at all
				this.FinishOrder();
			}
		},


		"IDLE": {
			"enter": function() {
				if (this.IsAnimal())
					this.SetNextState("ANIMAL.IDLE");
				else
					this.SetNextState("INDIVIDUAL.IDLE");
				return true;
			},
		},

		"WALKING": {
			"enter": function() {
				var cmpFormation = Engine.QueryInterface(this.formationController, IID_Formation);
				var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
				if (cmpFormation && cmpVisual)
				{
					cmpVisual.ReplaceMoveAnimation("walk", cmpFormation.GetFormationAnimation(this.entity, "walk"));
					cmpVisual.ReplaceMoveAnimation("run", cmpFormation.GetFormationAnimation(this.entity, "run"));
				}
				this.SelectAnimation("move");
			},

			// Occurs when the unit has reached its destination and the controller
			// is done moving. The controller is notified.
			"MoveCompleted": function(msg) {
				// We can only finish this order if the move was really completed.
				if (!msg.data.error && this.FinishOrder())
					return;
				var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
				if (cmpVisual)
				{
					cmpVisual.ResetMoveAnimation("walk");
					cmpVisual.ResetMoveAnimation("run");
				}

				var cmpFormation = Engine.QueryInterface(this.formationController, IID_Formation);
				if (cmpFormation)
					cmpFormation.SetInPosition(this.entity);
			},
		},

		// Special case used by Order.LeaveFoundation
		"WALKINGTOPOINT": {
			"enter": function() {
				var cmpFormation = Engine.QueryInterface(this.formationController, IID_Formation);
				if (cmpFormation)
					cmpFormation.UnsetInPosition(this.entity);
				this.SelectAnimation("move");
			},

			"MoveCompleted": function() {
				this.FinishOrder();
			},
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
			// Respond to attack if we always target attackers or during unforced orders
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
				if (this.order.data.target != msg.data.attacker && this.TargetIsAlive(msg.data.attacker))
					return;

			var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
			var cmpHealth = Engine.QueryInterface(this.isGuardOf, IID_Health);
			if (cmpIdentity && cmpIdentity.HasClass("Support") &&
			    cmpHealth && cmpHealth.GetHitpoints() < cmpHealth.GetMaxHitpoints())
			{
				if (this.CanHeal(this.isGuardOf))
					this.PushOrderFront("Heal", { "target": this.isGuardOf, "force": false });
				else if (this.CanRepair(this.isGuardOf))
					this.PushOrderFront("Repair", { "target": this.isGuardOf, "autocontinue": false, "force": false });
				return;
			}

			// if the attacker is a building and we can repair the guarded, repair it rather than attacking
			var cmpBuildingAI = Engine.QueryInterface(msg.data.attacker, IID_BuildingAI);
			if (cmpBuildingAI && this.CanRepair(this.isGuardOf))
			{
				this.PushOrderFront("Repair", { "target": this.isGuardOf, "autocontinue": false, "force": false });
				return;
			}

			// target the unit
			if (this.CheckTargetVisible(msg.data.attacker))
				this.PushOrderFront("Attack", { "target": msg.data.attacker, "force": false, "allowCapture": true });
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
			"enter": function() {
				// Switch back to idle animation to guarantee we won't
				// get stuck with an incorrect animation
				var animationName = "idle";
				if (this.IsFormationMember())
				{
					var cmpFormation = Engine.QueryInterface(this.formationController, IID_Formation);
					if (cmpFormation)
						animationName = cmpFormation.GetFormationAnimation(this.entity, animationName);
				}
				this.SelectAnimation(animationName);

				// If we have some orders, it is because we are in an intermediary state
				// from FinishOrder (SetNextState("IDLE") is only executed when we get
				// a ProcessMessage), and thus we should not start another order which could
				// put us in a weird state
				if (this.orderQueue.length > 0 && !this.IsGarrisoned())
					return false;

				// If the unit is guarding/escorting, go back to its duty
				if (this.isGuardOf)
				{
					this.Guard(this.isGuardOf, false);
					return true;
				}

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
				var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
				if (this.losRangeQuery)
					cmpRangeManager.DisableActiveQuery(this.losRangeQuery);
				if (this.losHealRangeQuery)
					cmpRangeManager.DisableActiveQuery(this.losHealRangeQuery);

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
					this.AttackEntitiesByPreference(msg.data.added);
				}
			},

			"LosHealRangeUpdate": function(msg) {
				this.RespondToHealableEntities(msg.data.added);
			},

			"MoveStarted": function() {
				this.SelectAnimation("move");
			},

			"MoveCompleted": function() {
				this.SelectAnimation("idle");
			},

			"Timer": function(msg) {
				if (!this.isIdle)
				{
					this.isIdle = true;
					Engine.PostMessage(this.entity, MT_UnitIdleChanged, { "idle": this.isIdle });
				}
			},
		},

		"WALKING": {
			"enter": function() {
				this.SelectAnimation("move");
			},

			"MoveCompleted": function() {
				this.FinishOrder();
			},
		},

		"WALKINGANDFIGHTING": {
			"enter": function() {
				// Show weapons rather than carried resources.
				this.SetAnimationVariant("combat");

				this.StartTimer(0, 1000);
				this.SelectAnimation("move");
			},

			"Timer": function(msg) {
				this.FindWalkAndFightTargets();
			},

			"leave": function(msg) {
				this.StopTimer();
				this.SetDefaultAnimationVariant();
			},

			"MoveCompleted": function() {
				this.FinishOrder();
			},
		},

		"PATROL": {
			"enter": function() {
				// Memorize the origin position in case that we want to go back
				let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
				if (!cmpPosition || !cmpPosition.IsInWorld())
				{
					this.FinishOrder();
					return;
				}
				if (!this.patrolStartPosOrder)
				{
					this.patrolStartPosOrder = cmpPosition.GetPosition();
					this.patrolStartPosOrder.targetClasses = this.order.data.targetClasses;
					this.patrolStartPosOrder.allowCapture = this.order.data.allowCapture;
				}

				this.StartTimer(0, 1000);
				this.SetAnimationVariant("combat");
				this.SelectAnimation("move");
			},

			"leave": function() {
				this.StopTimer();
				delete this.patrolStartPosOrder;
				this.SetDefaultAnimationVariant();
			},

			"Timer": function(msg) {
				this.FindWalkAndFightTargets();
			},

			"MoveCompleted": function() {
				if (this.orderQueue.length == 1)
					this.PushOrder("Patrol", this.patrolStartPosOrder);

				this.PushOrder(this.order.type, this.order.data);
				this.FinishOrder();
			},
		},

		"GUARD": {
			"RemoveGuard": function() {
				this.StopMoving();
				this.FinishOrder();
			},

			"ESCORTING": {
				"enter": function() {
					// Show weapons rather than carried resources.
					this.SetAnimationVariant("combat");

					this.StartTimer(0, 1000);
					this.SelectAnimation("move");
					this.SetHeldPositionOnEntity(this.isGuardOf);
					return false;
				},

				"Timer": function(msg) {
					// Check the target is alive
					if (!this.TargetIsAlive(this.isGuardOf))
					{
						this.StopMoving();
						this.FinishOrder();
						return;
					}
					this.SetHeldPositionOnEntity(this.isGuardOf);
				},

				"leave": function(msg) {
					this.SetMoveSpeed(this.GetWalkSpeed());
					this.StopTimer();
					this.SetDefaultAnimationVariant();
				},

				"MoveStarted": function(msg) {
					// Adapt the speed to the one of the target if needed
					var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
					if (cmpUnitMotion.IsInTargetRange(this.isGuardOf, 0, 3*this.guardRange))
					{
						var cmpUnitAI = Engine.QueryInterface(this.isGuardOf, IID_UnitAI);
						if (cmpUnitAI)
						{
							var speed = cmpUnitAI.GetWalkSpeed();
							if (speed < this.GetWalkSpeed())
								this.SetMoveSpeed(speed);
						}
					}
				},

				"MoveCompleted": function() {
					this.SetMoveSpeed(this.GetWalkSpeed());
					if (!this.MoveToTargetRangeExplicit(this.isGuardOf, 0, this.guardRange))
						this.SetNextState("GUARDING");
				},
			},

			"GUARDING": {
				"enter": function() {
					this.StartTimer(1000, 1000);
					this.SetHeldPositionOnEntity(this.entity);
					this.SetAnimationVariant("combat");
					this.SelectAnimation("idle");
					return false;
				},

				"LosRangeUpdate": function(msg) {
					// Start attacking one of the newly-seen enemy (if any)
					if (this.GetStance().targetVisibleEnemies)
						this.AttackEntitiesByPreference(msg.data.added);
				},

				"Timer": function(msg) {
					// Check the target is alive
					if (!this.TargetIsAlive(this.isGuardOf))
					{
						this.FinishOrder();
						return;
					}
					// then check is the target has moved
					if (this.MoveToTargetRangeExplicit(this.isGuardOf, 0, this.guardRange))
						this.SetNextState("ESCORTING");
					else
					{
						// if nothing better to do, check if the guarded needs to be healed or repaired
						var cmpHealth = Engine.QueryInterface(this.isGuardOf, IID_Health);
						if (cmpHealth && (cmpHealth.GetHitpoints() < cmpHealth.GetMaxHitpoints()))
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
				this.PlaySound("panic");

				// Run quickly
				var speed = this.GetRunSpeed();
				this.SelectAnimation("move");
				this.SetMoveSpeed(speed);
			},

			"HealthChanged": function() {
				var speed = this.GetRunSpeed();
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
			"Order.LeaveFoundation": function(msg) {
				// Ignore the order as we're busy.
				return { "discardOrder": true };
			},

			"Attacked": function(msg) {
				// If we're already in combat mode, ignore anyone else who's attacking us
				// unless it's a melee attack since they may be blocking our way to the target
				if (msg.data.type == "Melee" && (this.GetStance().targetAttackersAlways || !this.order.data.force))
					this.RespondToTargetedEntities([msg.data.attacker]);
			},

			"APPROACHING": {
				"enter": function() {
					// Show weapons rather than carried resources.
					this.SetAnimationVariant("combat");

					this.SelectAnimation("move");
					this.StartTimer(1000, 1000);
				},

				"leave": function() {
					// Show carried resources when walking.
					this.SetDefaultAnimationVariant();

					this.StopTimer();
				},

				"Timer": function(msg) {
					if (this.ShouldAbandonChase(this.order.data.target, this.order.data.force, IID_Attack, this.order.data.attackType))
					{
						this.StopMoving();
						this.FinishOrder();

						// Return to our original position
						if (this.GetStance().respondHoldGround)
							this.WalkToHeldPosition();
					}
				},

				"MoveCompleted": function() {

					if (this.CheckTargetAttackRange(this.order.data.target, this.order.data.attackType))
					{
						// If the unit needs to unpack, do so
						if (this.CanUnpack())
						{
							this.PushOrderFront("Unpack", { "force": true });
							return;
						}
						else
							this.SetNextState("ATTACKING");
					}
					else
					{
						if (this.MoveToTargetAttackRange(this.order.data.target, this.order.data.attackType))
						{
							this.SetNextState("APPROACHING");
						}
						else
						{
							// Give up
							this.FinishOrder();
						}
					}
				},
			},

			"ATTACKING": {
				"enter": function() {
					var target = this.order.data.target;
					var cmpFormation = Engine.QueryInterface(target, IID_Formation);
					// if the target is a formation, save the attacking formation, and pick a member
					if (cmpFormation)
					{
						this.order.data.formationTarget = target;
						target = cmpFormation.GetClosestMember(this.entity);
						this.order.data.target = target;
					}
					// Check the target is still alive and attackable
					if (this.CanAttack(target) && !this.CheckTargetAttackRange(target, this.order.data.attackType))
					{
						// Can't reach it - try to chase after it
						if (this.ShouldChaseTargetedEntity(target, this.order.data.force))
						{
							if (this.CanPack())
							{
								this.PushOrderFront("Pack", { "force": true });
								return;
							}
							if (this.MoveToTargetAttackRange(target, this.order.data.attackType))
							{
								this.SetNextState("COMBAT.CHASING");
								return;
							}
						}
					}

					var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
					this.attackTimers = cmpAttack.GetTimers(this.order.data.attackType);

					// If the repeat time since the last attack hasn't elapsed,
					// delay this attack to avoid attacking too fast.
					var prepare = this.attackTimers.prepare;
					if (this.lastAttacked)
					{
						var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
						var repeatLeft = this.lastAttacked + this.attackTimers.repeat - cmpTimer.GetTime();
						prepare = Math.max(prepare, repeatLeft);
					}

					this.oldAttackType = this.order.data.attackType;
					// add prefix + no capital first letter for attackType
					var animationName = "attack_" + this.order.data.attackType.toLowerCase();
					if (this.IsFormationMember())
					{
						var cmpFormation = Engine.QueryInterface(this.formationController, IID_Formation);
						if (cmpFormation)
							animationName = cmpFormation.GetFormationAnimation(this.entity, animationName);
					}
					this.SetAnimationVariant("combat");
					this.SelectAnimation(animationName);
					this.SetAnimationSync(prepare, this.attackTimers.repeat);
					this.StartTimer(prepare, this.attackTimers.repeat);
					// TODO: we should probably only bother syncing projectile attacks, not melee

					// If using a non-default prepare time, re-sync the animation when the timer runs.
					this.resyncAnimation = (prepare != this.attackTimers.prepare) ? true : false;

					this.FaceTowardsTarget(this.order.data.target);

					var cmpBuildingAI = Engine.QueryInterface(this.entity, IID_BuildingAI);
					if (cmpBuildingAI)
						cmpBuildingAI.SetUnitAITarget(this.order.data.target);
				},

				"leave": function() {
					var cmpBuildingAI = Engine.QueryInterface(this.entity, IID_BuildingAI);
					if (cmpBuildingAI)
						cmpBuildingAI.SetUnitAITarget(0);
					this.StopTimer();
					this.SetDefaultAnimationVariant();
				},

				"Timer": function(msg) {
					var target = this.order.data.target;
					var cmpFormation = Engine.QueryInterface(target, IID_Formation);
					// if the target is a formation, save the attacking formation, and pick a member
					if (cmpFormation)
					{
						var thisObject = this;
						var filter = function(t) {
							return thisObject.CanAttack(t);
						};
						this.order.data.formationTarget = target;
						target = cmpFormation.GetClosestMember(this.entity, filter);
						this.order.data.target = target;
					}
					// Check the target is still alive and attackable
					if (this.CanAttack(target))
					{
						// If we are hunting, first update the target position of the gather order so we know where will be the killed animal
						if (this.order.data.hunting && this.orderQueue[1] && this.orderQueue[1].data.lastPos)
						{
							var cmpPosition = Engine.QueryInterface(this.order.data.target, IID_Position);
							if (cmpPosition && cmpPosition.IsInWorld())
							{
								// Store the initial position, so that we can find the rest of the herd later
								if (!this.orderQueue[1].data.initPos)
									this.orderQueue[1].data.initPos = this.orderQueue[1].data.lastPos;
								this.orderQueue[1].data.lastPos = cmpPosition.GetPosition();
								// We still know where the animal is, so we shouldn't give up before going there
								this.orderQueue[1].data.secondTry = undefined;
							}
						}

						var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
						this.lastAttacked = cmpTimer.GetTime() - msg.lateness;

						this.FaceTowardsTarget(target);

						// BuildingAI has it's own attack-routine
						var cmpBuildingAI = Engine.QueryInterface(this.entity, IID_BuildingAI);
						if (!cmpBuildingAI)
						{
							let cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
							cmpAttack.PerformAttack(this.order.data.attackType, target);
						}

						// Check we can still reach the target for the next attack
						if (this.CheckTargetAttackRange(target, this.order.data.attackType))
						{
							if (this.resyncAnimation)
							{
								this.SetAnimationSync(this.attackTimers.repeat, this.attackTimers.repeat);
								this.resyncAnimation = false;
							}
							return;
						}

						// Can't reach it - try to chase after it
						if (this.ShouldChaseTargetedEntity(target, this.order.data.force))
						{
							if (this.CanPack())
							{
								this.PushOrderFront("Pack", { "force": true });
								return;
							}
							if (this.MoveToTargetRange(target, IID_Attack, this.order.data.attackType))
							{
								this.SetNextState("COMBAT.CHASING");
								return;
							}
						}
					}

					// if we're targetting a formation, find a new member of that formation
					var cmpTargetFormation = Engine.QueryInterface(this.order.data.formationTarget || INVALID_ENTITY, IID_Formation);
					// if there is no target, it means previously searching for the target inside the target formation failed, so don't repeat the search
					if (target && cmpTargetFormation)
					{
						this.order.data.target = this.order.data.formationTarget;
						this.TimerHandler(msg.data, msg.lateness);
						return;
					}

					// Can't reach it, no longer owned by enemy, or it doesn't exist any more - give up
					// Except if in WalkAndFight mode where we look for more ennemies around before moving again
					if (this.FinishOrder())
					{
						if (this.IsWalkingAndFighting())
							this.FindWalkAndFightTargets();
						return;
					}

					// See if we can switch to a new nearby enemy
					if (this.FindNewTargets())
					{
						// Attempt to immediately re-enter the timer function, to avoid wasting the attack.
						// Packable units may have switched to PACKING state, thus canceling the timer and having order.data.attackType undefined.
						if (this.orderQueue.length > 0 && this.orderQueue[0].data && this.orderQueue[0].data.attackType &&
						    this.orderQueue[0].data.attackType == this.oldAttackType)
							this.TimerHandler(msg.data, msg.lateness);
						return;
					}

					// Return to our original position
					if (this.GetStance().respondHoldGround)
						this.WalkToHeldPosition();
				},

				// TODO: respond to target deaths immediately, rather than waiting
				// until the next Timer event

				"Attacked": function(msg) {
					// If we are capturing and are attacked by something that we would not capture, attack that entity instead
					if (this.order.data.attackType == "Capture" && (this.GetStance().targetAttackersAlways || !this.order.data.force)
						&& this.order.data.target != msg.data.attacker && this.GetBestAttackAgainst(msg.data.attacker, true) != "Capture")
						this.RespondToTargetedEntities([msg.data.attacker]);
				},
			},

			"CHASING": {
				"enter": function() {
					// Show weapons rather than carried resources.
					this.SetAnimationVariant("combat");

					this.SelectAnimation("move");
					var cmpUnitAI = Engine.QueryInterface(this.order.data.target, IID_UnitAI);
					if (cmpUnitAI && cmpUnitAI.IsFleeing())
					{
						// Run after a fleeing target
						var speed = this.GetRunSpeed();
						this.SetMoveSpeed(speed);
					}
					this.StartTimer(1000, 1000);
				},

				"HealthChanged": function() {
					var cmpUnitAI = Engine.QueryInterface(this.order.data.target, IID_UnitAI);
					if (!cmpUnitAI || !cmpUnitAI.IsFleeing())
						return;
					var speed = this.GetRunSpeed();
					this.SetMoveSpeed(speed);
				},

				"leave": function() {
					// Reset normal speed in case it was changed
					this.SetMoveSpeed(this.GetWalkSpeed());
					// Show carried resources when walking.
					this.SetDefaultAnimationVariant();

					this.StopTimer();
				},

				"Timer": function(msg) {
					if (this.ShouldAbandonChase(this.order.data.target, this.order.data.force, IID_Attack, this.order.data.attackType))
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

					this.gatheringTarget = this.order.data.target;	// temporary, deleted in "leave".

					// check that we can gather from the resource we're supposed to gather from.
					var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
					var cmpSupply = Engine.QueryInterface(this.gatheringTarget, IID_ResourceSupply);
					var cmpMirage = Engine.QueryInterface(this.gatheringTarget, IID_Mirage);
					if ((!cmpMirage || !cmpMirage.Mirages(IID_ResourceSupply)) &&
					    (!cmpSupply || !cmpSupply.AddGatherer(cmpOwnership.GetOwner(), this.entity)))
					{
						// Save the current order's data in case we need it later
						var oldType = this.order.data.type;
						var oldTarget = this.order.data.target;
						var oldTemplate = this.order.data.template;

						// Try the next queued order if there is any
						if (this.FinishOrder())
							return true;

						// Try to find another nearby target of the same specific type
						// Also don't switch to a different type of huntable animal
						var nearby = this.FindNearbyResource(function(ent, type, template) {
							return (
								ent != oldTarget
								 && ((type.generic == "treasure" && oldType.generic == "treasure")
								 || (type.specific == oldType.specific
								 && (type.specific != "meat" || oldTemplate == template)))
							);
						}, oldTarget);
						if (nearby)
						{
							this.PerformGather(nearby, false, false);
							return true;
						}
						else
						{
							// It's probably better in this case, to avoid units getting stuck around a dropsite
							// in a "Target is far away, full, nearby are no good resources, return to dropsite" loop
							// to order it to GatherNear the resource position.
							var cmpPosition = Engine.QueryInterface(oldTarget, IID_Position);
							if (cmpPosition)
							{
								var pos = cmpPosition.GetPosition();
								this.GatherNearPosition(pos.x, pos.z, oldType, oldTemplate);
								return true;
							}
							else
							{
								// we're kind of stuck here. Return resource.
								var nearby = this.FindNearestDropsite(oldType.generic);
								if (nearby)
								{
									this.PushOrderFront("ReturnResource", { "target": nearby, "force": false });
									return true;
								}
							}
						}
						return true;
					}
					return false;
				},

				"MoveCompleted": function(msg) {
					if (msg.data.error)
					{
						// We failed to reach the target

						// remove us from the list of entities gathering from Resource.
						var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
						var cmpSupply = Engine.QueryInterface(this.gatheringTarget, IID_ResourceSupply);
						if (cmpSupply && cmpOwnership)
							cmpSupply.RemoveGatherer(this.entity, cmpOwnership.GetOwner());
						else if (cmpSupply)
							cmpSupply.RemoveGatherer(this.entity);

						// Save the current order's data in case we need it later
						var oldType = this.order.data.type;
						var oldTarget = this.order.data.target;
						var oldTemplate = this.order.data.template;

						// Try the next queued order if there is any
						if (this.FinishOrder())
							return;

						// Try to find another nearby target of the same specific type
						// Also don't switch to a different type of huntable animal
						var nearby = this.FindNearbyResource(function(ent, type, template) {
							return (
								ent != oldTarget
								&& ((type.generic == "treasure" && oldType.generic == "treasure")
								|| (type.specific == oldType.specific
								&& (type.specific != "meat" || oldTemplate == template)))
							);
						});
						if (nearby)
						{
							this.PerformGather(nearby, false, false);
							return;
						}

						// Couldn't find anything else. Just try this one again,
						// maybe we'll succeed next time
						this.PerformGather(oldTarget, false, false);
						return;
					}

					// We reached the target - start gathering from it now
					this.SetNextState("GATHERING");
				},

				"leave": function() {
					// don't use ownership because this is called after a conversion/resignation
					// and the ownership would be invalid then.
					var cmpSupply = Engine.QueryInterface(this.gatheringTarget, IID_ResourceSupply);
					if (cmpSupply)
						cmpSupply.RemoveGatherer(this.entity);
					delete this.gatheringTarget;
					this.SetDefaultAnimationVariant();
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
					var nearby = this.FindNearbyResource(function(ent, type, template) {
						return (
							(type.generic == "treasure" && resourceType.generic == "treasure")
							|| (type.specific == resourceType.specific
							&& (type.specific != "meat" || resourceTemplate == template))
						);
					});

					// If there is a nearby resource start gathering
					if (nearby)
					{
						this.PerformGather(nearby, false, false);
						return;
					}

					// Couldn't find nearby resources, so give up
					if (this.FinishOrder())
						return;

					// Nothing better to do: go back to dropsite
					var nearby = this.FindNearestDropsite(resourceType.generic);
					if (nearby)
					{
						this.PushOrderFront("ReturnResource", { "target": nearby, "force": false });
						return;
					}

					// No dropsites, just give up
				},
			},

			"GATHERING": {
				"enter": function() {
					this.gatheringTarget = this.order.data.target;	// deleted in "leave".

					// Check if the resource is full.
					if (this.gatheringTarget)
					{
						// Check that we can gather from the resource we're supposed to gather from.
						// Will only be added if we're not already in.
						var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
						var cmpSupply = Engine.QueryInterface(this.gatheringTarget, IID_ResourceSupply);
						if (!cmpSupply || !cmpSupply.AddGatherer(cmpOwnership.GetOwner(), this.entity))
						{
							this.gatheringTarget = INVALID_ENTITY;
							this.StartTimer(0);
							return false;
						}
					}

					// If this order was forced, the player probably gave it, but now we've reached the target
					//	switch to an unforced order (can be interrupted by attacks)
					this.order.data.force = false;
					this.order.data.autoharvest = true;

					// Calculate timing based on gather rates
					// This allows the gather rate to control how often we gather, instead of how much.
					var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
					var rate = cmpResourceGatherer.GetTargetGatherRate(this.gatheringTarget);

					if (!rate)
					{
						// Try to find another target if the current one stopped existing
						if (!Engine.QueryInterface(this.gatheringTarget, IID_Identity))
						{
							// Let the Timer logic handle this
							this.StartTimer(0);
							return false;
						}

						// No rate, give up on gathering
						this.FinishOrder();
						return true;
					}

					// Scale timing interval based on rate, and start timer
					// The offset should be at least as long as the repeat time so we use the same value for both.
					var offset = 1000/rate;
					var repeat = offset;
					this.StartTimer(offset, repeat);

					// We want to start the gather animation as soon as possible,
					// but only if we're actually at the target and it's still alive
					// (else it'll look like we're chopping empty air).
					// (If it's not alive, the Timer handler will deal with sending us
					// off to a different target.)
					if (this.CheckTargetRange(this.gatheringTarget, IID_ResourceGatherer))
					{
						this.SetDefaultAnimationVariant();
						var typename = "gather_" + this.order.data.type.specific;
						this.SelectAnimation(typename);
					}
					return false;
				},

				"leave": function() {
					this.StopTimer();

					// don't use ownership because this is called after a conversion/resignation
					// and the ownership would be invalid then.
					var cmpSupply = Engine.QueryInterface(this.gatheringTarget, IID_ResourceSupply);
					if (cmpSupply)
						cmpSupply.RemoveGatherer(this.entity);
					delete this.gatheringTarget;

					// Show the carried resource, if we've gathered anything.
					this.SetDefaultAnimationVariant();
				},

				"Timer": function(msg) {
					var resourceTemplate = this.order.data.template;
					var resourceType = this.order.data.type;

					var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
					if (!cmpOwnership)
						return;

					var cmpSupply = Engine.QueryInterface(this.gatheringTarget, IID_ResourceSupply);
					if (cmpSupply && cmpSupply.IsAvailable(cmpOwnership.GetOwner(), this.entity))
					{
						// Check we can still reach and gather from the target
						if (this.CheckTargetRange(this.gatheringTarget, IID_ResourceGatherer) && this.CanGather(this.gatheringTarget))
						{
							// Gather the resources:

							var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);

							// Try to gather treasure
							if (cmpResourceGatherer.TryInstantGather(this.gatheringTarget))
								return;

							// If we've already got some resources but they're the wrong type,
							// drop them first to ensure we're only ever carrying one type
							if (cmpResourceGatherer.IsCarryingAnythingExcept(resourceType.generic))
								cmpResourceGatherer.DropResources();

							// Collect from the target
							var status = cmpResourceGatherer.PerformGather(this.gatheringTarget);

							// If we've collected as many resources as possible,
							// return to the nearest dropsite
							if (status.filled)
							{
								var nearby = this.FindNearestDropsite(resourceType.generic);
								if (nearby)
								{
									// (Keep this Gather order on the stack so we'll
									// continue gathering after returning)
									this.PushOrderFront("ReturnResource", { "target": nearby, "force": false });
									return;
								}

								// Oh no, couldn't find any drop sites. Give up on gathering.
								this.FinishOrder();
								return;
							}

							// We can gather more from this target, do so in the next timer
							if (!status.exhausted)
								return;
						}
						else
						{
							// Try to follow the target
							if (this.MoveToTargetRange(this.gatheringTarget, IID_ResourceGatherer))
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
						}
					}

					// We're already in range, can't get anywhere near it or the target is exhausted.

					var herdPos = this.order.data.initPos;

					// Give up on this order and try our next queued order
					// but first check what is our next order and, if needed, insert a returnResource order
					var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
					if (cmpResourceGatherer.IsCarrying(resourceType.generic) &&
						this.orderQueue.length > 1 && this.orderQueue[1] !== "ReturnResource" &&
						(this.orderQueue[1].type !== "Gather" || this.orderQueue[1].data.type.generic !== resourceType.generic))
					{
						let nearby = this.FindNearestDropsite(resourceType.generic);
						if (nearby)
							this.orderQueue.splice(1, 0, { "type": "ReturnResource", "data": { "target": nearby, "force": false } });
					}
					if (this.FinishOrder())
						return;

					// No remaining orders - pick a useful default behaviour

					// Try to find a new resource of the same specific type near our current position:
					// Also don't switch to a different type of huntable animal
					var nearby = this.FindNearbyResource(function(ent, type, template) {
						return (
							(type.generic == "treasure" && resourceType.generic == "treasure")
							|| (type.specific == resourceType.specific
							&& (type.specific != "meat" || resourceTemplate == template))
						);
					});
					if (nearby)
					{
						this.PerformGather(nearby, false, false);
						return;
					}

					// If hunting, try to go to the initial herd position to see if we are more lucky
					if (herdPos)
					{
						this.GatherNearPosition(herdPos.x, herdPos.z, resourceType, resourceTemplate);
						return;
					}

					// Nothing else to gather - if we're carrying anything then we should
					// drop it off, and if not then we might as well head to the dropsite
					// anyway because that's a nice enough place to congregate and idle

					var nearby = this.FindNearestDropsite(resourceType.generic);
					if (nearby)
					{
						this.PushOrderFront("ReturnResource", { "target": nearby, "force": false });
						return;
					}

					// No dropsites - just give up
				},
			},
		},

		"HEAL": {
			"Attacked": function(msg) {
				// If we stand ground we will rather die than flee
				if (!this.GetStance().respondStandGround && !this.order.data.force)
					this.Flee(msg.data.attacker, false);
			},

			"APPROACHING": {
				"enter": function() {
					this.SelectAnimation("move");
					this.StartTimer(1000, 1000);
				},

				"leave": function() {
					this.StopTimer();
				},

				"Timer": function(msg) {
					if (this.ShouldAbandonChase(this.order.data.target, this.order.data.force, IID_Heal, null))
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

					// If the repeat time since the last heal hasn't elapsed,
					// delay the action to avoid healing too fast.
					var prepare = this.healTimers.prepare;
					if (this.lastHealed)
					{
						var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
						var repeatLeft = this.lastHealed + this.healTimers.repeat - cmpTimer.GetTime();
						prepare = Math.max(prepare, repeatLeft);
					}

					this.SelectAnimation("heal");
					this.SetAnimationSync(prepare, this.healTimers.repeat);
					this.StartTimer(prepare, this.healTimers.repeat);

					// If using a non-default prepare time, re-sync the animation when the timer runs.
					this.resyncAnimation = (prepare != this.healTimers.prepare) ? true : false;

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
							var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
							this.lastHealed = cmpTimer.GetTime() - msg.lateness;

							this.FaceTowardsTarget(target);
							var cmpHeal = Engine.QueryInterface(this.entity, IID_Heal);
							cmpHeal.PerformHeal(target);

							if (this.resyncAnimation)
							{
								this.SetAnimationSync(this.healTimers.repeat, this.healTimers.repeat);
								this.resyncAnimation = false;
							}
							return;
						}
						// Can't reach it - try to chase after it
						if (this.ShouldChaseTargetedEntity(target, this.order.data.force))
						{
							if (this.CanPack())
							{
								this.PushOrderFront("Pack", { "force": true });
								return;
							}
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
				"enter": function() {
					this.SelectAnimation("move");
					this.StartTimer(1000, 1000);
				},

				"leave": function() {
					this.StopTimer();
				},
				"Timer": function(msg) {
					if (this.ShouldAbandonChase(this.order.data.target, this.order.data.force, IID_Heal, null))
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
		},

		// Returning to dropsite
		"RETURNRESOURCE": {
			"APPROACHING": {
				"enter": function() {
					this.SelectAnimation("move");
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

							// Stop showing the carried resource animation.
							this.SetDefaultAnimationVariant();

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
						this.PushOrderFront("ReturnResource", { "target": nearby, "force": false });
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

			"APPROACHINGMARKET": {
				"enter": function() {
					this.SelectAnimation("move");
				},

				"MoveCompleted": function() {
					if (this.waypoints && this.waypoints.length)
					{
						if (!this.MoveToMarket(this.order.data.target))
							this.StopTrading();
					}
					else
						this.PerformTradeAndMoveToNextMarket(this.order.data.target);
				},
			},

			"TradingCanceled": function(msg) {
				if (msg.market != this.order.data.target)
					return;
				let cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
				let otherMarket = cmpTrader && cmpTrader.GetFirstMarket();
				this.StopTrading();
				if (otherMarket)
					this.WalkToTarget(otherMarket);
			},
		},

		"REPAIR": {
			"APPROACHING": {
				"enter": function() {
					this.SelectAnimation("move");
				},

				"MoveCompleted": function() {
					this.SetNextState("REPAIRING");
				},
			},

			"REPAIRING": {
				"enter": function() {
					// If this order was forced, the player probably gave it, but now we've reached the target
					//	switch to an unforced order (can be interrupted by attacks)
					if (this.order.data.force)
						this.order.data.autoharvest = true;

					this.order.data.force = false;

					this.repairTarget = this.order.data.target;	// temporary, deleted in "leave".
					// Check we can still reach and repair the target
					if (!this.CanRepair(this.repairTarget))
					{
						// Can't reach it, no longer owned by ally, or it doesn't exist any more
						this.FinishOrder();
						return true;
					}

					if (!this.CheckTargetRange(this.repairTarget, IID_Builder))
					{
						if (this.MoveToTargetRange(this.repairTarget, IID_Builder))
							this.SetNextState("APPROACHING");
						else
							this.FinishOrder();
						return true;
					}
					// Check if the target is still repairable
					var cmpHealth = Engine.QueryInterface(this.repairTarget, IID_Health);
					if (cmpHealth && cmpHealth.GetHitpoints() >= cmpHealth.GetMaxHitpoints())
					{
						// The building was already finished/fully repaired before we arrived;
						// let the ConstructionFinished handler handle this.
						this.OnGlobalConstructionFinished({"entity": this.repairTarget, "newentity": this.repairTarget});
						return true;
					}

					let cmpBuilderList = QueryBuilderListInterface(this.repairTarget);
					if (cmpBuilderList)
						cmpBuilderList.AddBuilder(this.entity);

					this.SelectAnimation("build");
					this.StartTimer(1000, 1000);
					return false;
				},

				"leave": function() {
					let cmpBuilderList = QueryBuilderListInterface(this.repairTarget);
					if (cmpBuilderList)
						cmpBuilderList.RemoveBuilder(this.entity);
					delete this.repairTarget;
					this.StopTimer();
				},

				"Timer": function(msg) {
					// Check we can still reach and repair the target
					if (!this.CanRepair(this.repairTarget))
					{
						// No longer owned by ally, or it doesn't exist any more
						this.FinishOrder();
						return;
					}

					var cmpBuilder = Engine.QueryInterface(this.entity, IID_Builder);
					cmpBuilder.PerformBuilding(this.repairTarget);
					// if the building is completed, the leave() function will be called
					// by the ConstructionFinished message
					// in that case, the repairTarget is deleted, and we can just return
					if (!this.repairTarget)
						return;
					if (this.MoveToTargetRange(this.repairTarget, IID_Builder))
						this.SetNextState("APPROACHING");
					else if (!this.CheckTargetRange(this.repairTarget, IID_Builder))
						this.FinishOrder(); //can't approach and isn't in reach
				},
			},

			"ConstructionFinished": function(msg) {
				if (msg.data.entity != this.order.data.target)
					return; // ignore other buildings

				// Save the current order's data in case we need it later
				var oldData = this.order.data;

				// Save the current state so we can continue walking if necessary
				// FinishOrder() below will switch to IDLE if there's no order, which sets the idle animation.
				// Idle animation while moving towards finished construction looks weird (ghosty).
				var oldState = this.GetCurrentState();

				// Drop any resource we can if we are in range when the construction finishes
				var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
				var cmpResourceDropsite = Engine.QueryInterface(msg.data.newentity, IID_ResourceDropsite);
				if (cmpResourceGatherer && cmpResourceDropsite && this.CheckTargetRange(msg.data.newentity, IID_Builder) &&
					this.CanReturnResource(msg.data.newentity, true))
				{
					let dropsiteTypes = cmpResourceDropsite.GetTypes();
					cmpResourceGatherer.CommitResources(dropsiteTypes);
					this.SetDefaultAnimationVariant();
				}

				// We finished building it.
				// Switch to the next order (if any)
				if (this.FinishOrder())
				{
					if (this.CanReturnResource(msg.data.newentity, true))
					{
						this.SetDefaultAnimationVariant();
						this.PushOrderFront("ReturnResource", { "target": msg.data.newentity, "force": false });
					}
					return;
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
					if (this.CanReturnResource(msg.data.newentity, true))
					{
						this.SetDefaultAnimationVariant();
						this.PushOrder("ReturnResource", { "target": msg.data.newentity, "force": false });
					}
					this.PerformGather(msg.data.newentity, true, false);
					return;
				}

				// If this building was e.g. a farmstead of ours, entities that received
				// the build command should look for nearby resources to gather
				if ((oldData.force || oldData.autoharvest) && this.CanReturnResource(msg.data.newentity, false))
				{
					var cmpResourceDropsite = Engine.QueryInterface(msg.data.newentity, IID_ResourceDropsite);
					var types = cmpResourceDropsite.GetTypes();
					// TODO: Slightly undefined behavior here, we don't know what type of resource will be collected,
					//   may cause problems for AIs (especially hunting fast animals), but avoid ugly hacks to fix that!
					var nearby = this.FindNearbyResource(function(ent, type, template) {
						return (types.indexOf(type.generic) != -1);
					}, msg.data.newentity);
					if (nearby)
					{
						this.PerformGather(nearby, true, false);
						return;
					}
				}

				// Look for a nearby foundation to help with
				var nearbyFoundation = this.FindNearbyFoundation();
				if (nearbyFoundation)
				{
					this.AddOrder("Repair", { "target": nearbyFoundation, "autocontinue": oldData.autocontinue, "force": false }, true);
					return;
				}

				// Unit was approaching and there's nothing to do now, so switch to walking
				if (oldState === "INDIVIDUAL.REPAIR.APPROACHING")
				{
					// We're already walking to the given point, so add this as a order.
					this.WalkToTarget(msg.data.newentity, true);
				}
			},
		},

		"GARRISON": {
			"enter": function() {
				// If the garrisonholder should pickup, warn it so it can take needed action
				var cmpGarrisonHolder = Engine.QueryInterface(this.order.data.target, IID_GarrisonHolder);
				if (cmpGarrisonHolder && cmpGarrisonHolder.CanPickup(this.entity))
				{
					this.pickup = this.order.data.target;       // temporary, deleted in "leave"
					Engine.PostMessage(this.pickup, MT_PickupRequested, { "entity": this.entity });
				}
			},

			"leave": function() {
				// If a pickup has been requested and not yet canceled, cancel it
				if (this.pickup)
				{
					Engine.PostMessage(this.pickup, MT_PickupCanceled, { "entity": this.entity });
					delete this.pickup;
				}

			},

			"APPROACHING": {
				"enter": function() {
					this.SelectAnimation("move");
				},

				"MoveCompleted": function() {
					this.SetNextState("GARRISONED");
				},
			},

			"GARRISONED": {
				"enter": function() {
					if (this.order.data.target)
						var target = this.order.data.target;
					else
					{
						this.FinishOrder();
						return true;
					}

					if (this.IsGarrisoned())
						return false;

					// Check that we can garrison here
					if (this.CanGarrison(target))
					{
						// Check that we're in range of the garrison target
						if (this.CheckGarrisonRange(target))
						{
							var cmpGarrisonHolder = Engine.QueryInterface(target, IID_GarrisonHolder);
							// Check that garrisoning succeeds
							if (cmpGarrisonHolder.Garrison(this.entity))
							{
								this.isGarrisoned = true;

								if (this.formationController)
								{
									var cmpFormation = Engine.QueryInterface(this.formationController, IID_Formation);
									if (cmpFormation)
									{
										// disable rearrange for this removal,
										// but enable it again for the next
										// move command
										var rearrange = cmpFormation.rearrange;
										cmpFormation.SetRearrange(false);
										cmpFormation.RemoveMembers([this.entity]);
										cmpFormation.SetRearrange(rearrange);
									}
								}

								// Check if we are garrisoned in a dropsite
								var cmpResourceDropsite = Engine.QueryInterface(target, IID_ResourceDropsite);
								if (cmpResourceDropsite && this.CanReturnResource(target, true))
								{
									// Dump any resources we can
									var dropsiteTypes = cmpResourceDropsite.GetTypes();
									var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
									if (cmpResourceGatherer)
									{
										cmpResourceGatherer.CommitResources(dropsiteTypes);
										this.SetDefaultAnimationVariant();
									}
								}

								// If a pickup has been requested, remove it
								if (this.pickup)
								{
									var cmpHolderPosition = Engine.QueryInterface(target, IID_Position);
									var cmpHolderUnitAI = Engine.QueryInterface(target, IID_UnitAI);
									if (cmpHolderUnitAI && cmpHolderPosition)
										cmpHolderUnitAI.lastShorelinePosition = cmpHolderPosition.GetPosition();
									Engine.PostMessage(this.pickup, MT_PickupCanceled, { "entity": this.entity });
									delete this.pickup;
								}

								if (this.IsTurret())
									this.SetNextState("IDLE");

								return false;
							}
						}
						else
						{
							// Unable to reach the target, try again (or follow if it is a moving target)
							// except if the does not exits anymore or its orders have changed
							if (this.pickup)
							{
								var cmpUnitAI = Engine.QueryInterface(this.pickup, IID_UnitAI);
								if (!cmpUnitAI || !cmpUnitAI.HasPickupOrder(this.entity))
								{
									this.FinishOrder();
									return true;
								}

							}
							if (this.MoveToTarget(target))
							{
								this.SetNextState("APPROACHING");
								return false;
							}
						}
					}
					// Garrisoning failed for some reason, so finish the order
					this.FinishOrder();
					return true;
				},

				"leave": function() {
				}
			},
		},

		"CHEERING": {
			"enter": function() {
				// Unit is invulnerable while cheering
				var cmpDamageReceiver = Engine.QueryInterface(this.entity, IID_DamageReceiver);
				cmpDamageReceiver.SetInvulnerability(true);
				this.SelectAnimation("promotion");
				this.StartTimer(2800, 2800);
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

		"PACKING": {
			"enter": function() {
				var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
				cmpPack.Pack();
			},

			"PackFinished": function(msg) {
				this.FinishOrder();
			},

			"leave": function() {
			},

			"Attacked": function(msg) {
				// Ignore attacks while packing
			},
		},

		"UNPACKING": {
			"enter": function() {
				var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
				cmpPack.Unpack();
			},

			"PackFinished": function(msg) {
				this.FinishOrder();
			},

			"leave": function() {
			},

			"Attacked": function(msg) {
				// Ignore attacks while unpacking
			},
		},

		"PICKUP": {
			"APPROACHING": {
				"enter": function() {
					this.SelectAnimation("move");
				},

				"MoveCompleted": function() {
					this.SetNextState("LOADING");
				},

				"PickupCanceled": function() {
					this.StopMoving();
					this.FinishOrder();
				},
			},

			"LOADING": {
				"enter": function() {
					this.SelectAnimation("idle");
					var cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
					if (!cmpGarrisonHolder || cmpGarrisonHolder.IsFull())
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

	"ANIMAL": {
		"Attacked": function(msg) {
			if (this.template.NaturalBehaviour == "skittish" ||
			    this.template.NaturalBehaviour == "passive")
			{
				this.Flee(msg.data.attacker, false);
			}
			else if (this.IsDangerousAnimal() || this.template.NaturalBehaviour == "defensive")
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
			// Move a tile outside the building
			var range = 4;
			if (this.MoveToTargetRangeExplicit(msg.data.target, range, range))
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
			// (We need an IDLE state so that FinishOrder works)

			"enter": function() {
				// Start feeding immediately
				this.SetNextState("FEEDING");
				return true;
			},
		},

		"ROAMING": {
			"enter": function() {
				// Walk in a random direction
				this.SelectAnimation("walk", false, this.GetWalkSpeed());
				this.SetFacePointAfterMove(false);
				this.MoveRandomly(+this.template.RoamDistance);
				// Set a random timer to switch to feeding state
				this.StartTimer(randIntInclusive(+this.template.RoamTimeMin, +this.template.RoamTimeMax));
			},

			"leave": function() {
				this.StopTimer();
				this.SetFacePointAfterMove(true);
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
				else if (this.IsDangerousAnimal())
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
				this.StartTimer(randIntInclusive(+this.template.FeedTimeMin, +this.template.FeedTimeMax));
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

UnitAI.prototype.Init = function()
{
	this.orderQueue = []; // current order is at the front of the list
	this.order = undefined; // always == this.orderQueue[0]
	this.formationController = INVALID_ENTITY; // entity with IID_Formation that we belong to
	this.isGarrisoned = false;
	this.isIdle = false;
	this.finishedOrder = false; // used to find if all formation members finished the order

	this.heldPosition = undefined;

	// Queue of remembered works
	this.workOrders = [];

	this.isGuardOf = undefined;

	// For preventing increased action rate due to Stop orders or target death.
	this.lastAttacked = undefined;
	this.lastHealed = undefined;

	this.SetStance(this.template.DefaultStance);
};

UnitAI.prototype.IsTurret = function()
{
	if (!this.IsGarrisoned())
		return false;
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	return cmpPosition && cmpPosition.GetTurretParent() != INVALID_ENTITY;
};

UnitAI.prototype.IsFormationController = function()
{
	return (this.template.FormationController == "true");
};

UnitAI.prototype.IsFormationMember = function()
{
	return (this.formationController != INVALID_ENTITY);
};

UnitAI.prototype.HasFinishedOrder = function()
{
	return this.finishedOrder;
};

UnitAI.prototype.ResetFinishOrder = function()
{
	this.finishedOrder = false;
};

UnitAI.prototype.IsAnimal = function()
{
	return (this.template.NaturalBehaviour ? true : false);
};

UnitAI.prototype.IsDangerousAnimal = function()
{
	return (this.IsAnimal() && (this.template.NaturalBehaviour == "violent" ||
			this.template.NaturalBehaviour == "aggressive"));
};

UnitAI.prototype.IsDomestic = function()
{
	var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	return cmpIdentity && cmpIdentity.HasClass("Domestic");
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

UnitAI.prototype.SetGarrisoned = function()
{
	this.isGarrisoned = true;
};

UnitAI.prototype.GetGarrisonHolder = function()
{
	if (this.IsGarrisoned())
	{
		for (let order of this.orderQueue)
			if (order.type == "Garrison")
				return order.data.target;
	}
	return INVALID_ENTITY;
};

UnitAI.prototype.ShouldRespondToEndOfAlert = function()
{
	return !this.orderQueue.length || this.orderQueue[0].type == "Garrison";
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
		return false;

	return this.orderQueue.length > 0 && (this.orderQueue[0].type == "WalkAndFight" || this.orderQueue[0].type == "Patrol");
};

UnitAI.prototype.OnCreate = function()
{
	if (this.IsAnimal())
		this.UnitFsm.Init(this, "ANIMAL.FEEDING");
	else if (this.IsFormationController())
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
		// except if garrisoned or cheering or (un)packing, in which case we only clear the order queue
		if (this.isGarrisoned || this.IsPacking() || this.orderQueue[0] && this.orderQueue[0].type == "Cheering")
		{
			this.orderQueue.length = Math.min(this.orderQueue.length, 1);
			Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
		}
		else
		{
			let index = this.GetCurrentState().indexOf(".");
			if (index != -1)
				this.UnitFsm.SwitchToNextState(this, this.GetCurrentState().slice(0,index));
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

	// Clean up range queries
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (this.losRangeQuery)
		cmpRangeManager.DestroyActiveQuery(this.losRangeQuery);
	if (this.losHealRangeQuery)
		cmpRangeManager.DestroyActiveQuery(this.losHealRangeQuery);
};

UnitAI.prototype.OnVisionRangeChanged = function(msg)
{
	// Update range queries
	if (this.entity == msg.entity)
		this.SetupRangeQueries();
};

UnitAI.prototype.HasPickupOrder = function(entity)
{
	return this.orderQueue.some(order => order.type == "PickupUnit" && order.data.target == entity);
};

UnitAI.prototype.OnPickupRequested = function(msg)
{
	// First check if we already have such a request
	if (this.HasPickupOrder(msg.entity))
		return;
	// Otherwise, insert the PickUp order after the last forced order
	this.PushOrderAfterForced("PickupUnit", { "target": msg.entity });
};

UnitAI.prototype.OnPickupCanceled = function(msg)
{
	for (let i = 0; i < this.orderQueue.length; ++i)
	{
		if (this.orderQueue[i].type != "PickupUnit" || this.orderQueue[i].data.target != msg.entity)
			continue;
		if (i == 0)
			this.UnitFsm.ProcessMessage(this, {"type": "PickupCanceled", "data": msg});
		else
			this.orderQueue.splice(i, 1);
		Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
		break;
	}
};

// Wrapper function that sets up the normal and healer range queries.
UnitAI.prototype.SetupRangeQueries = function()
{
	this.SetupRangeQuery();

	if (this.IsHealer())
		this.SetupHealRangeQuery();
};

UnitAI.prototype.UpdateRangeQueries = function()
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (this.losRangeQuery)
		this.SetupRangeQuery(cmpRangeManager.IsActiveQueryEnabled(this.losRangeQuery));

	if (this.IsHealer() && this.losHealRangeQuery)
		this.SetupHealRangeQuery(cmpRangeManager.IsActiveQueryEnabled(this.losHealRangeQuery));
};

// Set up a range query for all enemy and gaia units within LOS range
// which can be attacked.
// This should be called whenever our ownership changes.
UnitAI.prototype.SetupRangeQuery = function(enable = true)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);

	if (this.losRangeQuery)
	{
		cmpRangeManager.DestroyActiveQuery(this.losRangeQuery);
		this.losRangeQuery = undefined;
	}

	var cmpPlayer = QueryOwnerInterface(this.entity);
	// If we are being destructed (owner -1), creating a range query is pointless
	if (!cmpPlayer)
		return;

	// Exclude allies, and self
	// TODO: How to handle neutral players - Special query to attack military only?
	var players = cmpPlayer.GetEnemies();
	var range = this.GetQueryRange(IID_Attack);

	this.losRangeQuery = cmpRangeManager.CreateActiveQuery(this.entity, range.min, range.max, players, IID_DamageReceiver, cmpRangeManager.GetEntityFlagMask("normal"));

	if (enable)
		cmpRangeManager.EnableActiveQuery(this.losRangeQuery);
};

// Set up a range query for all own or ally units within LOS range
// which can be healed.
// This should be called whenever our ownership changes.
UnitAI.prototype.SetupHealRangeQuery = function(enable = true)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);

	if (this.losHealRangeQuery)
	{
		cmpRangeManager.DestroyActiveQuery(this.losHealRangeQuery);
		this.losHealRangeQuery = undefined;
	}

	var cmpPlayer = QueryOwnerInterface(this.entity);
	// If we are being destructed (owner -1), creating a range query is pointless
	if (!cmpPlayer)
		return;

	var players = cmpPlayer.GetAllies();
	var range = this.GetQueryRange(IID_Heal);

	this.losHealRangeQuery = cmpRangeManager.CreateActiveQuery(this.entity, range.min, range.max, players, IID_Health, cmpRangeManager.GetEntityFlagMask("injured"));

	if (enable)
		cmpRangeManager.EnableActiveQuery(this.losHealRangeQuery);
};


//// FSM linkage functions ////

// Setting the next state to the current state will leave/re-enter the top-most substate.
UnitAI.prototype.SetNextState = function(state)
{
	this.UnitFsm.SetNextState(this, state);
};

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

	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (this.orderQueue.length && (this.IsGarrisoned() || cmpPosition && cmpPosition.IsInWorld()))
	{
		let ret = this.UnitFsm.ProcessMessage(this,
			{ "type": "Order."+this.order.type, "data": this.order.data }
		);

		Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });

		// If the order was rejected then immediately take it off
		// and process the remaining queue
		if (ret && ret.discardOrder)
			return this.FinishOrder();

		// Otherwise we've successfully processed a new order
		return true;
	}

	this.orderQueue = [];
	this.order = undefined;

	// Switch to IDLE as a default state, but only if our current state is not IDLE
	// as this can trigger infinite loops by entering IDLE repeatedly.
	if (!this.GetCurrentState().endsWith(".IDLE"))
		this.SetNextState("IDLE");

	Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });

	// Check if there are queued formation orders
	if (this.IsFormationMember())
	{
		let cmpUnitAI = Engine.QueryInterface(this.formationController, IID_UnitAI);
		if (cmpUnitAI)
		{
			// Inform the formation controller that we finished this task
			this.finishedOrder = true;
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

	// If we didn't already have an order, then process this new one
	if (this.orderQueue.length == 1)
	{
		this.order = order;
		let ret = this.UnitFsm.ProcessMessage(this,
			{ "type": "Order."+this.order.type, "data": this.order.data }
		);

		// If the order was rejected then immediately take it off
		// and process the remaining queue
		if (ret && ret.discardOrder)
			this.FinishOrder();
	}

	Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
};

/**
 * Add an order onto the front of the queue,
 * and execute it immediately.
 */
UnitAI.prototype.PushOrderFront = function(type, data)
{
	var order = { "type": type, "data": data };
	// If current order is cheering then add new order after it
	// same thing if current order if packing/unpacking
	if (this.order && this.order.type == "Cheering")
	{
		var cheeringOrder = this.orderQueue.shift();
		this.orderQueue.unshift(cheeringOrder, order);
	}
	else if (this.order && this.IsPacking())
	{
		var packingOrder = this.orderQueue.shift();
		this.orderQueue.unshift(packingOrder, order);
	}
	else
	{
		this.orderQueue.unshift(order);
		this.order = order;
		let ret = this.UnitFsm.ProcessMessage(this,
			{ "type": "Order."+this.order.type, "data": this.order.data }
		);

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
			this.orderQueue.splice(i, 0, {"type": type, "data": data});
			Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
			return;
		}
		this.PushOrder(type, data);
	}

	Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
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

	let garrisonHolder = this.IsGarrisoned() && type != "Ungarrison" ? this.GetGarrisonHolder() : null;

	// Special cases of orders that shouldn't be replaced:
	// 1. Cheering - we're invulnerable, add order after we finish
	// 2. Packing/unpacking - we're immobile, add order after we finish (unless it's cancel)
	// TODO: maybe a better way of doing this would be to use priority levels
	if (this.order && this.order.type == "Cheering")
	{
		var order = { "type": type, "data": data };
		var cheeringOrder = this.orderQueue.shift();
		this.orderQueue = [cheeringOrder, order];
	}
	else if (this.IsPacking() && type != "CancelPack" && type != "CancelUnpack")
	{
		var order = { "type": type, "data": data };
		var packingOrder = this.orderQueue.shift();
		this.orderQueue = [packingOrder, order];
	}
	else
	{
		this.orderQueue = [];
		this.PushOrder(type, data);
	}

	if (garrisonHolder)
		this.PushOrder("Garrison", { "target": garrisonHolder });

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

	// If we are being re-affected to a work order, forget the previous ones
	if (isWorkType(type))
	{
		this.workOrders = [];
		return;
	}

	// Then if we already have work orders, keep them
	if (this.workOrders.length)
		return;

	// First if the unit is in a formation, get its workOrders from it
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

	if (this.IsGarrisoned())
	{
		let cmpGarrisonHolder = Engine.QueryInterface(this.GetGarrisonHolder(), IID_GarrisonHolder);
		if (!cmpGarrisonHolder || !cmpGarrisonHolder.PerformEject([this.entity], false))
			return false;
	}

	// Clear the order queue considering special orders not to avoid
	if (this.order && this.order.type == "Cheering")
	{
		var cheeringOrder = this.orderQueue.shift();
		this.orderQueue = [cheeringOrder];
	}
	else
		this.orderQueue = [];

	this.AddOrders(this.workOrders);
	Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });

	// And if the unit is in a formation, remove it from the formation
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

	this.UnitFsm.ProcessMessage(this, {"type": "Timer", "data": data, "lateness": lateness});
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

//// Message handlers /////

UnitAI.prototype.OnMotionChanged = function(msg)
{
	if (msg.starting && !msg.error)
		this.UnitFsm.ProcessMessage(this, {"type": "MoveStarted", "data": msg});
	else if (!msg.starting || msg.error)
		this.UnitFsm.ProcessMessage(this, {"type": "MoveCompleted", "data": msg});
};

UnitAI.prototype.OnGlobalConstructionFinished = function(msg)
{
	// TODO: This is a bit inefficient since every unit listens to every
	// construction message - ideally we could scope it to only the one we're building

	this.UnitFsm.ProcessMessage(this, {"type": "ConstructionFinished", "data": msg});
};

UnitAI.prototype.OnGlobalEntityRenamed = function(msg)
{
	let changed = false;
	for (let order of this.orderQueue)
	{
		if (order.data && order.data.target && order.data.target == msg.entity)
		{
			changed = true;
			order.data.target = msg.newentity;
		}
		if (order.data && order.data.formationTarget && order.data.formationTarget == msg.entity)
		{
			changed = true;
			order.data.formationTarget = msg.newentity;
		}
	}
	if (changed)
		Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });
};

UnitAI.prototype.OnAttacked = function(msg)
{
	this.UnitFsm.ProcessMessage(this, {"type": "Attacked", "data": msg});
};

UnitAI.prototype.OnGuardedAttacked = function(msg)
{
	this.UnitFsm.ProcessMessage(this, {"type": "GuardedAttacked", "data": msg.data});
};

UnitAI.prototype.OnHealthChanged = function(msg)
{
	this.UnitFsm.ProcessMessage(this, {"type": "HealthChanged", "from": msg.from, "to": msg.to});
};

UnitAI.prototype.OnRangeUpdate = function(msg)
{
	if (msg.tag == this.losRangeQuery)
		this.UnitFsm.ProcessMessage(this, {"type": "LosRangeUpdate", "data": msg});
	else if (msg.tag == this.losHealRangeQuery)
		this.UnitFsm.ProcessMessage(this, {"type": "LosHealRangeUpdate", "data": msg});
};

UnitAI.prototype.OnPackFinished = function(msg)
{
	this.UnitFsm.ProcessMessage(this, {"type": "PackFinished", "packed": msg.packed});
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
	var runSpeed = cmpUnitMotion.GetRunSpeed();
	var walkSpeed = cmpUnitMotion.GetWalkSpeed();
	if (runSpeed <= walkSpeed)
		return runSpeed;
	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	var health = cmpHealth.GetHitpoints()/cmpHealth.GetMaxHitpoints();
	return (health*runSpeed + (1-health)*walkSpeed);
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
 * Returns the entity ID of the nearest resource supply where the given
 * filter returns true, or undefined if none can be found.
 * if target if given, the nearest is computed versus this target position.
 * TODO: extend this to exclude resources that already have lots of
 * gatherers.
 */
UnitAI.prototype.FindNearbyResource = function(filter, target)
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() == INVALID_PLAYER)
		return undefined;
	var owner = cmpOwnership.GetOwner();

	// We accept resources owned by Gaia or any player
	var players = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetAllPlayers();

	var range = 64; // TODO: what's a sensible number?

	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	let entity = this.entity;
	if (target)
	{
		let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
		if (cmpPosition && cmpPosition.IsInWorld())
			entity = target;
	}
	var nearby = cmpRangeManager.ExecuteQuery(entity, 0, range, players, IID_ResourceSupply);
	return nearby.find(ent => {
		if (!this.CanGather(ent) || !this.CheckTargetVisible(ent))
			return false;
		var cmpResourceSupply = Engine.QueryInterface(ent, IID_ResourceSupply);
		var type = cmpResourceSupply.GetType();
		var amount = cmpResourceSupply.GetCurrentAmount();

		var template = cmpTemplateManager.GetCurrentTemplateName(ent);
		// Remove "resource|" prefix from template names, if present.
		if (template.indexOf("resource|") != -1)
			template = template.slice(9);

		return amount > 0 && cmpResourceSupply.IsAvailable(owner, this.entity) && filter(ent, type, template);
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

	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position)
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return undefined;

	let pos = cmpPosition.GetPosition2D();
	let bestDropsite;
	let bestDist = Infinity;
	// Maximum distance a point on an obstruction can be from the center of the obstruction.
	let maxDifference = 40;

	// Find dropsites owned by this unit's player or allied ones if allowed.
	let owner = cmpOwnership.GetOwner();
	let cmpPlayer = QueryOwnerInterface(this.entity);
	let players = cmpPlayer && cmpPlayer.HasSharedDropsites() ? cmpPlayer.GetMutualAllies() : [owner];
	let nearbyDropsites = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).ExecuteQuery(this.entity, 0, -1, players, IID_ResourceDropsite);

	let isShip = Engine.QueryInterface(this.entity, IID_Identity).HasClass("Ship");
	let cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);
	for (let dropsite of nearbyDropsites)
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
 * Returns the entity ID of the nearest building that needs to be constructed,
 * or undefined if none can be found close enough.
 */
UnitAI.prototype.FindNearbyFoundation = function()
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() == INVALID_PLAYER)
		return undefined;

	// Find buildings owned by this unit's player
	var players = [cmpOwnership.GetOwner()];

	var range = 64; // TODO: what's a sensible number?

	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var nearby = cmpRangeManager.ExecuteQuery(this.entity, 0, range, players, IID_Foundation);

	// Skip foundations that are already complete. (This matters since
	// we process the ConstructionFinished message before the foundation
	// we're working on has been deleted.)
	return nearby.find(ent => !Engine.QueryInterface(ent, IID_Foundation).IsFinished());
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
	return;
};

/*
 * Reset the animation variant to default behavior
 * Default behavior is to pick a resource-carrying variant if resources are being carried.
 * Otherwise pick nothing in particular.
 */
UnitAI.prototype.SetDefaultAnimationVariant = function()
{
	let cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
	if (!cmpResourceGatherer)
	{
		this.SetAnimationVariant("");
		return;
	}

	let type = cmpResourceGatherer.GetLastCarriedType();
	if (type)
	{
		let typename = "carry_" + type.generic;

		// Special case for meat
		if (type.specific == "meat")
			typename = "carry_" + type.specific;

		this.SetAnimationVariant(typename);
		return;
	}

	this.SetAnimationVariant("");
};

UnitAI.prototype.SelectAnimation = function(name, once = false, speed = 1.0)
{
	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpVisual)
		return;

	// Special case: the "move" animation gets turned into a special
	// movement mode that deals with speeds and walk/run automatically
	if (name == "move")
	{
		// Speed to switch from walking to running animations
		cmpVisual.SelectMovementAnimation((this.GetWalkSpeed() + this.GetRunSpeed()) / 2);
		return;
	}

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
	if (!this.CheckTargetVisible(target) || this.IsTurret())
		return false;

	var cmpRanged = Engine.QueryInterface(this.entity, iid);
	if (!cmpRanged)
		return false;
	var range = cmpRanged.GetRange(type);

	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.MoveToTargetRange(target, range.min, range.max);
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
		var cmpFormationUnitAI = Engine.QueryInterface(this.formationController, IID_UnitAI);
		if (cmpFormationUnitAI && cmpFormationUnitAI.IsAttackingAsFormation())
			return false;
	}

	var cmpFormation = Engine.QueryInterface(target, IID_Formation);
	if (cmpFormation)
		target = cmpFormation.GetClosestMember(this.entity);

	if (type != "Ranged")
		return this.MoveToTargetRange(target, IID_Attack, type);

	if (!this.CheckTargetVisible(target))
		return false;

	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	var range = cmpAttack.GetRange(type);

	var thisCmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!thisCmpPosition.IsInWorld())
		return false;
	var s = thisCmpPosition.GetPosition();

	var targetCmpPosition = Engine.QueryInterface(target, IID_Position);
	if (!targetCmpPosition.IsInWorld())
		return false;

	var t = targetCmpPosition.GetPosition();
	// h is positive when I'm higher than the target
	var h = s.y-t.y+range.elevationBonus;

	// No negative roots please
	if (h>-range.max/2)
		var parabolicMaxRange = Math.sqrt(Math.square(range.max) + 2 * range.max * h);
	else
		// return false? Or hope you come close enough?
		var parabolicMaxRange = 0;
		//return false;

	// the parabole changes while walking, take something in the middle
	var guessedMaxRange = (range.max + parabolicMaxRange)/2;

	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (cmpUnitMotion.MoveToTargetRange(target, range.min, guessedMaxRange))
		return true;

	// if that failed, try closer
	return cmpUnitMotion.MoveToTargetRange(target, range.min, Math.min(range.max, parabolicMaxRange));
};

UnitAI.prototype.MoveToTargetRangeExplicit = function(target, min, max)
{
	if (!this.CheckTargetVisible(target))
		return false;

	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.MoveToTargetRange(target, min, max);
};

UnitAI.prototype.MoveToGarrisonRange = function(target)
{
	if (!this.CheckTargetVisible(target))
		return false;

	var cmpGarrisonHolder = Engine.QueryInterface(target, IID_GarrisonHolder);
	if (!cmpGarrisonHolder)
		return false;
	var range = cmpGarrisonHolder.GetLoadingRange();

	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.MoveToTargetRange(target, range.min, range.max);
};

UnitAI.prototype.CheckPointRangeExplicit = function(x, z, min, max)
{
	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.IsInPointRange(x, z, min, max);
};

UnitAI.prototype.CheckTargetRange = function(target, iid, type)
{
	var cmpRanged = Engine.QueryInterface(this.entity, iid);
	if (!cmpRanged)
		return false;
	var range = cmpRanged.GetRange(type);

	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.IsInTargetRange(target, range.min, range.max);
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
		var cmpFormationUnitAI = Engine.QueryInterface(this.formationController, IID_UnitAI);
		if (cmpFormationUnitAI && cmpFormationUnitAI.IsAttackingAsFormation()
			&& cmpFormationUnitAI.order.data.target == target)
			return true;
	}

	var cmpFormation = Engine.QueryInterface(target, IID_Formation);
	if (cmpFormation)
		target = cmpFormation.GetClosestMember(this.entity);

	if (type != "Ranged")
		return this.CheckTargetRange(target, IID_Attack, type);

	var targetCmpPosition = Engine.QueryInterface(target, IID_Position);
	if (!targetCmpPosition || !targetCmpPosition.IsInWorld())
		return false;

	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	var range = cmpAttack.GetRange(type);

	var thisCmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!thisCmpPosition.IsInWorld())
		return false;

	var s = thisCmpPosition.GetPosition();

	var t = targetCmpPosition.GetPosition();

	var h = s.y-t.y+range.elevationBonus;
	var maxRangeSq = 2*range.max*(h + range.max/2);

	if (maxRangeSq < 0)
		return false;

	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.IsInTargetRange(target, range.min, Math.sqrt(maxRangeSq));
};

UnitAI.prototype.CheckTargetRangeExplicit = function(target, min, max)
{
	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.IsInTargetRange(target, min, max);
};

UnitAI.prototype.CheckGarrisonRange = function(target)
{
	var cmpGarrisonHolder = Engine.QueryInterface(target, IID_GarrisonHolder);
	if (!cmpGarrisonHolder)
		return false;
	var range = cmpGarrisonHolder.GetLoadingRange();

	var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (cmpObstruction)
		range.max += cmpObstruction.GetUnitRadius()*1.5; // multiply by something larger than sqrt(2)

	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpUnitMotion.IsInTargetRange(target, range.min, range.max);
};

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

	// Entities that are hidden and miraged are considered visible
	var cmpFogging = Engine.QueryInterface(target, IID_Fogging);
	if (cmpFogging && cmpFogging.IsMiraged(cmpOwnership.GetOwner()))
		return true;

	if (cmpRangeManager.GetLosVisibility(target, cmpOwnership.GetOwner()) == "hidden")
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
	var targetpos = cmpTargetPosition.GetPosition2D();
	var angle = cmpPosition.GetPosition2D().angleTo(targetpos);
	var rot = cmpPosition.GetRotation();
	var delta = (rot.y - angle + Math.PI) % (2 * Math.PI) - Math.PI;
	if (Math.abs(delta) > 0.2)
	{
		var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
		if (cmpUnitMotion)
			cmpUnitMotion.FaceTowardsPoint(targetpos.x, targetpos.y);
	}
};

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
	var heldPosition = this.heldPosition;
	if (heldPosition === undefined)
		heldPosition = { "x": pos.x, "z": pos.z };

	return Math.euclidDistance2D(pos.x, pos.z, heldPosition.x, heldPosition.z) < halfvision + range.max;
};

UnitAI.prototype.CheckTargetIsInVisionRange = function(target)
{
	var cmpVision = Engine.QueryInterface(this.entity, IID_Vision);
	if (!cmpVision)
		return false;
	var range = cmpVision.GetRange();

	var distance = DistanceBetweenEntities(this.entity, target);

	return distance < range;
};

UnitAI.prototype.GetBestAttackAgainst = function(target, allowCapture)
{
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return undefined;
	return cmpAttack.GetBestAttackAgainst(target, allowCapture);
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

	this.PushOrderFront("Attack", { "target": target, "force": false, "allowCapture": true });
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
		this.CanAttack(target)
		&& this.CheckTargetDistanceFromHeldPosition(target, IID_Attack, this.GetBestAttackAgainst(target, true))
		&& (this.GetStance().respondChaseBeyondVision || this.CheckTargetIsInVisionRange(target))
	);
	if (!target)
		return false;

	this.PushOrderFront("Attack", { "target": target, "force": false, "allowCapture": true });
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
		this.PushOrderFront("Flee", { "target": ents[0], "force": false });
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
	var ent = ents.find(ent => this.CanHeal(ent));
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
	// Forced orders shouldn't be interrupted.
	if (force)
		return false;

	// If we are guarding/escorting, don't abandon as long as the guarded unit is in target range of the attacker
	if (this.isGuardOf)
	{
		var cmpUnitAI =  Engine.QueryInterface(target, IID_UnitAI);
		var cmpAttack = Engine.QueryInterface(target, IID_Attack);
		if (cmpUnitAI && cmpAttack &&
		    cmpAttack.GetAttackTypes().some(type => cmpUnitAI.CheckTargetAttackRange(this.isGuardOf, type)))
				return false;
	}

	// Stop if we're in hold-ground mode and it's too far from the holding point
	if (this.GetStance().respondHoldGround)
	{
		if (!this.CheckTargetDistanceFromHeldPosition(target, iid, type))
			return true;
	}

	// Stop if it's left our vision range, unless we're especially persistent
	if (!this.GetStance().respondChaseBeyondVision)
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
	if (this.IsTurret())
		return false;

	if (this.GetStance().respondChase)
		return true;

	// If we are guarding/escorting, chase at least as long as the guarded unit is in target range of the attacker
	if (this.isGuardOf)
	{
		let cmpUnitAI =  Engine.QueryInterface(target, IID_UnitAI);
		let cmpAttack = Engine.QueryInterface(target, IID_Attack);
		if (cmpUnitAI && cmpAttack &&
		    cmpAttack.GetAttackTypes().some(type => cmpUnitAI.CheckTargetAttackRange(this.isGuardOf, type)))
			return true;
	}

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
		this.UnitFsm.ProcessMessage(this, { "type": "FormationLeave" });
};

UnitAI.prototype.GetFormationController = function()
{
	return this.formationController;
};

UnitAI.prototype.GetFormationTemplate = function()
{
	return Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager).GetCurrentTemplateName(this.formationController) || "special/formations/null";
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

	// Add new order to move into formation at the current position
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
			// Find the target unit's position
			var cmpTargetPosition = Engine.QueryInterface(order.data.target, IID_Position);
			if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld())
				return targetPositions;
			targetPositions.push(cmpTargetPosition.GetPosition2D());
			return targetPositions;

		case "Stop":
			return [];

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

	// Return the total distance to the end of the order queue
	return distance;
};

UnitAI.prototype.AddOrder = function(type, data, queued)
{
	if (this.expectedRoute)
		this.expectedRoute = undefined;

	if (queued)
		this.PushOrder(type, data);
	else
	{
		// May happen if an order arrives on the same turn the unit is garrisoned
		// in that case, just forget the order as this will lead to an infinite loop
		if (this.IsGarrisoned() && !this.IsTurret() && type != "Ungarrison")
			return;
		this.ReplaceOrder(type, data);
	}
};

/**
 * Adds guard/escort order to the queue, forced by the player.
 */
UnitAI.prototype.Guard = function(target, queued)
{
	if (!this.CanGuard())
	{
		this.WalkToTarget(target, queued);
		return;
	}

	// if we already had an old guard order, do nothing if the target is the same
	// and the order is running, otherwise remove the previous order
	if (this.isGuardOf)
	{
		if (this.isGuardOf == target && this.order && this.order.type == "Guard")
			return;
		else
			this.RemoveGuard();
	}

	this.AddOrder("Guard", { "target": target, "force": false }, queued);
};

UnitAI.prototype.AddGuard = function(target)
{
	if (!this.CanGuard())
		return false;

	var cmpGuard = Engine.QueryInterface(target, IID_Guard);
	if (!cmpGuard)
		return false;

	// Do not allow to guard a unit already guarding
	var cmpUnitAI = Engine.QueryInterface(target, IID_UnitAI);
	if (cmpUnitAI && cmpUnitAI.IsGuardOf())
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

	// Do not let a unit already guarded to guard. This would work in principle,
	// but would clutter the gui with too much buttons to take all cases into account
	var cmpGuard = Engine.QueryInterface(this.entity, IID_Guard);
	if (cmpGuard && cmpGuard.GetEntities().length)
		return false;

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
UnitAI.prototype.Walk = function(x, z, queued)
{
	if (this.expectedRoute && queued)
		this.expectedRoute.push({ "x": x, "z": z });
	else
		this.AddOrder("Walk", { "x": x, "z": z, "force": true }, queued);
};

/**
 * Adds walk to point range order to queue, forced by the player.
 */
UnitAI.prototype.WalkToPointRange = function(x, z, min, max, queued)
{
	this.AddOrder("Walk", { "x": x, "z": z, "min": min, "max": max, "force": true }, queued);
};

/**
 * Adds stop order to queue, forced by the player.
 */
UnitAI.prototype.Stop = function(queued)
{
	this.AddOrder("Stop", { "force": true }, queued);
};

/**
 * Adds walk-to-target order to queue, this only occurs in response
 * to a player order, and so is forced.
 */
UnitAI.prototype.WalkToTarget = function(target, queued)
{
	this.AddOrder("WalkToTarget", { "target": target, "force": true }, queued);
};

/**
 * Adds walk-and-fight order to queue, this only occurs in response
 * to a player order, and so is forced.
 * If targetClasses is given, only entities matching the targetClasses can be attacked.
 */
UnitAI.prototype.WalkAndFight = function(x, z, targetClasses, allowCapture = true, queued = false)
{
	this.AddOrder("WalkAndFight", { "x": x, "z": z, "targetClasses": targetClasses, "allowCapture": allowCapture, "force": true }, queued);
};

UnitAI.prototype.Patrol = function(x, z, targetClasses, allowCapture = true, queued = false)
{
	if (!this.CanPatrol())
	{
		this.Walk(x, z, queued);
		return;
	}

	this.AddOrder("Patrol", { "x": x, "z": z, "targetClasses": targetClasses, "allowCapture": allowCapture, "force": true }, queued);
};

/**
 * Adds leave foundation order to queue, treated as forced.
 */
UnitAI.prototype.LeaveFoundation = function(target)
{
	// If we're already being told to leave a foundation, then
	// ignore this new request so we don't end up being too indecisive
	// to ever actually move anywhere
	// Ignore also the request if we are packing
	if (this.order && (this.order.type == "LeaveFoundation" || (this.order.type == "Flee" && this.order.data.target == target) || this.IsPacking()))
		return;

	this.PushOrderFront("LeaveFoundation", { "target": target, "force": true });
};

/**
 * Adds attack order to the queue, forced by the player.
 */
UnitAI.prototype.Attack = function(target, allowCapture = true, queued = false)
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
	this.AddOrder("Attack", { "target": target, "force": true, "allowCapture": allowCapture}, queued);
};

/**
 * Adds garrison order to the queue, forced by the player.
 */
UnitAI.prototype.Garrison = function(target, queued)
{
	if (target == this.entity)
		return;
	if (!this.CanGarrison(target))
	{
		this.WalkToTarget(target, queued);
		return;
	}
	this.AddOrder("Garrison", { "target": target, "force": true }, queued);
};

/**
 * Adds ungarrison order to the queue.
 */
UnitAI.prototype.Ungarrison = function()
{
	if (this.IsGarrisoned())
		this.AddOrder("Ungarrison", null, false);
};

/**
 * Adds a garrison order for units that are already garrisoned in the garrison holder.
 */
UnitAI.prototype.Autogarrison = function(target)
{
	this.isGarrisoned = true;
	this.PushOrderFront("Garrison", { "target": target });
};

/**
 * Adds gather order to the queue, forced by the player
 * until the target is reached
 */
UnitAI.prototype.Gather = function(target, queued)
{
	this.PerformGather(target, queued, true);
};

/**
 * Internal function to abstract the force parameter.
 */
UnitAI.prototype.PerformGather = function(target, queued, force)
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

	// Remove "resource|" prefix from template name, if present.
	if (template.indexOf("resource|") != -1)
		template = template.slice(9);

	// Remember the position of our target, if any, in case it disappears
	// later and we want to head to its last known position
	var lastPos = undefined;
	var cmpPosition = Engine.QueryInterface(target, IID_Position);
	if (cmpPosition && cmpPosition.IsInWorld())
		lastPos = cmpPosition.GetPosition();

	this.AddOrder("Gather", { "target": target, "type": type, "template": template, "lastPos": lastPos, "force": force }, queued);
};

/**
 * Adds gather-near-position order to the queue, not forced, so it can be
 * interrupted by attacks.
 */
UnitAI.prototype.GatherNearPosition = function(x, z, type, template, queued)
{
	// Remove "resource|" prefix from template name, if present.
	if (template.indexOf("resource|") != -1)
		template = template.slice(9);

	if (this.IsFormationController() || Engine.QueryInterface(this.entity, IID_ResourceGatherer))
		this.AddOrder("GatherNearPosition", { "type": type, "template": template, "x": x, "z": z, "force": false }, queued);
	else
		this.AddOrder("Walk", { "x": x, "z": z, "force": false }, queued);
};

/**
 * Adds heal order to the queue, forced by the player.
 */
UnitAI.prototype.Heal = function(target, queued)
{
	if (!this.CanHeal(target))
	{
		this.WalkToTarget(target, queued);
		return;
	}

	this.AddOrder("Heal", { "target": target, "force": true }, queued);
};

/**
 * Adds return resource order to the queue, forced by the player.
 */
UnitAI.prototype.ReturnResource = function(target, queued)
{
	if (!this.CanReturnResource(target, true))
	{
		this.WalkToTarget(target, queued);
		return;
	}

	this.AddOrder("ReturnResource", { "target": target, "force": true }, queued);
};

/**
 * Adds trade order to the queue. Either walk to the first market, or
 * start a new route. Not forced, so it can be interrupted by attacks.
 * The possible route may be given directly as a SetupTradeRoute argument
 * if coming from a RallyPoint, or through this.expectedRoute if a user command.
 */
UnitAI.prototype.SetupTradeRoute = function(target, source, route, queued)
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
			this.AddOrder("Trade", data, queued);
	}
	else
	{
		if (this.IsFormationController())
			this.CallMemberFunction("WalkToTarget", [cmpTrader.GetFirstMarket(), queued]);
		else
			this.WalkToTarget(cmpTrader.GetFirstMarket(), queued);
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
	if (this.waypoints && this.waypoints.length > 1)
	{
		let point = this.waypoints.pop();
		return this.MoveToPoint(point.x, point.z) || this.MoveToMarket(targetMarket);
	}

	this.waypoints = undefined;
	return this.MoveToTarget(targetMarket);
};

UnitAI.prototype.PerformTradeAndMoveToNextMarket = function(currentMarket)
{
	if (!this.CanTrade(currentMarket))
	{
		this.StopTrading();
		return;
	}

	if (!this.CheckTargetRange(currentMarket, IID_Trader))
	{
		if (!this.MoveToMarket(currentMarket))	// If the current market is not reached try again
			this.StopTrading();
		return;
	}

	let cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
	let nextMarket = cmpTrader.PerformTrade(currentMarket);
	let amount = cmpTrader.GetGoods().amount;
	if (!nextMarket || !amount || !amount.traderGain)
	{
		this.StopTrading();
		return;
	}

	this.order.data.target = nextMarket;

	if (this.order.data.route && this.order.data.route.length)
	{
		this.waypoints = this.order.data.route.slice();
		if (this.order.data.target == cmpTrader.GetSecondMarket())
			this.waypoints.reverse();
		this.waypoints.unshift(null);  // additionnal dummy point for the market
	}

	if (this.MoveToMarket(nextMarket))	// We've started walking to the next market
		this.SetNextState("APPROACHINGMARKET");
	else
		this.StopTrading();
};

UnitAI.prototype.MarketRemoved = function(market)
{
	if (this.order && this.order.data && this.order.data.target && this.order.data.target == market)
		this.UnitFsm.ProcessMessage(this, { "type": "TradingCanceled", "market": market });
};

UnitAI.prototype.StopTrading = function()
{
	this.StopMoving();
	this.FinishOrder();
	var cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
	cmpTrader.StopTrading();
};

/**
 * Adds repair/build order to the queue, forced by the player
 * until the target is reached
 */
UnitAI.prototype.Repair = function(target, autocontinue, queued)
{
	if (!this.CanRepair(target))
	{
		this.WalkToTarget(target, queued);
		return;
	}

	this.AddOrder("Repair", { "target": target, "autocontinue": autocontinue, "force": true }, queued);
};

/**
 * Adds flee order to the queue, not forced, so it can be
 * interrupted by attacks.
 */
UnitAI.prototype.Flee = function(target, queued)
{
	this.AddOrder("Flee", { "target": target, "force": false }, queued);
};

/**
 * Adds cheer order to the queue. Forced so it won't be interrupted by attacks.
 */
UnitAI.prototype.Cheer = function()
{
	this.AddOrder("Cheering", { "force": true }, false);
};

UnitAI.prototype.Pack = function(queued)
{
	// Check that we can pack
	if (this.CanPack())
		this.AddOrder("Pack", { "force": true }, queued);
};

UnitAI.prototype.Unpack = function(queued)
{
	// Check that we can unpack
	if (this.CanUnpack())
		this.AddOrder("Unpack", { "force": true }, queued);
};

UnitAI.prototype.CancelPack = function(queued)
{
	var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	if (cmpPack && cmpPack.IsPacking() && !cmpPack.IsPacked())
		this.AddOrder("CancelPack", { "force": true }, queued);
};

UnitAI.prototype.CancelUnpack = function(queued)
{
	var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	if (cmpPack && cmpPack.IsPacking() && cmpPack.IsPacked())
		this.AddOrder("CancelUnpack", { "force": true }, queued);
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
	// Stop moving if switching to stand ground
	// TODO: Also stop existing orders in a sensible way
	if (stance == "standground")
		this.StopMoving();

	// Reset the range queries, since the range depends on stance.
	this.SetupRangeQueries();
};

UnitAI.prototype.SetTurretStance = function()
{
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
	if (!this.previousStance)
		return;
	this.SwitchToStance(this.previousStance);
	this.previousStance = undefined;
};

/**
 * Resets losRangeQuery, and if there are some targets in range that we can
 * attack then we start attacking and this returns true; otherwise, returns false.
 */
UnitAI.prototype.FindNewTargets = function()
{
	if (!this.losRangeQuery)
		return false;

	if (!this.GetStance().targetVisibleEnemies)
		return false;

	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	return this.AttackEntitiesByPreference(cmpRangeManager.ResetActiveQuery(this.losRangeQuery));
};

UnitAI.prototype.FindWalkAndFightTargets = function()
{
	if (this.IsFormationController())
	{
		var cmpUnitAI;
		var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
		for (var ent of cmpFormation.members)
		{
			if (!(cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI)))
				continue;
			var targets = cmpUnitAI.GetTargetsFromUnit();
			for (var targ of targets)
			{
				if (!cmpUnitAI.CanAttack(targ))
					continue;
				if (this.order.data.targetClasses)
				{
					var cmpIdentity = Engine.QueryInterface(targ, IID_Identity);
					var targetClasses = this.order.data.targetClasses;
					if (targetClasses.attack && cmpIdentity
						&& !MatchesClassList(cmpIdentity.GetClassesList(), targetClasses.attack))
						continue;
					if (targetClasses.avoid && cmpIdentity
						&& MatchesClassList(cmpIdentity.GetClassesList(), targetClasses.avoid))
						continue;
					// Only used by the AIs to prevent some choices of targets
					if (targetClasses.vetoEntities && targetClasses.vetoEntities[targ])
						continue;
				}
				this.PushOrderFront("Attack", { "target": targ, "force": false, "allowCapture": this.order.data.allowCapture });
				return true;
			}
		}
		return false;
	}

	var targets = this.GetTargetsFromUnit();
	for (var targ of targets)
	{
		if (!this.CanAttack(targ))
			continue;
		if (this.order.data.targetClasses)
		{
			var cmpIdentity = Engine.QueryInterface(targ, IID_Identity);
			var targetClasses = this.order.data.targetClasses;
			if (cmpIdentity && targetClasses.attack
				&& !MatchesClassList(cmpIdentity.GetClassesList(), targetClasses.attack))
				continue;
			if (cmpIdentity && targetClasses.avoid
				&& MatchesClassList(cmpIdentity.GetClassesList(), targetClasses.avoid))
				continue;
			// Only used by the AIs to prevent some choices of targets
			if (targetClasses.vetoEntities && targetClasses.vetoEntities[targ])
				continue;
		}
		this.PushOrderFront("Attack", { "target": targ, "force": false, "allowCapture": this.order.data.allowCapture });
		return true;
	}

	// healers on a walk-and-fight order should heal injured units
	if (this.IsHealer())
		return this.FindNewHealTargets();

	return false;
};

UnitAI.prototype.GetTargetsFromUnit = function()
{
	if (!this.losRangeQuery)
		return [];

	if (!this.GetStance().targetVisibleEnemies)
		return [];

	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return [];

	var attackfilter = function(e) {
		var cmpOwnership = Engine.QueryInterface(e, IID_Ownership);
		if (cmpOwnership && cmpOwnership.GetOwner() > 0)
			return true;
		var cmpUnitAI = Engine.QueryInterface(e, IID_UnitAI);
		return cmpUnitAI && (!cmpUnitAI.IsAnimal() || cmpUnitAI.IsDangerousAnimal());
	};

	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var entities = cmpRangeManager.ResetActiveQuery(this.losRangeQuery);
	var targets = entities.filter(function(v) { return cmpAttack.CanAttack(v) && attackfilter(v); })
		.sort(function(a, b) { return cmpAttack.CompareEntitiesByPreference(a, b); });

	return targets;
};

/**
 * Resets losHealRangeQuery, and if there are some targets in range that we can heal
 * then we start healing and this returns true; otherwise, returns false.
 */
UnitAI.prototype.FindNewHealTargets = function()
{
	if (!this.losHealRangeQuery)
		return false;

	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	return this.RespondToHealableEntities(cmpRangeManager.ResetActiveQuery(this.losHealRangeQuery));
};

UnitAI.prototype.GetQueryRange = function(iid)
{
	var ret = { "min": 0, "max": 0 };
	if (this.GetStance().respondStandGround)
	{
		var cmpRanged = Engine.QueryInterface(this.entity, iid);
		if (!cmpRanged)
			return ret;
		var range = iid !== IID_Attack ? cmpRanged.GetRange() : cmpRanged.GetFullAttackRange();
		var cmpVision = Engine.QueryInterface(this.entity, IID_Vision);
		if (!cmpVision)
			return ret;
		ret.min = range.min;
		ret.max = Math.min(range.max, cmpVision.GetRange());
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
		var range = iid !== IID_Attack ? cmpRanged.GetRange() : cmpRanged.GetFullAttackRange();
		var cmpVision = Engine.QueryInterface(this.entity, IID_Vision);
		if (!cmpVision)
			return ret;
		var vision = cmpVision.GetRange();
		ret.max = Math.min(range.max + vision / 2, vision);
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


UnitAI.prototype.SetMoveSpeed = function(speed)
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	cmpMotion.SetSpeed(speed);
};

UnitAI.prototype.SetHeldPosition = function(x, z)
{
	this.heldPosition = {"x": x, "z": z};
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
		this.AddOrder("Walk", { "x": this.heldPosition.x, "z": this.heldPosition.z, "force": false }, false);
		return true;
	}
	return false;
};

//// Helper functions ////

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
	// (then the individual units can make up their own minds)
	if (this.IsFormationController())
		return true;

	var cmpGarrisonHolder = Engine.QueryInterface(target, IID_GarrisonHolder);
	if (!cmpGarrisonHolder)
		return false;

	// Verify that the target is owned by this entity's player or a mutual ally of this player
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || !(IsOwnedByPlayer(cmpOwnership.GetOwner(), target) || IsOwnedByMutualAllyOfPlayer(cmpOwnership.GetOwner(), target)))
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
	if (this.IsTurret())
		return false;
	// The target must be a valid resource supply, or the mirage of one.
	var cmpResourceSupply = QueryMiragedInterface(target, IID_ResourceSupply);
	if (!cmpResourceSupply)
		return false;

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
	// No need to call "cmpResourceSupply.IsAvailable()" either because that
	// would cause units to walk to full entities instead of choosing another one
	// nearby to gather from, which is undesirable.
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

	if (MatchesClassList(cmpIdentity.GetClassesList(), cmpHeal.GetUnhealableClasses()))
		return false;

	// Verify that the target is a healable class
	if (MatchesClassList(cmpIdentity.GetClassesList(), cmpHeal.GetHealableClasses()))
		return true;

	return false;
};

UnitAI.prototype.CanReturnResource = function(target, checkCarriedResource)
{
	if (this.IsTurret())
		return false;
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

	// Verify that the dropsite is owned by this entity's player (or a mutual ally's if allowed)
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpOwnership && IsOwnedByPlayer(cmpOwnership.GetOwner(), target))
		return true;
	var cmpPlayer = QueryOwnerInterface(this.entity);
	return cmpPlayer && cmpPlayer.HasSharedDropsites() && cmpResourceDropsite.IsShared() &&
	       cmpOwnership && IsOwnedByMutualAllyOfPlayer(cmpOwnership.GetOwner(), target);
};

UnitAI.prototype.CanTrade = function(target)
{
	if (this.IsTurret())
		return false;
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds)
	if (this.IsFormationController())
		return true;

	// Verify that we're able to respond to Trade commands
	var cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
	return cmpTrader && cmpTrader.CanTrade(target);
};

UnitAI.prototype.CanRepair = function(target)
{
	if (this.IsTurret())
		return false;
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds)
	if (this.IsFormationController())
		return true;

	// Verify that we're able to respond to Repair (Builder) commands
	var cmpBuilder = Engine.QueryInterface(this.entity, IID_Builder);
	if (!cmpBuilder)
		return false;

	// Verify that the target can be either built or repaired
	var cmpFoundation = QueryMiragedInterface(target, IID_Foundation);
	var cmpRepairable = Engine.QueryInterface(target, IID_Repairable);
	if (!cmpFoundation && !cmpRepairable)
		return false;

	// Verify that the target is owned by an ally of this entity's player
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	return cmpOwnership && IsOwnedByAllyOfPlayer(cmpOwnership.GetOwner(), target);
};

UnitAI.prototype.CanPack = function()
{
	var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	return cmpPack && !cmpPack.IsPacking() && !cmpPack.IsPacked();
};

UnitAI.prototype.CanUnpack = function()
{
	var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	return cmpPack && !cmpPack.IsPacking() && cmpPack.IsPacked();
};

UnitAI.prototype.IsPacking = function()
{
	var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	return cmpPack && cmpPack.IsPacking();
};

//// Formation specific functions ////

UnitAI.prototype.IsAttackingAsFormation = function()
{
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	return cmpAttack && cmpAttack.CanAttackAsFormation()
		&& this.GetCurrentState() == "FORMATIONCONTROLLER.COMBAT.ATTACKING";
};

//// Animal specific functions ////

UnitAI.prototype.MoveRandomly = function(distance)
{
	// To minimize drift all across the map, animals describe circles
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
	cmpUnitMotion.MoveToPointRange(pos.x - 0.5 * Math.sin(ang), pos.z - 0.5 * Math.cos(ang), dist, dist);
};

UnitAI.prototype.SetFacePointAfterMove = function(val)
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (cmpMotion)
		cmpMotion.SetFacePointAfterMove(val);
};

UnitAI.prototype.AttackEntitiesByPreference = function(ents)
{
	if (!ents.length)
		return false;

	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return false;

	var attackfilter = function(e) {
		var cmpOwnership = Engine.QueryInterface(e, IID_Ownership);
		if (cmpOwnership && cmpOwnership.GetOwner() > 0)
			return true;
		var cmpUnitAI = Engine.QueryInterface(e, IID_UnitAI);
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
 * Call obj.funcname(args) on UnitAI components of all formation members.
 */
UnitAI.prototype.CallMemberFunction = function(funcname, args)
{
	var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
	if (!cmpFormation)
		return;

	cmpFormation.GetMembers().forEach(ent => {
		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		cmpUnitAI[funcname].apply(cmpUnitAI, args);
	});
};

/**
 * Call obj.functname(args) on UnitAI components of all formation members,
 * and return true if all calls return true.
 */
UnitAI.prototype.TestAllMemberFunction = function(funcname, args)
{
	var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
	if (!cmpFormation)
		return false;

	return cmpFormation.GetMembers().every(ent => {
		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		return cmpUnitAI[funcname].apply(cmpUnitAI, args);
	});
};

UnitAI.prototype.UnitFsm = new FSM(UnitAI.prototype.UnitFsmSpec);

Engine.RegisterComponentType(IID_UnitAI, "UnitAI", UnitAI);
