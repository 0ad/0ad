#include "PMDExp.h"
#include "ExpMesh.h"
#include "ExpProp.h"
#include "ExpSkeleton.h"

#undef PI
#include "ModelDef.h"


//////////////////////////////////////////////////////////////////////
// PMDExp constructor
PMDExp::PMDExp()
{
}

//////////////////////////////////////////////////////////////////////
// PMDExp destructor
PMDExp::~PMDExp() 
{
}

//////////////////////////////////////////////////////////////////////
// ExtCount: return the number of file name extensions supported 
// by the plug-in. 
int PMDExp::ExtCount()
{
	return 1;
}

//////////////////////////////////////////////////////////////////////
// Ext: return the ith file name extension 
const TCHAR* PMDExp::Ext(int n)
{		
	return _T("pmd");
}

//////////////////////////////////////////////////////////////////////
// LongDesc: return long ASCII description 
const TCHAR* PMDExp::LongDesc()
{
	return _T("Prometheus Model Data");
}
	
//////////////////////////////////////////////////////////////////////
// ShortDesc: return short ASCII description 
const TCHAR* PMDExp::ShortDesc() 
{			
	return _T("Prometheus Model");
}

//////////////////////////////////////////////////////////////////////
// AuthorName: return author name
const TCHAR* PMDExp::AuthorName()
{			
	return _T("Rich Cross");
}

//////////////////////////////////////////////////////////////////////
// CopyrightMessage: return copyright message
const TCHAR* PMDExp::CopyrightMessage() 
{	
	return _T("(c) Wildfire Games 2004");
}

//////////////////////////////////////////////////////////////////////
// OtherMessage1: return some other message (or don't, in this case)
const TCHAR* PMDExp::OtherMessage1() 
{		
	return _T("");
}

//////////////////////////////////////////////////////////////////////
// OtherMessage2: return some other message (or don't, in this case)
const TCHAR* PMDExp::OtherMessage2() 
{		
	return _T("");
}

//////////////////////////////////////////////////////////////////////
// Version: return version number * 100 (i.e. v3.01 = 301)
unsigned int PMDExp::Version()
{				
	return 1;
}

//////////////////////////////////////////////////////////////////////
// ShowAbout: show an about box (or don't, in this case)
void PMDExp::ShowAbout(HWND hWnd)
{			
}

//////////////////////////////////////////////////////////////////////
// SupportsOptions: return true for each option supported by each 
// extension the exporter supports
BOOL PMDExp::SupportsOptions(int ext, DWORD options)
{
	// return TRUE to indicate export selected supported
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// DoExport: actually perform the export to the given filename
int	PMDExp::DoExport(const TCHAR *name,ExpInterface *ei,Interface *ip, BOOL suppressPrompts, DWORD options)
{
	// result of the export: assume we'll fail somewhere along the way
	BOOL res=FALSE;

	// save off the interface ptr
	m_IP=ip;

	// save off the options
	m_Options=options;
	
	// build any skeletons in MAXs heirarchy before going any further
	std::vector<ExpSkeleton*> skeletons;
	ExpSkeleton::BuildSkeletons(m_IP->GetRootNode(),skeletons);
	if (skeletons.size()>1) {
		MessageBox(GetActiveWindow(),"Found more than one skeleton in scene","Error",MB_OK);
	} else {
		// build list of meshes and props from nodes in MAXs heirarchy
		std::vector<ExpMesh*> meshes;
		std::vector<ExpProp*> props;
		ExpSkeleton* skeleton=skeletons.size()>0 ? skeletons[0] : 0;
		BuildOutputList(m_IP->GetRootNode(),skeleton,meshes,props);
		if (meshes.size()==0) {
			// hmm .. nothing there?
			MessageBox(GetActiveWindow(),"Failed to find any meshes to export","Error",MB_OK);
		} else {
			// build a model from the node tree and any skeleton in scene
			CModelDef* model=BuildModel(meshes,props,skeleton);
			if (!model) {
				MessageBox(GetActiveWindow(),"Failed to create model","Error",MB_OK);
			} else {
				try {
					CModelDef::Save(name,model);
					res=TRUE;
				} catch (...) {
					res=FALSE;
				}

				MessageBox(GetActiveWindow(),res ? "Export Complete" : "Error saving model",res ? "Info" : "Error",MB_OK);
			}
			
			for (int i=0;i<meshes.size();i++) {
				delete meshes[i];
			}
		}
	}

	// clean up
	for (int i=0;i<skeletons.size();i++) {
		delete skeletons[i];
	}

	// return result
	return res;
}

/////////////////////////////////////////////////////////////////////////////////
// BuildOutputList: traverse heirarchy collecting meshes/props
void PMDExp::BuildOutputList(INode* node,ExpSkeleton* skeleton,
				   std::vector<ExpMesh*>& meshes,std::vector<ExpProp*>& props)
{
	// get object attached to node
	ObjectState os=node->EvalWorldState(0);
	if (os.obj) {
		// handle export selected
		if ((m_Options & SCENE_EXPORT_SELECTED) && !node->Selected()) {
			// ignore ..
		} else {
			// mesh? 
			bool isMesh=true;		// assume so
			if (isMesh && !PMDExpMesh::IsMesh(os.obj)) isMesh=false;
			if (isMesh && ExpSkeleton::IsBone(node)) isMesh=false;
			if (isMesh && ExpSkeleton::IsFootprints(node)) isMesh=false;

			if (isMesh) {
				PMDExpMesh meshbuilder(node,skeleton);
				ExpMesh* mesh=meshbuilder.Build();
				meshes.push_back(mesh);
			} else {
				// not a mesh - a prop?
				if (PMDExpProp::IsProp(os.obj)) {
					PMDExpProp propbuilder(node);
					ExpProp* prop=propbuilder.Build();
					props.push_back(prop);
				}
			}
		}
	}

	// traverse into children
	for (int i=0;i<node->NumberOfChildren();i++) {
		BuildOutputList(node->GetChildNode(i),skeleton,meshes,props);
	}
}


/////////////////////////////////////////////////////////////////////////////////
// BuildModel: weld together given list of meshes, and attach props; return 
// result as a CModelDef
CModelDef* PMDExp::BuildModel(std::vector<ExpMesh*>& meshes,std::vector<ExpProp*>& props,
							  ExpSkeleton* skeleton)
{	
	SVertexBlend nullBlend;
	memset(nullBlend.m_Bone,0xff,sizeof(nullBlend.m_Bone));
	memset(nullBlend.m_Weight,0,sizeof(nullBlend.m_Weight));

	// sum up total number of vertices and faces
	u32 totalVerts=0,totalFaces=0;
	for (int i=0;i<meshes.size();i++) {
		totalVerts+=meshes[i]->m_Vertices.size();
		totalFaces+=meshes[i]->m_Faces.size();
	}
	
	CModelDef* mdl=new CModelDef;

	mdl->m_NumVertices=totalVerts;
	mdl->m_pVertices=new SModelVertex[totalVerts];
	
	mdl->m_NumFaces=totalFaces;
	mdl->m_pFaces=new SModelFace[totalFaces];
	
	mdl->m_NumBones=skeleton ? skeleton->m_Bones.size() : 0;
	mdl->m_Bones=new CBoneState[mdl->m_NumBones];

	// build vertices
	int vcount=0;
	for (i=0;i<meshes.size();i++) {
		const VertexList& verts=meshes[i]->m_Vertices;
		const VBlendList& blends=meshes[i]->m_Blends;
		for (int j=0;j<verts.size();j++) {
			mdl->m_pVertices[vcount].m_Coords=verts[j].m_Pos;
			mdl->m_pVertices[vcount].m_Norm=verts[j].m_Normal;
			mdl->m_pVertices[vcount].m_U=verts[j].m_UVs[0];
			mdl->m_pVertices[vcount].m_V=verts[j].m_UVs[1];
			mdl->m_pVertices[vcount].m_Blend=blends.size()>0 ? blends[verts[j].m_Index] : nullBlend;
			vcount++;
		}
	}

	// build faces
	int fcount=0;
	int offset=0;
	for (i=0;i<meshes.size();i++) {
		const FaceList& faces=meshes[i]->m_Faces;
		for (int j=0;j<faces.size();j++) {
			mdl->m_pFaces[fcount].m_Verts[0]=faces[j].m_V[0]+offset;
			mdl->m_pFaces[fcount].m_Verts[1]=faces[j].m_V[1]+offset;
			mdl->m_pFaces[fcount].m_Verts[2]=faces[j].m_V[2]+offset;
			fcount++;
		}
		offset+=meshes[i]->m_Vertices.size();
	}

	// build bones
	for (i=0;i<mdl->m_NumBones;i++) {
		mdl->m_Bones[i].m_Translation=skeleton->m_Bones[i]->m_Translation;
		mdl->m_Bones[i].m_Rotation=skeleton->m_Bones[i]->m_Rotation;
	}

	// attach props
	mdl->m_NumPropPoints=props.size();	
	mdl->m_PropPoints=new SPropPoint[mdl->m_NumPropPoints];
	for (i=0;i<mdl->m_NumPropPoints;i++) {
		strcpy(mdl->m_PropPoints[i].m_Name,props[i]->m_Name.c_str());
		mdl->m_PropPoints[i].m_Position=props[i]->m_Position;
		mdl->m_PropPoints[i].m_Rotation=props[i]->m_Rotation;
		mdl->m_PropPoints[i].m_BoneIndex=skeleton ? skeleton->FindBoneByNode(props[i]->m_Parent) : 0xff;
	}

	// all done
	return mdl;
}


