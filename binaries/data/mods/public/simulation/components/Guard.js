function Guard() {}

Guard.prototype.Schema =
	"<empty/>";

Guard.prototype.Init = function()
{
	this.entities = [];
};

Guard.prototype.GetRange = function(entity)
{
	var range = 8;
	var cmpFootprint = Engine.QueryInterface(entity, IID_Footprint);
	if (cmpFootprint)
	{
		var shape = cmpFootprint.GetShape();
		if (shape.type == "square")
			range += Math.sqrt(shape.depth*shape.depth + shape.width*shape.width)*2/3;
		else if (shape.type == "circle")
			range += shape.radius*3/2;
	}
	return range;
};

Guard.prototype.GetEntities = function()
{
	return this.entities.slice();
};

Guard.prototype.SetEntities = function(entities)
{
	this.entities = entities;
};

Guard.prototype.AddGuard = function(ent)
{
	if (this.entities.indexOf(ent) != -1)
		return;
	this.entities.push(ent);
};

Guard.prototype.RemoveGuard = function(ent)
{
	var index = this.entities.indexOf(ent);
	if (index != -1)
		this.entities.splice(index, 1);
};

Guard.prototype.OnAttacked = function(msg)
{
	for each (var ent in this.entities)
		Engine.PostMessage(ent, MT_GuardedAttacked, { "guarded": this.entity, "data": msg });
};

/**
 * Update list of guarding/escorting entities if one gets renamed (e.g. by promotion)
 */
Guard.prototype.OnGlobalEntityRenamed = function(msg)
{
	var entityIndex = this.entities.indexOf(msg.entity);
	if (entityIndex != -1)
		this.entities[entityIndex] = msg.newentity;
};

/**
 * If an entity is captured, or about to be killed (so its owner
 * changes to '-1'), update the guards list
 */
Guard.prototype.OnGlobalOwnershipChanged = function(msg)
{
	// the ownership change may be on the guarded
	if (this.entity == msg.entity)
	{
		var entities = this.GetEntities();
		for each (var entity in entities)
		{
			if (msg.to == -1 || !IsOwnedByMutualAllyOfEntity(this.entity, entity))
			{
				var cmpUnitAI = Engine.QueryInterface(entity, IID_UnitAI);
				if (cmpUnitAI && cmpUnitAI.IsGuardOf() && cmpUnitAI.IsGuardOf() == this.entity)
					cmpUnitAI.RemoveGuard();
				else
					this.RemoveGuard(entity);
			}
		}
		this.entities = entities;
		return;
	}

	// or on some of its guards
	if (this.entities.indexOf(msg.entity) != -1)
	{
		if (msg.to == -1)
			this.RemoveGuard(msg.entity);
		else if(!IsOwnedByMutualAllyOfEntity(this.entity, msg.entity))
		{
			var cmpUnitAI = Engine.QueryInterface(msg.entity, IID_UnitAI);
			if (cmpUnitAI)
				cmpUnitAI.RemoveGuard();
			else
				this.RemoveGuard(msg.entity);
		}
	}
};

Engine.RegisterComponentType(IID_Guard, "Guard", Guard);
