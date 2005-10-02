#include "precompiled.h"

#include "ogl.h"
#include "Vector3D.h"
#include "Vector4D.h"
#include "ps/CLogger.h"
#include "graphics/Color.h"
#include "graphics/Model.h"
#include "renderer/ModelRData.h"
#include "renderer/ModelDefRData.h"


#define LOG_CATEGORY "graphics"


// Shared list of all submitted models this frame, sorted by CModelDef
CModelDefRData* CModelDefRData::m_Submissions = 0;

CModelDefRData::CModelDefRData(CModelDef* mdef)
	: m_ModelDef(mdef)
{
	m_SubmissionNext = 0;
	m_SubmissionSlots = 0;
	
	Build();
}

CModelDefRData::~CModelDefRData()
{
}


// Create and upload shared vertex arrays
void CModelDefRData::Build()
{
}

// Submit one model.
// Models are sorted into a hash-table to avoid ping-ponging between
// different render states later on.
void CModelDefRData::Submit(CModelRData* data)
{
	debug_assert(data->GetModel()->GetModelDef()->GetRenderData() == this);
	
	if (!m_SubmissionSlots)
	{
		m_SubmissionNext = m_Submissions;
		m_Submissions = this;
	}
	
	Handle htex = data->GetModel()->GetTexture()->GetHandle();
	uint idx;
	
	for(idx = 0; idx < m_SubmissionSlots; ++idx)
	{
		CModelRData* in = m_SubmissionModels[idx];
		
		if (in->GetModel()->GetTexture()->GetHandle() == htex)
			break;
	}
	
	if (idx >= m_SubmissionSlots)
	{
		++m_SubmissionSlots;
		if (m_SubmissionSlots > m_SubmissionModels.size())
		{
			m_SubmissionModels.push_back(0);
			debug_assert(m_SubmissionModels.size() == m_SubmissionSlots);
		}
		m_SubmissionModels[idx] = 0;
	}

	data->m_SubmissionNext = m_SubmissionModels[idx];
	m_SubmissionModels[idx] = data;
}


// Clear all submissions for this CModelDef
// Do not shrink the submissions array immediately to avoid unnecessary
// allocate/free roundtrips through memory management.
void CModelDefRData::ClearSubmissions()
{
	static uint mostslots = 1;
	if (m_SubmissionSlots > mostslots)
	{
		mostslots = m_SubmissionSlots;
		debug_printf("CModelDefRData: SubmissionSlots maximum: %u\n", mostslots);
	}
	m_SubmissionSlots = 0;
}
