function entity_event_attack( evt )
{
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

function entity_event_attack_ranged( evt )
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
	
	projectile = new Projectile( this, this, evt.target, 12.0, this, projectile_event_impact )
	
	// We'll attach the damage information to the projectile, just to show you can
	// do that like you can with most other objects. Could also do this by making
	// the function we pass a closure.
	
	projectile.damage = dmg;
	
	// Finally, tell the engine not to send this event to anywhere else - 
	// in particular, this shouldn't get to the melee event handler, above.
	
	evt.stopPropagation();
	
	console.write( "Fire!" );
}

// ====================================================================

function projectile_event_impact( evt )
{
	console.write( "Hit!" );
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

function entity_event_gather( evt )
{
	if (this.actions.gather[evt.target.traits.supply.type][evt.target.traits.supply.subtype])
		gather_amt = parseInt( this.actions.gather[evt.target.traits.supply.type][evt.target.traits.supply.subtype].speed );
	else
		gather_amt = parseInt( this.actions.gather[evt.target.traits.supply.type].speed );

	if( evt.target.traits.supply.max > 0 )
	{
		if( evt.target.traits.supply.curr <= gather_amt )
		{
			gather_amt = evt.target.traits.supply.curr;
			evt.target.kill();
		}

		// Remove amount from target.
		evt.target.traits.supply.curr -= gather_amt;
		// Add extracted resources to player's resource pool.
		getGUIGlobal().giveResources(evt.target.traits.supply.type.toString(), parseInt(gather_amt));
	}
}

// ====================================================================

function entity_event_takesdamage( evt )
{
	// Apply armour and work out how much damage we actually take
	crushDamage = parseInt(evt.damage.crush - this.traits.armour.value * this.traits.armour.crush);
	if ( crushDamage < 0 ) crushDamage = 0;
	pierceDamage = parseInt(evt.damage.pierce - this.traits.armour.value * this.traits.armour.pierce);
	if ( pierceDamage < 0 ) pierceDamage = 0;
	hackDamage = parseInt(evt.damage.hack - this.traits.armour.value * this.traits.armour.hack);
	if ( hackDamage < 0 ) hackDamage = 0;

	totalDamage = parseInt(evt.damage.typeless + crushDamage + pierceDamage + hackDamage);

	// Minimum of 1 damage

	if( totalDamage < 1 ) totalDamage = 1;

	this.traits.health.curr -= totalDamage;

	if( this.traits.health.curr <= 0 )
	{
		// If the inflictor gains promotions, and he's capable of earning more ranks,
		if (evt.inflictor.traits.up && evt.inflictor.traits.up.curr && evt.inflictor.traits.up.req && evt.inflictor.traits.up.newentity && evt.inflictor.traits.up.newentity != "")
		{
			// Give him the fallen's upgrade points (if he has any).
			if (this.traits.loot.up)
				evt.inflictor.traits.up.curr = parseInt(evt.inflictor.traits.up.curr) + parseInt(this.traits.loot.up);
			// Notify player.
			if (this.traits.id.specific)
				console.write(this.traits.id.specific + " has earned " + this.traits.loot.up + " upgrade points!");
			else
				console.write("One of your units has earned " + this.traits.loot.up + " upgrade points!");

			// If he now has maximum upgrade points for his rank,
			if (evt.inflictor.traits.up.curr >= evt.inflictor.traits.up.req)
			{
				// Notify the player.
				if (this.traits.id.specific)
					console.write(this.traits.id.specific + " has gained a promotion!");
				else
					console.write("One of your units has gained a promotion!");
				
				// Reset his upgrade points.
				evt.inflictor.traits.up.curr = 0; 

				// Upgrade his portrait to the next level.
				evt.inflictor.traits.id.icon_cell++; 

				// Transmogrify him into his next rank.
				evt.inflictor.template = getEntityTemplate(evt.inflictor.traits.up.newentity);
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
		if( evt.inflictor )
			console.write( this.traits.id.generic + " got the point of " + evt.inflictor.traits.id.generic + "'s weapon." );
		else
			console.write( this.traits.id.generic + " died in mysterious circumstances." );

		// Make him cry out in pain.
		curr_pain = getGUIGlobal().newRandomSound("voice", "pain", this.traits.audio.path);
		curr_pain.play();

		// We've taken what we need. Kill the swine.
		this.kill();
	}
	else if( evt.inflictor && this.actions.attack )
	{
		// If we're not already doing something else, take a measured response - hit 'em back.
		// You know, I think this is quite possibly the first AI code the AI divlead has written
		// for 0 A.D....
		if( this.isIdle() )
			this.order( ORDER_ATTACK, evt.inflictor );
	}
}

function entity_event_targetchanged( evt )
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
	
	evt.defaultAction = NMT_Goto;
	evt.defaultCursor = "arrow-default";
	if( evt.target )
	{
	    if( this.actions.attack && 
			( evt.target.player != this.player ) )
		{
			evt.defaultAction = NMT_AttackMelee;
			evt.defaultCursor = "action-attack";
		}
	    if( this.actions.gather && evt.target.traits.supply &&
			this.actions.gather[evt.target.traits.supply.type] &&
			( ( evt.target.traits.supply.curr > 0 ) || ( evt.target.traits.supply.max == 0 ) ) )
		{
		    evt.defaultAction = NMT_Gather;
		    // Set cursor (eg "action-gather-fruit").
		    evt.defaultCursor = "action-gather-" + evt.target.traits.supply.subtype;
		}
	}
}

// ====================================================================

function entity_event_prepareorder( evt )
{
	// This event gives us a chance to veto any order we're given before we execute it.
	// Not sure whether this really belongs here like this: the alternative is to override it in
	// subtypes - then you wouldn't need to check tags, you could hardcode results.
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
		if( !this.actions.gather ||
		    !( this.actions.gather[evt.target.traits.supply.type] ) ||
		    ( ( evt.target.traits.supply.curr == 0 ) && ( evt.target.traits.supply.max > 0 ) ) )
			evt.preventDefault();
		break;
	default:
		evt.preventDefault();
	}
}

// ====================================================================

function entity_add_create_queue( template, tab, list )
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
		 this.actions.create.progress = new ProgressTimer( template.traits.creation.time, this.actions.create.construct / 1000, entity_create_complete, this )
	}
}

// ====================================================================

// This is the syntax to add a function (or a property) to absolutely every entity.

Entity.prototype.add_create_queue = entity_add_create_queue;

// ====================================================================

function entity_create_complete()
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
		this.actions.create.progress = new ProgressTimer( template.traits.creation.time, this.actions.create.construct / 1000, entity_create_complete, this )
	}
	else
	{
		// Otherwise, delete the timer.
		this.actions.create.progress = null;
	}
}

// ====================================================================

function attempt_add_to_build_queue( entity, create_tag, tab, list )
{
	result = entity_CheckQueueReq (entity);

	if (result == "true") // If the entry meets requirements to be added to the queue (eg sufficient resources) 
	{
		// Cycle through all costs of this entry.
		pool = entity.traits.creation.resource;
		for( resource in pool )
		{
			switch( resource.toString().toUpperCase() )
			{
				case "POPULATION" || "HOUSING":
				break;
				default:
					// Deduct the given quantity of resources.
					getGUIGlobal().deductResources(resource.toString(), parseInt(pool[resource].cost));

					console.write("Spent " + pool[resource].cost + " " + resource + " to purchase " + getEntityTemplate( create_tag ).traits.id.generic);
				break;
			}
		}

		// Add entity to queue.
		console.write( "Adding ", create_tag, " to build queue..." );
		entity.add_create_queue( getEntityTemplate( create_tag ), tab, list );
	}
	else		// If not, output the error message.
		console.write(result);
}

// ====================================================================

function entity_CheckQueueReq (entry) 
{ 
	// Determines if the given entity meets requirements for production by the player, and returns an appropriate 
	// error string.
	// A return value of 0 equals success -- entry meets requirements for production. 
	
	// Cycle through all resources that this item costs, and check the player can afford the cost.
	resources = entry.traits.creation.resource;
	for( resource in resources )
	{
		resourceU = resource.toString().toUpperCase();
		
		switch( resourceU )
		{
			case "POPULATION":
				// If the item costs more of this resource type than we have,
				if (resources[resource].cost > (localPlayer.resource["HOUSING"]-localPlayer.resource[resourceU]))
				{
					// Return an error.
					return ("Insufficient Housing; " + (resources[resource].cost-localPlayer.resource["HOUSING"]-localPlayer.resource.valueOf()[resourceU].toString()) + " required."); 
				}
			break;
			case "HOUSING": // Ignore housing. It's handled in combination with population.
			break
			default:
				// If the item costs more of this resource type than we have,
				if (resources[resource].cost > localPlayer.resource[resourceU])
				{
					// Return an error.
					return ("Insufficient " + resource + "; " + (localPlayer.resource[resourceU]-resources[resource].cost)*-1 + " required."); 
				}
				else
					console.write("Player has at least " + resources[resource].cost + " " + resource + ".");
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
