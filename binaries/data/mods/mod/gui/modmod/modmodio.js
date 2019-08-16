function downloadModsButton()
{
	initTerms({
		"Disclaimer": {
			"title": translate("Disclaimer"),
			"file": "gui/modio/Disclaimer.txt",
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
		Engine.PushGuiPage("page_modio.xml", {}, initMods);
}
