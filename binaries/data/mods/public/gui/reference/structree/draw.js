/**
 * Functions used to collate the contents of a tooltip.
 */
var g_StructreeTooltipFunctions = [
	getEntityNamesFormatted,
	getEntityCostTooltip,
	getEntityTooltip,
	getAurasTooltip
].concat(g_StatsFunctions);

/**
 * Draw the structree
 *
 * (Actually resizes and changes visibility of elements, and populates text)
 */
function draw()
{
	// Set basic state (positioning of elements mainly), but only once
	if (!Object.keys(g_DrawLimits).length)
		predraw();

	let leftMargin = Engine.GetGUIObjectByName("tree_display").size.left;
	let defWidth = 96;
	let defMargin = 4;

	let phaseList = g_ParsedData.phaseList;

	Engine.GetGUIObjectByName("civEmblem").sprite = "stretched:" + g_CivData[g_SelectedCiv].Emblem;
	Engine.GetGUIObjectByName("civName").caption = g_CivData[g_SelectedCiv].Name;
	Engine.GetGUIObjectByName("civHistory").caption = g_CivData[g_SelectedCiv].History;

	let i = 0;
	for (let pha of phaseList)
	{
		let prodBarWidth = 0;
		let s = 0;
		let y = 0;

		for (let stru of g_BuildList[g_SelectedCiv][pha])
		{
			let thisEle = Engine.GetGUIObjectByName("phase["+i+"]_struct["+s+"]");
			if (thisEle === undefined)
			{
				error("\""+g_SelectedCiv+"\" has more structures in phase " +
				      pha + " than can be supported by the current GUI layout");
				break;
			}

			let c = 0;
			let rowCounts = [];
			stru = g_ParsedData.structures[stru];

			Engine.GetGUIObjectByName("phase["+i+"]_struct["+s+"]_icon").sprite =
				"stretched:session/portraits/"+stru.icon;

			Engine.GetGUIObjectByName("phase["+i+"]_struct["+s+"]_icon").tooltip =
				compileTooltip(stru);

			Engine.GetGUIObjectByName("phase["+i+"]_struct["+s+"]_name").caption =
				translate(stru.name.specific);

			setViewerOnPress("phase["+i+"]_struct["+s+"]_icon", stru.name.internal);
			thisEle.hidden = false;

			for (let r in g_DrawLimits[pha].prodQuant)
			{
				let p = 0;
				r = +r; // force int
				let prod_pha = phaseList[phaseList.indexOf(pha) + r];

				if (stru.production.units[prod_pha])
					for (let prod of stru.production.units[prod_pha])
					{
						prod = g_ParsedData.units[prod];
						if (!drawProdIcon(i, s, r, p, prod))
							break;
						++p;
					}

				if (stru.upgrades[prod_pha])
					for (let upgrade of stru.upgrades[prod_pha])
					{
						if (!drawProdIcon(i, s, r, p, upgrade))
							break;
						++p;
					}

				if (stru.wallset && prod_pha == pha)
				{
					if (!drawProdIcon(i, s, r, p, stru.wallset.tower))
						break;
					++p;
				}

				if (stru.production.techs[prod_pha])
					for (let prod of stru.production.techs[prod_pha])
					{
						prod = clone(basename(prod).startsWith("phase") ?
							g_ParsedData.phases[prod] :
							g_ParsedData.techs[g_SelectedCiv][prod]);

						for (let res in stru.techCostMultiplier)
							if (prod.cost[res])
								prod.cost[res] *= stru.techCostMultiplier[res];

						if (!drawProdIcon(i, s, r, p, prod))
							break;

						++p;
					}

				rowCounts[r] = p;

				if (p>c)
					c = p;

				hideRemaining("phase["+i+"]_struct["+s+"]_row["+r+"]", p);
			}

			let size = thisEle.size;
			size.left = y;
			size.right = size.left + ((c*24 < defWidth) ? defWidth : c*24) + 4;
			y = size.right + defMargin;
			thisEle.size = size;

			let eleWidth = size.right - size.left;
			let r;
			for (r in rowCounts)
			{
				let wid = rowCounts[r] * 24 - 4;
				let phaEle = Engine.GetGUIObjectByName("phase["+i+"]_struct["+s+"]_row["+r+"]");
				size = phaEle.size;
				size.left = (eleWidth - wid)/2;
				phaEle.size = size;
			}
			++r;
			hideRemaining("phase["+i+"]_struct["+s+"]_rows", r);
			++s;
			prodBarWidth += eleWidth + defMargin;
		}

		hideRemaining("phase["+i+"]", s);

		// Resize phase bars
		for (let j = 1; j < phaseList.length - i; ++j)
		{
			let prodBar = Engine.GetGUIObjectByName("phase["+i+"]_bar["+(j-1)+"]");
			let prodBarSize = prodBar.size;
			prodBarSize.right = leftMargin + prodBarWidth;
			prodBar.size = prodBarSize;
		}

		++i;
	}

	let t = 0;
	for (let trainer of g_TrainList[g_SelectedCiv])
	{
		let thisEle = Engine.GetGUIObjectByName("trainer["+t+"]");
		if (thisEle === undefined)
		{
			error("\""+g_SelectedCiv+"\" has more unit trainers than can be supported by the current GUI layout");
			break;
		}

		trainer = g_ParsedData.units[trainer];
		Engine.GetGUIObjectByName("trainer["+t+"]_icon").sprite = "stretched:session/portraits/"+trainer.icon;
		Engine.GetGUIObjectByName("trainer["+t+"]_icon").tooltip = compileTooltip(trainer);
		Engine.GetGUIObjectByName("trainer["+t+"]_name").caption = translate(trainer.name.specific);
		setViewerOnPress("trainer["+t+"]_icon", trainer.name.internal);
		thisEle.hidden = false;

		let p = 0;
		if (trainer.production)
			for (let prodType in trainer.production)
				for (let prod of trainer.production[prodType])
				{
					switch (prodType)
					{
					case "units":
						prod = g_ParsedData.units[prod];
						break;
					case "techs":
						prod = clone(g_ParsedData.techs[g_SelectedCiv][prod]);
						for (let res in trainer.techCostMultiplier)
							if (prod.cost[res])
								prod.cost[res] *= trainer.techCostMultiplier[res];
						break;
					default:
						continue;
					}
					if (!drawProdIcon(null, t, null, p, prod))
						break;
					++p;
				}

		if (trainer.upgrades)
			for (let upgrade of trainer.upgrades)
			{
				if (!drawProdIcon(null, t, null, p, upgrade))
					break;
				++p;
			}

		hideRemaining("trainer["+t+"]_row", p);

		let size = thisEle.size;
		size.right = size.left + Math.max(p*24, defWidth) + 4;
		thisEle.size = size;

		let eleWidth = size.right - size.left;
		let wid = p * 24 - 4;
		let phaEle = Engine.GetGUIObjectByName("trainer["+t+"]_row");
		size = phaEle.size;
		size.left = (eleWidth - wid)/2;
		phaEle.size = size;
		++t;
	}
	hideRemaining("trainers", t);

	let size = Engine.GetGUIObjectByName("display_tree").size;
	size.right = t > 0 ? -124 : -4;
	Engine.GetGUIObjectByName("display_tree").size = size;

	Engine.GetGUIObjectByName("display_trainers").hidden = t === 0;
}

/**
 * Draws production icons.
 *
 * These are the small icons on the gray bars.
 *
 * @param {string} phase - The phase that the parent entity (the entity that
 *                         builds/trains/researches this entity) belongs to.
 * @param {number} parentID - Which parent entity it belongs to on the main phase rows.
 * @param {number} rowID - Which production row of the parent entity the production
 *                         icon sits on, if applicable.
 * @param {number} iconID - Which production icon to affect.
 * @param {object} template - The entity being produced.
 * @return {boolean} True is successfully drawn, False if no space left to draw.
 */
function drawProdIcon(phase, parentID, rowID, iconID, template)
{
	let prodEle = Engine.GetGUIObjectByName("phase["+phase+"]_struct["+parentID+"]_row["+rowID+"]_prod["+iconID+"]");

	if (phase === null)
		prodEle = Engine.GetGUIObjectByName("trainer["+parentID+"]_prod["+iconID+"]");

	if (prodEle === undefined)
	{
		error("The " + (phase === null ? "trainer units" : "structures") + " of \"" + g_SelectedCiv +
		      "\" have more production icons than can be supported by the current GUI layout");
		return false;
	}

	prodEle.sprite = "stretched:session/portraits/"+template.icon;
	prodEle.tooltip = compileTooltip(template);
	prodEle.hidden = false;
	setViewerOnPress(prodEle.name, template.name.internal);
	return true;
}

function compileTooltip(template)
{
	return buildText(template, g_StructreeTooltipFunctions) + "\n" + showTemplateViewerOnClickTooltip();
}

/**
 * Calculate row position offset (accounting for different number of prod rows per phase).
 *
 * @param {number} idx
 * @return {number}
 */
function getPositionOffset(idx)
{
	let phases = g_ParsedData.phaseList.length;

	let size = 92*idx; // text, image and offset
	size += 24 * (phases*idx - (idx-1)*idx/2); // phase rows (phase-currphase+1 per row)

	return size;
}

/**
 * Positions certain elements that only need to be positioned once
 * (as <repeat> does not position automatically).
 *
 * Also detects limits on what the GUI can display by iterating through the set
 * elements of the GUI. These limits are then used by draw().
 */
function predraw()
{
	let phaseList = g_ParsedData.phaseList;
	let initIconSize = Engine.GetGUIObjectByName("phase[0]_struct[0]_row[0]_prod[0]").size;

	let phaseCount = phaseList.length;
	let i = 0;
	for (let pha of phaseList)
	{
		let offset = getPositionOffset(i);
		// Align the phase row
		Engine.GetGUIObjectByName("phase["+i+"]").size = "8 16+" + offset + " 100% 100%";

		// Set phase icon
		let phaseIcon = Engine.GetGUIObjectByName("phase["+i+"]_phase");
		phaseIcon.size = "16 32+"+offset+" 48+16 48+32+"+offset;

		// Set initial prod bar size
		let j = 1;
		for (; j < phaseList.length - i; ++j)
		{
			let prodBar = Engine.GetGUIObjectByName("phase["+i+"]_bar["+(j-1)+"]");
			prodBar.size = "40 1+"+(24*j)+"+98+"+offset+" 0 1+"+(24*j)+"+98+"+offset+"+22";
		}
		// Hide remaining prod bars
		hideRemaining("phase["+i+"]_bars", j-1);

		let s = 0;
		let ele = Engine.GetGUIObjectByName("phase["+i+"]_struct["+s+"]");
		g_DrawLimits[pha] = {
			"structQuant": 0,
			"prodQuant": []
		};

		do
		{
			// Position production icons
			for (let r in phaseList.slice(phaseList.indexOf(pha)))
			{
				let p=1;
				let prodEle = Engine.GetGUIObjectByName("phase["+i+"]_struct["+s+"]_row["+r+"]_prod["+p+"]");

				do
				{
					let prodsize = prodEle.size;
					prodsize.left = (initIconSize.right+4) * p;
					prodsize.right = (initIconSize.right+4) * (p+1) - 4;
					prodEle.size = prodsize;

					p++;
					prodEle = Engine.GetGUIObjectByName("phase["+i+"]_struct["+s+"]_row["+r+"]_prod["+p+"]");
				} while (prodEle !== undefined);

				// Set quantity of productions in this row
				g_DrawLimits[pha].prodQuant[r] = p;

				// Position the prod row
				Engine.GetGUIObjectByName("phase["+i+"]_struct["+s+"]_row["+r+"]").size = "4 100%-"+24*(phaseCount - i - r)+" 100%-4 100%";
			}

			// Hide unused struct rows
			for (let r = phaseCount - i; r < phaseCount; ++r)
				Engine.GetGUIObjectByName("phase["+i+"]_struct["+s+"]_row["+r+"]").hidden = true;

			let size = ele.size;
			size.bottom += Object.keys(g_DrawLimits[pha].prodQuant).length*24;
			ele.size = size;

			s++;
			ele = Engine.GetGUIObjectByName("phase["+i+"]_struct["+s+"]");
		} while (ele !== undefined);

		// Set quantity of structures in each phase
		g_DrawLimits[pha].structQuant = s;
		++i;
	}
	hideRemaining("phase_rows", i);
	hideRemaining("phase_ident", i);

	let t = 0;
	let ele = Engine.GetGUIObjectByName("trainer["+t+"]");
	g_DrawLimits.trainer = {
		"trainerQuant": 0,
		"prodQuant": 0
	};

	let x = 4;
	do
	{
		let p = 0;
		let prodEle = Engine.GetGUIObjectByName("trainer["+t+"]_prod["+p+"]");
		do
		{
			let prodsize = prodEle.size;
			prodsize.left = (initIconSize.right+4) * p;
			prodsize.right = (initIconSize.right+4) * (p+1) - 4;
			prodEle.size = prodsize;

			p++;
			prodEle = Engine.GetGUIObjectByName("trainer["+t+"]_prod["+p+"]");
		} while (prodEle !== undefined);
		Engine.GetGUIObjectByName("trainer["+t+"]_row").size = "4 100%-24 100%-4 100%";
		g_DrawLimits.trainer.prodQuant = p;

		let size = ele.size;
		size.top += x;
		size.bottom += x + 24;
		x += size.bottom - size.top + 8;
		ele.size = size;

		t++;
		ele = Engine.GetGUIObjectByName("trainer["+t+"]");

	} while (ele !== undefined);

	g_DrawLimits.trainer.trainerQuant = t;
}

function drawPhaseIcons()
{
	for (let i = 0; i < g_ParsedData.phaseList.length; ++i)
	{
		drawPhaseIcon("phase["+i+"]_phase", i);

		for (let j = 1; j < g_ParsedData.phaseList.length - i; ++j)
			drawPhaseIcon("phase["+i+"]_bar["+(j-1)+"]_icon", j+i);
	}
}

/**
 * @param {string} guiObjectName
 * @param {number} phaseIndex
 */
function drawPhaseIcon(guiObjectName, phaseIndex)
{
	let phaseName = g_ParsedData.phaseList[phaseIndex];
	let prodPhaseTemplate = g_ParsedData.phases[phaseName + "_" + g_SelectedCiv] || g_ParsedData.phases[phaseName];

	let phaseIcon = Engine.GetGUIObjectByName(guiObjectName);
	phaseIcon.sprite = "stretched:session/portraits/" + prodPhaseTemplate.icon;
	phaseIcon.tooltip = getEntityNamesFormatted(prodPhaseTemplate);
}
