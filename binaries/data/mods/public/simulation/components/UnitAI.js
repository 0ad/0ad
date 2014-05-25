function UnitAI() {}

UnitAI.prototype.Schema =
	"<a:help>Controls the unit's movement, attacks, etc, in response to commands from the player.</a:help>" +
	"<a:example/>" +
	"<element name='AlertReactiveLevel'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>" +
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
//   targetAttackersPassive: anything that hurts us is a viable target, 
//     if we're on a passive/unforced order (e.g. gathering/building)
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
		targetAttackersAlways: true,
		targetAttackersPassive: true,
		respondFlee: false,
		respondChase: true,
		respondChaseBeyondVision: true,
		respondStandGround: false,
		respondHoldGround: false,
	},
	"aggressive": {
		targetVisibleEnemies: true,
		targetAttackersAlways: false,
		targetAttackersPassive: true,
		respondFlee: false,
		respondChase: true,
		respondChaseBeyondVision: false,
		respondStandGround: false,
		respondHoldGround: false,
	},
	"defensive": {
		targetVisibleEnemies: true,
		targetAttackersAlways: false,
		targetAttackersPassive: true,
		respondFlee: false,
		respondChase: false,
		respondChaseBeyondVision: false,
		respondStandGround: false,
		respondHoldGround: true,
	},
	"passive": {
		targetVisibleEnemies: false,
		targetAttackersAlways: false,
		targetAttackersPassive: true,
		respondFlee: true,
		respondChase: false,
		respondChaseBeyondVision: false,
		respondStandGround: false,
		respondHoldGround: false,
	},
	"standground": {
		targetVisibleEnemies: true,
		targetAttackersAlways: false,
		targetAttackersPassive: true,
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

	"PackFinished": function(msg) {
		// ignore
	},

	"PickupCanceled": function(msg) {
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
		if (this.IsAnimal() && !this.IsDomestic())
		{
			this.FinishOrder();
			return;
		}

		// For packable units:
		// 1. If packed, we can move.
		// 2. If unpacked, we first need to pack, then follow case 1.
		if (this.CanPack())
		{
			// Case 2: pack
			this.PushOrderFront("Pack", { "force": true });
			return;
		}

		var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
		cmpUnitMotion.MoveToFormationOffset(msg.data.target, msg.data.x, msg.data.z);

		this.SetNextStateAlwaysEntering("FORMATIONMEMBER.WALKING");
	},

	// Special orders:
	// (these will be overridden by various states)

	"Order.LeaveFoundation": function(msg) {
		// If foundation is not ally of entity, or if entity is unpacked siege,
		// ignore the order
		if (!IsOwnedByAllyOfEntity(this.entity, msg.data.target) || this.IsPacking() || this.CanPack())
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
		if (this.IsAnimal() && !this.IsDomestic())
		{
			this.FinishOrder();
			return;
		}

		// For packable units:
		// 1. If packed, we can move.
		// 2. If unpacked, we first need to pack, then follow case 1.
		if (this.CanPack())
		{
			// Case 2: pack
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
		if (this.IsAnimal() && !this.IsDomestic())
		{
			this.FinishOrder();
			return;
		}

		// For packable units:
		// 1. If packed, we can move.
		// 2. If unpacked, we first need to pack, then follow case 1.
		if (this.CanPack())
		{
			// Case 2: pack
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
		if (this.IsAnimal() && !this.IsDomestic())
		{
			this.FinishOrder();
			return;
		}

		// For packable units:
		// 1. If packed, we can move.
		// 2. If unpacked, we first need to pack, then follow case 1.
		if (this.CanPack())
		{
			// Case 2: pack
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
		var type = this.GetBestAttackAgainst(this.order.data.target);
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
				// Ignore unforced attacks
				// TODO: use special stances instead?
				if (!this.order.data.force)
				{
					this.FinishOrder();
					return;
				}

				// Case 2: unpack
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
					this.SetNextStateAlwaysEntering("ANIMAL.COMBAT.ATTACKING");
				else
					this.SetNextStateAlwaysEntering("INDIVIDUAL.COMBAT.ATTACKING");
			}
			return;
		}

		// For packable units out of attack range:
		// 1. If packed, we need to move to attack range and then unpack.
		// 2. If unpacked, we first need to pack, then follow case 1.
		var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
		if (cmpPack)
		{
			// Ignore unforced attacks
			// TODO: use special stances instead?
			if (!this.order.data.force)
			{
				this.FinishOrder();
				return;
			}
			
			if (this.CanPack())
			{
				// Case 2: pack
				this.PushOrderFront("Pack", { "force": true });
				return;
			}
		}

		// If we can't reach the target, but are standing ground, then abandon this attack order.
		// Unless we're hunting, that's a special case where we should continue attacking our target.
		if (this.GetStance().respondStandGround && !this.order.data.force && !this.order.data.hunting)
		{
			this.FinishOrder();
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
			if (!this.GetBestAttackAgainst(this.order.data.target))
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

			this.PushOrderFront("Attack", { "target": this.order.data.target, "force": false, "hunting": true });
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
			this.SetNextStateAlwaysEntering("INDIVIDUAL.GATHER.GATHERING");
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
		if (!cmpTrader || ! cmpTrader.HasBothMarkets())
		{
			this.FinishOrder();
			return;
		}

		var nextMarket = cmpTrader.GetNextMarket();
		if (nextMarket == this.order.data.firstMarket)
			var state = "TRADE.APPROACHINGFIRSTMARKET";
		else
			var state = "TRADE.APPROACHINGSECONDMARKET";

		// TODO find the nearest way-point from our position, and start with it
		this.waypoints = undefined;
		if (this.MoveToMarket(nextMarket))
		{
			// We've started walking to the next market
			this.SetNextState(state);
		}
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
		// For packable units:
		// 1. If packed, we can move to the garrison target.
		// 2. If unpacked, we first need to pack, then follow case 1.
		if (this.CanPack())
		{
			// Case 2: pack
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

	"Order.Autogarrison": function(msg) {
		this.SetNextState("INDIVIDUAL.AUTOGARRISON");
	},

	"Order.Alert": function(msg) {
		this.alertRaiser = this.order.data.raiser;
		
		// Find a target to garrison into, if we don't already have one
		if (!this.alertGarrisoningTarget)
			this.alertGarrisoningTarget = this.FindNearbyGarrisonHolder();
		
		if (this.alertGarrisoningTarget)
			this.ReplaceOrder("Garrison", {"target": this.alertGarrisoningTarget});
		else
			this.FinishOrder();
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

		"Order.Guard": function(msg) {
			this.CallMemberFunction("Guard", [msg.data.target, false]);
			var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
			cmpFormation.Disband();
		},

		"Order.Stop": function(msg) {
			this.CallMemberFunction("Stop", [false]);
			this.FinishOrder();
		},

		"Order.Attack": function(msg) {
			var target = msg.data.target;
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
			this.CallMemberFunction("Attack", [target, false]); 
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

				this.PushOrderFront("Attack", { "target": msg.data.target, "hunting": true });
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

			this.SetNextStateAlwaysEntering("MEMBER");
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

			this.SetNextStateAlwaysEntering("MEMBER");
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

			this.SetNextStateAlwaysEntering("MEMBER");
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

			this.SetNextStateAlwaysEntering("MEMBER");
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

			this.SetNextStateAlwaysEntering("MEMBER");
		},

		"Order.Pack": function(msg) {
			this.CallMemberFunction("Pack", [false]);

			this.SetNextStateAlwaysEntering("MEMBER");
		},

		"Order.Unpack": function(msg) {
			this.CallMemberFunction("Unpack", [false]);

			this.SetNextStateAlwaysEntering("MEMBER");
		},

		"IDLE": {
			"enter": function(msg) {
				var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.SetRearrange(false);
			},
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
					this.SetNextStateAlwaysEntering("MEMBER");
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
					this.CallMemberFunction("Attack", [this.order.data.target, false]); 
					if (cmpAttack.CanAttackAsFormation())
						this.SetNextState("COMBAT.ATTACKING");
					else
						this.SetNextState("MEMBER");
				},
			},

			"ATTACKING": {
				// Wait for individual members to finish
				"enter": function(msg) {
					var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
					cmpFormation.SetRearrange(true);
					cmpFormation.MoveMembersIntoFormation(false, false);
					this.StartTimer(200, 200);

					var target = this.order.data.target;
					// Check if we are already in range, otherwise walk there
					if (!this.CheckTargetAttackRange(target, target))
					{
						if (this.TargetIsAlive(target) && this.CheckTargetVisible(target))
						{
							var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
							var range = cmpAttack.GetRange(target);
							this.PushOrderFront("WalkToTargetRange", { "target": target, "min": range.min, "max": range.max }); 
							return;
						}
						this.FinishOrder();
						return;
					}

				},

				"Timer": function(msg) {
					var target = this.order.data.target;
					// Check if we are already in range, otherwise walk there
					if (!this.CheckTargetAttackRange(target, target))
					{
						if (this.TargetIsAlive(target) && this.CheckTargetVisible(target))
						{
							var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
							var range = cmpAttack.GetRange(target);
							this.FinishOrder();
							this.PushOrderFront("Attack", { "target": target, "force": false }); 
							return;
						}
						this.FinishOrder();
						return;
					}
				},

				"leave": function(msg) {
					this.StopTimer();
				},
			},
		},

		"MEMBER": {
			// Wait for individual members to finish
			"enter": function(msg) {
				var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
				cmpFormation.SetRearrange(false);
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
			if (!IsOwnedByAllyOfEntity(this.entity, msg.data.target) || this.IsPacking() || this.CanPack())
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
			"enter": function () {
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
			// Respond to attack if we always target attackers, or if we target attackers
			//	during passive orders (e.g. gathering/repairing are never forced)
			if (this.GetStance().targetAttackersAlways || (this.GetStance().targetAttackersPassive && (!this.order || !this.order.data || !this.order.data.force)))
			{
				this.RespondToTargetedEntities([msg.data.attacker]);
			}
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
				else if (this.CanRepair(this.isGuardOf) && cmpHealth.IsRepairable())
					this.PushOrderFront("Repair", { "target": this.isGuardOf, "autocontinue": false, "force": false });
				return;
			}

			// if the attacker is a building and we can repair the guarded, repair it rather than attacking
			var cmpBuildingAI = Engine.QueryInterface(msg.data.attacker, IID_BuildingAI);
			if (cmpBuildingAI && this.CanRepair(this.isGuardOf) && cmpHealth.IsRepairable())
			{
				this.PushOrderFront("Repair", { "target": this.isGuardOf, "autocontinue": false, "force": false });
				return;
			}

			// target the unit
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
					this.orderQueue.splice(1, 1);
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
				var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
				if (this.losRangeQuery)
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
					this.AttackEntitiesByPreference(msg.data.added);
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
		},

		"WALKING": {
			"enter": function () {
				this.SelectAnimation("move");
			},

			"MoveCompleted": function() {
				this.FinishOrder();
			},
		},

		"WALKINGANDFIGHTING": {
			"enter": function () {
				// Show weapons rather than carried resources.
				this.SetGathererAnimationOverride(true);

				this.StartTimer(0, 1000);
				this.SelectAnimation("move");
			},

			"Timer": function(msg) {
				this.FindWalkAndFightTargets();
			},

			"leave": function(msg) {
				this.StopTimer();
			},

			"MoveCompleted": function() {
				this.FinishOrder();
			},
		},

		"GUARD": {
			"RemoveGuard": function() {
				this.StopMoving();
				this.FinishOrder();
			},

			"ESCORTING": {
				"enter": function () {
					// Show weapons rather than carried resources.
					this.SetGathererAnimationOverride(true);

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
				"enter": function () {
					this.StartTimer(1000, 1000);
					this.SetHeldPositionOnEntity(this.entity);
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
							else if (this.CanRepair(this.isGuardOf) && cmpHealth.IsRepairable())
								this.PushOrderFront("Repair", { "target": this.isGuardOf, "autocontinue": false, "force": false });
						}
					}
				},

				"leave": function(msg) {
					this.StopTimer();
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
				// If we're already in combat mode, ignore anyone else
				// who's attacking us
			},

			"APPROACHING": {
				"enter": function () {
					// Show weapons rather than carried resources.
					this.SetGathererAnimationOverride(true);

					this.SelectAnimation("move");
					this.StartTimer(1000, 1000);
				},

				"leave": function() {
					// Show carried resources when walking.
					this.SetGathererAnimationOverride();

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
							this.SetNextState("UNPACKING");
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

				"Attacked": function(msg) {
					// If we're attacked by a close enemy, we should try to defend ourself
					//  but only if we're not forced to target something else
					if (msg.data.type == "Melee" && (this.GetStance().targetAttackersAlways || (this.GetStance().targetAttackersPassive && !this.order.data.force)))
					{
						this.RespondToTargetedEntities([msg.data.attacker]);
					}
				},
			},

			"UNPACKING": {
				"enter": function() {
					// If we're not in range yet (maybe we stopped moving), move to target again
					if (!this.CheckTargetAttackRange(this.order.data.target, this.order.data.attackType))
					{
						if (this.MoveToTargetAttackRange(this.order.data.target, this.order.data.attackType))
							this.SetNextState("APPROACHING");
						else
						{
							// Give up
							this.FinishOrder();
						}
						return true;
					}

					// In range, unpack
					var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
					cmpPack.Unpack();
					return false;
				},

				"PackFinished": function(msg) {
					this.SetNextState("ATTACKING");
				},

				"leave": function() {
				},
				
				"Attacked": function(msg) {
					// Ignore further attacks while unpacking
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
					if 
					(
						this.TargetIsAlive(target) && 
						this.CanAttack(target, this.order.data.forceResponse || null) && 
						!this.CheckTargetAttackRange(target, this.order.data.attackType)
					)
					{
						// Can't reach it - try to chase after it
						if (this.ShouldChaseTargetedEntity(target, this.order.data.force))
						{
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

					// add prefix + no capital first letter for attackType
					var animationName = "attack_" + this.order.data.attackType.toLowerCase();
					if (this.IsFormationMember())
					{
						var cmpFormation = Engine.QueryInterface(this.formationController, IID_Formation);
						if (cmpFormation)
							animationName = cmpFormation.GetFormationAnimation(this.entity, animationName);
					}
					this.SelectAnimation(animationName, false, 1.0, "attack");
					this.SetAnimationSync(prepare, this.attackTimers.repeat);
					this.StartTimer(prepare, this.attackTimers.repeat);
					// TODO: we should probably only bother syncing projectile attacks, not melee

					// If using a non-default prepare time, re-sync the animation when the timer runs.
					this.resyncAnimation = (prepare != this.attackTimers.prepare) ? true : false;

					this.FaceTowardsTarget(this.order.data.target);
				},

				"leave": function() {
					this.StopTimer();
				},

				"Timer": function(msg) {
					var target = this.order.data.target;
					var cmpFormation = Engine.QueryInterface(target, IID_Formation);
					// if the target is a formation, save the attacking formation, and pick a member
					if (cmpFormation)
					{
						var thisObject = this;
						var filter = function(t) {
							return thisObject.TargetIsAlive(t) && thisObject.CanAttack(t, thisObject.order.data.forceResponse || null);
						};
						this.order.data.formationTarget = target;
						target = cmpFormation.GetClosestMember(this.entity, filter);
						this.order.data.target = target;
					}
					// Check the target is still alive and attackable
					if (this.TargetIsAlive(target) && this.CanAttack(target, this.order.data.forceResponse || null))
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
						var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
						cmpAttack.PerformAttack(this.order.data.attackType, target);

						
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

					this.oldAttackType = this.order.data.attackType;
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
						if (this.orderQueue.length > 0 && this.orderQueue[0].data.attackType == this.oldAttackType)
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
					if (this.order.data.target != msg.data.attacker)
					{
						// If we're attacked by a close enemy, stronger than our current target,
						//  we choose to attack it, but only if we're not forced to target something else
						if (msg.data.type == "Melee" && (this.GetStance().targetAttackersAlways || (this.GetStance().targetAttackersPassive && !this.order.data.force)))
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
					// Show weapons rather than carried resources.
					this.SetGathererAnimationOverride(true);

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
					this.SetGathererAnimationOverride();

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
					if (!cmpSupply || !cmpSupply.AddGatherer(cmpOwnership.GetOwner(), this.entity))
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
						var nearby = this.FindNearbyResource(function (ent, type, template) {
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
							return true;
						}
						else
						{
							// It's probably better in this case, to avoid units getting stuck around a dropsite
							// in a "Target is far away, full, nearby are no good resources, return to dropsite" loop
							// to order it to GatherNear the resource position.
							var cmpPosition = Engine.QueryInterface(this.gatheringTarget, IID_Position);
							if (cmpPosition)
							{
								var pos = cmpPosition.GetPosition();
								this.GatherNearPosition(pos.x, pos.z, oldType, oldTemplate);
								return true;
							} else {
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
						var nearby = this.FindNearbyResource(function (ent, type, template) {
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
						var typename = "gather_" + this.order.data.type.specific;
						this.SelectAnimation(typename, false, 1.0, typename);
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
					this.SetGathererAnimationOverride();
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
					if (this.FinishOrder())
						return;

					// No remaining orders - pick a useful default behaviour

					// Try to find a new resource of the same specific type near our current position:
					// Also don't switch to a different type of huntable animal
					var nearby = this.FindNearbyResource(function (ent, type, template) {
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
						this.GatherNearPosition(herdPos.x, herdPos.z, resourceType, resourceTemplate);

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
				"enter": function () {
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

					this.SelectAnimation("heal", false, 1.0, "heal");
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
					if (this.ShouldAbandonChase(this.order.data.target, this.order.data.force, IID_Heal, null))
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
							this.SetGathererAnimationOverride();

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

			"APPROACHINGFIRSTMARKET": {
				"enter": function () {
					this.SelectAnimation("move");
				},

				"MoveCompleted": function() {
					if (this.waypoints && this.waypoints.length)
					{
						if (!this.MoveToMarket(this.order.data.firstMarket))
							this.stopTrading();
					}
					else
						this.PerformTradeAndMoveToNextMarket(this.order.data.firstMarket, this.order.data.secondMarket, "APPROACHINGSECONDMARKET");
				},
			},

			"APPROACHINGSECONDMARKET": {
				"enter": function () {
					this.SelectAnimation("move");
				},

				"MoveCompleted": function() {
					if (this.waypoints && this.waypoints.length)
					{
						if (!this.MoveToMarket(this.order.data.secondMarket))
							this.stopTrading();
					}
					else
						this.PerformTradeAndMoveToNextMarket(this.order.data.secondMarket, this.order.data.firstMarket, "APPROACHINGFIRSTMARKET");
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

					var cmpFoundation = Engine.QueryInterface(this.repairTarget, IID_Foundation);
					if (cmpFoundation)
						cmpFoundation.AddBuilder(this.entity);

					this.SelectAnimation("build", false, 1.0, "build");
					this.StartTimer(1000, 1000);
					return false;
				},

				"leave": function() {
					var cmpFoundation = Engine.QueryInterface(this.repairTarget, IID_Foundation);
					if (cmpFoundation)
						cmpFoundation.RemoveBuilder(this.entity);
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

				// We finished building it.
				// Switch to the next order (if any)
				if (this.FinishOrder())
					return;

				// No remaining orders - pick a useful default behaviour

				// If autocontinue explicitly disabled (e.g. by AI) then
				// do nothing automatically
				if (!oldData.autocontinue)
					return;

				// If this building was e.g. a farm of ours, the entities that recieved
				// the build command should start gathering from it
				if ((oldData.force || oldData.autoharvest) && this.CanGather(msg.data.newentity))
				{
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
					var nearby = this.FindNearbyResource(function (ent, type, template) {
						return (types.indexOf(type.generic) != -1);
					});
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
					if(this.IsUnderAlert())
					{
						// check that we can garrison in the building we're supposed to garrison in
						var cmpGarrisonHolder = Engine.QueryInterface(this.alertGarrisoningTarget, IID_GarrisonHolder);
						if (!cmpGarrisonHolder || cmpGarrisonHolder.IsFull())
						{
							// Try to find another nearby building
							var nearby = this.FindNearbyGarrisonHolder();
							if (nearby)
							{
								this.alertGarrisoningTarget = nearby;
								this.ReplaceOrder("Garrison", {"target": this.alertGarrisoningTarget});
							}
							else
								this.FinishOrder();
						}
						else
							this.SetNextState("GARRISONED");
					}
					else
						this.SetNextState("GARRISONED");
				},
			},

			"GARRISONED": {
				"enter": function() {
					// Target is not handled the same way with Alert and direct garrisoning
					if(this.order.data.target)
						var target = this.order.data.target;
					else
					{
						if(!this.alertGarrisoningTarget)
						{
							// We've been unable to find a target nearby, so give up
							this.FinishOrder();
							return true;
						}
						var target = this.alertGarrisoningTarget;
					}

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
								if (cmpResourceDropsite)
								{
									// Dump any resources we can
									var dropsiteTypes = cmpResourceDropsite.GetTypes();
									var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
									if (cmpResourceGatherer)
									{
										cmpResourceGatherer.CommitResources(dropsiteTypes);
										this.SetGathererAnimationOverride();
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
				
				"Order.Ungarrison": function() {
					if (this.FinishOrder())
						return;
				},

				"leave": function() {
					this.isGarrisoned = false;
				}
			},
		},

		"AUTOGARRISON": {
			"enter": function() {
				this.isGarrisoned = true;
				return false;
			},

			"Order.Ungarrison": function() {
				if (this.FinishOrder())
					return;
			},

			"leave": function() {
				this.isGarrisoned = false;
			}
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
			// Run away from the foundation
			this.FinishOrder();
			this.PushOrderFront("Flee", { "target": msg.data.target, "force": false });
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
				this.MoveRandomly(+this.template.RoamDistance);
				// Set a random timer to switch to feeding state
				this.StartTimer(RandomInt(+this.template.RoamTimeMin, +this.template.RoamTimeMax));
				this.SetFacePointAfterMove(false);
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
	this.lastFormationTemplate = "";
	this.finishedOrder = false; // used to find if all formation members finished the order
	
	this.heldPosition = undefined;

	// Queue of remembered works
	this.workOrders = [];

	this.isGuardOf = undefined;

	// "Town Bell" behaviour
	this.alertRaiser = undefined;
	this.alertGarrisoningTarget = undefined;

	// For preventing increased action rate due to Stop orders or target death.
	this.lastAttacked = undefined;
	this.lastHealed = undefined;

	this.SetStance(this.template.DefaultStance);
};

UnitAI.prototype.ReactsToAlert = function(level)
{
	return this.template.AlertReactiveLevel <= level;
};

UnitAI.prototype.IsUnderAlert = function()
{
	return this.alertRaiser != undefined;
};

UnitAI.prototype.ResetAlert = function()
{
	this.alertGarrisoningTarget = undefined;
	this.alertRaiser = undefined;
};

UnitAI.prototype.GetAlertRaiser = function()
{
	return this.alertRaiser;
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
 * return true if in WalkAndFight looking for new targets
 */
UnitAI.prototype.IsWalkingAndFighting = function()
{
	if (this.IsFormationMember())
	{
		var cmpUnitAI = Engine.QueryInterface(this.formationController, IID_UnitAI);
		return (cmpUnitAI && cmpUnitAI.IsWalkingAndFighting());
	}

	return (this.orderQueue.length > 0 && this.orderQueue[0].type == "WalkAndFight");
};

UnitAI.prototype.OnCreate = function()
{
	if (this.IsAnimal())
		UnitFsm.Init(this, "ANIMAL.FEEDING");
	else if (this.IsFormationController())
		UnitFsm.Init(this, "FORMATIONCONTROLLER.IDLE");
	else
		UnitFsm.Init(this, "INDIVIDUAL.IDLE");
};

UnitAI.prototype.OnDiplomacyChanged = function(msg)
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpOwnership && cmpOwnership.GetOwner() == msg.player)
		this.SetupRangeQuery();
};

UnitAI.prototype.OnOwnershipChanged = function(msg)
{
	this.SetupRangeQueries();

	// If the unit isn't being created or dying, reset stance and clear orders (if not garrisoned).
	if (msg.to != -1 && msg.from != -1)
	{
		// Switch to a virgin state to let states execute their leave handlers.
		var index = this.GetCurrentState().indexOf(".");
		if (index != -1)
			UnitFsm.SwitchToNextState(this, this.GetCurrentState().slice(0,index));

		this.SetStance(this.template.DefaultStance);
		if(!this.isGarrisoned)
			this.Stop(false);
	}
};

UnitAI.prototype.OnDestroy = function()
{
	// Switch to an empty state to let states execute their leave handlers.
	UnitFsm.SwitchToNextState(this, "");

	// Clean up range queries
	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (this.losRangeQuery)
		rangeMan.DestroyActiveQuery(this.losRangeQuery);
	if (this.losHealRangeQuery)
		rangeMan.DestroyActiveQuery(this.losHealRangeQuery);
};

UnitAI.prototype.OnVisionRangeChanged = function(msg)
{
	// Update range queries
	if (this.entity == msg.entity)
		this.SetupRangeQueries();
};

UnitAI.prototype.HasPickupOrder = function(entity)
{
	for each (var order in this.orderQueue)
		if (order.type == "PickupUnit" && order.data.target == entity)
			return true;
	return false;
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
	var cmpUnitAI = Engine.QueryInterface(msg.entity, IID_UnitAI);
	for (var i = 0; i < this.orderQueue.length; ++i)
	{
		if (this.orderQueue[i].type == "PickupUnit" && this.orderQueue[i].data.target == msg.entity)
		{
			if (i == 0)
				UnitFsm.ProcessMessage(this, {"type": "PickupCanceled", "data": msg});
			else
				this.orderQueue.splice(i, 1);
			break;
		}
	}
};

// Wrapper function that sets up the normal and healer range queries.
UnitAI.prototype.SetupRangeQueries = function()
{
	this.SetupRangeQuery();

	if (this.IsHealer())
		this.SetupHealRangeQuery();

}

// Set up a range query for all enemy and gaia units within LOS range
// which can be attacked.
// This should be called whenever our ownership changes.
UnitAI.prototype.SetupRangeQuery = function()
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var owner = cmpOwnership.GetOwner();

	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var playerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);

	if (this.losRangeQuery)
	{
		rangeMan.DestroyActiveQuery(this.losRangeQuery);
		this.losRangeQuery = undefined;
	}

	var players = [];

	if (owner != -1)
	{
		// If unit not just killed, get enemy players via diplomacy
		var cmpPlayer = Engine.QueryInterface(playerMan.GetPlayerByID(owner), IID_Player);
		var numPlayers = playerMan.GetNumPlayers();
		for (var i = 0; i < numPlayers; ++i)
		{
			// Exclude allies, and self
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

// This will make sure that the state is always entered even if this means leaving it and reentering it
// This is so that a state can be reinitialized with new order data without having to switch to an intermediate state
UnitAI.prototype.SetNextStateAlwaysEntering = function(state)
{
	UnitFsm.SetNextStateAlwaysEntering(this, state);
};

UnitAI.prototype.DeferMessage = function(msg)
{
	UnitFsm.DeferMessage(this, msg);
};

UnitAI.prototype.GetCurrentState = function()
{
	return UnitFsm.GetCurrentState(this);
};

UnitAI.prototype.FsmStateNameChanged = function(state)
{
	Engine.PostMessage(this.entity, MT_UnitAIStateChanged, { "to": state });
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
	{
		var stack = new Error().stack.trimRight().replace(/^/mg, '  '); // indent each line
		var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
		var template = cmpTemplateManager.GetCurrentTemplateName(this.entity);
		error("FinishOrder called for entity " + this.entity + " (" + template + ") when order queue is empty\n" + stack);
	}

	this.orderQueue.shift();
	this.order = this.orderQueue[0];

	if (this.orderQueue.length)
	{
		var ret = UnitFsm.ProcessMessage(this,
			{"type": "Order."+this.order.type, "data": this.order.data}
		);

		Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });

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

		Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });

		// Check if there are queued formation orders
		if (this.IsFormationMember())
		{
			var cmpUnitAI = Engine.QueryInterface(this.formationController, IID_UnitAI);
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

		Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });

		// If the order was rejected then immediately take it off
		// and process the remaining queue
		if (ret && ret.discardOrder)
			this.FinishOrder();
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
	// same thing if current order if packing/unpacking
	if (this.order && this.order.type == "Cheering")
	{
		var cheeringOrder = this.orderQueue.shift();
		this.orderQueue.unshift(cheeringOrder, order);
	}
	else if (this.order && this.IsPacking())
	{
		var packingOrder = this.orderQueue.shift();
		this.orderQueue.unshift = (packingOrder, order);
	}
	else
	{
		this.orderQueue.unshift(order);
		this.order = order;
		var ret = UnitFsm.ProcessMessage(this,
			{"type": "Order."+this.order.type, "data": this.order.data}
		);

		Engine.PostMessage(this.entity, MT_UnitAIOrderDataChanged, { "to": this.GetOrderData() });

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

/**
 * Insert an order after the last forced order onto the queue
 * and after the other orders of the same type
 */
UnitAI.prototype.PushOrderAfterForced = function(type, data)
{
	if (!this.order || ((!this.order.data || !this.order.data.force) && this.order.type != type))
	{
		this.PushOrderFront(type, data);
	}
	else
	{
		for (var i = 1; i < this.orderQueue.length; ++i)
		{
			if (this.orderQueue[i].data && this.orderQueue[i].data.force)
				continue;
			if (this.orderQueue[i].type == type)
				continue;
			this.orderQueue.splice(i, 0, {"type": type, "data": data});
			return;
		}
		this.PushOrder(type, data);
	}
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
};

UnitAI.prototype.GetOrders = function()
{
	return this.orderQueue.slice();
};

UnitAI.prototype.AddOrders = function(orders)
{
	for each (var order in orders)
	{
		this.PushOrder(order.type, order.data);
	}
};

UnitAI.prototype.GetOrderData = function()
{
	var orders = [];
	for (var i in this.orderQueue)
	{
		if (this.orderQueue[i].data)
			orders.push(deepcopy(this.orderQueue[i].data));
	}
	return orders;
};

UnitAI.prototype.UpdateWorkOrders = function(type)
{
	// Under alert, remembered work orders won't be forgotten
	if (this.IsUnderAlert())
		return;

	var isWorkType = function(type){
		return (type == "Gather" || type == "Trade" || type == "Repair" || type == "ReturnResource");
	};

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

	// Clear the order queue considering special orders not to avoid
	if (this.order && this.order.type == "Cheering")
	{
		var cheeringOrder = this.orderQueue.shift();
		this.orderQueue = [cheeringOrder];
	}
	else
		this.orderQueue = [];

	this.AddOrders(this.workOrders);

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
	{
		this.timer = undefined;
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
	for each (var order in this.orderQueue)
	{
		if (order.data && order.data.target && order.data.target == msg.entity)
			order.data.target = msg.newentity;
		if (order.data && order.data.formationTarget && order.data.formationTarget == msg.entity)
			order.data.formationTarget = msg.newentity;
	}

	if (this.isGuardOf && this.isGuardOf == msg.entity)
		this.isGuardOf = msg.newentity;
};

UnitAI.prototype.OnAttacked = function(msg)
{
	UnitFsm.ProcessMessage(this, {"type": "Attacked", "data": msg});
};

UnitAI.prototype.OnGuardedAttacked = function(msg)
{
	UnitFsm.ProcessMessage(this, {"type": "GuardedAttacked", "data": msg.data});
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

UnitAI.prototype.OnPackFinished = function(msg)
{
	UnitFsm.ProcessMessage(this, {"type": "PackFinished", "packed": msg.packed});
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
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var nearby = cmpRangeManager.ExecuteQuery(this.entity, 0, range, players, IID_ResourceSupply);
	for each (var ent in nearby)
	{
		if (!this.CanGather(ent))
			continue;
		var cmpResourceSupply = Engine.QueryInterface(ent, IID_ResourceSupply);
		var type = cmpResourceSupply.GetType();
		var amount = cmpResourceSupply.GetCurrentAmount();
		var template = cmpTemplateManager.GetCurrentTemplateName(ent);

		// Remove "resource|" prefix from template names, if present.
		if (template.indexOf("resource|") != -1)
			template = template.slice(9);

		if (amount > 0 && cmpResourceSupply.IsAvailable(cmpOwnership.GetOwner(), this.entity) && filter(ent, type, template))
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
		players = [cmpOwnership.GetOwner()];

	// Ships are unable to reach land dropsites and shouldn't attempt to do so.
	var excludeLand = Engine.QueryInterface(this.entity, IID_Identity).HasClass("Ship");

	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var nearby = rangeMan.ExecuteQuery(this.entity, 0, -1, players, IID_ResourceDropsite);
	if (excludeLand)
	{
		nearby = nearby.filter( function(e) {
			return Engine.QueryInterface(e, IID_Identity).HasClass("Naval");
		});
	}

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
		players = [cmpOwnership.GetOwner()];

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
 * Returns the entity ID of the nearest building in which the unit can garrison,
 * or undefined if none can be found close enough.
 */
UnitAI.prototype.FindNearbyGarrisonHolder = function()
{
	var range = 128; // TODO: what's a sensible number?

	// Find buildings owned by this unit's player
	var players = [];
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpOwnership)
		players = [cmpOwnership.GetOwner()];

	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var nearby = rangeMan.ExecuteQuery(this.entity, 0, range, players, IID_GarrisonHolder);
	for each (var ent in nearby)
	{
        var cmpGarrisonHolder = Engine.QueryInterface(ent, IID_GarrisonHolder);
		// We only want to garrison in buildings, not in moving units like ships,...
		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
        if (!cmpUnitAI && cmpGarrisonHolder.AllowedToGarrison(this.entity) && !cmpGarrisonHolder.IsFull())
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

UnitAI.prototype.SetGathererAnimationOverride = function(disable)
{
	var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
	if (!cmpResourceGatherer)
		return;

	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpVisual)
		return;

	// Remove the animation override, so that weapons are shown again.
	if (disable)
	{
		cmpVisual.ResetMoveAnimation("walk");
		return;
	}

	// Work out what we're carrying, in order to select an appropriate animation
	var type = cmpResourceGatherer.GetLastCarriedType();
	if (type)
	{
		var typename = "carry_" + type.generic;

		// Special case for meat
		if (type.specific == "meat")
			typename = "carry_" + type.specific;

		cmpVisual.ReplaceMoveAnimation("walk", typename);
	}
	else
		cmpVisual.ResetMoveAnimation("walk");
}

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
		var cmpFormationAttack = Engine.QueryInterface(this.formationController, IID_Attack);
		var cmpFormationUnitAI = Engine.QueryInterface(this.formationController, IID_UnitAI);
		if (cmpFormationAttack && cmpFormationAttack.CanAttackAsFormation() && cmpFormationUnitAI && cmpFormationUnitAI.GetCurrentState == "FORMATIONCONTROLLER.ATTACKING")
			return false;
	}

	var cmpFormation = Engine.QueryInterface(target, IID_Formation)
	if (cmpFormation)
		target = cmpFormation.GetClosestMember(this.entity);

	if(type!= "Ranged") 
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
	if(!targetCmpPosition.IsInWorld()) 
		return false;   

	var t = targetCmpPosition.GetPosition();
	// h is positive when I'm higher than the target
	var h = s.y-t.y+range.elevationBonus;

	// No negative roots please
	if(h>-range.max/2) 
		var parabolicMaxRange = Math.sqrt(range.max*range.max+2*range.max*h);
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
		var cmpFormationAttack = Engine.QueryInterface(this.formationController, IID_Attack);
		var cmpFormationUnitAI = Engine.QueryInterface(this.formationController, IID_UnitAI);

		if
		(
			cmpFormationAttack && 
			cmpFormationAttack.CanAttackAsFormation() &&
			cmpFormationUnitAI && 
			cmpFormationUnitAI.GetCurrentState() == "FORMATIONCONTROLLER.COMBAT.ATTACKING" &&
			cmpFormationUnitAI.order.data.target == target
		)
			return true;
	}

	var cmpFormation = Engine.QueryInterface(target, IID_Formation)
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

	return maxRangeSq >= distanceSq && range.min*range.min <= distanceSq;

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
		heldPosition = {"x": pos.x, "z": pos.z};
	var dx = heldPosition.x - pos.x;
	var dz = heldPosition.z - pos.z;
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

UnitAI.prototype.GetBestAttackAgainst = function(target)
{
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return undefined;
	return cmpAttack.GetBestAttackAgainst(target);
};

UnitAI.prototype.GetAttackBonus = function(type, target)
{
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return 1;
	return cmpAttack.GetAttackBonus(type, target);
};

/**
 * Try to find one of the given entities which can be attacked,
 * and start attacking it.
 * Returns true if it found something to attack.
 */
UnitAI.prototype.AttackVisibleEntity = function(ents, forceResponse)
{
	for each (var target in ents)
	{
		if (this.CanAttack(target, forceResponse))
		{
			this.PushOrderFront("Attack", { "target": target, "force": false, "forceResponse": forceResponse });
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
UnitAI.prototype.AttackEntityInZone = function(ents, forceResponse)
{
	for each (var target in ents)
	{
		var type = this.GetBestAttackAgainst(target);
		if (this.CanAttack(target, forceResponse) && this.CheckTargetDistanceFromHeldPosition(target, IID_Attack, type)
		    && (this.GetStance().respondChaseBeyondVision || this.CheckTargetIsInVisionRange(target)))
		{
			this.PushOrderFront("Attack", { "target": target, "force": false, "forceResponse": forceResponse });
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
		return this.AttackVisibleEntity(ents, true);

	if (this.GetStance().respondStandGround)
		return this.AttackVisibleEntity(ents, true);

	if (this.GetStance().respondHoldGround)
		return this.AttackEntityInZone(ents, true);

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
		if (cmpUnitAI && cmpAttack)
		{
			for each (var targetType in cmpAttack.GetAttackTypes())
				if (cmpUnitAI.CheckTargetAttackRange(this.isGuardOf, targetType))
					return false;
		}
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
	// TODO: use special stances instead?
	var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	if (cmpPack)
		return false;

	if (this.GetStance().respondChase)
		return true;

	// If we are guarding/escorting, chase at least as long as the guarded unit is in target range of the attacker
	if (this.isGuardOf)
	{
		var cmpUnitAI =  Engine.QueryInterface(target, IID_UnitAI);
		var cmpAttack = Engine.QueryInterface(target, IID_Attack);
		if (cmpUnitAI && cmpAttack)
		{
			for each (var type in cmpAttack.GetAttackTypes())
				if (cmpUnitAI.CheckTargetAttackRange(this.isGuardOf, type))
					return true;
		}
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
		UnitFsm.ProcessMessage(this, { "type": "FormationLeave" });
};

UnitAI.prototype.GetFormationController = function()
{
	return this.formationController;
};

UnitAI.prototype.SetLastFormationTemplate = function(template)
{
	this.lastFormationTemplate = template;
};

UnitAI.prototype.GetLastFormationTemplate = function()
{
	return this.lastFormationTemplate;
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
	for (var i = 0; i < targetPositions.length; i++)
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
		this.ReplaceOrder(type, data);
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
	if (this.isGuardOf)
	{
		var cmpGuard = Engine.QueryInterface(this.isGuardOf, IID_Guard);
		if (cmpGuard)
			cmpGuard.RemoveGuard(this.entity);
		this.guardRange = undefined;
		this.isGuardOf = undefined;
	}

	if (!this.order)
		return;

	if (this.order.type == "Guard")
		UnitFsm.ProcessMessage(this, {"type": "RemoveGuard"});
	else
		for (var i = 1; i < this.orderQueue.length; ++i)
			if (this.orderQueue[i].type == "Guard")
				this.orderQueue.splice(i, 1);
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

	return (this.template.CanGuard == "true");
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
	this.AddOrder("Stop", undefined, queued);
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
 */
UnitAI.prototype.WalkAndFight = function(x, z, queued)
{
	this.AddOrder("WalkAndFight", { "x": x, "z": z, "force": true }, queued);
};

/**
 * Adds leave foundation order to queue, treated as forced.
 */
UnitAI.prototype.LeaveFoundation = function(target)
{
	// If we're already being told to leave a foundation, then
	// ignore this new request so we don't end up being too indecisive
	// to ever actually move anywhere
	if (this.order && this.order.type == "LeaveFoundation")
		return;

	this.PushOrderFront("LeaveFoundation", { "target": target, "force": true });
};

/**
 * Adds attack order to the queue, forced by the player.
 */
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
	{
		this.AddOrder("Ungarrison", null, false);
	}
};

/**
 * Adds autogarrison order to the queue (only used by ProductionQueue for auto-garrisoning
 * and Promotion when promoting already garrisoned entities).
 */
UnitAI.prototype.Autogarrison = function()
{
	this.AddOrder("Autogarrison", null, false);
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
	var cmpResourceSupply = Engine.QueryInterface(target, IID_ResourceSupply);
	var type = cmpResourceSupply.GetType();

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

	this.AddOrder("GatherNearPosition", { "type": type, "template": template, "x": x, "z": z, "force": false }, queued);
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

	var marketsChanged = this.SetTargetMarket(target, source);
	if (marketsChanged)
	{
		var cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
		if (cmpTrader.HasBothMarkets())
		{
			var data = { "firstMarket": cmpTrader.GetFirstMarket(), "secondMarket": cmpTrader.GetSecondMarket(), "route": route, "force": false };

			if (this.expectedRoute)
			{
				if (!route && this.expectedRoute.length)
					data.route = this.expectedRoute.slice();
				this.expectedRoute = undefined;
			}

			if (this.IsFormationController())
			{
				this.CallMemberFunction("AddOrder", ["Trade", data, queued]);
				var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
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
	}
};

UnitAI.prototype.SetTargetMarket = function(target, source)
{
	var  cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
	if (!cmpTrader)
		return false;
	var marketsChanged = cmpTrader.SetTargetMarket(target, source);

	if (this.IsFormationController())
		this.CallMemberFunction("SetTargetMarket", [target, source]);

	return marketsChanged;
};

UnitAI.prototype.MoveToMarket = function(targetMarket)
{
	if (this.waypoints && this.waypoints.length > 1)
	{
		var point = this.waypoints.pop();
		var ok = this.MoveToPoint(point.x, point.z);
		if (!ok)
			ok = this.MoveToMarket(targetMarket);
	}
	else
	{
		this.waypoints = undefined;
		var ok = this.MoveToTarget(targetMarket);
	}

	return ok;
};

UnitAI.prototype.PerformTradeAndMoveToNextMarket = function(currentMarket, nextMarket, nextFsmStateName)
{
	if (!this.CanTrade(currentMarket))
	{
		this.StopTrading();
		return;
	}

	if (this.CheckTargetRange(currentMarket, IID_Trader))
	{
		var cmpTrader = Engine.QueryInterface(this.entity, IID_Trader);
		cmpTrader.PerformTrade(currentMarket);
		if (!cmpTrader.GetGain().traderGain)
		{
			this.StopTrading();
			return;
		}

		if (this.order.data.route && this.order.data.route.length)
		{
			this.waypoints = this.order.data.route.slice();
			if (nextFsmStateName == "APPROACHINGSECONDMARKET")
				this.waypoints.reverse();
			this.waypoints.unshift(null);  // additionnal dummy point for the market
		}

		if (this.MoveToMarket(nextMarket))	// We've started walking to the next market
			this.SetNextState(nextFsmStateName);
		else
			this.StopTrading();
	}
	else
	{
		if (!this.MoveToMarket(currentMarket))	// If the current market is not reached try again
			this.StopTrading();
	}
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

	// Reset the range queries, since the range depends on stance.
	this.SetupRangeQueries();
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

	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (this.AttackEntitiesByPreference( rangeMan.ResetActiveQuery(this.losRangeQuery) ))
		return true;

	return false;
};

UnitAI.prototype.FindWalkAndFightTargets = function()
{
	if (this.IsFormationController())
	{
		var cmpUnitAI;
		var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
		for each (var ent in cmpFormation.members)
		{
			if (!(cmpUnitAI =  Engine.QueryInterface(ent, IID_UnitAI)))
				continue;
			var targets = cmpUnitAI.GetTargetsFromUnit();
			for each (var targ in targets)
			{
				if (cmpUnitAI.CanAttack(targ))
				{
					this.PushOrderFront("Attack", { "target": targ, "force": true });
					return true;
				}
			}
		}
		return false;
	}

	var targets = this.GetTargetsFromUnit();
	for each (var targ in targets)
	{
		if (this.CanAttack(targ))
		{
			this.PushOrderFront("Attack", { "target": targ, "force": true });
			return true;
		}
	}
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

	const attackfilter = function(e) {
		var cmpOwnership = Engine.QueryInterface(e, IID_Ownership);
		if (cmpOwnership && cmpOwnership.GetOwner() > 0)
			return true;
		var cmpUnitAI = Engine.QueryInterface(e, IID_UnitAI);
		return cmpUnitAI && (!cmpUnitAI.IsAnimal() || cmpUnitAI.IsDangerousAnimal());
	};

	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var entities = rangeMan.ResetActiveQuery(this.losRangeQuery);
	var targets = entities.filter(function (v) { return cmpAttack.CanAttack(v) && attackfilter(v); })
		.sort(function (a, b) { return cmpAttack.CompareEntitiesByPreference(a, b); });

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

UnitAI.prototype.CanAttack = function(target, forceResponse)
{
	// Formation controllers should always respond to commands
	// (then the individual units can make up their own minds)
	if (this.IsFormationController())
		return true;

	// Verify that we're able to respond to Attack commands
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return false;

	if (!cmpAttack.CanAttack(target))
		return false;

	// Verify that the target is alive
	if (!this.TargetIsAlive(target))
		return false;

	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
		return false;

	// Verify that the target is an attackable resource supply like a domestic animal
	// or that it isn't owned by an ally of this entity's player or is responding to
	// an attack.
	var owner = cmpOwnership.GetOwner();
	if (!this.MustKillGatherTarget(target)
	    && !(IsOwnedByEnemyOfPlayer(owner, target)
	         || IsOwnedByNeutralOfPlayer(owner, target)
	         || (forceResponse && !IsOwnedByPlayer(owner, target))))
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
	// The target must be a valid resource supply.
	var cmpResourceSupply = Engine.QueryInterface(target, IID_ResourceSupply);
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

	for (var unhealableClass of cmpHeal.GetUnhealableClasses())
		if (cmpIdentity.HasClass(unhealableClass))
			return false;

	// Verify that the target is a healable class
	for (var healableClass of cmpHeal.GetHealableClasses())
		if (cmpIdentity.HasClass(healableClass))
			return true;

	return false;
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
};

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

UnitAI.prototype.CanPack = function()
{
	var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	return (cmpPack && !cmpPack.IsPacking() && !cmpPack.IsPacked());
};

UnitAI.prototype.CanUnpack = function()
{
	var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	return (cmpPack && !cmpPack.IsPacking() && cmpPack.IsPacked());
};

UnitAI.prototype.IsPacking = function()
{
	var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	return (cmpPack && cmpPack.IsPacking());
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

UnitAI.prototype.SetFacePointAfterMove = function(val)
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (cmpMotion)
		cmpMotion.SetFacePointAfterMove(val);
};

UnitAI.prototype.AttackEntitiesByPreference = function(ents)
{
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);

	if (!cmpAttack)
		return false;

	const attackfilter = function(e) {
		var cmpOwnership = Engine.QueryInterface(e, IID_Ownership);
		if (cmpOwnership && cmpOwnership.GetOwner() > 0)
			return true;
		var cmpUnitAI = Engine.QueryInterface(e, IID_UnitAI);
		return cmpUnitAI && (!cmpUnitAI.IsAnimal() || cmpUnitAI.IsDangerousAnimal());
	};

	return this.RespondToTargetedEntities(
		ents.filter(function (v) { return cmpAttack.CanAttack(v) && attackfilter(v); })
		.sort(function (a, b) { return cmpAttack.CompareEntitiesByPreference(a, b); })
	);
};

/**
 * Call obj.funcname(args) on UnitAI components of all formation members.
 */
UnitAI.prototype.CallMemberFunction = function(funcname, args)
{
	var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
	if (!cmpFormation)
		return;
	var members = cmpFormation.GetMembers();
	for each (var ent in members)
	{
		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		cmpUnitAI[funcname].apply(cmpUnitAI, args);
	}
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
	var members = cmpFormation.GetMembers();
	for each (var ent in members)
	{
		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		if (!cmpUnitAI[funcname].apply(cmpUnitAI, args))
			return false;
	}
	return true;
};

Engine.RegisterComponentType(IID_UnitAI, "UnitAI", UnitAI);
