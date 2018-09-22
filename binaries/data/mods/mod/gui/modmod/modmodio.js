function downloadModsButton()
{
	initTerms({
		"Disclaimer": {
			"title": translate("Download Mods"),
			"file": translate("You are about to connect to the mod.io online service. This provides easy access to community-made mods, but is not under the control of Wildfire Games.\n\nWhile we have taken care to make this secure, we cannot guarantee with absolute certainty that this is not a security risk.\n\nDo you really want to connect?"),
			"config": "modio.disclaimer",
			"accepted": false,
			"callback": openModIo,
			"urlButtons": [
				{
					"caption": translate("mod.io Terms"),
					"url": "https://mod.io/terms"
				},
				{
					"caption": translate("mod.io Privacy Policy"),
					"url": "https://mod.io/privacy"
				}
			]
		}
	});

	openTerms("Disclaimer");
}

function openModIo(data)
{
	if (data.accepted)
		Engine.PushGuiPage("page_modio.xml", {
			"callback": "initMods"
		});
}
