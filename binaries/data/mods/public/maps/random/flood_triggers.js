{
	// In what steps the watherlevel rises [s].
	const deltaTime = 2.4;
	// By how much the water level changes each step [m].
	const deltaWaterLevel = 0.5;

	const warningDuration = 14.4;

	const drownDepth = 2;

	const warning = markForTranslation("The flood continues. Soon the waters will swallow the land. " +
		"You should evacuate the units.");

	const riseWaterLevel = (targetWaterLevel) =>
	{
		Engine.QueryInterface(SYSTEM_ENTITY, IID_WaterManager).SetWaterLevel(targetWaterLevel);

		const cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
		const cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);

		for (const ent of cmpRangeManager.GetGaiaAndNonGaiaEntities())
		{
			const cmpPosition = Engine.QueryInterface(ent, IID_Position);
			if (!cmpPosition || !cmpPosition.IsInWorld())
				continue;

			const pos = cmpPosition.GetPosition();
			if (pos.y + drownDepth >= targetWaterLevel)
				continue;

			const cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
			if (!cmpIdentity)
				continue;

			const templateName = cmpTemplateManager.GetCurrentTemplateName(ent);

			// Animals and units drown
			const cmpHealth = Engine.QueryInterface(ent, IID_Health);
			if (cmpHealth && cmpIdentity.HasClass("Organic"))
			{
				cmpHealth.Kill();
				continue;
			}

			// Resources and buildings become actors
			// Do not use ChangeEntityTemplate for performance and
			// because we don't need nor want the effects of MT_EntityRenamed

			const cmpVisualActor = Engine.QueryInterface(ent, IID_Visual);
			if (!cmpVisualActor)
				continue;

			const height = cmpPosition.GetHeightOffset();
			const rot = cmpPosition.GetRotation();

			const actorTemplate = cmpTemplateManager.GetTemplate(templateName).VisualActor.Actor;
			const seed = cmpVisualActor.GetActorSeed();
			Engine.DestroyEntity(ent);

			const newEnt = Engine.AddEntity("actor|" + actorTemplate);
			Engine.QueryInterface(newEnt, IID_Visual).SetActorSeed(seed);

			const cmpNewPos = Engine.QueryInterface(newEnt, IID_Position);
			cmpNewPos.JumpTo(pos.x, pos.z);
			cmpNewPos.SetHeightOffset(height);
			cmpNewPos.SetXZRotation(rot.x, rot.z);
			cmpNewPos.SetYRotation(rot.y);
		}
	};

	Trigger.prototype.RiseWaterLevelStep = (finalWaterLevel) =>
	{
		const waterManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_WaterManager);
		const nextExpectedWaterLevel = waterManager.GetWaterLevel() + deltaWaterLevel;

		if (nextExpectedWaterLevel >= finalWaterLevel)
		{
			riseWaterLevel(finalWaterLevel);
			return;
		}

		riseWaterLevel(nextExpectedWaterLevel);
		Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger).DoAfterDelay(deltaTime * 1000,
			"RiseWaterLevelStep", finalWaterLevel);
	};

	Trigger.prototype.DisplayWarning = () =>
	{
		Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface).AddTimeNotification(
			{ "message": warning });
	};

	if (InitAttributes.settings.waterLevel === "Shallow")
		return;

	const currentWaterLevel = Engine.QueryInterface(SYSTEM_ENTITY, IID_WaterManager).GetWaterLevel();

	if (InitAttributes.settings.waterLevel === "Deep")
	{
		riseWaterLevel(currentWaterLevel + 1);
		return;
	}

	const cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	const schedule = (targetWaterLevel, riseStartTime) =>
	{
		cmpTrigger.DoAfterDelay(riseStartTime * 1000, "RiseWaterLevelStep", targetWaterLevel);
		cmpTrigger.DoAfterDelay((riseStartTime - warningDuration) * 1000, "DisplayWarning");
	};

	schedule(currentWaterLevel + 1, 260);
	schedule(currentWaterLevel + 5, 1560);
}
