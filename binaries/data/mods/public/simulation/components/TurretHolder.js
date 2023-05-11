/**
 * This class holds the functions regarding entities being visible on
 * another entity, but tied to their parents location.
 */
class TurretHolder
{
	Init()
	{
		this.turretPoints = [];

		let points = this.template.TurretPoints;
		for (let point in points)
			this.turretPoints.push({
				"name": point,
				"offset": {
					"x": +points[point].X,
					"y": +points[point].Y,
					"z": +points[point].Z
				},
				"allowedClasses": points[point].AllowedClasses?._string,
				"angle": points[point].Angle ? +points[point].Angle * Math.PI / 180 : null,
				"entity": null,
				"template": points[point].Template,
				"ejectable": "Ejectable" in points[point] ? points[point].Ejectable == "true" : true
			});
	}

	/**
	 * Add a subunit as specified in the template.
	 * This function creates an entity and places it on the turret point.
	 *
	 * @param {Object} turretPoint - A turret point to (re)create the predefined subunit for.
	 *
	 * @return {boolean} - Whether the turret creation has succeeded.
	 */
	CreateSubunit(turretPointName)
	{
		const turretPoint = this.TurretPointByName(turretPointName);
		if (!turretPoint || turretPoint.entity ||
			this.initTurrets?.has(turretPointName) ||
			this.reservedTurrets?.has(turretPointName))
			return false;

		const cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);

		const upgradedTemplate = GetUpgradedTemplate(cmpOwnership.GetOwner(), turretPoint.template);
		const ent = Engine.AddEntity(upgradedTemplate);

		const cmpEntOwnership = Engine.QueryInterface(ent, IID_Ownership);
		cmpEntOwnership?.SetOwner(cmpOwnership.GetOwner());

		const cmpTurretable = Engine.QueryInterface(ent, IID_Turretable);
		return cmpTurretable?.OccupyTurret(this.entity, turretPoint.name, turretPoint.ejectable) || Engine.DestroyEntity(ent);
	}

	/**
	 * @param {string} name - The name of a turret point to reserve, e.g. for promotion.
	 */
	SetReservedTurretPoint(name)
	{
		if (!this.reservedTurrets)
			this.reservedTurrets = new Set();
		this.reservedTurrets.add(name);
	}

	/**
	 * @return {Object[]} - An array of the turret points this entity has.
	 */
	GetTurretPoints()
	{
		return this.turretPoints;
	}

	/**
	 * @param {number} entity - The entity to check for.
	 * @param {Object} turretPoint - The turret point to use.
	 *
	 * @return {boolean} - Whether the entity is allowed to occupy the specified turret point.
	 */
	AllowedToOccupyTurretPoint(entity, turretPoint)
	{
		if (!turretPoint || turretPoint.entity)
			return false;

		if (!IsOwnedByMutualAllyOfEntity(entity, this.entity))
			return false;

		if (!turretPoint.allowedClasses)
			return true;

		let cmpIdentity = Engine.QueryInterface(entity, IID_Identity);
		return cmpIdentity && MatchesClassList(cmpIdentity.GetClassesList(), turretPoint.allowedClasses);
	}

	/**
	 * @param {number} entity - The entity to check for.
	 * @return {boolean} - Whether the entity is allowed to occupy any turret point.
	 */
	CanOccupy(entity)
	{
		return !!this.turretPoints.find(turretPoint => this.AllowedToOccupyTurretPoint(entity, turretPoint));
	}

	/**
	 * Occupy a turret point with the given entity.
	 * @param {number} entity - The entity to use.
	 * @param {Object} requestedTurretPoint - Optionally the specific turret point to occupy.
	 *
	 * @return {boolean} - Whether the occupation was successful.
	 */
	OccupyTurretPoint(entity, requestedTurretPoint)
	{
		let cmpPositionOccupant = Engine.QueryInterface(entity, IID_Position);
		if (!cmpPositionOccupant)
			return false;

		let cmpPositionSelf = Engine.QueryInterface(this.entity, IID_Position);
		if (!cmpPositionSelf)
			return false;

		if (this.OccupiesTurretPoint(entity))
			return false;

		let turretPoint;
		if (requestedTurretPoint)
		{
			if (this.AllowedToOccupyTurretPoint(entity, requestedTurretPoint))
				turretPoint = requestedTurretPoint;
		}
		else
			turretPoint = this.turretPoints.find(turret => !turret.entity && this.AllowedToOccupyTurretPoint(entity, turret));

		if (!turretPoint)
			return false;

		turretPoint.entity = entity;

		// Angle of turrets:
		// Renamed entities (turretPoint != undefined) should keep their angle.
		// Otherwise if an angle is given in the turretPoint, use it.
		// If no such angle given (usually walls for which outside/inside not well defined), we keep
		// the current angle as it was used for garrisoning and thus quite often was from inside to
		// outside, except when garrisoning from outWorld where we take as default PI.
		if (!turretPoint && turretPoint.angle != null)
			cmpPositionOccupant.SetYRotation(cmpPositionSelf.GetRotation().y + turretPoint.angle);
		else if (!turretPoint && !cmpPosition.IsInWorld())
			cmpPositionOccupant.SetYRotation(cmpPositionSelf.GetRotation().y + Math.PI);

		cmpPositionOccupant.SetTurretParent(this.entity, turretPoint.offset);

		Engine.PostMessage(this.entity, MT_TurretsChanged, {
			"added": [entity],
			"removed": []
		});

		return true;
	}

	/**
	 * @param {number} entity - The entityID of the entity.
	 * @param {String} turretName - The name of the turret point to occupy.
	 * @return {boolean} - Whether the occupation has succeeded.
	 */
	OccupyNamedTurretPoint(entity, turretName)
	{
		return this.OccupyTurretPoint(entity, this.TurretPointByName(turretName));
	}

	/**
	 * @param {string} turretPointName - The name of the requested turret point.
	 * @return {Object} - The requested turret point.
	 */
	TurretPointByName(turretPointName)
	{
		return this.turretPoints.find(turret => turret.name == turretPointName);
	}

	/**
	 * Remove the entity from a turret.
	 * @param {number} entity - The specific entity to eject.
	 * @param {boolean} forced - Whether ejection is forced (e.g. due to death or renaming).
	 * @param {Object} turret - Optionally the turret to abandon.
	 *
	 * @return {boolean} - Whether the entity succesfully left us.
	 */
	LeaveTurretPoint(entity, forced, requestedTurretPoint)
	{
		let turretPoint;
		if (requestedTurretPoint)
		{
			if (requestedTurretPoint.entity == entity)
				turretPoint = requestedTurretPoint;
		}
		else
			turretPoint = this.GetOccupiedTurretPoint(entity);

		if (!turretPoint || (!turretPoint.ejectable && !forced))
			return false;

		turretPoint.entity = null;

		Engine.PostMessage(this.entity, MT_TurretsChanged, {
			"added": [],
			"removed": [entity]
		});

		return true;
	}

	/**
	 * @param {number} entity - The entity's id.
	 * @param {Object} turret - Optionally the turret to check.
	 *
	 * @return {boolean} - Whether the entity is positioned on a turret of this entity.
	 */
	OccupiesTurretPoint(entity, requestedTurretPoint)
	{
		return requestedTurretPoint ? requestedTurretPoint.entity == entity :
			!!this.GetOccupiedTurretPoint(entity);
	}

	/**
	 * @param {number} entity - The entity's id.
	 * @return {Object} - The turret this entity is positioned on, if applicable.
	 */
	GetOccupiedTurretPoint(entity)
	{
		return this.turretPoints.find(turretPoint => turretPoint.entity == entity);
	}

	/**
	 * @param {number} entity - The entity's id.
	 * @return {Object} - The turret this entity is positioned on, if applicable.
	 */
	GetOccupiedTurretPointName(entity)
	{
		let turret = this.GetOccupiedTurretPoint(entity);
		return turret ? turret.name : "";
	}

	/**
	 * @return {number[]} - The turretted entityIDs.
	 */
	GetEntities()
	{
		let entities = [];
		for (let turretPoint of this.turretPoints)
			if (turretPoint.entity)
				entities.push(turretPoint.entity);
		return entities;
	}

	/**
	 * @return {boolean} - Whether all the turret points are occupied.
	 */
	IsFull()
	{
		return !!this.turretPoints.find(turretPoint => turretPoint.entity == null);
	}

	/**
	 * @return {Object} - Max and min ranges at which entities can occupy any turret.
	 */
	LoadingRange()
	{
		return { "min": 0, "max": +(this.template.LoadingRange || 2) };
	}

	/**
	 * @param {number} ent - The entity ID of the turret to be potentially picked up.
	 * @return {boolean} - Whether this entity can pick the specified entity up.
	 */
	CanPickup(ent)
	{
		if (!this.template.Pickup || this.IsFull())
			return false;
		let cmpOwner = Engine.QueryInterface(this.entity, IID_Ownership);
		return !!cmpOwner && IsOwnedByPlayer(cmpOwner.GetOwner(), ent);
	}

	/**
	 * @param {number[]} entities - The entities to ask to leave or to kill.
	 */
	EjectOrKill(entities)
	{
		let removedEntities = [];
		for (let entity of entities)
		{
			let cmpTurretable = Engine.QueryInterface(entity, IID_Turretable);
			if (!cmpTurretable || !cmpTurretable.LeaveTurret(true))
			{
				let cmpHealth = Engine.QueryInterface(entity, IID_Health);
				if (cmpHealth)
					cmpHealth.Kill();
				else
					Engine.DestroyEntity(entity);
				removedEntities.push(entity);
			}
		}
		if (removedEntities.length)
			Engine.PostMessage(this.entity, MT_TurretsChanged, {
				"added": [],
				"removed": removedEntities
			});
	}

	/**
	 * Sets an init turret, present from game start. (E.g. set in Atlas.)
	 * @param {String} turretName - The name of the turret point to be used.
	 * @param {number} entity - The entity-ID to be placed.
	 */
	SetInitEntity(turretName, entity)
	{
		if (!this.initTurrets)
			this.initTurrets = new Map();

		if (this.initTurrets.has(turretName))
			warn("The turret position " + turretName + " of entity " +
				this.entity + " is already set! Overwriting.");

		this.initTurrets.set(turretName, entity);
	}

	/**
	 * Update list of turreted entities when a game inits.
	 */
	OnGlobalSkirmishReplacerReplaced(msg)
	{
		if (!this.initTurrets)
			return;

		if (msg.entity == this.entity)
		{
			let cmpTurretHolder = Engine.QueryInterface(msg.newentity, IID_TurretHolder);
			if (cmpTurretHolder)
				cmpTurretHolder.initTurrets = this.initTurrets;
		}
		else
		{
			let entityIndex = this.initTurrets.indexOf(msg.entity);
			if (entityIndex != -1)
				this.initTurrets[entityIndex] = msg.newentity;
		}
	}

	/**
	 * Initialise turreted units.
	 */
	OnGlobalInitGame(msg)
	{
		if (!this.initTurrets)
			return;

		for (let [turretPointName, entity] of this.initTurrets)
		{
			let cmpTurretable = Engine.QueryInterface(entity, IID_Turretable);
			if (!cmpTurretable || !cmpTurretable.OccupyTurret(this.entity, turretPointName, this.TurretPointByName(turretPointName).ejectable))
				warn("Entity " + entity + " could not occupy the turret point " +
					turretPointName + " of turret holder " + this.entity + ".");
		}

		delete this.initTurrets;
	}

	/**
	 * @param {Object} msg - { "entity": number, "newentity": number }.
	 */
	OnEntityRenamed(msg)
	{
		for (let entity of this.GetEntities())
		{
			let cmpTurretable = Engine.QueryInterface(entity, IID_Turretable);
			if (!cmpTurretable)
				continue;
			let currentPoint = this.GetOccupiedTurretPointName(entity);
			cmpTurretable.LeaveTurret(true);
			cmpTurretable.OccupyTurret(msg.newentity, currentPoint);
		}
	}

	/**
	 * @param {Object} msg - { "entity": number, "from": number, "to": number }.
	 */
	OnOwnershipChanged(msg)
	{
		if (msg.to === INVALID_PLAYER)
		{
			this.EjectOrKill(this.GetEntities());
			return;
		}

		for (let point of this.turretPoints)
		{
			// If we were created, create any subunits now.
			// This has to be done here (instead of on Init)
			// for Ownership ought to be initialised.
			if (point.template && msg.from === INVALID_PLAYER)
			{
				this.CreateSubunit(point.name);
				continue;
			}
			if (!point.entity)
				continue;
			if (!point.ejectable)
			{
				let cmpTurretOwnership = Engine.QueryInterface(point.entity, IID_Ownership);
				if (cmpTurretOwnership)
					cmpTurretOwnership.SetOwner(msg.to);
			}
			else if (!IsOwnedByMutualAllyOfEntity(point.entity, this.entity))
			{
				let cmpTurretable = Engine.QueryInterface(point.entity, IID_Turretable);
				if (cmpTurretable)
					cmpTurretable.LeaveTurret();
			}
		}
		delete this.reservedTurrets;
	}
}

TurretHolder.prototype.Schema =
	"<element name='TurretPoints' a:help='Points that will be used to visibly garrison a unit.'>" +
		"<oneOrMore>" +
			"<element a:help='Element containing the offset coordinates.'>" +
				"<anyName/>" +
				"<interleave>" +
					"<element name='X'>" +
						"<data type='decimal'/>" +
					"</element>" +
					"<element name='Y'>" +
						"<data type='decimal'/>" +
					"</element>" +
					"<element name='Z'>" +
						"<data type='decimal'/>" +
					"</element>" +
					"<optional>" +
						"<interleave>" +
							"<element name='Template'>" +
								"<text/>" +
							"</element>" +
							"<element name='Ejectable' a:help='Whether this template is tied to the turret position (i.e. not allowed to leave the turret point).'>" +
								"<data type='boolean'/>" +
							"</element>" +
						"</interleave>" +
					"</optional>" +
					"<optional>" +
						"<element name='AllowedClasses' a:help='If specified, only entities matching the given classes will be able to use this turret.'>" +
							"<attribute name='datatype'>" +
								"<value>tokens</value>" +
							"</attribute>" +
							"<text/>" +
						"</element>" +
					"</optional>"+
					"<optional>" +
						"<element name='Angle' a:help='Angle in degrees relative to the turretHolder direction.'>" +
							"<data type='decimal'/>" +
						"</element>" +
					"</optional>" +
				"</interleave>" +
			"</element>" +
		"</oneOrMore>" +
	"</element>" +
	"<optional>" +
		"<element name='LoadingRange' a:help='The maximum distance from this holder at which entities are allowed to occupy a turret point. Should be about 2.0 for land entities and preferably greater for ships.'>" +
			"<ref name='nonNegativeDecimal'/>" +
		"</element>" +
	"</optional>"
	"<optional>" +
		"<element name='Pickup' a:help='This entity will try to move to pick up units to be turreted.'>" +
			"<data type='boolean'/>" +
		"</element>" +
	"</optional>";

Engine.RegisterComponentType(IID_TurretHolder, "TurretHolder", TurretHolder);
