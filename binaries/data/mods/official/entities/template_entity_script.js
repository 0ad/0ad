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
	    console.write( evt.target.traits.supply.type);
	    console.write( evt.target.traits.supply.type.toString().toUpperCase() );
	    evt.target.traits.supply.curr -= gather_amt;
	    this.player.resource.valueOf()[evt.target.traits.supply.type.toString().toUpperCase()] += gather_amt;
	}
}

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

				// Transmogrify him into his next rank.
				evt.inflictor.template = getEntityTemplate(evt.inflictor.traits.up.newentity);
			}
		}

		// If the fallen is worth any loot,
		if (this.traits.loot && (this.traits.loot.food || this.traits.loot.wood || this.traits.loot.stone || this.traits.loot.ore))
		{
			// Give the inflictor his resources.
			if (this.traits.loot.food)
				getGUIGlobal().GiveResources("Food", parseInt(this.traits.loot.food));
			if (this.traits.loot.wood)
				getGUIGlobal().GiveResources("Wood", parseInt(this.traits.loot.wood));
			if (this.traits.loot.stone)
				getGUIGlobal().GiveResources("Stone", parseInt(this.traits.loot.stone));
			if (this.traits.loot.ore)
				getGUIGlobal().GiveResources("Ore", parseInt(this.traits.loot.ore));
		}

		// Notify player.
		if( evt.inflictor )
			console.write( this.traits.id.generic + " got the point of " + evt.inflictor.traits.id.generic + "'s Gladius." );
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

	// Attack iff there's a target, it's our enemy, and we're armed. Otherwise, if we can gather, and
	// the target supplies, gather. If all else fails, move.
	// ToString is needed because every property is actually an object (though that's usually
	// hidden from you) and comparing an object to any other object in JavaScript (1.5, at least)
	// yields false. ToString converts them to their actual values (i.e. the four character
	// string) first.

	evt.defaultAction = ORDER_GOTO;
	if( evt.target )
	{
	    if( this.actions.attack && 
		  ( evt.target.traits.id.civ_code != "gaia" ) &&
		  ( evt.target.traits.id.civ_code.toString() != this.traits.id.civ_code.toString() ) )
		    evt.defaultAction = ORDER_ATTACK;
	    if( this.actions.gather && evt.target.traits.supply &&
		this.actions.gather[evt.target.traits.supply.type] &&
		  ( ( evt.target.traits.supply.curr > 0 ) || ( evt.target.traits.supply.max == 0 ) ) )
		    evt.defaultAction = ORDER_GATHER;
	}	
}

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
		// If we can't attack, we're not targeting a unit, or that unit is the same civ as us.
		// (Should of course be same /player/ as us - not ready yet.)
		if( !this.actions.attack || 
		    !evt.target || 
		    ( evt.target.traits.id.civ_code.toString() == this.traits.id.civ_code.toString() ) )
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

function entity_add_create_queue( template )
{
	// Make sure we have a queue to put things in...
	if( !this.actions.create.queue )
		this.actions.create.queue = new Array();
	// Append to the end of this queue
	this.actions.create.queue.valueOf().push( template );
	// If we're not already building something...
	if( !this.actions.create.progress || !this.actions.create.progress.valueOf() )
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

// This is the syntax to add a function (or a property) to absolutely every entity.

Entity.prototype.add_create_queue = entity_add_create_queue;

function entity_create_complete()
{
	// Get the unit that was at the head of our queue, and remove it.
	// (Oh, for information about all these nifty properties and functions
	//  of the Array object that I use, see the ECMA-262 documentation
	//  at http://www.mozilla.org/js/language/E262-3.pdf. Bit technical but
	//  the sections on 'Native ECMAScript Objects' are quite useful)
	
	var template = this.actions.create.queue.valueOf().shift();

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
		return;
	}
	
	created = new Entity( template, position );
	
	// Above shouldn't ever fail, but just in case...
	if( !created )
		return;
	
	console.write( "Created: ", template.tag );
	
	// Entities start under Gaia control - make the controller
	// the same as our controller
	created.player = this.player;
	
	// If there's something else in the build queue...
	if( this.actions.create.queue.valueOf().length > 0 )
	{
		// Start on the next item.
		template = this.actions.create.queue.valueOf()[0];
		console.write( "Starting work on (queued) ", template.tag );
		this.actions.create.progress = new ProgressTimer( template.traits.creation.time, this.actions.create.construct / 1000, entity_create_complete, this )
	}
	else
	{
		// Otherwise, delete the timer.
		this.actions.create.progress = null;
	}
}

function attempt_add_to_build_queue( entity, create_tag )
{
	console.write( "Adding ", create_tag, " to build queue..." );
	entity.add_create_queue( getEntityTemplate( create_tag ) );
}