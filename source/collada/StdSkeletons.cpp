#include "precompiled.h"

#include "StdSkeletons.h"

namespace
{
	const char* standardBoneNames[] = {
		/* */ "Bip01",
		/* */   "Bip01_Pelvis",
		/* */     "Bip01_Spine",
		/* */       "Bip01_Spine1",
		/* */         "Bip01_Neck",
		/* */           "Bip01_Head",
		/* */             "Bip01_HeadNub",
		/* */             "Bip01_L_Clavicle",
		/* */               "Bip01_L_UpperArm",
		/* */                 "Bip01_L_Forearm",
		/* */                   "Bip01_L_Hand",
		/* */                     "Bip01_L_Finger0",
		/* */                       "Bip01_L_Finger0Nub",
		/* */             "Bip01_R_Clavicle",
		/* */               "Bip01_R_UpperArm",
		/* */                 "Bip01_R_Forearm",
		/* */                   "Bip01_R_Hand",
		/* */                     "Bip01_R_Finger0",
		/* */                       "Bip01_R_Finger0Nub",
		/* */       "Bip01_L_Thigh",
		/* */         "Bip01_L_Calf",
		/* */           "Bip01_L_Foot",
		/* */             "Bip01_L_Toe0",
		/* */               "Bip01_L_Toe0Nub",
		/* */       "Bip01_R_Thigh",
		/* */         "Bip01_R_Calf",
		/* */           "Bip01_R_Foot",
		/* */             "Bip01_R_Toe0",
		/* */               "Bip01_R_Toe0Nub",
		// (the above comments just stop the indentation being dropped by
		// automatic code-formatting things...)
		NULL
	};
}

namespace StdSkeletons
{
	int GetBoneCount()
	{
		int i = 0;
		while (standardBoneNames[i] != NULL)
			++i;
		return i;
	}

	int FindStandardBoneID(const std::string& name)
	{
		for (int i = 0; standardBoneNames[i] != NULL; ++i)
			if (standardBoneNames[i] == name)
				return i;
		return -1;
	}
}
