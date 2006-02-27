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

	if( shouldUpdateStat( "actions" ) )
		refreshCommandButtons();
}

// ====================================================================
