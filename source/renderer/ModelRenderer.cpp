/* Copyright (C) 2013 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "lib/allocators/allocator_adapters.h"
#include "lib/allocators/arena.h"
#include "lib/ogl.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"

#include "ps/CLogger.h"
#include "ps/Profile.h"

#include "graphics/Color.h"
#include "graphics/LightEnv.h"
#include "graphics/Material.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"
#include "graphics/ShaderManager.h"
#include "graphics/TextureManager.h"

#include "renderer/MikktspaceWrap.h"
#include "renderer/ModelRenderer.h"
#include "renderer/ModelVertexRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/RenderModifiers.h"
#include "renderer/SkyManager.h"
#include "renderer/WaterManager.h"

#include <boost/weak_ptr.hpp>

#if ARCH_X86_X64
# include "lib/sysdep/arch/x86_x64/x86_x64.h"
#endif

///////////////////////////////////////////////////////////////////////////////////////////////
// ModelRenderer implementation

#if ARCH_X86_X64
static bool g_EnableSSE = false;
#endif

void ModelRenderer::Init()
{
#if ARCH_X86_X64
	if (x86_x64::Cap(x86_x64::CAP_SSE))
		g_EnableSSE = true;
#endif
}

// Helper function to copy object-space position and normal vectors into arrays.
void ModelRenderer::CopyPositionAndNormals(
		const CModelDefPtr& mdef,
		const VertexArrayIterator<CVector3D>& Position,
		const VertexArrayIterator<CVector3D>& Normal)
{
	size_t numVertices = mdef->GetNumVertices();
	SModelVertex* vertices = mdef->GetVertices();

	for(size_t j = 0; j < numVertices; ++j)
	{
		Position[j] = vertices[j].m_Coords;
		Normal[j] = vertices[j].m_Norm;
	}
}

// Helper function to transform position and normal vectors into world-space.
void ModelRenderer::BuildPositionAndNormals(
		CModel* model,
		const VertexArrayIterator<CVector3D>& Position,
		const VertexArrayIterator<CVector3D>& Normal)
{
	CModelDefPtr mdef = model->GetModelDef();
	size_t numVertices = mdef->GetNumVertices();
	SModelVertex* vertices=mdef->GetVertices();

	if (model->IsSkinned())
	{
		// boned model - calculate skinned vertex positions/normals

		// Avoid the noisy warnings that occur inside SkinPoint/SkinNormal in
		// some broken situations
		if (numVertices && vertices[0].m_Blend.m_Bone[0] == 0xff)
		{
			LOGERROR(L"Model %ls is boned with unboned animation", mdef->GetName().string().c_str());
			return;
		}

#if HAVE_SSE
		if (g_EnableSSE)
		{
			CModelDef::SkinPointsAndNormals_SSE(numVertices, Position, Normal, vertices, mdef->GetBlendIndices(), model->GetAnimatedBoneMatrices());
		}
		else
#endif
		{
			CModelDef::SkinPointsAndNormals(numVertices, Position, Normal, vertices, mdef->GetBlendIndices(), model->GetAnimatedBoneMatrices());
		}
	}
	else
	{
		PROFILE( "software transform" );
		// just copy regular positions, transform normals to world space
		const CMatrix3D& transform = model->GetTransform();
		const CMatrix3D& invtransform = model->GetInvTransform();
		for (size_t j=0; j<numVertices; j++)
		{
			transform.Transform(vertices[j].m_Coords,Position[j]);
			invtransform.RotateTransposed(vertices[j].m_Norm,Normal[j]);
		}
	}
}


// Helper function for lighting
void ModelRenderer::BuildColor4ub(
		CModel* model,
		const VertexArrayIterator<CVector3D>& Normal,
		const VertexArrayIterator<SColor4ub>& Color)
{
	PROFILE( "lighting vertices" );

	CModelDefPtr mdef = model->GetModelDef();
	size_t numVertices = mdef->GetNumVertices();
	const CLightEnv& lightEnv = g_Renderer.GetLightEnv();
	CColor shadingColor = model->GetShadingColor();

	for (size_t j=0; j<numVertices; j++)
	{
		RGBColor tempcolor = lightEnv.EvaluateUnitScaled(Normal[j]);
		tempcolor.X *= shadingColor.r;
		tempcolor.Y *= shadingColor.g;
		tempcolor.Z *= shadingColor.b;
		Color[j] = ConvertRGBColorTo4ub(tempcolor);
	}
}


void ModelRenderer::GenTangents(const CModelDefPtr& mdef, std::vector<float>& newVertices, bool gpuSkinning)
{
	MikkTSpace ms(mdef, newVertices, gpuSkinning);

	ms.generate();
}


// Copy UV coordinates
void ModelRenderer::BuildUV(
		const CModelDefPtr& mdef,
		const VertexArrayIterator<float[2]>& UV,
		int UVset)
{
	size_t numVertices = mdef->GetNumVertices();
	SModelVertex* vertices = mdef->GetVertices();

	for (size_t j=0; j < numVertices; ++j)
	{
		UV[j][0] = vertices[j].m_UVs[UVset * 2];
		UV[j][1] = 1.0-vertices[j].m_UVs[UVset * 2 + 1];
	}
}


// Build default indices array.
void ModelRenderer::BuildIndices(
		const CModelDefPtr& mdef,
		const VertexArrayIterator<u16>& Indices)
{
	size_t idxidx = 0;
	SModelFace* faces = mdef->GetFaces();

	for (size_t j = 0; j < mdef->GetNumFaces(); ++j) {
		SModelFace& face=faces[j];
		Indices[idxidx++]=face.m_Verts[0];
		Indices[idxidx++]=face.m_Verts[1];
		Indices[idxidx++]=face.m_Verts[2];
	}
}



///////////////////////////////////////////////////////////////////////////////////////////////
// ShaderModelRenderer implementation


/**
 * Internal data of the ShaderModelRenderer.
 *
 * Separated into the source file to increase implementation hiding (and to
 * avoid some causes of recompiles).
 */
struct ShaderModelRendererInternals
{
	ShaderModelRendererInternals(ShaderModelRenderer* r) : m_Renderer(r) { }

	/// Back-link to "our" renderer
	ShaderModelRenderer* m_Renderer;

	/// ModelVertexRenderer used for vertex transformations
	ModelVertexRendererPtr vertexRenderer;

	/// List of submitted models for rendering in this frame
	std::vector<CModel*> submissions;
};


// Construction/Destruction
ShaderModelRenderer::ShaderModelRenderer(ModelVertexRendererPtr vertexrenderer)
{
	m = new ShaderModelRendererInternals(this);
	m->vertexRenderer = vertexrenderer;
}

ShaderModelRenderer::~ShaderModelRenderer()
{
	delete m;
}

// Submit one model.
void ShaderModelRenderer::Submit(CModel* model)
{
	CModelDefPtr mdef = model->GetModelDef();
	CModelRData* rdata = (CModelRData*)model->GetRenderData();

	// Ensure model data is valid
	const void* key = m->vertexRenderer.get();
	if (!rdata || rdata->GetKey() != key)
	{
		rdata = m->vertexRenderer->CreateModelData(key, model);
		model->SetRenderData(rdata);
		model->SetDirty(~0u);
	}

	m->submissions.push_back(model);
}


// Call update for all submitted models and enter the rendering phase
void ShaderModelRenderer::PrepareModels()
{
	for (size_t i = 0; i < m->submissions.size(); ++i)
	{
		CModel* model = m->submissions[i];

 		CModelRData* rdata = static_cast<CModelRData*>(model->GetRenderData());
 		ENSURE(rdata->GetKey() == m->vertexRenderer.get());

		m->vertexRenderer->UpdateModelData(model, rdata, rdata->m_UpdateFlags);
		rdata->m_UpdateFlags = 0;
	}
}


// Clear the submissions list
void ShaderModelRenderer::EndFrame()
{
	m->submissions.clear();
}


// Helper structs for ShaderModelRenderer::Render():

struct SMRSortByDistItem
{
	size_t techIdx;
	CModel* model;
	float dist;
};

struct SMRBatchModel
{
	bool operator()(CModel* a, CModel* b)
	{
		if (a->GetModelDef() < b->GetModelDef())
			return true;
		if (b->GetModelDef() < a->GetModelDef())
			return false;

		if (a->GetMaterial().GetDiffuseTexture() < b->GetMaterial().GetDiffuseTexture())
			return true;
		if (b->GetMaterial().GetDiffuseTexture() < a->GetMaterial().GetDiffuseTexture())
			return false;

		return a->GetMaterial().GetStaticUniforms() < b->GetMaterial().GetStaticUniforms();
	}
};

struct SMRCompareSortByDistItem
{
	bool operator()(const SMRSortByDistItem& a, const SMRSortByDistItem& b)
	{
		// Prefer items with greater distance, so we draw back-to-front
		return (a.dist > b.dist);

		// (Distances will almost always be distinct, so we don't need to bother
		// tie-breaking on modeldef/texture/etc)
	}
};

struct SMRMaterialBucketKey
{
	SMRMaterialBucketKey(CStrIntern effect, const CShaderDefines& defines)
		: effect(effect), defines(defines) { }

	CStrIntern effect;
	CShaderDefines defines;

	bool operator==(const SMRMaterialBucketKey& b) const
	{
		return (effect == b.effect && defines == b.defines);
	}

private:
	SMRMaterialBucketKey& operator=(const SMRMaterialBucketKey&);
};

struct SMRMaterialBucketKeyHash
{
	size_t operator()(const SMRMaterialBucketKey& key) const
	{
		size_t hash = 0;
		boost::hash_combine(hash, key.effect.GetHash());
		boost::hash_combine(hash, key.defines.GetHash());
		return hash;
	}
};

struct SMRTechBucket
{
	CShaderTechniquePtr tech;
	CModel** models;
	size_t numModels;

	// Model list is stored as pointers, not as a std::vector,
	// so that sorting lists of this struct is fast
};

struct SMRCompareTechBucket
{
	bool operator()(const SMRTechBucket& a, const SMRTechBucket& b)
	{
		return a.tech < b.tech;
	}
};

void ShaderModelRenderer::Render(const RenderModifierPtr& modifier, const CShaderDefines& context, int flags)
{
	if (m->submissions.empty())
		return;

	CMatrix3D worldToCam;
	g_Renderer.GetViewCamera().m_Orientation.GetInverse(worldToCam);

	/*
	 * Rendering approach:
	 * 
	 * m->submissions contains the list of CModels to render.
	 * 
	 * The data we need to render a model is:
	 *  - CShaderTechnique
	 *  - CTexture
	 *  - CShaderUniforms
	 *  - CModelDef (mesh data)
	 *  - CModel (model instance data)
	 * 
	 * For efficient rendering, we need to batch the draw calls to minimise state changes.
	 * (Uniform and texture changes are assumed to be cheaper than binding new mesh data,
	 * and shader changes are assumed to be most expensive.)
	 * First, group all models that share a technique to render them together.
	 * Within those groups, sub-group by CModelDef.
	 * Within those sub-groups, sub-sub-group by CTexture.
	 * Within those sub-sub-groups, sub-sub-sub-group by CShaderUniforms.
	 * 
	 * Alpha-blended models have to be sorted by distance from camera,
	 * then we can batch as long as the order is preserved.
	 * Non-alpha-blended models can be arbitrarily reordered to maximise batching.
	 * 
	 * For each model, the CShaderTechnique is derived from:
	 *  - The current global 'context' defines
	 *  - The CModel's material's defines
	 *  - The CModel's material's shader effect name
	 * 
	 * There are a smallish number of materials, and a smaller number of techniques.
	 * 
	 * To minimise technique lookups, we first group models by material,
	 * in 'materialBuckets' (a hash table).
	 * 
	 * For each material bucket we then look up the appropriate shader technique.
	 * If the technique requires sort-by-distance, the model is added to the
	 * 'sortByDistItems' list with its computed distance.
	 * Otherwise, the bucket's list of models is sorted by modeldef+texture+uniforms,
	 * then the technique and model list is added to 'techBuckets'.
	 * 
	 * 'techBuckets' is then sorted by technique, to improve batching when multiple
	 * materials map onto the same technique.
	 * 
	 * (Note that this isn't perfect batching: we don't sort across models in
	 * multiple buckets that share a technique. In practice that shouldn't reduce
	 * batching much (we rarely have one mesh used with multiple materials),
	 * and it saves on copying and lets us sort smaller lists.)
	 * 
	 * Extra tech buckets are added for the sorted-by-distance models without reordering.
	 * Finally we render by looping over each tech bucket, then looping over the model
	 * list in each, rebinding the GL state whenever it changes.
	 */

	Allocators::DynamicArena arena(256 * KiB);
	typedef ProxyAllocator<CModel*, Allocators::DynamicArena> ModelListAllocator;
	typedef std::vector<CModel*, ModelListAllocator> ModelList_t;
	typedef boost::unordered_map<SMRMaterialBucketKey, ModelList_t,
		SMRMaterialBucketKeyHash, std::equal_to<SMRMaterialBucketKey>,
		ProxyAllocator<std::pair<const SMRMaterialBucketKey, ModelList_t>, Allocators::DynamicArena>
	> MaterialBuckets_t;
	MaterialBuckets_t materialBuckets((MaterialBuckets_t::allocator_type(arena)));

	{
		PROFILE3("bucketing by material");

		for (size_t i = 0; i < m->submissions.size(); ++i)
		{
			CModel* model = m->submissions[i];

			uint32_t condFlags = 0;

			const CShaderConditionalDefines& condefs = model->GetMaterial().GetConditionalDefines();
			for (size_t j = 0; j < condefs.GetSize(); ++j)
			{
				const CShaderConditionalDefines::CondDefine& item = condefs.GetItem(j);
				int type = item.m_CondType;
				switch (type)
				{
					case DCOND_DISTANCE:
					{
						CVector3D modelpos = model->GetTransform().GetTranslation();
						float dist = worldToCam.Transform(modelpos).Z;
						
						float dmin = item.m_CondArgs[0];
						float dmax = item.m_CondArgs[1];
						
						if ((dmin < 0 || dist >= dmin) && (dmax < 0 || dist < dmax))
							condFlags |= (1 << j);
						
						break;
					}
				}
			}

			CShaderDefines defs = model->GetMaterial().GetShaderDefines(condFlags);
			SMRMaterialBucketKey key(model->GetMaterial().GetShaderEffect(), defs);

			MaterialBuckets_t::iterator it = materialBuckets.find(key);
			if (it == materialBuckets.end())
			{
				std::pair<MaterialBuckets_t::iterator, bool> inserted = materialBuckets.insert(
					std::make_pair(key, ModelList_t(ModelList_t::allocator_type(arena))));
				inserted.first->second.reserve(32);
				inserted.first->second.push_back(model);
			}
			else
			{
				it->second.push_back(model);
			}
		}
	}

	typedef ProxyAllocator<SMRSortByDistItem, Allocators::DynamicArena> SortByDistItemsAllocator;
	std::vector<SMRSortByDistItem, SortByDistItemsAllocator> sortByDistItems((SortByDistItemsAllocator(arena)));

	typedef ProxyAllocator<CShaderTechniquePtr, Allocators::DynamicArena> SortByTechItemsAllocator;
	std::vector<CShaderTechniquePtr, SortByTechItemsAllocator> sortByDistTechs((SortByTechItemsAllocator(arena)));
		// indexed by sortByDistItems[i].techIdx
		// (which stores indexes instead of CShaderTechniquePtr directly
		// to avoid the shared_ptr copy cost when sorting; maybe it'd be better
		// if we just stored raw CShaderTechnique* and assumed the shader manager
		// will keep it alive long enough)

	typedef ProxyAllocator<SMRTechBucket, Allocators::DynamicArena> TechBucketsAllocator;
	std::vector<SMRTechBucket, TechBucketsAllocator> techBuckets((TechBucketsAllocator(arena)));

	{
		PROFILE3("processing material buckets");
		for (MaterialBuckets_t::iterator it = materialBuckets.begin(); it != materialBuckets.end(); ++it)
		{
			CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect(it->first.effect, context, it->first.defines);

			// Skip invalid techniques (e.g. from data file errors)
			if (!tech)
				continue;

			if (tech->GetSortByDistance())
			{
				// Add the tech into a vector so we can index it
				// (There might be duplicates in this list, but that doesn't really matter)
				if (sortByDistTechs.empty() || sortByDistTechs.back() != tech)
					sortByDistTechs.push_back(tech);
				size_t techIdx = sortByDistTechs.size()-1;

				// Add each model into sortByDistItems
				for (size_t i = 0; i < it->second.size(); ++i)
				{
					SMRSortByDistItem itemWithDist;
					itemWithDist.techIdx = techIdx;

					CModel* model = it->second[i];
					itemWithDist.model = model;

					CVector3D modelpos = model->GetTransform().GetTranslation();
					itemWithDist.dist = worldToCam.Transform(modelpos).Z;

					sortByDistItems.push_back(itemWithDist);
				}
			}
			else
			{
				// Sort model list by modeldef+texture, for batching
				// TODO: This only sorts by base texture. While this is an OK approximation
				// for most cases (as related samplers are usually used together), it would be better
				// to take all the samplers into account when sorting here.
				std::sort(it->second.begin(), it->second.end(), SMRBatchModel());

				// Add a tech bucket pointing at this model list
				SMRTechBucket techBucket = { tech, &it->second[0], it->second.size() };
				techBuckets.push_back(techBucket);
			}
		}
	}

	{
		PROFILE3("sorting tech buckets");
		// Sort by technique, for better batching
		std::sort(techBuckets.begin(), techBuckets.end(), SMRCompareTechBucket());
	}

	// List of models corresponding to sortByDistItems[i].model
	// (This exists primarily because techBuckets wants a CModel**;
	// we could avoid the cost of copying into this list by adding
	// a stride length into techBuckets and not requiring contiguous CModel*s)
	std::vector<CModel*, ModelListAllocator> sortByDistModels((ModelListAllocator(arena)));

	if (!sortByDistItems.empty())
	{
		{
			PROFILE3("sorting items by dist");
			std::sort(sortByDistItems.begin(), sortByDistItems.end(), SMRCompareSortByDistItem());
		}

		{
			PROFILE3("batching dist-sorted items");

			sortByDistModels.reserve(sortByDistItems.size());

			// Find runs of distance-sorted models that share a technique,
			// and create a new tech bucket for each run

			size_t start = 0; // start of current run
			size_t currentTechIdx = sortByDistItems[start].techIdx;

			for (size_t end = 0; end < sortByDistItems.size(); ++end)
			{
				sortByDistModels.push_back(sortByDistItems[end].model);

				size_t techIdx = sortByDistItems[end].techIdx;
				if (techIdx != currentTechIdx)
				{
					// Start of a new run - push the old run into a new tech bucket
					SMRTechBucket techBucket = { sortByDistTechs[currentTechIdx], &sortByDistModels[start], end-start };
					techBuckets.push_back(techBucket);
					start = end;
					currentTechIdx = techIdx;
				}
			}

			// Add the tech bucket for the final run
			SMRTechBucket techBucket = { sortByDistTechs[currentTechIdx], &sortByDistModels[start], sortByDistItems.size()-start };
			techBuckets.push_back(techBucket);
		}
	}

	{
		PROFILE3("rendering bucketed submissions");

		size_t idxTechStart = 0;
		
		// This vector keeps track of texture changes during rendering. It is kept outside the
		// loops to avoid excessive reallocations. The token allocation of 64 elements 
		// should be plenty, though it is reallocated below (at a cost) if necessary.
		typedef ProxyAllocator<CTexture*, Allocators::DynamicArena> TextureListAllocator;
		std::vector<CTexture*, TextureListAllocator> currentTexs((TextureListAllocator(arena)));
		currentTexs.reserve(64);
		
		// texBindings holds the identifier bindings in the shader, which can no longer be defined 
		// statically in the ShaderRenderModifier class. texBindingNames uses interned strings to
		// keep track of when bindings need to be reevaluated.
		typedef ProxyAllocator<CShaderProgram::Binding, Allocators::DynamicArena> BindingListAllocator;
		std::vector<CShaderProgram::Binding, BindingListAllocator> texBindings((BindingListAllocator(arena)));
		texBindings.reserve(64);

		typedef ProxyAllocator<CStrIntern, Allocators::DynamicArena> BindingNamesListAllocator;
		std::vector<CStrIntern, BindingNamesListAllocator> texBindingNames((BindingNamesListAllocator(arena)));
		texBindingNames.reserve(64);

		while (idxTechStart < techBuckets.size())
		{
			CShaderTechniquePtr currentTech = techBuckets[idxTechStart].tech;

			// Find runs [idxTechStart, idxTechEnd) in techBuckets of the same technique
			size_t idxTechEnd;
			for (idxTechEnd = idxTechStart + 1; idxTechEnd < techBuckets.size(); ++idxTechEnd)
			{
				if (techBuckets[idxTechEnd].tech != currentTech)
					break;
			}

			// For each of the technique's passes, render all the models in this run
			for (int pass = 0; pass < currentTech->GetNumPasses(); ++pass)
			{
				currentTech->BeginPass(pass);

				const CShaderProgramPtr& shader = currentTech->GetShader(pass);
				int streamflags = shader->GetStreamFlags();

				modifier->BeginPass(shader);

				m->vertexRenderer->BeginPass(streamflags);
				
				// When the shader technique changes, textures need to be
				// rebound, so ensure there are no remnants from the last pass.
				// (the vector size is set to 0, but memory is not freed)
				currentTexs.clear();
				texBindings.clear();
				texBindingNames.clear();
				
				CModelDef* currentModeldef = NULL;
				CShaderUniforms currentStaticUniforms;

				for (size_t idx = idxTechStart; idx < idxTechEnd; ++idx)
				{
					CModel** models = techBuckets[idx].models;
					size_t numModels = techBuckets[idx].numModels;
					for (size_t i = 0; i < numModels; ++i)
					{
						CModel* model = models[i];

						if (flags && !(model->GetFlags() & flags))
							continue;

						const CMaterial::SamplersVector& samplers = model->GetMaterial().GetSamplers();
						size_t samplersNum = samplers.size();
						
						// make sure the vectors are the right virtual sizes, and also
						// reallocate if there are more samplers than expected.
						if (currentTexs.size() != samplersNum)
						{
							currentTexs.resize(samplersNum, NULL);
							texBindings.resize(samplersNum, CShaderProgram::Binding());
							texBindingNames.resize(samplersNum, CStrIntern());
							
							// ensure they are definitely empty
							std::fill(texBindings.begin(), texBindings.end(), CShaderProgram::Binding());
							std::fill(currentTexs.begin(), currentTexs.end(), (CTexture*)NULL);
							std::fill(texBindingNames.begin(), texBindingNames.end(), CStrIntern());
						}
						
						// bind the samplers to the shader
						for (size_t s = 0; s < samplersNum; ++s)
						{
							const CMaterial::TextureSampler& samp = samplers[s];
							
							CShaderProgram::Binding bind = texBindings[s];
							// check that the handles are current
							// and reevaluate them if necessary
							if (texBindingNames[s] == samp.Name && bind.Active())
							{
								bind = texBindings[s];
							}
							else
							{
								bind = shader->GetTextureBinding(samp.Name);
								texBindings[s] = bind;
								texBindingNames[s] = samp.Name;
							}

							// same with the actual sampler bindings
							CTexture* newTex = samp.Sampler.get();
							if (bind.Active() && newTex != currentTexs[s])
							{
								shader->BindTexture(bind, samp.Sampler->GetHandle());
								currentTexs[s] = newTex;
							}
						}
						
						// Bind modeldef when it changes
						CModelDef* newModeldef = model->GetModelDef().get();
						if (newModeldef != currentModeldef)
						{
							currentModeldef = newModeldef;
							m->vertexRenderer->PrepareModelDef(shader, streamflags, *currentModeldef);
						}

						// Bind all uniforms when any change
						CShaderUniforms newStaticUniforms = model->GetMaterial().GetStaticUniforms();
						if (newStaticUniforms != currentStaticUniforms)
						{
							currentStaticUniforms = newStaticUniforms;
							currentStaticUniforms.BindUniforms(shader);
						}
						
						const CShaderRenderQueries& renderQueries = model->GetMaterial().GetRenderQueries();
						
						for (size_t q = 0; q < renderQueries.GetSize(); q++)
						{
							CShaderRenderQueries::RenderQuery rq = renderQueries.GetItem(q);
							if (rq.first == RQUERY_TIME)
							{
								CShaderProgram::Binding binding = shader->GetUniformBinding(rq.second);
								if (binding.Active())
								{
									double time = g_Renderer.GetTimeManager().GetGlobalTime();
									shader->Uniform(binding, time, 0,0,0);
								}
							}
							else if (rq.first == RQUERY_WATER_TEX)
							{
								WaterManager* WaterMgr = g_Renderer.GetWaterManager();
								double time = WaterMgr->m_WaterTexTimer;
								double period = 1.6;
								int curTex = (int)(time*60/period) % 60;
								
								if (WaterMgr->m_RenderWater && WaterMgr->WillRenderFancyWater())
									shader->BindTexture(str_waterTex, WaterMgr->m_NormalMap[curTex]);
								else
									shader->BindTexture(str_waterTex, g_Renderer.GetTextureManager().GetErrorTexture());
							}
							else if (rq.first == RQUERY_SKY_CUBE)
							{
								shader->BindTexture(str_skyCube, g_Renderer.GetSkyManager()->GetSkyCube());
							}
						}

						modifier->PrepareModel(shader, model);

						CModelRData* rdata = static_cast<CModelRData*>(model->GetRenderData());
						ENSURE(rdata->GetKey() == m->vertexRenderer.get());

						m->vertexRenderer->RenderModel(shader, streamflags, model, rdata);
					}
				}

				m->vertexRenderer->EndPass(streamflags);

				currentTech->EndPass(pass);
			}

			idxTechStart = idxTechEnd;
		}
	}
}

void ShaderModelRenderer::Filter(CModelFilter& filter, int passed, int flags)
{
	for (size_t i = 0; i < m->submissions.size(); ++i)
	{
		CModel* model = m->submissions[i];

		if (flags && !(model->GetFlags() & flags))
			continue;

		if (filter.Filter(model))
			model->SetFlags(model->GetFlags() | passed);
		else
			model->SetFlags(model->GetFlags() & ~passed);
	}
}
