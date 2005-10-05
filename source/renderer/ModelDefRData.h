#ifndef __MODELDEFRDATA_H__
#define __MODELDEFRDATA_H__

#include "graphics/ModelDef.h"
#include "renderer/ModelRData.h"
#include "renderer/VertexArray.h"


class CModelRData;

// Maintain rendering data that can be shared across models
// that are based on the same model definition.
class CModelDefRData : public CSharedRenderData
{
	friend class CModelRData;
	
public:
	CModelDefRData(CModelDef* mdef);
	virtual ~CModelDefRData();

	// Setup shared vertex arrays required by streamflags
	void PrepareStream(uint streamflags);
	
	// Submit one model
	void Submit(CModelRData* data);
	// Clear all submissions for this CModelDef
	void ClearSubmissions();

private:
	// Build and upload vertex arrays
	void Build();

private:
	CModelDef* m_ModelDef;
	
	VertexArray m_Array;
	VertexArray::Attribute m_UV; // only used in RP_FIXED
	
	CModelDefRData* m_SubmissionNext;
	uint m_SubmissionSlots;
	std::vector<CModelRData*> m_SubmissionModels;
	static CModelDefRData* m_Submissions;
};


#endif // __MODELDEFRDATA_H__
