/*
	DESCRIPTION	: Functions for the "Status Pane" section of the session GUI.
	NOTES		: 
*/

// ====================================================================

function refreshStatusPane()
{
	// Update civilisation emblem.
	if ( selectionChanged )
	{
		var emblemObject = getGUIObjectByName ("snStatusPaneEmblem");
		if (selection[0].traits.id.civ_code != "gaia")
			emblemObject.sprite = "snStatusPaneEmblem" + toTitleCase (selection[0].traits.id.civ_code);
		else
			emblemObject.sprite = "";
	}

	if ( shouldUpdateStat ( "traits.id.icon" ) )
	{
		// Update portrait
		if (validProperty("selection[0].traits.id.icon"))
			setPortrait ("snStatusPanePortrait", selection[0].traits.id.icon,
				toTitleCase(selection[0].traits.id.civ_code), selection[0].traits.id.icon_cell);
	}
	
	// Update portrait tooltip.
	if ( shouldUpdateStat ( "traits.id.generic" ) )
	{
		getGUIObjectByName ("snStatusPanePortrait").tooltip = selection[0].traits.id.generic;
	}
	
	// Update hitpoint bar.
	if ( selectionChanged || shouldUpdateStat ( "traits.health.max" ) || shouldUpdateStat ( "traits.health.curr" ) )
	{
		var barObject = getGUIObjectByName ("snStatusPaneHealthBar");
		var textObject = getGUIObjectByName ("snStatusPaneHealthBarText");
		if (selection[0].traits.health.max && selection[0].traits.health.max != 0)
		{
			barObject.caption = (selection[0].traits.health.curr * 100) / selection[0].traits.health.max;
			barObject.hidden = false;
			textObject.caption = "[font=verdana8][color=white]" + selection[0].traits.health.curr + "[/color][/font]";
		}
		else
		{
			barObject.hidden = true;		
			textObject.caption = "";
		}
	}
	
	// Update stamina bar.
	if ( selectionChanged || shouldUpdateStat ( "traits.stamina.max" ) || shouldUpdateStat ( "traits.stamina.curr" ) )
	{
		var barObject = getGUIObjectByName ("snStatusPaneStaminaBar");
		var textObject = getGUIObjectByName ("snStatusPaneStaminaBarText");
		if (selection[0].traits.stamina.max && selection[0].traits.stamina.max != 0)
		{
			barObject.caption = (selection[0].traits.stamina.curr * 100) / selection[0].traits.stamina.max;
			barObject.hidden = false;
			textObject.caption = "[font=verdana8][color=white]" + selection[0].traits.stamina.curr + "[/color][/font]";
		}
		else
		{
			barObject.hidden = true;		
			textObject.caption = "";
		}
	}	
	
	// Update unit text panel.
	if ( shouldUpdateStat ("player") || shouldUpdateStat ( "traits.id.civ" ) || shouldUpdateStat ( "traits.id.generic" ) || shouldUpdateStat ( "traits.id.specific" ) )
	{
		var textCaption = "";
textCaption += '[font=verdana10][color="' + Math.round(selection[0].player.getColour().r*255) + ' ' + Math.round(selection[0].player.getColour().g*255) + ' ' + Math.round(selection[0].player.getColour().b*255) + '"]' + selection[0].player.name + '[/color][/font]\n';		
		textCaption += "[font=verdana10][color=white]" + selection[0].traits.id.civ + "[/color][/font]\n";
		textCaption += "[font=verdana10][color=white]" + selection[0].traits.id.specific + "[/color][/font]\n";
		textCaption += "[font=optimus14b][color=gold]" + selection[0].traits.id.generic + "[/color][/font]";
		getGUIObjectByName ("snStatusPaneText").caption = textCaption;
	}
	
	// Update rank icon.
	if ( shouldUpdateStat ( "traits.promotion" ) )
	{
		var rankObject = getGUIObjectByName ("snStatusPaneRank");

		// Don't show a rank icon for Basic or unranked units.
		if (selection[0].traits.promotion && selection[0].traits.promotion.rank > 1)
		{
			rankObject.cell_id = selection[0].traits.promotion.rank-2;
			rankObject.tooltip = "Next Promotion: " + selection[0].traits.promotion.curr + "/" + selection[0].traits.promotion.req;
			rankObject.hidden = false;
		}
		else
			rankObject.hidden = true;
	}
	
	// Update garrison capacity.
	if( shouldUpdateStat( "traits.garrison" ) )
	{
		var guiObject = getGUIObjectByName("snStatusPaneGarrison");
		guiObject.caption = '';

		if (selection[0].traits.garrison)
		{
			// Set garrison icon.
			getGUIObjectByName ("snStatusPaneGarrisonIcon").cell_id = cellGroup["Garrison"]["garrison"].id;		
			if (selection[0].traits.garrison.curr && selection[0].traits.garrison.max)
			{
				guiObject.caption += '[color="blue"]' + selection[0].traits.garrison.curr + '/' + selection[0].traits.garrison.max + '[/color] ';
			}
			guiObject.hidden = false;
			getGUIObjectByName ("snStatusPaneGarrisonIcon").hidden = false;
		}
		else
		{
			guiObject.hidden = true;
			getGUIObjectByName ("snStatusPaneGarrisonIcon").hidden = true;
		}
	}
	
	if( shouldUpdateStat( "traits.supply" ) )
	{
		guiObject = getGUIObjectByName("snStatusPaneSupply");
		guiObject.caption = '';

		if (selection[0].traits.supply)
		{
			if (selection[0].traits.supply.curr && selection[0].traits.supply.max && selection[0].traits.supply.type)
			{
				// Set resource icon.
				getGUIObjectByName ("snStatusPaneSupplyIcon").cell_id = cellGroup["Resource"][selection[0].traits.supply.type].id;
				// Special case for infinity.
				if (selection[0].traits.supply.curr == "0" && selection[0].traits.supply.max == "0")
					guiObject.caption += '[color="brown"] [icon="iconInfinity"] [/color] ';
				else
					guiObject.caption += '[color="brown"]' + selection[0].traits.supply.curr + '/' + selection[0].traits.supply.max + '[/color] ';
			}
			guiObject.hidden = false;
			getGUIObjectByName ("snStatusPaneSupplyIcon").hidden = false;
		}
		else
		{
			guiObject.hidden = true;
			getGUIObjectByName ("snStatusPaneSupplyIcon").hidden = true;
		}
	}	
	
	// Update statistic icons.
	statCurr = 1;
	if (validProperty ("selection[0].actions.attack.melee.damage"))
		updateStat ("snStatusPaneStat_", "Attack", "rating", selection[0].actions.attack.melee.damage);		
	if (validProperty ("selection[0].actions.attack.melee.range"))
		updateStat ("snStatusPaneStat_", "Statistic", "range", selection[0].actions.attack.melee.range);		
	if (validProperty ("selection[0].actions.attack.ranged.damage"))
		updateStat ("snStatusPaneStat_", "Attack", "rating", selection[0].actions.attack.ranged.damage);		
	if (validProperty ("selection[0].actions.attack.ranged.range"))
		updateStat ("snStatusPaneStat_", "Statistic", "range", selection[0].actions.attack.ranged.range);		
	if (validProperty ("selection[0].traits.armour.value"))
		updateStat ("snStatusPaneStat_", "Armour", "rating", selection[0].traits.armour.value);	
	if (validProperty ("selection[0].traits.vision.los"))
		updateStat ("snStatusPaneStat_", "Statistic", "vision", selection[0].traits.vision.los);	

	// Refresh command buttons.
	refreshCommandButtons();
}

// ====================================================================

function updateStat (baseName, cellSheet, cell, statistic)
{
	var textStat = getGUIObjectByName (baseName + statCurr);
	if( !textStat ) 
	{
		//console.write("No textStat for " + baseName + " " + statCurr);
		return;
	}
	textStat.sprite = "snIconSheet" + cellSheet;
	textStat.cell_id = cellGroup[cellSheet][cell].id;
	textStat.tooltip = cellGroup[cellSheet][cell].name;
	var iconStat = getGUIObjectByName (baseName + (statCurr + 1));
	
	if (!isNaN(statistic)) {
		statistic = Math.ceil(statistic);
	}
	
	iconStat.caption = statistic;
	iconStat.tooltip = cellGroup[cellSheet][cell].name + ": " + statistic + ".";
	statCurr = (statCurr + 2);
}

// ====================================================================
