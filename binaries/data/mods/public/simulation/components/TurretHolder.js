/**
 * This class holds the functions regarding entities being visible on
 * another entity, but tied to their parents location.
 * Currently renaming and changing ownership are still managed by GarrisonHolder.js,
 * but in the future these components should be independent.
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
				"allowedClasses": points[point].AllowedClasses,
				"angle": points[point].Angle ? +points[point].Angle * Math.PI / 180 : null,
				"entity": null
			});
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
	AllowedToOccupyTurret(entity, turretPoint)
	{
		if (!turretPoint || turretPoint.entity)
			return false;

		if (!IsOwnedByMutualAllyOfEntity(entity, this.entity))
			return false;

		if (!turretPoint.allowedClasses)
			return true;

		let cmpIdentity = Engine.QueryInterface(entity, IID_Identity);
		return cmpIdentity && MatchesClassList(cmpIdentity.GetClassesList(), turretPoint.allowedClasses._string);
	}

	/**
	 * Occupy a turret point with the given entity.
	 * @param {number} entity - The entity to use.
	 * @param {Object} requestedTurretPoint - Optionally the specific turret point to occupy.
	 *
	 * @return {boolean} - Whether the occupation was successful.
	 */
	OccupyTurret(entity, requestedTurretPoint)
	{
		let cmpPositionOccupant = Engine.QueryInterface(entity, IID_Position);
		if (!cmpPositionOccupant)
			return false;

		let cmpPositionSelf = Engine.QueryInterface(this.entity, IID_Position);
		if (!cmpPositionSelf)
			return false;

		if (this.OccupiesTurret(entity))
			return false;

		let turretPoint;
		if (requestedTurretPoint)
		{
			if (this.AllowedToOccupyTurret(entity, requestedTurretPoint))
				turretPoint = requestedTurretPoint;
		}
		else
			turretPoint = this.turretPoints.find(turret => !turret.entity && this.AllowedToOccupyTurret(entity, turret));

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

		let cmpUnitMotion = Engine.QueryInterface(entity, IID_UnitMotion);
		if (cmpUnitMotion)
			cmpUnitMotion.SetFacePointAfterMove(false);

		let cmpUnitAI = Engine.QueryInterface(entity, IID_UnitAI);
		if (cmpUnitAI)
			cmpUnitAI.SetTurretStance();

		// Remove the unit's obstruction to avoid interfering with pathing.
		let cmpObstruction = Engine.QueryInterface(entity, IID_Obstruction);
		if (cmpObstruction)
			cmpObstruction.SetActive(false);

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
	OccupyNamedTurret(entity, turretName)
	{
		return this.OccupyTurret(entity, this.turretPoints.find(turret => turret.name == turretName));
	}

	/**
	 * Remove the entity from a turret.
	 * @param {number} entity - The specific entity to eject.
	 * @param {Object} turret - Optionally the turret to abandon.
	 *
	 * @return {boolean} - Whether the entity was occupying a/the turret before.
	 */
	LeaveTurret(entity, requestedTurretPoint)
	{
		let turretPoint;
		if (requestedTurretPoint)
		{
			if (requestedTurretPoint.entity == entity)
				turretPoint = requestedTurretPoint;
		}
		else
			turretPoint = this.turretPoints.find(turret => turret.entity == entity);

		if (!turretPoint)
			return false;

		let cmpPositionEntity = Engine.QueryInterface(entity, IID_Position);
		cmpPositionEntity.SetTurretParent(INVALID_ENTITY, new Vector3D());

		let cmpUnitMotionEntity = Engine.QueryInterface(entity, IID_UnitMotion);
		if (cmpUnitMotionEntity)
			cmpUnitMotionEntity.SetFacePointAfterMove(true);

		let cmpUnitAIEntity = Engine.QueryInterface(entity, IID_UnitAI);
		if (cmpUnitAIEntity)
			cmpUnitAIEntity.ResetTurretStance();

		turretPoint.entity = null;

		// Reset the obstruction flags to template defaults.
		let cmpObstruction = Engine.QueryInterface(entity, IID_Obstruction);
		if (cmpObstruction)
			cmpObstruction.SetActive(true);

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
	OccupiesTurret(entity, requestedTurretPoint)
	{
		return requestedTurretPoint ? requestedTurretPoint.entity == entity :
			this.turretPoints.some(turretPoint => turretPoint.entity == entity);
	}

	/**
	 * @param {number} entity - The entity's id.
	 * @return {Object} - The turret this entity is positioned on, if applicable.
	 */
	GetOccupiedTurret(entity)
	{
		return this.turretPoints.find(turretPoint => turretPoint.entity == entity);
	}

	/**
	 * @param {number} entity - The entity's id.
	 * @return {Object} - The turret this entity is positioned on, if applicable.
	 */
	GetOccupiedTurretName(entity)
	{
		return this.GetOccupiedTurret(entity).name || "";
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
	 * @param {number} from - The entity to substitute.
	 * @param {number} to - The entity to subtitute with.
	 */
	SwapEntities(from, to)
	{
		let turretPoint = this.GetOccupiedTurret(from);
		if (turretPoint)
		{
			this.LeaveTurret(from, turretPoint);
			this.OccupyTurret(to, turretPoint);
		}
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
	 * Initialise the turreted units.
	 * Really ugly, but because GarrisonHolder is processed earlier, and also turrets
	 * entities on init, we can find an entity that already is present.
	 * In that case we reject and occupy.
	 */
	OnGlobalInitGame(msg)
	{
		if (!this.initTurrets)
			return;

		for (let [turretPointName, entity] of this.initTurrets)
		{
			if (this.OccupiesTurret(entity))
				this.LeaveTurret(entity);
			if (!this.OccupyNamedTurret(entity, turretPointName))
				warn("Entity " + entity + " could not occupy the turret point " +
					turretPointName + " of turret holder " + this.entity + ".");
		}

		delete this.initTurrets;
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
	"</element>";

Engine.RegisterComponentType(IID_TurretHolder, "TurretHolder", TurretHolder);
