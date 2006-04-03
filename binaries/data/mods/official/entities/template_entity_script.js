/*
	DESCRIPTION	: Functions that define the scripted behaviour of properties, particularly the effects of entity events and their properties when initialised.
	NOTES		: 
*/

// ====================================================================

// To add a new generic order, do the following all within template_entity_script.js:

//    * Pick a number to be its ID (add this to the "const"s directly below). 

//    * Add code in the entityInit() function below that will call setActionParams to set the action's range, speed and animation if the entity supports this action. For example this.setActionParams( ACTION_GATHER, 0.0, a.range, a.speed, "gather" ) tells the entity that the action with ID of ACTION_GATHER has min range 0, max range a.range, speed a.speed, and should play the animation called "gather" while active. 

//    * Add code in entityEventTargetChanged() to tell the GUI whether the entity should use this action depending on what target the mouse is hovering over. This is also where you can set a cursor for the action. 

//    * Add code in entityEventGeneric() to deal with new generic order events of your type. Note that if you want to have your action handler in a separate function (is preferable), you need to also add this function to the entity object in entityInit() (its initialize event), e.g. this.processGather = entityEventGather. 

const ACTION_NONE	= 0;
const ACTION_ATTACK	= 1;
const ACTION_GATHER	= 2;
const ACTION_HEAL	= 3;
const ACTION_ATTACK_RANGED = 4;
const ACTION_BUILD = 5;

// ====================================================================

function entityInit()
{
	// Initialise an entity when it is first spawned (generate starting hitpoints, etc).

	this.addCreateQueue = entityAddCreateQueue;
	
	// If this is a foundation, initialize traits from the building we're converting into
	if( this.building && this.building != "" )
	{
		var building = getEntityTemplate( this.building );
		this.traits.id.generic = building.traits.id.generic;
		this.traits.id.specific = building.traits.id.specific;
		this.traits.id.civ = building.traits.id.civ;
		this.traits.id.icon_cell = building.traits.id.icon_cell;
		this.traits.health.max = building.traits.health.max;
		this.build_points = new Object();
		this.build_points.curr = 0.0;
		this.build_points.max = parseFloat( building.traits.creation.time );
	}
	
	// Generate civ code (1st four characters of civ name, in lower case eg "Carthaginians" => "cart").
	if (this.traits.id && this.traits.id.civ)
	{
		this.traits.id.civ_code = this.traits.id.civ.toString().substring (0, 4);
		this.traits.id.civ_code = this.traits.id.civ_code.toString().toLowerCase();

		// Exception to make the Romans into rome.
		if (this.traits.id.civ_code == "roma") this.traits.id.civ_code = "rome";
		// Exception to make the Hellenes into hele.
		if (this.traits.id.civ_code == "hell") this.traits.id.civ_code = "hele";
	}

	// If entity can contain garrisoned units, empty it.
	if ( this.traits.garrison && this.traits.garrison.max )
		this.traits.garrison.curr = 0;

	// If entity has health, set current to same, unless it's a foundation, in which case we set it to 0.
	if ( this.traits.health && this.traits.health.max  )
		this.traits.health.curr = ( this.building!="" ? 0.0 : this.traits.health.max );

	// If entity has stamina, set current to same.
	if ( this.traits.stamina && this.traits.stamina.max )
		this.traits.stamina.curr = this.traits.stamina.max;

	if (this.traits.supply)
	{
		// If entity has supply, set current to same.
		if (this.traits.supply.max)
			this.traits.supply.curr = this.traits.supply.max;

		// If entity has type of supply and no subtype, set subtype to same
		// (so we don't have to say type="wood", subtype="wood"
		if (this.traits.supply.type && !this.traits.supply.subtype)
			this.traits.supply.subtype = this.traits.supply.type;
			
		// The "dropsitecount" array holds the number of units with gather aura in range of the object;
		// this is important so that if you have two mills near something and one of them is destroyed,
		// you can still gather from the thing. Initialize it to 0 (ungatherable) for every player unless
		// the entity is forageable (e.g. for huntable animals).
		this.traits.supply.dropsitecount = new Array();
		initialCount = this.traits.supply.subtype.meat ? 1 : 0;
		for( i=0; i<=8; i++ )
		{
			this.traits.supply.dropsitecount[i] = initialCount;
		}
	}

	if (!this.traits.promotion)
		this.traits.promotion = new Object();
		
	// If entity becomes another entity after it gains enough experience points, set up secondary attributes.
	if (this.traits.promotion.req)
	{
		// Get the name of the entity. 
		entityName = this.tag.toString();
	
		// Determine whether current is basic, advanced or elite, and set rank to suit.
		switch (entityName.substring (entityName.length-2, entityName.length))
		{
			case "_b":
				// Basic. Upgrades to Advanced.
				this.traits.promotion.rank = "1";
				nextSuffix = "_a";
				// Set rank image to put over entity's head.
				this.traits.rank.name = "";
			break;
			case "_a":
				// Advanced. Upgrades to Elite.
				this.traits.promotion.rank = "2";
				nextSuffix = "_e";
				// Set rank image to put over entity's head.
				this.traits.rank.name = "advanced.dds";				
			break;
			case "_e":
				// Elite. Maximum rank.
				this.traits.promotion.rank = "3";
				nextSuffix = "";
				// Set rank image to put over entity's head.
				this.traits.rank.name = "elite.dds";				
			break;
			default:
				// Does not gain promotions.
				this.traits.promotion.rank = "0"
				nextSuffix = ""
			break;
		}
		
		// Give the entity an initial value of 0 earned XP at startup if a default value is not specified.
		if (!this.traits.promotion.curr)
			this.traits.promotion.curr = 0
	
		// The entity it should become (unless specified otherwise) is the base entity plus promotion suffix.
		if (!this.traits.promotion.newentity && nextSuffix != "" && this.traits.promotion.rank != "0")
			this.traits.promotion.newentity = entityName.substring (0, entityName.length-2) + nextSuffix

		// If entity is an additional rank and the correct actor has not been specified
		// (it's just inherited the Basic), point it to the correct suffix. (Saves us specifying it each time.)
		actorStr = this.actor.toString();
		if (this.traits.promotion.rank > "1"
			&& actorStr.substring (actorStr.length-5, actorStr.length) != nextSuffix + ".xml")
			this.actor = actorStr.substring (1,actorStr.length-5) + nextSuffix + ".xml";
	}
	else
	{
		this.traits.promotion.rank = "0";
	}
	
	// Register our actions with the generic order system
	if( this.actions )
	{
		if( this.actions.attack && this.actions.attack.melee )
		{
			a = this.actions.attack.melee;
			minRange = ( a.minRange ? a.minRange : 0.0 );
			this.setActionParams( ACTION_ATTACK, minRange, a.range, a.speed, "melee" );
		}
		if( this.actions.gather )
		{
			a = this.actions.gather;
			this.setActionParams( ACTION_GATHER, 0.0, a.range, a.speed, "gather" );
		}
		if( this.actions.heal )
		{
			a = this.actions.heal;
			this.setActionParams( ACTION_HEAL, 0.0, a.range, a.speed, "heal" );
		}
		if( this.actions.attack && this.actions.attack.ranged )
		{
			a = this.actions.attack.ranged;
			minRange = ( a.minRange ? a.minRange : 0.0 );
			// this animation should actually be "ranged" except the current actors still have it called "melee"
			this.setActionParams( ACTION_ATTACK_RANGED, minRange, a.range, a.speed, "melee" );
		}
		if( this.actions.build )
		{
			this.setActionParams( ACTION_BUILD, 0.0, 2.0, this.actions.build.speed, "build" );
		}
	}
	
	// Attach functions to ourselves
	
	this.performAttack = performAttack;
	this.performAttackRanged = performAttackRanged;
	this.performGather = performGather;
	this.performHeal = performHeal;
	this.performBuild = performBuild;
	this.damage = damage;
	this.entityComplete = entityComplete;
	this.GotoInRange = GotoInRange;
	this.attachAuras = attachAuras;

	this.attachAuras();
	
	if ( !this.traits.id )
	{
		this.traits.id = new Object();
	}
	
	// If the entity either costs population or adds to it,
	if (this.traits.population)
	{
		// If the entity increases the population limit (provides Housing),
		if (this.traits.population.add)
			getGUIGlobal().giveResources ("Housing", this.traits.population.add);
		// If the entity occupies population slots (occupies Housing),
		if (this.traits.population.rem)
			getGUIGlobal().giveResources ("Population", this.traits.population.rem);
	}
	
	// Build Unit AI Stance list, and set default stance.
	if (this.actions && this.actions.move)
	{
		if ( !this.traits.ai )
			this.traits.ai = new Object();
		if ( !this.traits.ai.stance )
			this.traits.ai.stance = new Object();
		if ( !this.traits.ai.stance.list )
			this.traits.ai.stance.list = new Object();
		// Create standard stances that all units have.
		this.traits.ai.stance.list.avoid = new Object();
		this.traits.ai.stance.list.hold = new Object();	
		if ( this.actions && this.actions.attack )
		{
			// Create stances that units only have if they can attack.
			this.traits.ai.stance.list.aggress = new Object();	
			this.traits.ai.stance.list.defend = new Object();
			this.traits.ai.stance.list.stand = new Object();
			// Set default stance for combat units.
			this.traits.ai.stance.curr = "Defend";
		}
		else
		{
			// Set default stance for non-combat units.
			this.traits.ai.stance.curr = "Avoid";
		}
	}
	
/*	
	// Generate entity's personal name (if it needs one).
	if (this.traits.id.personal)
	{
		// todo

		// Look in the appropriate array for a random string.

		switch (this.traits.id.classes[])
		{
			case "Male":
				this.traits.id.personal.table1 = "Male"
			break;
			case "Female":
				this.traits.id.personal.table1 = "Female"
			break;
			default:
				this.traits.id.personal.table1 = "Gen"
			break;	
		}
		
		
		this.traits.id.personal.name = getRandom (this.traits.id.civ_code + this.traits.id.personal.table1 + "1")
		this.traits.id.personal.name = this.traits.id.personal.name + " " + getRandom (this.traits.id.civ_code + this.traits.id.personal.table1 + "2")
	}
	else
		this.traits.id.personal.name = "";
*/
	// Log creation of entity to console.
	//if (this.traits.id.personal && this.traits.id.personal.name)
	//	console.write ("A new " + this.traits.id.specific + " (" + this.traits.id.generic + ") called " + this.traits.id.personal.name + " has entered your dungeon.")
	//else	
	//	console.write ("A new " + this.traits.id.specific + " (" + this.traits.id.generic + ") has entered your dungeon.")
}

// ====================================================================

// Attach any auras the entity is entitled to. This was moved to a separate function so that buildings can have their auras 
// attached to them only when they finish construction.
function attachAuras() 
{
	if( this.traits.auras )
	{
		if( this.traits.auras.courage )
		{
			a = this.traits.auras.courage;
			this.addAura ( "courage", a.radius, new DamageModifyAura( this, true, a.bonus ) );
		}
		if( this.traits.auras.fear ) 
		{
			a = this.traits.auras.fear;
			this.addAura ( "fear", a.radius, new DamageModifyAura( this, false, -a.bonus ) );
		}
		if( this.traits.auras.infidelity ) 
		{
			a = this.traits.auras.infidelity;
			this.addAura ( "infidelity", a.radius, new InfidelityAura( this, a.time ) );
		}
		if( this.traits.auras.dropsite ) 
		{
			a = this.traits.auras.dropsite;
			this.addAura ( "dropsite", a.radius, new DropsiteAura( this, a.types ) );
		}
	}		
}

// ====================================================================

function foundationDestroyed( evt )
{
	if( this.building != "" )	// Check that we're *really* a foundation since the event handler is kept when we change templates (probably a bug)
	{
		//console.write( "Hari Seldon made a small calculation error." );
		
		var bp = this.build_points;
		var fractionToReturn = (bp.max - bp.curr) / bp.max;
		
		var resources = getEntityTemplate( this.building ).traits.creation.resource;
		for( r in resources )
		{
			amount = parseInt( fractionToReturn * parseInt(resources[r]) );
			getGUIGlobal().giveResources( r.toString(), amount );
		}
	}
}

// ====================================================================

function performAttack( evt )
{
	this.last_combat_time = getGameTime();
	
	var curr_hit = getGUIGlobal().newRandomSound("voice", "hit", this.traits.audio.path);
	curr_hit.play();

	// Attack logic.
	var dmg = new DamageType();
	
	if ( this.getRunState() )
	{
		dmg.crush = parseInt(this.actions.attack.charge.damage * this.actions.attack.charge.crush);
		dmg.hack = parseInt(this.actions.attack.charge.damage * this.actions.attack.charge.hack);
		dmg.pierce = parseInt(this.actions.attack.charge.damage * this.actions.attack.charge.pierce);
	}
	else
	{
		dmg.crush = parseInt(this.actions.attack.melee.damage * this.actions.attack.melee.crush);
		dmg.hack = parseInt(this.actions.attack.melee.damage * this.actions.attack.melee.hack);
		dmg.pierce = parseInt(this.actions.attack.melee.damage * this.actions.attack.melee.pierce);
	}

	evt.target.damage( dmg, this );
}

// ====================================================================

function performAttackRanged( evt )
{
	this.last_combat_time = getGameTime();

	// Create a projectile from us, to the target, that will do some damage when it hits them.
	dmg = new DamageType();
	dmg.crush = parseInt(this.actions.attack.ranged.damage * this.actions.attack.ranged.crush);
	dmg.hack = parseInt(this.actions.attack.ranged.damage * this.actions.attack.ranged.hack);
	dmg.pierce = parseInt(this.actions.attack.ranged.damage * this.actions.attack.ranged.pierce);
	
	// The parameters for Projectile are:
	// 1 - The actor to use as the projectile. There are two ways of specifying this:
	//     the first is by giving an entity. The projectile's actor is found by looking
	//     in the actor of that entity. This way is usual, and preferred - visual
	//     information, like the projectile model, should be stored in the actor files.
	//     The second way is to give a actor/file name string (e.g. "props/weapon/weap_
	//     arrow_front.xml"). This is only meant to be used for 'Acts of Gaia' where
	//     there is no originating entity. Right now, this entity is the one doing the
	//     firing, so pass this.
	// 2 - Where the projectile is coming from. This can be an entity or a Vector3D.
	//     For now, that's also us.
	// 3 - Where the projectile is going to. Again, either a vector (attack ground?) 
	//     or an entity. Let's shoot at our target, lest we get people terribly confused.
	// 4 - How fast the projectile should go. To keep things clear, we'll set it to 
	//     just a bit faster than the average cavalry.
	// 5 - Who fired it? Erm... yep, us again.
	// 6 - The function you'd like to call when it hits an entity.
	// There's also a seventh parameter, for a function to call when it misses (more
	//  accurately, when it hits the floor). At the moment, however, the default
	//  action (do nothing) is what we want.
	// Parameters 5, 6, and 7 are all optional.
	
	projectile = new Projectile( this, this, evt.target, 12.0, this, projectileEventImpact )
	
	// We'll attach the damage information to the projectile, just to show you can
	// do that like you can with most other objects. Could also do this by making
	// the function we pass a closure.
	
	projectile.damage = dmg;
	
	// Finally, tell the engine not to send this event to anywhere else - 
	// in particular, this shouldn't get to the melee event handler, above.
	
	evt.stopPropagation();
	
	//console.write( "Fire!" );
}

// ====================================================================

function projectileEventImpact( evt )
{
	//console.write( "Hit!" );
	evt.impacted.damage( this.damage, evt.originator );
	
	// Just so you know - there's no guarantee that evt.impacted is the thing you were
	// aiming at. This function gets called when the projectile hits *anything*.
	// For example:
	
	if( evt.impacted.player == evt.originator.player )
		console.write( "Friendly fire!" );
		
	// The three properties of the ProjectileImpact event are:
	// - impacted, the thing it hit
	// - originator, the thing that fired it (the fifth parameter of Projectile's
	//   constructor) - may be null
	// - position, the position the arrow was in the world when it hit.
	
	// The properties of the ProjectileMiss event (the one that gets sent to the
	// handler that was the seventh parameter of the constructor) are similar,
	// but it doesn't have 'impacted' - for obvious reasons.
}

// ====================================================================

function performGather( evt )
{
	g = this.actions.gather;
	s = evt.target.traits.supply;
	
	if( !s.dropsitecount[this.player.id] )
	{
		// Entity has become ungatherable for us, probably meaning our mill near it was killed; cancel order
		evt.preventDefault();
		return;
	}

	if( g.resource[s.type][s.subtype])
		gather_amt = parseInt( g.resource[s.type][s.subtype] );
	else
		gather_amt = parseInt( g.resource[s.type] );

	if( s.max > 0 )
	{
		if( s.curr <= gather_amt )
		{
			gather_amt = s.curr;
		}

		// Remove amount from target.
		s.curr -= gather_amt;
		// Add extracted resources to player's resource pool.
		getGUIGlobal().giveResources(s.type.toString(), parseInt(gather_amt));
		
		// Kill the target if it's now out of resources
		if( s.curr == 0 )
		{
			evt.target.kill();
		}
	}
}

// ====================================================================

function performHeal( evt )
{
	if ( evt.target.player != this.player )
	{
		console.write( "You have a traitor!" );
		return;
	}

	// Cycle through all resources.
	pool = this.player.resource;
	for( resource in pool )
	{
		switch( resource.toString() )
		{
			case "Population" || "Housing":
			break;
			default:
				// Make sure we have enough resources.
				if ( pool[resource] - evt.target.actions.heal.cost * evt.target.traits.creation.cost[resource] < 0 )
				{
					console.write( "Not enough " + resource.toString() + " for healing." );
					return;
				}
			break;
		}
	}

	evt.target.traits.health.curr += this.actions.heal.speed;
	console.write( this.traits.id.specific + " has performed a miracle!" );
	
	if (evt.target.traits.health.curr >= evt.target.traits.health.max)
	{		
		evt.target.traits.health.curr = evt.target.traits.health.max;
	}

	// Cycle through all resources.
	pool = this.player.resource;
	for( resource in pool )
	{
		switch( resource.toString() )
		{
			case "Population":
			case "Housing":
				break;
			default:
				// Deduct resources to pay for healing.
				getGUIGlobal().deductResources(toTitleCase (resource.toString()), parseInt(evt.target.actions.heal.cost * evt.target.traits.creation.cost[resource]));
				break;
		}
	}
}

// ====================================================================

function performBuild( evt )
{
	var t = evt.target;
	var b = this.actions.build;
	var bp = t.build_points;
	var hp = t.traits.health;
	
	var points = parseFloat( b.rate ) * parseFloat( b.speed ) / 1000.0;
	bp.curr += points;
	hp.curr = Math.min( hp.max, hp.curr + (points/bp.max)*hp.max );
	
	if( bp.curr >= bp.max )
	{
		if( t.building != "" )	// Might be false if another unit finished building the thing during our last anim cycle
		{
			t.template = getEntityTemplate( t.building );
			t.building = "";
			t.attachAuras();
		}
		evt.preventDefault();	// Stop performing this action
	}
}

// ====================================================================

function damage( dmg, inflictor )
{	
	if(!this.traits.armour) return;		// corpses have no armour, everything else should
	
	this.last_combat_time = getGameTime();

	// Apply armour and work out how much damage we actually take
	crushDamage = parseInt(dmg.crush - this.traits.armour.value * this.traits.armour.crush);
	if ( crushDamage < 0 ) crushDamage = 0;
	pierceDamage = parseInt(dmg.pierce - this.traits.armour.value * this.traits.armour.pierce);
	if ( pierceDamage < 0 ) pierceDamage = 0;
	hackDamage = parseInt(dmg.hack - this.traits.armour.value * this.traits.armour.hack);
	if ( hackDamage < 0 ) hackDamage = 0;

	totalDamage = parseInt(dmg.typeless + crushDamage + pierceDamage + hackDamage);

	// Minimum of 1 damage

	if( totalDamage < 1 ) totalDamage = 1;

	this.traits.health.curr -= totalDamage;

	if( this.traits.health.curr <= 0 )
	{
		// If the fallen is worth any loot and the inflictor is capable of looting
		if (this.traits.loot && inflictor.actions && inflictor.actions.loot)
		{
			// Cycle through all loot on this entry.
			for( loot in this.traits.loot )
			{
				switch( loot.toString().toUpperCase() )
				{
					case "XP":
						// If the inflictor gains promotions, and he's capable of earning more ranks,
						if (inflictor.traits.promotion && inflictor.traits.promotion.curr && inflictor.traits.promotion.req
								&& inflictor.traits.promotion.newentity && inflictor.traits.promotion.newentity != ""
								&& this.traits.loot && this.traits.loot.xp && inflictor.actions.loot.xp)
						{
							// Give him the fallen's upgrade points (if he has any).
							if (this.traits.loot.xp)
								inflictor.traits.promotion.curr = parseInt(inflictor.traits.promotion.curr) + parseInt(this.traits.loot.xp);

							// Notify player.
							if (inflictor.traits.id.specific)
								console.write(inflictor.traits.id.specific + " has earned " + this.traits.loot.xp + " upgrade points!");
							else
								console.write("One of your units has earned " + this.traits.loot.xp + " upgrade points!");

							// If he now has maximum upgrade points for his rank,
							if (inflictor.traits.promotion.curr >= inflictor.traits.promotion.req)
							{
								// Notify the player.
								if (inflictor.traits.id.specific)
									console.write(inflictor.traits.id.specific + " has gained a promotion!");
								else
									console.write("One of your units has gained a promotion!");
								
								// Reset his upgrade points.
								inflictor.traits.promotion.curr = 0; 

								// Upgrade his portrait to the next level.
								inflictor.traits.id.icon_cell++; 

								// Transmogrify him into his next rank.
								inflictor.template = getEntityTemplate(inflictor.traits.promotion.newentity);
							}
						}
						break;
					default:
						if ( inflictor.actions.loot.resources )
						{
							// Notify player.
							console.write ("Spoils of war! " + this.traits.loot[loot] + " " + loot.toString() + "!");
							// Give the inflictor his resources.
							getGUIGlobal().giveResources( loot.toString(), parseInt(this.traits.loot[loot]) );
						}
						break;
				}
			}
		}

		// Notify player.
		if ( inflictor )
			console.write( this.traits.id.generic + " got the point of " + inflictor.traits.id.generic + "'s weapon." );
		else
			console.write( this.traits.id.generic + " died in mysterious circumstances." );

		// Make him cry out in pain.
		if (this.traits.audio && this.traits.audio.path)
		{
			curr_pain = getGUIGlobal().newRandomSound("voice", "pain", this.traits.audio.path);
			if (curr_pain) curr_pain.play();
		}
		else
		{
			console.write ("Sorry, no death sound for this unit; you'll just have to imagine it ...");
		}

		// We've taken what we need. Kill the swine.
		console.write("Kill!!");
		this.kill();
	}
	else if( inflictor && this.actions && this.actions.attack )
	{
		// If we're not already doing something else, take a measured response - hit 'em back.
		// You know, I think this is quite possibly the first AI code the AI divlead has written
		// for 0 A.D....
		if( this.isIdle() )
			this.order( ORDER_GENERIC, inflictor, getAttackAction( this, inflictor ) );
	}
}

// ====================================================================

function entityEventGeneric( evt )
{
	switch( evt.action )
	{
		case ACTION_ATTACK:
			this.performAttack( evt ); break;
		case ACTION_GATHER:
			if ( !this.actions.gather )
				evt.preventDefault();
			evt.notifyType = NOTIFY_GATHER;

			// Change our gather animation based on the type of target
			var a = this.actions.gather;
			this.setActionParams( ACTION_GATHER, 0.0, a.range, a.speed,
				"gather_" + evt.target.traits.supply.subtype );
			
			break;
		case ACTION_HEAL:
			this.performHeal( evt ); break;
		case ACTION_ATTACK_RANGED:
			this.performAttackRanged( evt ); break;
		case ACTION_BUILD:
			this.performBuild( evt ); break;

		default:
			console.write( "Unknown generic action: " + evt.action );
	}
}

//======================================================================

function entityEventNotification( evt )
{
	//Add "true" to the end of order() to indicate that this is a notification order.
	switch( evt.notifyType )
	{
		
		case NOTIFY_GOTO:
			this.GotoInRange( evt.location.x, evt.location.z, false);
			break;
		case NOTIFY_RUN:
			this.GotoInRange( evt.location.x, evt.location.z, true );
			break;
		case NOTIFY_ATTACK:
		case NOTIFY_DAMAGE:
			this.order( ORDER_GENERIC, evt.target, ACTION_ATTACK, true );
			break;
		case NOTIFY_HEAL:
			this.order( ORDER_GENERIC, evt.target, ACTION_HEAL, true );
			break;
		case NOTIFY_GATHER:
			this.order( ORDER_GENERIC, evt.target, ACTION_GATHER, true );
			break;
		default:
			console.write( "Unknown notification request " + evt.notifyType );
			break;
	}
}		

// ====================================================================

function getAttackAction( source, target )
{
	if (!source.actions.attack)
		return ACTION_NONE;
	attack = source.actions.attack;
	if ( attack.melee )
		return ACTION_ATTACK;
	else if ( attack.ranged )
		return ACTION_ATTACK_RANGED;
	else
		return ACTION_NONE;
}

// ====================================================================

// TODO: Change this to an event so that it gets passed to our parent too, like other events
function entityComplete()
{
	console.write( this + " is finished building." );
}

// ====================================================================

function entityEventTargetChanged( evt )
{
	// This event lets us know when the user moves his/her cursor to a different unit (provided this
	// unit is selected) - use it to tell the engine what context cursor should be displayed, given
	// the target.

	// If we can gather, and the target supplies, gather. If it's our enemy, and we're armed, attack. 
	// If all else fails, move (or run on a right-click).
	
	
	evt.defaultOrder = NMT_Goto;
	evt.defaultCursor = "arrow-default";
	evt.defaultAction = ACTION_NONE;
	evt.secondaryAction = ACTION_NONE;
	evt.secondaryCursor = "arrow-default";
	if ( this.actions && this.actions.run && this.actions.run.speed > 0 )
	{
		evt.secondaryOrder = NMT_Run;
	}

	if( evt.target && this.actions )
	{
		if( this.actions.attack && 
			evt.target.player != this.player &&
			evt.target.traits.health &&
			evt.target.traits.health.max != 0 )
		{
			evt.defaultOrder = NMT_Generic;
			evt.defaultAction = getAttackAction( this, evt.target );
			evt.defaultCursor = "action-attack";

			evt.secondaryOrder = NMT_Generic;
			evt.secondaryAction = getAttackAction( this, evt.target );
			evt.secondaryCursor = "action-attack";
		}
		
		if ( this.actions.escort &&
			this != evt.target &&
			evt.target.player == this.player &&
			evt.target.actions )
		{
			if (evt.target.actions.move)
			{ 
				//Send an empty order
				evt.defaultOrder = NMT_NotifyRequest;
				evt.secondaryOrder = NMT_NotifyRequest;

				evt.defaultAction = NOTIFY_ESCORT;
				evt.secondaryAction = NOTIFY_ESCORT;
			}
		}

		if ( canGather( this, evt.target ) )
		{
			evt.defaultOrder = NMT_Generic;
			evt.defaultAction = ACTION_GATHER;
		    // Set cursor (eg "action-gather-fruit").
		    evt.defaultCursor = "action-gather-" + evt.target.traits.supply.subtype;

			evt.secondaryOrder = NMT_Generic;
			evt.secondaryAction = ACTION_GATHER;
		  	// Set cursor (eg "action-gather-fruit").
		    evt.secondaryCursor = "action-gather-" + evt.target.traits.supply.subtype;
		}
		
		if ( canBuild( this, evt.target ) )
		{
			evt.defaultOrder = NMT_Generic;
			evt.defaultAction = ACTION_BUILD;
		    evt.defaultCursor = "action-build";

			evt.secondaryOrder = NMT_Generic;
			evt.secondaryAction = ACTION_BUILD;
		    evt.secondaryCursor = "action-build";
		}
	}

		
}

// ====================================================================

function entityEventPrepareOrder( evt )
{
	// This event gives us a chance to veto any order we're given before we execute it.
	// Not sure whether this really belongs here like this: the alternative is to override it in
	// subtypes - then you wouldn't need to check tags, you could hardcode results.

	if ( !this.actions )
	{
		evt.preventDefault();
		return;
	}
	
	//evt.notifySource is the entity order data will be obtained from, so if we're attacking and we 
	//want our listeners to copy us, then we will use our own order as the source.

	switch( evt.orderType )
	{
		case ORDER_GOTO:
			if ( !this.actions.move )
				evt.preventDefault();
			evt.notifyType = NOTIFY_GOTO;
			evt.notifySource = this;
			break;
			
		case ORDER_RUN:
			if ( !this.actions.move.run )	
				evt.preventDefault();
			evt.notifyType = NOTIFY_RUN;
			evt.notifySource = this;
			break;
			
		case ORDER_PATROL:
			if ( !this.actions.patrol )
				evt.preventDefault();
			break;	
			
		case ORDER_GENERIC:
			evt.notifySource = this;
			switch ( evt.action )
			{
				case ACTION_ATTACK:
				case ACTION_ATTACK_RANGED:
					evt.action = getAttackAction( this, evt.target );
					if ( evt.action == ACTION_NONE )
					 	evt.preventDefault();
					evt.notifyType = NOTIFY_ATTACK;
					break;

				case ACTION_GATHER:
					if ( !this.actions.gather )
						evt.preventDefault();
					evt.notifyType = NOTIFY_GATHER;
					break;
					
				case ACTION_HEAL:
					if ( !this.actions.heal )
						evt.preventDefault();
					evt.notifyType = NOTIFY_HEAL;
					break;	
					
				case ACTION_BUILD:
					if ( !this.actions.build )
						evt.preventDefault();
					evt.notifyType = NOTIFY_NONE;
					break;	
			}
			break;
		
		case ORDER_PRODUCE:
			evt.notifyType = NOTIFY_NONE;
			break;
			
		default:
			console.write("Unknown order type " + evt.orderType + "; ignoring.");
			evt.preventDefault();
			break;
	}
}

// ====================================================================

function entityStartProduction( evt )
{
	console.write("StartProduction: " + evt.productionType + " " + evt.name);
	// Set the amount of time it will take to complete production of the production object.
	evt.time = getTemplate(evt.name).traits.creation.time;
}

function entityFinishProduction( evt )
{
	console.write("FinishProduction: " + evt.productionType + " " + evt.name);
}

function entityCancelProduction( evt )
{
	console.write("CancelProduction: " + evt.productionType + " " + evt.name);
}

// ====================================================================

function entityAddCreateQueue( template, tab, list )
{
	// Make sure we have a queue to put things in...
	if( !this.actions.create.queue )
		this.actions.create.queue      = new Array();

	// Construct template object.
	comboTemplate = template;
	comboTemplate.list = list;
	comboTemplate.tab = tab;

	// Append to the end of this queue

	this.actions.create.queue.push( template );

	// If we're not already building something...
	if( !this.actions.create.progress )
	{
		console.write( "Starting work on (unqueued) ", template.tag );
		 // Start the progress timer.
		 // - First parameter is target value (in this case, base build time in seconds)
		 // - Second parameter is increment per millisecond (use build rate modifier and correct for milliseconds)
		 // - Third parameter is the function to call when the timer finishes.
		 // - Fourth parameter is the scope under which to run that function (what the 'this' parameter should be)
		 this.actions.create.progress = new ProgressTimer( template.traits.creation.time, 
			this.actions.create.speed / 1000, entityCreateComplete, this )
	}
}

// ====================================================================

// This is the syntax to add a function (or a property) to absolutely every entity.

Entity.prototype.add_create_queue = entityAddCreateQueue;

// ====================================================================

function entityCreateComplete()
{
	// Get the unit that was at the head of our queue, and remove it.
	// (Oh, for information about all these nifty properties and functions
	//  of the Array object that I use, see the ECMA-262 documentation
	//  at http://www.mozilla.org/js/language/E262-3.pdf. Bit technical but
	//  the sections on 'Native ECMAScript Objects' are quite useful)
	
	var template = this.actions.create.queue.shift();

	// Code to find a free space around an object is tedious and slow, so 
	// I wrote it in C. Takes the template object so it can determine how
	// much space it needs to leave.
	position = this.getSpawnPoint( template );
	
	// The above function returns null if it couldn't find a large enough space.
	if( !position )
	{
		console.write( "Couldn't train unit - not enough space" );
		// Oh well. The player's just lost all the resources and time they put into
		// construction - serves them right for not paying attention to the land
		// around their barracks, doesn't it?
	}
	else
	{
		created = new Entity( template, position );
	
		// Above shouldn't ever fail, but just in case...
		if( created )
		{
			console.write( "Created: ", template.tag );
	
			// Entities start under Gaia control - make the controller
			// the same as our controller
			created.player = this.player;
		}
	}		
	// If there's something else in the build queue...
	if( this.actions.create.queue.length > 0 )
	{
		// Start on the next item.
		template = this.actions.create.queue[0];
		console.write( "Starting work on (queued) ", template.tag );
		this.actions.create.progress = new ProgressTimer( template.traits.creation.time, 
			this.actions.create.speed / 1000, entityCreateComplete, this )
	}
	else
	{
		// Otherwise, delete the timer.
		this.actions.create.progress = null;
	}
}

// ====================================================================

function attemptAddToBuildQueue( entity, create_tag, tab, list )
{
	template = getEntityTemplate( create_tag );
	result = entityCheckQueueReq( entity, template );	

	if (result == true) // If the entry meets requirements to be added to the queue (eg sufficient resources) 
	{
		// Cycle through all costs of this entry.
		pool = template.traits.creation.resource;
		for ( resource in pool )
		{
			switch ( getGUIGlobal().toTitleCase(resource.toString()) )
			{
				case "Population":
				case "Housing":
				break;
				default:
					// Deduct the given quantity of resources.
					getGUIGlobal().deductResources (resource.toString(), parseInt(pool[resource]));

					console.write ("Spent " + pool[resource] + " " + resource + " to purchase " + 
						template.traits.id.generic);
				break;
			}
		}

		// Add entity to queue.
		console.write( "Adding ", create_tag, " to build queue..." );
		entity.addCreateQueue( template, tab, list );
		
		return true;
	}
	else
	{	// If not, output the error message.
		console.write(result);
		return false;
	}
}

// ====================================================================

function entityCheckQueueReq( entity, template ) 
{ 
	// Determines if the given entity meets requirements for production by the player, and returns an appropriate 
	// error string.
	// A return value of 0 equals success -- entry meets requirements for production. 
	
	// Cycle through all resources that this item costs, and check the player can afford the cost.
	resources = template.traits.creation.resource;
	for( resource in resources )
	{
		switch( getGUIGlobal().toTitleCase(resource.toString()) )
		{
			case "Population":
				// If the item costs more of this resource type than we have,
				if (template.traits.population.rem > (localPlayer.resource["Housing"]-localPlayer.resource[resource]))
				{
					// Return an error.
					return ("Insufficient Housing; " + (resources[resource]-localPlayer.resource["Housing"]-localPlayer.resource.valueOf()[resource].toString()) + " required."); 
				}
			break;
			case "Housing": // Ignore housing. It's handled in combination with population.
			break
			default:
				// If the item costs more of this resource type than we have,
				if (resources[resource] > localPlayer.resource[resource])
				{
					// Return an error.
					return ("Insufficient " + resource + "; " + (localPlayer.resource[resource]-resources[resource])*-1 + " required."); 
				}
				else
					console.write("Player has at least " + resources[resource] + " " + resource + ".");
			break;
		}
	}

	// Check if another entity must first exist. 

	// Check if another tech must first be researched. 

	// Check if the limit for this type of entity has been reached. 

	// If we passed all checks, return success. Entity can be queued.
	return true; 
}

// ====================================================================

function canGather( source, target )
{
	// Checks whether we're allowed to gather from a target entity (this involves looking at both the type and subtype).
	if( !source.actions )
		return false;
	g = source.actions.gather;
	s = target.traits.supply;
	return ( g && s && g.resource && g.resource[s.type] &&
		( s.subtype==s.type || g.resource[s.type][s.subtype] ) &&
		( s.curr > 0 || s.max == 0 ) && 
		s.dropsitecount[source.player.id] );
}

// ====================================================================

function canBuild( source, target )
{
	// Checks whether we're allowed to build a target entity
	if( !source.actions )
		return false;
	b = source.actions.build;
	return (b && target.building != "" && target.player.id == source.player.id );
}

// ====================================================================

function DamageType()
{
	this.typeless = 0.0;
	this.crush = 0.0;
	this.pierce = 0.0;
	this.hack = 0.0;
}

// ====================================================================

function DamageModifyAura( source, ally, bonus )
{
	// Defines the effects of the DamageModify Aura. (Adjacent units have modified attack bonus.)
	// The Courage Aura uses this function to give attack bonus to allies.	
	// The Fear Aura uses this function to give attack penalties to enemies.

	this.source = source;
	this.bonus = bonus;
	this.ally = ally;
	
	this.affects = function( e ) 
	{
		if( this.ally )
		{
			return ( e.player.id == this.source.player.id && e.actions && e.actions.attack );
		}
		else
		{
			return ( e.player.id != this.source.player.id && e.actions && e.actions.attack );
		}
	}
	
	this.onEnter = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			//console.write( "DamageModify aura: giving " + this.bonus + " damage to " + e );
			e.actions.attack.damage += this.bonus;
		}
	};
	
	this.onExit = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			//console.write( "DamageModify aura: taking away " + this.bonus + " damage from " + e );
			e.actions.attack.damage -= this.bonus;
		}
	};
}

// ====================================================================

function DropsiteAura( source, types )
{
	// Defines the effects of the Gather aura. Enables resource gathering on entities
	// near the source for it's owner.

	this.source = source;
	this.types = types;
	
	this.affects = function( e ) 
	{
		return( e.traits.supply && this.types[e.traits.supply.type] );
	}
	
	this.onEnter = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			//console.write( "Dropsite aura: adding +1 for " + this.source.player.id + " on " + e );
			e.traits.supply.dropsitecount[this.source.player.id]++;
		}
	};
	
	this.onExit = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			//console.write( "Dropsite aura: adding -1 for " + this.source.player.id + " on " + e );
			e.traits.supply.dropsitecount[this.source.player.id]--;
		}
	};
}

// ====================================================================

function InfidelityAura( source, time )
{
	// Defines the effects of the Infidelity Aura. Changes ownership of entity when only one player's units surrounds them.

	this.source = source;
	
	this.time = time;
	
	this.count = new Array( 9 );
	for( i = 0; i <= 8; i++ )
	{
		this.count[i] = 0;
	}
	
	this.convertTimer = 0;
		
	this.affects = function( e ) 
	{
		return ( e.player.id != 0 && ( !e.traits.auras || !e.traits.auras.infidelity ) );
	}
	
	this.onEnter = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			//console.write( "Infidelity aura: adding +1 count to " + e.player.id );
			this.count[e.player.id]++;
			this.changePlayerIfNeeded();
		}
	};
	
	this.onExit = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			//console.write( "Infidelity aura: adding -1 count to " + e.player.id );
			this.count[e.player.id]--;
			this.changePlayerIfNeeded();
		}
	};
	
	this.changePlayerIfNeeded = function()
	{
		if( this.count[this.source.player.id] == 0 )
		{
			// If our owner has nothing near us but someone else does, start a time to convert over if we haven't done so already
			if( !this.convertTimer )
			{
				for( i = 1; i <= 8; i++ )
				{
					if( this.count[i] > 0 )
					{
						console.write( "Starting convert timer" );
						this.convertTimer = setTimeout( this.convert, parseInt( this.time * 1000 ), this );
						return;
					}
				}
			}
		}
		
		// If we had started a convert timer before, cancel it (either we have units from our owner in range, or there are no units from anyone in range)
		if( this.convertTimer )
		{
			console.write( "Cancelling convert timer" );
			cancelTimer( this.convertTimer );
			this.convertTimer = 0;
		}
	}
	
	this.convert = function()
	{
		console.write( "Conversion time!" );
		
		// Switch ownership to whichever non-gaia player has the most units near us
		bestPlayer = 0;
		bestCount = 0;
		for( i = 1; i <= 8; i++ )
		{
			if( this.count[i] > bestCount )
			{
				bestCount = this.count[i];
				bestPlayer = i;
			}
		}
		if( bestCount > 0 )
		{
			//console.write( "Infidelity aura: changing ownership to " + bestPlayer );
			this.source.player = players[bestPlayer];
		}
		
		this.convertTimer = 0;
	}
}

// ====================================================================
function GotoInRange( x, y, run )
{	
	if ( !this.actions || !this.actions.move )
		return;
	//Add "true" at the end to indicate that this is a notification order.
	if (run && this.actions.move.run)
		this.order( ORDER_RUN, x, y - this.actions.escort.distance, true);
	else
		this.order( ORDER_GOTO, x, y - this.actions.escort.distance, true);
}
function entityEventFormation( evt )
{
	if ( evt.formationEvent == FORMATION_ENTER )
	{
		if ( this.getFormationBonus() && this.isInClass( this.getFormationBonusType() ) )
		{
			eval( this + this.getFormationBonus() ) += eval( this + this.getFormationBonus() ) *		
				 this.getFormationBonusVal();
		}
		if ( this.getFormationPenalty() && this.isInClass( this.getFormationPenaltyType() ) )
		{
			eval( this + this.getFormationPenalty() ) -= eval( this + this.getFormationbonus() ) *
				this.getFormationPenaltyVal();
		}
	}
	//Reverse the bonuses
	else if ( evt.formationEvent == FORMATION_LEAVE )
	{
		if ( this.getFormationBonus() && this.isInClass( this.getFormationBonusType() ) )
		{
			eval( this + this.getFormationBonus() ) -= eval( this + this.getFormationBonus() ) *						 this.getFormationBonusVal();
		}
		if ( this.getFormationPenalty() && this.isInClass( this.getFormationPenaltyType() ) )
		{
			eval( this + this.getFormationPenalty() ) += eval( this + this.getFormationbonus() ) *
				this.getFormationPenaltyVal();
		}
	}	
}