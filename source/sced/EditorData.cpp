

#include "EditorData.h"
#include "UIGlobals.h"
#include "ToolManager.h"
#include "ObjectManager.h"
#include "UnitManager.h"
#include "TextureManager.h"
#include "Model.h"
#include "SkeletonAnimManager.h"

#include "ogl.h"
#include "res/tex.h"
#include "time.h"

#include "BaseEntityCollection.h"
#include "Entity.h"
#include "EntityHandles.h"
#include "EntityManager.h"

const int NUM_ALPHA_MAPS = 14;
Handle AlphaMaps[NUM_ALPHA_MAPS];

CTerrain			g_Terrain;
CLightEnv			g_LightEnv;
CMiniMap			g_MiniMap;
CEditorData			g_EditorData;

CEditorData::CEditorData()
{
	m_ModelMatrix.SetIdentity();
	m_ScenarioName="Scenario01";
}

void CEditorData::SetMode(EditMode mode)
{ 
	if (m_Mode==TEST_MODE && mode!=TEST_MODE) StopTestMode();
	m_Mode=mode; 
	if (m_Mode==TEST_MODE) StartTestMode();
}

bool CEditorData::InitScene()
{
	// setup default lighting environment
	g_LightEnv.m_SunColor=RGBColor(1,1,1);
	g_LightEnv.m_Rotation=DEGTORAD(270);
	g_LightEnv.m_Elevation=DEGTORAD(45);
	g_LightEnv.m_TerrainAmbientColor=RGBColor(0,0,0);
	g_LightEnv.m_UnitsAmbientColor=RGBColor(0.4f,0.4f,0.4f);
	g_Renderer.SetLightEnv(&g_LightEnv);

	// load the default
	if (!LoadTerrain("terrain.raw")) return false;
	
	// get default texture to apply to terrain
	CTextureEntry* texture=0;
	for (uint i=0;i<g_TexMan.m_TerrainTextures.size();i++) {
		if (g_TexMan.m_TerrainTextures[i].m_Textures.size()) {
			texture=g_TexMan.m_TerrainTextures[i].m_Textures[0];
			break;
		}
	}

	// cover entire terrain with default texture
	u32 patchesPerSide=g_Terrain.GetPatchesPerSide();
	for (uint pj=0; pj<patchesPerSide; pj++) {
		for (uint pi=0; pi<patchesPerSide; pi++) {
			
			CPatch* patch=g_Terrain.GetPatch(pi,pj);
			
			for (int j=0;j<16;j++) {
				for (int i=0;i<16;i++) {
					patch->m_MiniPatches[j][i].Tex1=texture ? texture->m_Handle : 0;
				}
			}
		}
	}

	// setup camera
	InitCamera();

	// build the terrain plane
	float h=128*HEIGHT_SCALE;
	u32 mapSize=g_Terrain.GetVerticesPerSide();
	CVector3D pt0(0,h,0),pt1(float(CELL_SIZE*mapSize),h,0),pt2(0,h,float(CELL_SIZE*mapSize));
	m_TerrainPlane.Set(pt0,pt1,pt2);
	m_TerrainPlane.Normalize();

	return true;
}


struct TGAHeader {
	// header stuff
	unsigned char  iif_size;            
	unsigned char  cmap_type;           
	unsigned char  image_type;          
	unsigned char  pad[5];

	// origin : unused
	unsigned short d_x_origin;
	unsigned short d_y_origin;
	
	// dimensions
	unsigned short width;
	unsigned short height;

	// bits per pixel : 16, 24 or 32
	unsigned char  bpp;          

	// image descriptor : Bits 3-0: size of alpha channel
	//					  Bit 4: must be 0 (reserved)
	//					  Bit 5: should be 0 (origin)
	//					  Bits 6-7: should be 0 (interleaving)
   unsigned char image_descriptor;    
};

static bool saveTGA(const char* filename,int width,int height,unsigned char* data) 
{
	FILE* fp=fopen(filename,"wb");
	if (!fp) return false;

	// fill file header
	TGAHeader header;
	header.iif_size=0;
	header.cmap_type=0;
	header.image_type=2;
	memset(header.pad,0,sizeof(header.pad));
	header.d_x_origin=0;
	header.d_y_origin=0;
	header.width=width;
	header.height=height;
	header.bpp=24;
	header.image_descriptor=0;

	if (fwrite(&header,sizeof(TGAHeader),1,fp)!=1) {
		fclose(fp);
		return false;
	}

	// write data 
	if (fwrite(data,width*height*3,1,fp)!=1) {
		fclose(fp);
		return false;
	}

	// return success ..
    fclose(fp);
	return true;
}

void CEditorData::LoadAlphaMaps()
{
	const char* fns[CRenderer::NumAlphaMaps] = {
		"art/textures/terrain/alphamaps/special/blendcircle.png",
		"art/textures/terrain/alphamaps/special/blendlshape.png",
		"art/textures/terrain/alphamaps/special/blendedge.png",
		"art/textures/terrain/alphamaps/special/blendedgecorner.png",
		"art/textures/terrain/alphamaps/special/blendedgetwocorners.png",
		"art/textures/terrain/alphamaps/special/blendfourcorners.png",
		"art/textures/terrain/alphamaps/special/blendtwooppositecorners.png",
		"art/textures/terrain/alphamaps/special/blendlshapecorner.png",
		"art/textures/terrain/alphamaps/special/blendtwocorners.png",
		"art/textures/terrain/alphamaps/special/blendcorner.png",
		"art/textures/terrain/alphamaps/special/blendtwoedges.png",
		"art/textures/terrain/alphamaps/special/blendthreecorners.png",
		"art/textures/terrain/alphamaps/special/blendushape.png",
		"art/textures/terrain/alphamaps/special/blendbad.png"
	};

	g_Renderer.LoadAlphaMaps(fns);
}

void CEditorData::InitResources()
{
	g_TexMan.LoadTerrainTextures();
	LoadAlphaMaps();
	g_ObjMan.LoadObjects();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// InitSingletons: create and initialise required singletons
void CEditorData::InitSingletons()
{
	// create terrain related stuff
	new CTextureManager;

	// create actor related stuff
	new CSkeletonAnimManager;
	new CObjectManager;
	new CUnitManager;

	// create entity related stuff 
	new CBaseEntityCollection;
	new CEntityManager;
	g_EntityTemplateCollection.loadTemplates();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Init: perform one time initialisation of the editor
bool CEditorData::Init()
{
	// create and initialise singletons
	InitSingletons();

	// load default textures
	InitResources();

	// create the scene - terrain, camera, light environment etc
	if (!InitScene()) return false;

	// set up an empty minimap
	g_MiniMap.Initialise();

	// set up the info box
	m_InfoBox.Initialise();

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Terminate: close down the editor (destroy singletons in reverse order to construction)
void CEditorData::Terminate()
{
	// destroy entity related stuff 
	delete CEntityManager::GetSingletonPtr();
	delete CBaseEntityCollection::GetSingletonPtr();

	// destroy actor related stuff
	delete CUnitManager::GetSingletonPtr();
	delete CObjectManager::GetSingletonPtr();
	delete CSkeletonAnimManager::GetSingletonPtr();

	// destroy terrain related stuff
	delete CTextureManager::GetSingletonPtr();
}

void CEditorData::InitCamera() 
{
	g_NaviCam.GetCamera().SetProjection(1.0f,10000.0f,DEGTORAD(90));
	g_NaviCam.GetCamera().m_Orientation.SetIdentity();
#ifdef TOPDOWNVIEW
	g_NaviCam.GetCamera().m_Orientation.RotateX(DEGTORAD(90));
	g_NaviCam.GetCamera().m_Orientation.Translate(CELL_SIZE*250*0.5, 80, CELL_SIZE*250*0.5);
#else
	g_NaviCam.GetCamera().m_Orientation.RotateX(DEGTORAD(40));
	g_NaviCam.GetCamera().m_Orientation.RotateY(DEGTORAD(-45));
	g_NaviCam.GetCamera().m_Orientation.Translate(600, 200, 125);
#endif

	OnCameraChanged();
}

void CEditorData::OnCameraChanged() 
{
	int width=g_Renderer.GetWidth();
	int height=g_Renderer.GetHeight();

	// resize viewport
	SViewPort viewport;
	viewport.m_X=0;
	viewport.m_Y=0;
	viewport.m_Width=width;
	viewport.m_Height=height;
	g_NaviCam.GetCamera().SetViewPort(&viewport);

	// rebuild object camera
	m_ObjectCamera.SetViewPort(&viewport);
	m_ObjectCamera.SetProjection(1.0f,10000.0f,DEGTORAD(90));


	// recalculate projection matrix
	g_NaviCam.GetCamera().SetProjection(1.0f,10000.0f,DEGTORAD(20));

	// update viewing frustum
	g_NaviCam.GetCamera().UpdateFrustum();

	// calculate intersection of camera stabbing lines with terrain plane

	// get points of back plane of frustum in camera space
	float aspect=height>0 ? float(width)/float(height) : 1.0f;
	float zfar=g_NaviCam.GetCamera().GetFarPlane();
	CVector3D cPts[4];
	float x=zfar*float(tan(g_NaviCam.GetCamera().GetFOV()*aspect*0.5));
	float y=zfar*float(tan(g_NaviCam.GetCamera().GetFOV()*0.5));
	cPts[0].X=-x;
	cPts[0].Y=-y;
	cPts[0].Z=zfar;
	cPts[1].X=x;
	cPts[1].Y=-y;
	cPts[1].Z=zfar;
	cPts[2].X=x;
	cPts[2].Y=y;
	cPts[2].Z=zfar;
	cPts[3].X=-x;
	cPts[3].Y=y;
	cPts[3].Z=zfar;

	// transform to world space
	CVector3D wPts[4];
	for (int i=0;i<4;i++) {
		wPts[i]=g_NaviCam.GetCamera().m_Orientation.Transform(cPts[i]);
	}

	// now intersect a ray from the camera through each point 
	CVector3D rayOrigin=g_NaviCam.GetCamera().m_Orientation.GetTranslation();
	CVector3D rayDir=g_NaviCam.GetCamera().m_Orientation.GetIn();

	CVector3D hitPt[4];
	for (i=0;i<4;i++) {
		CVector3D rayDir=wPts[i]-rayOrigin;
		rayDir.Normalize();

		// get intersection point
		m_TerrainPlane.FindRayIntersection(rayOrigin,rayDir,&hitPt[i]);
	}

	for (i=0;i<4;i++) {
		// convert to minimap space
		float px=hitPt[i].X;
		float pz=hitPt[i].Z;
		g_MiniMap.m_ViewRect[i][0]=(197*px/float(CELL_SIZE*g_Terrain.GetVerticesPerSide()));
		g_MiniMap.m_ViewRect[i][1]=197*pz/float(CELL_SIZE*g_Terrain.GetVerticesPerSide());
	}
}

void CEditorData::RenderTerrain()
{
	CFrustum frustum=g_NaviCam.GetCamera().GetFustum();
	u32 patchesPerSide=g_Terrain.GetPatchesPerSide();
	for (uint j=0; j<patchesPerSide; j++) {
		for (uint i=0; i<patchesPerSide; i++) {
			CPatch* patch=g_Terrain.GetPatch(i,j);
			if (frustum.IsBoxVisible (CVector3D(0,0,0),patch->GetBounds())) {
				g_Renderer.Submit(patch);
			}
		}
	}
}

void CEditorData::OnScreenShot(const char* filename)
{
	g_Renderer.SetClearColor(0);
	g_Renderer.BeginFrame();
	g_Renderer.SetCamera(g_NaviCam.GetCamera());
	
	RenderWorld();

	g_Renderer.EndFrame();

	int width=g_Renderer.GetWidth();
	int height=g_Renderer.GetHeight();
	unsigned char* data=new unsigned char[width*height*3];

	glReadBuffer(GL_BACK);
	glReadPixels(0,0,width,height,GL_BGR_EXT,GL_UNSIGNED_BYTE,data);	

	saveTGA(filename,width,height,data);
	
	delete[] data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// SubmitModelRecursive: recurse down given model, submitting it and all it's descendents to the 
// renderer
void SubmitModelRecursive(CModel* model)
{
	g_Renderer.Submit(model);

	const std::vector<CModel::Prop>& props=model->GetProps();
	for (uint i=0;i<props.size();i++) {
		SubmitModelRecursive(props[i].m_Model);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// RenderNoCull: render absolutely everything to a blank frame to force renderer
// to load required assets 
void CEditorData::RenderNoCull()
{
	g_Renderer.BeginFrame();
	g_Renderer.SetCamera(g_NaviCam.GetCamera());

	uint i,j;
	const std::vector<CUnit*>& units=g_UnitMan.GetUnits();
	for (i=0;i<units.size();++i) {
		SubmitModelRecursive(units[i]->GetModel());
	}
	
	u32 patchesPerSide=g_Terrain.GetPatchesPerSide();
	for (j=0; j<patchesPerSide; j++) {
		for (i=0; i<patchesPerSide; i++) {
			CPatch* patch=g_Terrain.GetPatch(i,j);
			g_Renderer.Submit(patch);
		}
	}

	g_Renderer.FlushFrame();
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	g_Renderer.EndFrame();
}

void CEditorData::RenderModels()
{
	CFrustum frustum=g_NaviCam.GetCamera().GetFustum();

	const std::vector<CUnit*>& units=g_UnitMan.GetUnits();
	uint i;
	for (i=0;i<units.size();++i) {
		if (frustum.IsBoxVisible (CVector3D(0,0,0),units[i]->GetModel()->GetBounds())) {
			SubmitModelRecursive(units[i]->GetModel());
		}
	}
}

void CEditorData::RenderWorld()
{
	// render terrain
	RenderTerrain();

	// render all the units
	RenderModels();
	
	// flush prior to rendering overlays etc	
	g_Renderer.FlushFrame();
}

void CEditorData::RenderObEdGrid()
{
	int i;
	const int numSteps=32;
	const CVector3D start(-numSteps*CELL_SIZE/2,0,-numSteps*CELL_SIZE/2);
	const CVector3D end(numSteps*CELL_SIZE/2,0,numSteps*CELL_SIZE/2);

	glDisable(GL_TEXTURE_2D);

	glDepthMask(0);
	glColor4f(0.5f,0.5f,0.5f,0.35f);
	glLineWidth(1.0f);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glBegin(GL_LINES);
	for (i=0;i<=numSteps;i++) {
		if (i%8==0) continue;
		CVector3D v0(start.X+(i*(end.X-start.X))/float(numSteps),start.Y,start.Z);
		glVertex3fv(&v0.X);

		CVector3D v1(v0.X,end.Y,end.Z);
		glVertex3fv(&v1.X);
	}

	for (i=0;i<=numSteps;i++) {
		if (i%8==0) continue;
		CVector3D v0(start.X,start.Y,start.Z+(i*(end.Z-start.Z))/float(numSteps));
		glVertex3fv(&v0.X);

		CVector3D v1(end.X,end.Y,v0.Z);
		glVertex3fv(&v1.X);
	}
	glEnd();

	glDisable(GL_BLEND);

	glColor3f(0,0,0.5);
	glLineWidth(2.0f);

	glBegin(GL_LINES);
	for (i=0;i<=numSteps;i+=8) {
		CVector3D v0(start.X+(i*(end.X-start.X))/float(numSteps),start.Y,start.Z);
		glVertex3fv(&v0.X);

		CVector3D v1(v0.X,end.Y,end.Z);
		glVertex3fv(&v1.X);
	}

	for (i=0;i<=numSteps;i+=8) {
		CVector3D v0(start.X,start.Y,start.Z+(i*(end.Z-start.Z))/float(numSteps));
		glVertex3fv(&v0.X);

		CVector3D v1(end.X,end.Y,v0.Z);
		glVertex3fv(&v1.X);
	}
	glEnd();
	glDepthMask(1);
}

void CEditorData::OnDraw()
{	
	if (m_Mode==SCENARIO_EDIT || m_Mode==TEST_MODE) {
		g_Renderer.SetClearColor(0);
		g_Renderer.BeginFrame();

		// setup camera
		g_Renderer.SetCamera(g_NaviCam.GetCamera());

		// render base terrain plus models
		RenderWorld();

		if (m_Mode!=TEST_MODE) {
			// render the active tool
			g_ToolMan.OnDraw();
		}

		// flush prior to rendering overlays ..
		g_Renderer.FlushFrame();
		
		// .. and here's the overlays
		g_MiniMap.Render();
		m_InfoBox.Render();
	} else {
		g_Renderer.SetClearColor(0x00453015);
		g_Renderer.BeginFrame();
		
		CObjectEntry* selobject=g_ObjMan.GetSelectedObject();
		if (selobject && selobject->m_Model) {
			// setup camera such that object is in the centre of the viewport
			m_ModelMatrix.SetIdentity();
			selobject->m_Model->SetTransform(m_ModelMatrix);

			const CBound& bound=selobject->m_Model->GetBounds();
			CVector3D pt((bound[0].X+bound[1].X)*0.5f,(bound[0].Y+bound[1].Y)*0.5f,bound[0].Z);
			
			float hfov=tan(DEGTORAD(45));
			float vfov=hfov/g_Renderer.GetAspect();
			float zx=(bound[1].X-bound[0].X)*0.5f/hfov;
			float zy=(bound[1].Y-bound[0].Y)*0.5f/vfov;
			float z=zx>zy ? zx : zy;
			z=(z+1)*2;

			m_ObjectCamera.m_Orientation.SetIdentity();
			m_ObjectCamera.m_Orientation.Translate(pt.X,pt.Y,-z);

			g_Renderer.SetCamera(m_ObjectCamera);

			RenderObEdGrid();

			g_Renderer.Submit(selobject->m_Model);
		}

		// flush prior to rendering overlays ..
		g_Renderer.FlushFrame();
		
		// .. and here's the overlays
		m_InfoBox.Render();
	}

	g_Renderer.EndFrame();

	// notify info box frame is complete; gives it chance to accumulate stats, check
	// fps, etc
	m_InfoBox.OnFrameComplete();
}


bool CEditorData::LoadTerrain(const char* filename)
{		
	Handle h = tex_load(filename);
	if(h<=0) {
		char buf[1024];
		sprintf(buf,"Failed to load \"%s\"",filename);
		ErrorBox(buf);
		return false;
	} else {

		int width=0;
		int height=0;
		int bpp=0;
		void *ptr=0;

		int i=tex_info(h, &width, &height, NULL, &bpp, &ptr);
		if (i)
		{
			printf("tex_info error: %d\n",i);
			fflush(stdout);
		}

		// rescale the texture to fit to the nearest of the 4 possible map sizes
		u32 mapsize=9;	// assume smallest map
		if (width>11*16+1) mapsize=11;
		if (width>13*16+1) mapsize=13;
		if (width>17*16+1) mapsize=17;

		u32 targetsize=mapsize*16+1;
		unsigned char* data=new unsigned char[targetsize*targetsize*bpp/8];
		u32 fmt=(bpp==8) ? GL_RED : ((bpp==24) ? GL_RGB : GL_RGBA);
		gluScaleImage(fmt,width,height,GL_UNSIGNED_BYTE,ptr,
			targetsize,targetsize,GL_UNSIGNED_BYTE,data);

		// build 16 bit heightmap from red channel of texture
		u16* heightmap=new u16[targetsize*targetsize];
		int stride=bpp/8;
		u16* hmptr=heightmap;

		// get src of copy
		const u8* dataptr = (bpp==8) ? data : ((bpp==24) ? data+2 : data+3);
		
		// build heightmap
		for (uint j=0;j<targetsize;++j) {
			for (uint i=0;i<targetsize;++i) {
				*hmptr=(*dataptr) << 8;
				hmptr++;
				dataptr+=stride;
			}
		}

		// rebuild terrain
		g_Terrain.Resize(mapsize);
		g_Terrain.SetHeightMap(heightmap);
		
		// clean up
		delete[] data;
		delete[] heightmap;

		// re-initialise minimap - terrain size may have changed
		g_MiniMap.Initialise();

		return true;
	}
}

// UpdateWorld: update time dependent data in the world to account for changes over 
// the given time (in s)
void CEditorData::UpdateWorld(float time)
{
	if (m_Mode==SCENARIO_EDIT || m_Mode==TEST_MODE) {
		const std::vector<CUnit*>& units=g_UnitMan.GetUnits();
		for (uint i=0;i<units.size();++i) {
			units[i]->GetModel()->Update(time);
		}
		if (m_Mode==TEST_MODE) {
			g_EntityManager.updateAll( time );
		}
	} else {
		CObjectEntry* selobject=g_ObjMan.GetSelectedObject();
		if (selobject && selobject->m_Model) {
			selobject->m_Model->Update(time);
		}
	}	
}

void CEditorData::StartTestMode()
{	
	// initialise entities
	g_EntityManager.dispatchAll( &CMessage( CMessage::EMSG_INIT ) );
}

void CEditorData::StopTestMode()
{	
	// make all units idle again
	const std::vector<CUnit*>& units=g_UnitMan.GetUnits();
	for (uint i=0;i<units.size();++i) {
		if (units[i]->GetObject()->m_IdleAnim) {
			units[i]->GetModel()->SetAnimation(units[i]->GetObject()->m_IdleAnim);
		}
	}
}