// TODO: could be worth putting this in json files someday
const g_EffectTypes = ["Damage", "Capture", "GiveStatus"];
const g_EffectReceiver = {
	"Damage": {
		"IID": "IID_Health",
		"method": "TakeDamage"
	},
	"Capture": {
		"IID": "IID_Capturable",
		"method": "Capture"
	},
	"GiveStatus": {
		"IID": "IID_StatusEffectsReceiver",
		"method": "GiveStatus"
	}
};
