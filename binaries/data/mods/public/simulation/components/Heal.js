function Heal() {}

Heal.prototype.Schema =
	"<a:help>Controls the healing abilities of the unit.</a:help>" +
	"<a:example>" +
		"<Range>20</Range>" +
		"<RangeOverlay>" +
			"<LineTexture>heal_overlay_range.png</LineTexture>" +
			"<LineTextureMask>heal_overlay_range_mask.png</LineTextureMask>" +
			"<LineThickness>0.35</LineThickness>" +
		"</RangeOverlay>" +
		"<Health>5</Health>" +
		"<Interval>2000</Interval>" +
		"<UnhealableClasses datatype=\"tokens\">Cavalry</UnhealableClasses>" +
		"<HealableClasses datatype=\"tokens\">Support Infantry</HealableClasses>" +
	"</a:example>" +
	"<element name='Range' a:help='Range (in metres) where healing is possible.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<optional>" +
		"<element name='RangeOverlay'>" +
			"<interleave>" +
				"<element name='LineTexture'><text/></element>" +
				"<element name='LineTextureMask'><text/></element>" +
				"<element name='LineThickness'><ref name='nonNegativeDecimal'/></element>" +
			"</interleave>" +
		"</element>" +
	"</optional>" +
	"<element name='Health' a:help='Health healed per Interval.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='Interval' a:help='A heal is performed every Interval ms.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='UnhealableClasses' a:help='If the target has any of these classes it can not be healed (even if it has a class from HealableClasses).'>" +
		"<attribute name='datatype'>" +
			"<value>tokens</value>" +
		"</attribute>" +
		"<text/>" +
	"</element>" +
	"<element name='HealableClasses' a:help='The target must have one of these classes to be healable.'>" +
		"<attribute name='datatype'>" +
			"<value>tokens</value>" +
		"</attribute>" +
		"<text/>" +
	"</element>";

Heal.prototype.Init = function()
{
};

Heal.prototype.GetTimers = function()
{
	return {
		"prepare": 1000,
		"repeat": this.GetInterval()
	};
};

Heal.prototype.GetHealth = function()
{
	return ApplyValueModificationsToEntity("Heal/Health", +this.template.Health, this.entity);
};

Heal.prototype.GetInterval = function()
{
	return ApplyValueModificationsToEntity("Heal/Interval", +this.template.Interval, this.entity);
};

Heal.prototype.GetRange = function()
{
	return {
		"min": 0,
		"max": ApplyValueModificationsToEntity("Heal/Range", +this.template.Range, this.entity)
	};
};

Heal.prototype.GetUnhealableClasses = function()
{
	return this.template.UnhealableClasses._string || "";
};

Heal.prototype.GetHealableClasses = function()
{
	return this.template.HealableClasses._string || "";
};

/**
 * Whether this entity can heal the target.
 *
 * @param {number} target - The target's entity ID.
 * @return {boolean} - Whether the target can be healed.
 */
Heal.prototype.CanHeal = function(target)
{
	let cmpHealth = Engine.QueryInterface(target, IID_Health);
	if (!cmpHealth || cmpHealth.IsUnhealable() || !cmpHealth.IsInjured())
		return false;

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || !IsOwnedByAllyOfPlayer(cmpOwnership.GetOwner(), target))
		return false;

	let cmpIdentity = Engine.QueryInterface(target, IID_Identity);
	if (!cmpIdentity)
		return false;

	let targetClasses = cmpIdentity.GetClassesList();
	return !MatchesClassList(targetClasses, this.GetUnhealableClasses()) &&
		MatchesClassList(targetClasses, this.GetHealableClasses());
};

Heal.prototype.GetRangeOverlays = function()
{
	if (!this.template.RangeOverlay)
		return [];

	return [{
		"radius": this.GetRange().max,
		"texture": this.template.RangeOverlay.LineTexture,
		"textureMask": this.template.RangeOverlay.LineTextureMask,
		"thickness": +this.template.RangeOverlay.LineThickness
	}];
};

/**
 * @param {number} target - The target to heal.
 * @param {number} callerIID - The IID to notify on specific events.
 * @return {boolean} - Whether we started healing.
 */
Heal.prototype.StartHealing = function(target, callerIID)
{
	if (this.target)
		this.StopHealing();

	if (!this.CanHeal(target))
		return false;

	let timings = this.GetTimers();
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);

	// If the repeat time since the last heal hasn't elapsed,
	// delay the action to avoid healing too fast.
	let prepare = timings.prepare;
	if (this.lastHealed)
	{
		let repeatLeft = this.lastHealed + timings.repeat - cmpTimer.GetTime();
		prepare = Math.max(prepare, repeatLeft);
	}

	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
	{
		cmpVisual.SelectAnimation("heal", false, 1.0);
		cmpVisual.SetAnimationSyncRepeat(timings.repeat);
		cmpVisual.SetAnimationSyncOffset(prepare);
	}

	// If using a non-default prepare time, re-sync the animation when the timer runs.
	this.resyncAnimation = prepare != timings.prepare;
	this.target = target;
	this.callerIID = callerIID;
	this.timer = cmpTimer.SetInterval(this.entity, IID_Heal, "PerformHeal", prepare, timings.repeat, null);

	return true;
};

/**
 * @param {string} reason - The reason why we stopped healing.
 */
Heal.prototype.StopHealing = function(reason)
{
	if (!this.target)
		return;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.timer);
	delete this.timer;

	delete this.target;

	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SelectAnimation("idle", false, 1.0);

	// The callerIID component may start again,
	// replacing the callerIID, hence save that.
	let callerIID = this.callerIID;
	delete this.callerIID;

	if (reason && callerIID)
	{
		let component = Engine.QueryInterface(this.entity, callerIID);
		if (component)
			component.ProcessMessage(reason, null);
	}
};

/**
 * Heal our target entity.
 * @param data - Unused.
 * @param {number} lateness - The offset of the actual call and when it was expected.
 */
Heal.prototype.PerformHeal = function(data, lateness)
{
	if (!this.CanHeal(this.target))
	{
		this.StopHealing("TargetInvalidated");
		return;
	}
	if (!this.IsTargetInRange(this.target))
	{
		this.StopHealing("OutOfRange");
		return;
	}

	// ToDo: Enable entities to keep facing a target.
	Engine.QueryInterface(this.entity, IID_UnitAI)?.FaceTowardsTarget(this.target);

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.lastHealed = cmpTimer.GetTime() - lateness;

	let cmpHealth = Engine.QueryInterface(this.target, IID_Health);
	let targetState = cmpHealth.Increase(this.GetHealth());

	// Add experience.
	let cmpLoot = Engine.QueryInterface(this.target, IID_Loot);
	let cmpPromotion = Engine.QueryInterface(this.entity, IID_Promotion);
	if (targetState !== undefined && cmpLoot && cmpPromotion)
		// Health healed times experience per health.
		cmpPromotion.IncreaseXp((targetState.new - targetState.old) / cmpHealth.GetMaxHitpoints() * cmpLoot.GetXp());

	// TODO we need a sound file.
	// PlaySound("heal_impact", this.entity);

	if (!cmpHealth.IsInjured())
	{
		this.StopHealing("TargetInvalidated");
		return;
	}

	if (this.resyncAnimation)
	{
		let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
		if (cmpVisual)
		{
			let repeat = this.GetTimers().repeat;
			cmpVisual.SetAnimationSyncRepeat(repeat);
			cmpVisual.SetAnimationSyncOffset(repeat);
		}
		delete this.resyncAnimation;
	}
};

/**
 * @param {number} - The entity ID of the target to check.
 * @return {boolean} - Whether this entity is in range of its target.
 */
Heal.prototype.IsTargetInRange = function(target)
{
	let range = this.GetRange();
	let cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);
	return cmpObstructionManager.IsInTargetRange(this.entity, target, range.min, range.max, false);
};

Heal.prototype.OnValueModification = function(msg)
{
	if (msg.component != "Heal" || msg.valueNames.indexOf("Heal/Range") === -1)
		return;

	let cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	if (!cmpUnitAI)
		return;

	cmpUnitAI.UpdateRangeQueries();
};

Engine.RegisterComponentType(IID_Heal, "Heal", Heal);
