const ACTION_ATTACK = 1;
const ACTION_GATHER = 2;
const ACTION_HEAL = 3;

// ====================================================================

function entityInit()
{
	// Initialise an entity when it is first spawned (generate starting hitpoints, etc).

	this.addCreateQueue = entityAddCreateQueue;
	
	// Generate civ code (1st four characters of civ name, in lower case eg "Carthaginians" => "cart").
	if (this.traits.id && this.traits.id.civ)
	{
		this.traits.id.civ_code = this.traits.id.civ.toString().substring (0, 4)
		this.traits.id.civ_code = this.traits.id.civ_code.toString().toLowerCase()

		// Exception to make the Romans into rome.
		if (this.traits.id.civ_code == "roma") this.traits.id.civ_code = "rome"

		// Exception to make the Hellenes into hele.
		if (this.traits.id.civ_code == "hell") this.traits.id.civ_code = "hele"
	}

	// If entity can contain garrisoned units, empty it.
	if (this.traits.garrison && this.traits.garrison.max)
		this.traits.garrison.curr = 0

	// If entity has health, set current to same.
	if (this.traits.health && this.traits.health.max)
		this.traits.health.curr = this.traits.health.max

	if (this.traits.supply)
	{
		// If entity has supply, set current to same.
		if (this.traits.supply.max)
			this.traits.supply.curr = this.traits.supply.max

		// If entity has type of supply and no subtype, set subtype to same
		// (so we don't have to say type="wood", subtype="wood"
		if (this.traits.supply.type && !this.traits.supply.subtype)
			this.traits.supply.subtype = this.traits.supply.type
			
		// The "dropsitecount" array holds the number of units with gather aura in range of the object;
		// this is important so that if you have two mills near something and one of them is destroyed,
		// you can still gather from the thing. Initialize it to 0 (ungatherable) for every player unless
		// the entity is forageable (e.g. for huntable animals).
		this.traits.supply.dropsitecount = new Array();
		initialCount = this.traits.supply.forageable ? 1 : 0;
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
				this.traits.promotion.rank = "1"
				nextSuffix = "_a"
			break;
			case "_a":
				// Advanced. Upgrades to Elite.
				this.traits.promotion.rank = "2"
				nextSuffix = "_e"
			break;
			case "_e":
				// Elite. Maximum rank.
				this.traits.promotion.rank = "3"
				nextSuffix = ""
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
		if( this.actions.attack )
		{
			a = this.actions.attack;
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
	}
	
	// Attach functions to ourselves
	
	this.processAttack = entityEventAttack;
	this.processAttackRanged = entityEventAttackRanged;
	this.processGather = entityEventGather;
	this.processHeal = entityEventHeal;
	
	this.damage = entityDamage;

	// Attach Auras if the entity is entitled to them.
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
	
	if ( !this.traits.id )
	{
		this.traits.id = new Object();
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

function entityEventAttack( evt )
{
	if( this.actions.attack.ranged )
	{
		this.processAttackRanged( evt );
		return;
	}

	curr_hit = getGUIGlobal().newRandomSound("voice", "hit", this.traits.audio.path);
	curr_hit.play();

	// Attack logic.
	dmg = new DamageType();
	dmg.crush = parseInt(this.actions.attack.damage * this.actions.attack.crush);
	dmg.hack = parseInt(this.actions.attack.damage * this.actions.attack.hack);
	dmg.pierce = parseInt(this.actions.attack.damage * this.actions.attack.pierce);
	evt.target.damage( dmg, this );
}

// ====================================================================

function entityEventAttackRanged( evt )
{
	// Create a projectile from us, to the target, that will do some damage when it hits them.
	dmg = new DamageType();
	dmg.crush = parseInt(this.actions.attack.damage * this.actions.attack.crush);
	dmg.hack = parseInt(this.actions.attack.damage * this.actions.attack.hack);
	dmg.pierce = parseInt(this.actions.attack.damage * this.actions.attack.pierce);
	
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

function entityEventGather( evt )
{
	g = this.actions.gather;
	s = evt.target.traits.supply;
	
	if( !s.dropsitecount[this.player.id] )
	{
		// Entity has become ungatherable for us, probably meaning our mill near it was killed
		// Get that gather order off the queue (really there should be a stop() method for this)
		this.order( ORDER_GOTO, this.position.x, this.position.z );
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

function entityEventHeal( evt )
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
		switch( resource.toString().toUpperCase() )
		{
			case "POPULATION" || "HOUSING":
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
		switch( resource.toString().toUpperCase() )
		{
			case "POPULATION" || "HOUSING":
			break;
			default:
				// Deduct resources to pay for healing.
				this.player.resource[resource]-= evt.target.actions.heal.cost * evt.target.traits.creation.cost[resource];
			break;
		}
	}
}

// ====================================================================

function entityDamage( damage, inflictor )
{	
	// Apply armour and work out how much damage we actually take
	crushDamage = parseInt(damage.crush - this.traits.armour.value * this.traits.armour.crush);
	if ( crushDamage < 0 ) crushDamage = 0;
	pierceDamage = parseInt(damage.pierce - this.traits.armour.value * this.traits.armour.pierce);
	if ( pierceDamage < 0 ) pierceDamage = 0;
	hackDamage = parseInt(damage.hack - this.traits.armour.value * this.traits.armour.hack);
	if ( hackDamage < 0 ) hackDamage = 0;

	totalDamage = parseInt(damage.typeless + crushDamage + pierceDamage + hackDamage);

	// Minimum of 1 damage

	if( totalDamage < 1 ) totalDamage = 1;

	this.traits.health.curr -= totalDamage;

	if( this.traits.health.curr <= 0 )
	{
		// If the inflictor gains promotions, and he's capable of earning more ranks,
		if (inflictor.traits.promotion && inflictor.traits.promotion.curr && inflictor.traits.promotion.req
				&& inflictor.traits.promotion.newentity && inflictor.traits.promotion.newentity != ""
				&& this.traits.loot && this.traits.loot.xp)
		{
			// Give him the fallen's upgrade points (if he has any).
			if (this.traits.loot.xp)
				inflictor.traits.promotion.curr = parseInt(inflictor.traits.promotion.curr) + parseInt(this.traits.loot.XP);

			// Notify player.
			if (this.traits.id.specific)
				console.write(this.traits.id.specific + " has earned " + this.traits.loot.xp + " upgrade points!");
			else
				console.write("One of your units has earned " + this.traits.loot.xp + " upgrade points!");

			// If he now has maximum upgrade points for his rank,
			if (inflictor.traits.xp.curr >= inflictor.traits.xp.req)
			{
				// Notify the player.
				if (this.traits.id.specific)
					console.write(this.traits.id.specific + " has gained a promotion!");
				else
					console.write("One of your units has gained a promotion!");
				
				// Reset his upgrade points.
				inflictor.traits.xp.curr = 0; 

				// Upgrade his portrait to the next level.
				inflictor.traits.id.icon_cell++; 

				// Transmogrify him into his next rank.
				inflictor.template = getEntityTemplate(inflictor.traits.xp.newentity);
			}
		}

		// If the fallen is worth any loot,
		if (this.traits.loot)
		{
			// Cycle through all loot on this entry.
			pool = this.traits.loot;
			for( loot in pool )
			{
				switch( loot.toString().toUpperCase() )
				{
					case "xp":
					break;
					default:
						// Give the inflictor his resources.
						getGUIGlobal().giveResources( loot.toString(), parseInt(pool[loot]) );
						// Notify player.
						console.write ("Spoils of war! " + pool[loot] + " " + loot.toString() + "!");
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
	else if( inflictor && this.actions.attack )
	{
		// If we're not already doing something else, take a measured response - hit 'em back.
		// You know, I think this is quite possibly the first AI code the AI divlead has written
		// for 0 A.D....
		if( this.isIdle() )
			this.order( ORDER_GENERIC, inflictor, ACTION_ATTACK );
	}
}

// ====================================================================

function entityEventGeneric( evt )
{
	switch( evt.action )
	{
		case ACTION_ATTACK:
			this.processAttack( evt ); break;
		case ACTION_GATHER:
			this.processGather( evt ); break;
		case ACTION_HEAL:
			this.processHeal( evt ); break;
		default:
			console.write( "Unknown generic action: " + evt.action );
	}
}
function entityEventNotification( evt )
{
	switch( evt.type )
	{
		case NOTIFY_GOTO:
			this.Order( ORDER_GENERIC, evt.location.x, evt.location.y );
		case NOTIFY_ATTACK:
		case NOTIFY_DAMAGE:
			this.Order( ORDER_GENERIC, evt.target, ACTION_ATTACK);
			this.Order( ORDER_GENERIC, evt.target, ACTION_ATTACK);
		case NOTIFY_HEAL:
			this.Order( ORDER_GENERIC, evt.target, ACTION_HEAL);
		case NOTIFY_GATHER:
			this.Order( ORDER_GENERIC, evt.target, ACTION_GATHER);
			
	}
}		
// ====================================================================

function entityEventTargetChanged( evt )
{
	// This event lets us know when the user moves his/her cursor to a different unit (provided this
	// unit is selected) - use it to tell the engine what context cursor should be displayed, given
	// the target.

	// If we can gather, and the target supplies, gather. If it's our enemy, and we're armed, attack. 
	// If all else fails, move.
	// ToString is needed because every property is actually an object (though that's usually
	// hidden from you) and comparing an object to any other object in JavaScript (1.5, at least)
	// yields false. ToString converts them to their actual values (i.e. the four character
	// string) first.
	
	evt.defaultOrder = NMT_Goto;
	evt.defaultCursor = "arrow-default";
	if( evt.target && this.actions )
	{
	    /*
		if( this.actions.attack && 
			evt.target.player != this.player &&
			evt.target.traits.health &&
			evt.target.traits.health.max != 0 )
		{
			evt.defaultOrder = NMT_AttackMelee;
			evt.defaultCursor = "action-attack";
		}
		if( canGather( this, evt.target ) )
		{
		    evt.defaultOrder = NMT_Gather;
		    // Set cursor (eg "action-gather-fruit").
		    evt.defaultCursor = "action-gather-" + evt.target.traits.supply.subtype;
		}
		*/	
		if( this.actions.attack && 
			evt.target.player != this.player &&
			evt.target.traits.health &&
			evt.target.traits.health.max != 0 )
		{
			evt.defaultOrder = NMT_Generic;
			evt.defaultAction = ACTION_ATTACK;
			evt.defaultCursor = "action-attack";
		}
		if( canGather( this, evt.target ) )
		{
			evt.defaultOrder = NMT_Generic;
			evt.defaultAction = ACTION_GATHER;
		    // Set cursor (eg "action-gather-fruit").
		    evt.defaultCursor = "action-gather-" + evt.target.traits.supply.subtype;
		}
	}
}

// ====================================================================

function entityEventPrepareOrder( evt )
{
	// This event gives us a chance to veto any order we're given before we execute it.
	// Not sure whether this really belongs here like this: the alternative is to override it in
	// subtypes - then you wouldn't need to check tags, you could hardcode results.

	if( !this.actions )
	{
		evt.preventDefault();
		return;
	}

	switch( evt.orderType )
	{
		case ORDER_GOTO:
			if( !this.actions.move )
				evt.preventDefault();
			break;
		case ORDER_PATROL:
			if( !this.actions.patrol )
				evt.preventDefault();
			break;	
		case ORDER_ATTACK:
			if( !this.actions.attack || 
			    !evt.target )
				evt.preventDefault();
			break;
		case ORDER_GATHER:
			if( !canGather( this, evt.target ) )
				evt.preventDefault();
			break;
		case ORDER_GENERIC:
			// TODO: some checking here
			break;
		default:
			evt.preventDefault();
		break;
	}
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

	if (result == "true") // If the entry meets requirements to be added to the queue (eg sufficient resources) 
	{
		// Cycle through all costs of this entry.
		pool = template.traits.creation.resource;
		for( resource in pool )
		{
			switch( resource.toString().toUpperCase() )
			{
				case "POPULATION" || "HOUSING":
				break;
				default:
					// Deduct the given quantity of resources.
					getGUIGlobal().deductResources(resource.toString(), parseInt(pool[resource]));

					console.write("Spent " + pool[resource] + " " + resource + " to purchase " + 
						template.traits.id.generic);
				break;
			}
		}

		// Add entity to queue.
		console.write( "Adding ", create_tag, " to build queue..." );
		entity.addCreateQueue( template, tab, list );
	}
	else		// If not, output the error message.
		console.write(result);
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
		resourceU = resource.toString().toUpperCase();
		
		switch( resourceU )
		{
			case "POPULATION":
				// If the item costs more of this resource type than we have,
				if (resources[resource] > (localPlayer.resource["HOUSING"]-localPlayer.resource[resourceU]))
				{
					// Return an error.
					return ("Insufficient Housing; " + (resources[resource]-localPlayer.resource["HOUSING"]-localPlayer.resource.valueOf()[resourceU].toString()) + " required."); 
				}
			break;
			case "HOUSING": // Ignore housing. It's handled in combination with population.
			break
			default:
				// If the item costs more of this resource type than we have,
				if (resources[resource] > localPlayer.resource[resourceU])
				{
					// Return an error.
					return ("Insufficient " + resource + "; " + (localPlayer.resource[resourceU]-resources[resource])*-1 + " required."); 
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
	return "true"; 
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
