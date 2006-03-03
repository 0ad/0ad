/*
	DESCRIPTION	: Functions for the "Status Pane" section of the session GUI.
	NOTES		: 
*/

// ====================================================================

function refreshStatusPane()
{
	if ( shouldUpdateStat ( "traits.id.icon" ) )
	{
		// Update portrait
		if (selection[0].traits.id.icon)
			setPortrait ("snStatusPanePortrait", selection[0].traits.id.icon,
				toTitleCase(selection[0].traits.id.civ_code), selection[0].traits.id.icon_cell);
	}
	
	// Update portrait tooltip.
	if ( shouldUpdateStat ( "traits.id.generic" ) )
	{
		getGUIObjectByName ("snStatusPanePortrait").tooltip = selection[0].traits.id.generic;
	}
	
	// Update hitpoint bar.
	if ( (shouldUpdateStat ( "traits.health.max" ) || shouldUpdateStat ( "traits.health.curr" )) )
	{
		guiObject = getGUIObjectByName ("snStatusPaneHealthBar");
		if (selection[0].traits.health.max && selection[0].traits.health.max != 0)
		{
			guiObject.caption = (selection[0].traits.health.curr * 100) / selection[0].traits.health.max;
			guiObject.hidden = false;
		}
		else
		{
			guiObject.hidden = true;		
		}
		// Update text.
		getGUIObjectByName ("snStatusPaneHealthBarText").caption =
			"[font=verdana8][color=white]" + selection[0].traits.health.curr + "[/color][/font]";
	}
	
	// Update stamina bar.
	if ( (shouldUpdateStat ( "traits.stamina.max" ) || shouldUpdateStat ( "traits.stamina.curr" ))  )
	{
		guiObject = getGUIObjectByName ("snStatusPaneStaminaBar");
		if (selection[0].traits.stamina.max && selection[0].traits.stamina.max != 0)
		{
			guiObject.caption = (selection[0].traits.stamina.curr * 100) / selection[0].traits.stamina.max;
			guiObject.hidden = false;
		}
		else
		{
			guiObject.hidden = true;		
		}
		// Update text.
		getGUIObjectByName ("snStatusPaneStaminaBarText").caption =
			"[font=verdana8][color=white]" + selection[0].traits.stamina.curr + "[/color][/font]";		
	}	
	
	// Update unit text panel.
	if ( shouldUpdateStat ("player") || shouldUpdateStat ( "traits.id.civ" ) || shouldUpdateStat ( "traits.id.generic" ) || shouldUpdateStat ( "traits.id.specific" ) )
	{
		textCaption = "";
		// (Is there some way to grab the player colour RGB values? It'd be good if we could put the player name in player colour.)
		textCaption += "[font=verdana10][color=white]" + getCurrItemValue ("pgProfileName") + "[/color][/font]\n";
		textCaption += "[font=verdana10][color=white]" + selection[0].traits.id.civ + "[/color][/font]\n";
		textCaption += "[font=verdana10][color=white]" + selection[0].traits.id.specific + "[/color][/font]\n";
		textCaption += "[font=optimus12][color=gold]" + selection[0].traits.id.generic + "[/color][/font]\n";

		getGUIObjectByName ("snStatusPaneText").caption = textCaption;
	}
	
	// Update rank icon.
	if ( shouldUpdateStat ( "traits.promotion" ) )
	{
		rankObject = getGUIObjectByName ("snStatusPaneRank");

		// Don't show a rank icon for Basic or unranked units.
		if (selection[0].traits.promotion.rank > 1)
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
		// Update Supply/Garrison
		guiObject = getGUIObjectByName("snStatusPaneGarrison");
		guiObject.caption = '';

		if (selection[0].traits.garrison)
		{
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
				getGUIObjectByName ("snStatusPaneSupplyIcon").cell_id = cellGroup["Resource"][selection[0].traits.supply.type];
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

	// Refresh command buttons.
	if( shouldUpdateStat( "actions" ) )
		refreshCommandButtons();
}

// ====================================================================
