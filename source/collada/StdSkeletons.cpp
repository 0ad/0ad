#include "precompiled.h"

#include "StdSkeletons.h"

namespace
{
	const char* standardBoneNames0[] = {
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
// 		NULL
// 	};
// (TODO (important): do this stuff properly)
// 	const char* standardBoneNames1[] = {
		/* */ "Biped_GlobalSRT",
		/* */   "Biped_Spineroot",
		/* */     "Biped_Spine01",
		/* */       "Biped_Spine02",
		/* */         "Biped_Spine03",
		/* */     "Biped_Spineeffector",
		/* */       "Biped_Lshoulderroot",
		/* */         "Biped_Lshoulder",
		/* */         "Biped_Lshouldereffector",
		/* */           "Biped_Larmroot",
		/* */             "Biped_Lbicept",
		/* */               "Biped_Lforearm",
		/* */             "Biped_Larmupvector",
		/* */       "Biped_Rshoulderroot",
		/* */         "Biped_Rshoulder",
		/* */         "Biped_Rshouldereffector",
		/* */           "Biped_Rarmroot",
		/* */             "Biped_Rbicept",
		/* */               "Biped_Rforearm",
		/* */             "Biped_Rarmupvector",
		/* */       "Biped_neckroot",
		/* */         "Biped_neck",
		/* */           "Biped_head",
		/* */         "Biped_headeffector",
		/* */     "Biped_Llegroot",
		/* */       "Biped_Lthigh",
		/* */         "Biped_Lshin",
		/* */     "Biped_Rlegroot",
		/* */       "Biped_Rthigh",
		/* */         "Biped_Rshin",
		/* */     "Biped_Llegupvector",
		/* */     "Biped_Rlegupvector",
		/* */   "Biped_Larmeffector",
		/* */     "Biped_Lhandroot",
		/* */       "Biped_Lhand",
		/* */         "Biped_Lfingers",
		/* */       "Biped_Lhandeffector",
		/* */   "Biped_Llegeffector",
		/* */     "Biped_Lfooteffector",
		/* */       "Biped_Lfoot",
		/* */         "Biped_Ltoe",
		/* */       "Biped_Ltoeeffector",
		/* */   "Biped_Rarmeffector",
		/* */     "Biped_Rhandroot",
		/* */       "Biped_Rhand",
		/* */         "Biped_Rfingers",
		/* */       "Biped_Rhandeffector",
		/* */   "Biped_Rlegeffector",
		/* */     "Biped_Rfootroot",
		/* */       "Biped_Rfoot",
		/* */         "Biped_Rtoe",
		/* */       "Biped_Rtoeeffector",
		NULL
	};

}

namespace StdSkeletons
{
	int GetBoneCount()
	{
		int i = 0;
		while (standardBoneNames0[i] != NULL)
			++i;
		return i;
	}

	int FindStandardBoneID(const std::string& name)
	{
		for (int i = 0; standardBoneNames0[i] != NULL; ++i)
			if (standardBoneNames0[i] == name)
				return i;
		return -1;
	}
}
