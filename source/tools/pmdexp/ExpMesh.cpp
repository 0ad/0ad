#include "ExpMesh.h"
#include "ExpUtil.h"
#include "ExpSkeleton.h"
#include "VertexTree.h"
#include "VNormal.h"

#include "phyexp.h"


////////////////////////////////////////////////////////////////////////////////
// TMNegParity: return whether the given matrix has a negative scale or not
inline bool TMNegParity(Matrix3 &Mat)
{
	return (DotProd(CrossProd(Mat.GetRow(0),Mat.GetRow(1)),Mat.GetRow(2)) < 0.0) ? 1 : 0;
}


////////////////////////////////////////////////////////////////////////////////
// GetTriObjectFromNode: return a pointer to a TriObject 
// given an INode or return NULL if the node cannot be converted 
// to a TriObject
static TriObject* GetTriObjectFromNode(INode* node,bool& deleteIt)
{
	deleteIt = false;
	Object* obj=node->EvalWorldState(0).obj;
	if (obj) {
		if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) { 
			TriObject *tri=(TriObject*) obj->ConvertToType(0,Class_ID(TRIOBJ_CLASS_ID,0));
			
			// note that the TriObject should only be deleted if the pointer to it is 
			// not equal to the object pointer that called ConvertToType()
			if (obj != tri) {
				deleteIt = true;
			}
			return tri;
		}
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// GetPhysiqueFromNode: return a pointer to a Physique modifier found
// in the modifier stack of the given INode, or return NULL is no Physique 
// modifier can be found
static Modifier* GetPhysiqueFromNode(INode* node)
{
    // Get object from node. Abort if no object.
    Object* object=node->GetObjectRef();
    if (!object) return 0;
    
    // ss derived object?
    if (object->SuperClassID()==GEN_DERIVOB_CLASS_ID) {
        // yes -> cast
        IDerivedObject* derivedObj=static_cast<IDerivedObject*>(object);

        // iterate over all entries of the modifier stack
        int modStackIndex=0;
        while (modStackIndex<derivedObj->NumModifiers()) {
            
			// get current modifier
            Modifier* modifier=derivedObj->GetModifier(modStackIndex);
            Class_ID clsid=modifier->ClassID();
            
			// check if it's a Physique 
            if (modifier->ClassID()==Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B)) {
                // yes -> return it
                return modifier;
            }

            // advance to next modifier stack entry
            modStackIndex++;
        }
    }

    // not found ..
    return 0;
}



PMDExpMesh::PMDExpMesh(INode* node,const ExpSkeleton* skeleton) 
	: m_Node(node), m_Skeleton(skeleton)
{
}

bool PMDExpMesh::IsMesh(Object* obj)
{
	assert(obj);
	if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) {
		return true;
	} else {
		return false;
	}
}

////////////////////////////////////////////////////////////////////////////////
// InsertBlend: insert the given bone/weight blend into the given blend set
void PMDExpMesh::InsertBlend(unsigned char bone,float weight,SVertexBlend& blend)
{
	// get position where this blend should be inserted
	int i=0;
	while (weight<blend.m_Weight[i]) i++;
	if (i>=SVertexBlend::SIZE) {
		// all other blends carry greater weight; reject this blend
		return;
	}

	for (int j=SVertexBlend::SIZE-2;j>=i;j--) {
		blend.m_Bone[j+1]=blend.m_Bone[j];
		blend.m_Weight[j+1]=blend.m_Weight[j];
	}

	blend.m_Bone[i]=bone;
	blend.m_Weight[i]=weight;
}

void PMDExpMesh::BuildVertexBlends(Mesh& mesh,VBlendList& blends)
{
	// allocate blends
	int numVerts=mesh.getNumVerts();
	blends.resize(mesh.getNumVerts());

	// set all blends to null
	for (int i=0;i<numVerts;i++) {
		memset(blends[i].m_Bone,0xff,sizeof(blends[i].m_Bone));
		memset(blends[i].m_Weight,0,sizeof(blends[i].m_Weight));
	}

	// query presence of physique modifier first
	Modifier* physique=GetPhysiqueFromNode(m_Node);
	if (!physique) {
		// no Physique, no blends: no support for Skin at present
//		LogError("No physique export");
		return;
	}

	// get physique export interface
	IPhysiqueExport *phyExport=(IPhysiqueExport*) physique->GetInterface(I_PHYINTERFACE);
	if(phyExport) {
		// get ModContext interface from export interface for this INode
		IPhyContextExport *contextExport=(IPhyContextExport*) phyExport->GetContextInterface(m_Node);
		if(contextExport) {
			// export all the vertices as if they are rigid ..
			contextExport->ConvertToRigid(TRUE);
			// .. and disable blending
            contextExport->AllowBlending(FALSE);

			// check number of vertices assigned to physique matches mesh total
			if (contextExport->GetNumberVertices()!=mesh.getNumVerts()) {
//				LogError("vtx mismatch!\n");
				return;
			}

			for (int i=0;i<numVerts;i++) {
				
				SVertexBlend& blend=blends[i];

				// get export interface for this vertex
				IPhyVertexExport* vertexexport=contextExport->GetVertexInterface(i);
				if (vertexexport) {

					// get type of vertex
					int type=vertexexport->GetVertexType();

					if (type==RIGID_TYPE) {
						IPhyRigidVertex* rigidVertex=(IPhyRigidVertex*) vertexexport;

						// get attached bone        
						INode* boneNode=rigidVertex->GetNode();
						if (boneNode) {
							// store blend		
							blend.m_Bone[0]=m_Skeleton->FindBoneByNode(boneNode);

							if (blend.m_Bone[0]==0xff) {
//								LogError("Failed to find bonenode %s (0x%p)\n",boneNode->GetName(),boneNode);
							}
							blend.m_Weight[0]=1.0f;
						}
					} else {
						// must be RIGID_BLENDED_TYPE vertex
						IPhyBlendedRigidVertex* blendedVertex=(IPhyBlendedRigidVertex*) vertexexport;

						// get number of nodes affecting vertex; clamp to SVertexBlend::SIZE
						int numBones=blendedVertex->GetNumberNodes();

						for (int boneindex=0;boneindex<numBones;boneindex++) {
						
							// get weight of boneindex'th bone        
							float boneWeight=blendedVertex->GetWeight(boneindex);
							if (boneWeight>0.001f) {
								// get boneindex'th bone        
								INode* boneNode=blendedVertex->GetNode(boneindex);
								// store blend - (need to check for prior bones using same bone?)
								unsigned char bone=m_Skeleton->FindBoneByNode(boneNode);
								if (bone==0xff) {
									//LogError("Failed to find bonenode %s (0x%p)\n",boneNode->GetName(),boneNode);
								} else {
									InsertBlend(bone,boneWeight,blend);
								}
							}
						}

#if 0
						if (blend.m_Bone[0]==0xff) {
							// ugh - all bones must have zero weight .. use the root bone with a weight of 1?
							blend.m_Bone[0]=0;
							blend.m_Weight[0]=1;
						}
#else
						if (blend.m_Bone[0]==0xff) {
							// ugh - all bones must have zero weight .. try again and assign all bones equal weight 
							// TODO, RC 13/03/04 - log warning for this; most likely user error in enveloping/bone assignment
							for (boneindex=0;boneindex<numBones;boneindex++) {
							
								float boneWeight=1.0f;

								// get boneindex'th bone        
								INode* boneNode=blendedVertex->GetNode(boneindex);
								// store blend - (need to check for prior bones using same bone?)
								unsigned char bone=m_Skeleton->FindBoneByNode(boneNode);
								if (bone==0xff) {
									//LogError("Failed to find bonenode %s (0x%p)\n",boneNode->GetName(),boneNode);
								} else {
									InsertBlend(bone,boneWeight,blend);
								}
							}
						}
#endif

						// normalise bone weights of this blend such that they sum to 1
						int j;
						float totalweight=0;
						for (j=0;j<SVertexBlend::SIZE && blend.m_Bone[j]!=0xff;j++) {
							totalweight+=blend.m_Weight[j];
						}
						for (j=0;j<SVertexBlend::SIZE && blend.m_Bone[j]!=0xff;j++) {
							blend.m_Weight[j]/=totalweight;
						}
					}

					// done with this vertex
					contextExport->ReleaseVertexInterface(vertexexport);
				} else {
					//LogError("No vertexexport interface!\n");
				}
			} 
		} else {
			//LogError("No contextexport interface!\n");
		}
	}
}



static CVector3D SkinVertex(const ExpSkeleton* skeleton,const CVector3D& pos,const SVertexBlend& blend)
{
	CVector3D result(0,0,0);
	for (int i=0;i<SVertexBlend::SIZE && blend.m_Bone[i]!=0xff;i++) {
		const CMatrix3D& m=skeleton->m_Bones[blend.m_Bone[i]]->m_Transform;		
		CVector3D tmp=m.Transform(pos);
		result+=tmp*blend.m_Weight[i];
	}

	return result;
}

static CVector3D SkinNormal(const ExpSkeleton* skeleton,const CVector3D& pos,const SVertexBlend& blend)
{
	CMatrix3D minv,minvtrans;
	CVector3D result(0,0,0);
	for (int i=0;i<SVertexBlend::SIZE && blend.m_Bone[i]!=0xff;i++) {
		const CMatrix3D& m=skeleton->m_Bones[blend.m_Bone[i]]->m_Transform;		
		m.GetInverse(minv);
		minv.GetTranspose(minvtrans);
		CVector3D tmp=minvtrans.Transform(pos);
		result+=tmp*blend.m_Weight[i];
	}

	return result;
}

static CVector3D UnskinPoint(const ExpSkeleton* skeleton,const CVector3D& pos,const SVertexBlend& blend)
{
	CMatrix3D m,minv;
	m.SetZero();
	for (int i=0;i<SVertexBlend::SIZE && blend.m_Bone[i]!=0xff;i++) {
		const CMatrix3D& m2=skeleton->m_Bones[blend.m_Bone[i]]->m_Transform;		
		m+=m2*blend.m_Weight[i];
	}
	m.GetInverse(minv);

	return minv.Transform(pos);
}

static CVector3D UnskinNormal(const ExpSkeleton* skeleton,const CVector3D& nrm,const SVertexBlend& blend)
{
	CMatrix3D m,minv,minvtrans;
	m.SetZero();
	for (int i=0;i<SVertexBlend::SIZE && blend.m_Bone[i]!=0xff;i++) {
		const CMatrix3D& m2=skeleton->m_Bones[blend.m_Bone[i]]->m_Transform;		
		m+=m2*blend.m_Weight[i];
	}
	m.GetInverse(minv);
	minv.GetTranspose(minvtrans);
	minvtrans.GetInverse(m);

	return m.Rotate(nrm);
}

CVector3D* PMDExpMesh::BuildVertexPoints(Mesh& mesh,VBlendList& vblends)
{
	// get transform to world space
	Matrix3 tm=m_Node->GetObjectTM(0);
	bool negScale=TMNegParity(tm);

	int numverts=mesh.getNumVerts();
	CVector3D* transPts=new CVector3D[numverts];

	// transform each vertex and store in output array
	for (int i=0;i<numverts;i++) {
		Point3 maxpos=mesh.getVert(i)*tm;
		transPts[i]=CVector3D(maxpos.x,maxpos.z,maxpos.y);
	}

	if (m_Skeleton) {
		BuildVertexBlends(mesh,vblends);
		for (i=0;i<numverts;i++) {
			transPts[i]=UnskinPoint(m_Skeleton,transPts[i],vblends[i]);
		} 
	}

	return transPts;
}

VNormal* PMDExpMesh::BuildVertexNormals(Mesh& mesh,VBlendList& vblends)
{
	// get transform to world space
	Matrix3 tm=m_Node->GetObjectTM(0);
	bool negScale=TMNegParity(tm);

	// allocate basis vectors for dot product lighting 
	VNormal* vnormals=new VNormal[mesh.getNumVerts()];

	for (u32 i=0;i<mesh.getNumFaces();++i) {
		
		// build face normal
		Point3 P0=mesh.getVert(mesh.faces[i].v[0])*tm;
		Point3 P1=mesh.getVert(mesh.faces[i].v[1])*tm;
		Point3 P2=mesh.getVert(mesh.faces[i].v[2])*tm;

		Point3 e0=P1-P0;
		Point3 e1=P2-P0;
		Point3 faceNormal=e0^e1;		
		if (negScale) faceNormal*=-1;
		
		CVector3D N(faceNormal.x,faceNormal.z,faceNormal.y);		
		N.Normalize();

		// accumulate normal
		vnormals[mesh.faces[i].v[0]].add(N,mesh.faces[i].smGroup);
		vnormals[mesh.faces[i].v[1]].add(N,mesh.faces[i].smGroup);
		vnormals[mesh.faces[i].v[2]].add(N,mesh.faces[i].smGroup);
	}

	for (i=0;i<mesh.getNumVerts();i++) {
		if (m_Skeleton) {
			VNormal* vnorm=&vnormals[i];
			vnorm->normalize();
			while (vnorm) {
				vnorm->_normal=UnskinNormal(m_Skeleton,vnorm->_normal,vblends[i]);
				vnorm=vnorm->next;
			}
		}
		vnormals[i].normalize();
	}	

	// .. and return them
	return vnormals;
}

void PMDExpMesh::BuildVerticesAndFaces(Mesh& mesh,VertexList& vertices,CVector3D* vpoints,VBlendList& vblends,VNormal* vnormals,FaceList& faces)
{
	// assume worst case and reserve enough space up front ..
	u32 numFaces=mesh.getNumFaces();
	vertices.reserve(numFaces*3);
	
	// initialise outgoing face array
	faces.resize(numFaces);

	// create a vertex tree setup to fill unique vertices into outgoing array
	VertexTree<UniqueVertexCmp> vtxtree(vertices);

	// get transform to world space
	Matrix3 tm=m_Node->GetObjectTM(0);
	bool negScale=TMNegParity(tm);
	
	// iterate through faces
	for (u32 i=0;i<numFaces;++i) {

		for (int k=0;k<3;++k) {
			ExpVertex vtx;
			vtx.m_Index=mesh.faces[i].v[k];

			// get position
			vtx.m_Pos=vpoints[vtx.m_Index];

			// get UVs
			if (mesh.getNumTVerts()) {
				const TVFace& tvface=mesh.tvFace[i];
				const UVVert& uv=mesh.getTVert(tvface.t[k]);
				vtx.m_UVs[0]=uv.x;
				vtx.m_UVs[1]=uv.y;
			} else {
				vtx.m_UVs[0]=0;
				vtx.m_UVs[1]=0;
			}

			VNormal* vnrm=vnormals[mesh.faces[i].v[k]].get(mesh.faces[i].smGroup);
			vtx.m_Normal=vnrm ? vnrm->_normal : CVector3D(0,0,0);
			faces[i].m_V[k]=vtxtree.insert(vtx);
		}

		if (negScale) {
			int t=faces[i].m_V[1];
			faces[i].m_V[1]=faces[i].m_V[2];
			faces[i].m_V[2]=t;
		}

		faces[i].m_MAXindex=i;
		faces[i].m_Smooth=mesh.faces[i].smGroup;
	}
}

ExpMesh* PMDExpMesh::Build()
{
	// get mesh from node
	bool delTriObj;
	TriObject* triObj=GetTriObjectFromNode(m_Node,delTriObj);
	Mesh& mesh=triObj->mesh;

	ExpMesh* expmesh=new ExpMesh;

	// build vertex positions/blends from given mesh data	
	CVector3D* vpoints=BuildVertexPoints(mesh,expmesh->m_Blends);

	// build vertex normals from mesh data and vertex positions
	VNormal* vnormals=BuildVertexNormals(mesh,expmesh->m_Blends);

	// build face and vertex lists
	BuildVerticesAndFaces(mesh,expmesh->m_Vertices,vpoints,expmesh->m_Blends,vnormals,expmesh->m_Faces);

	// clean up ..
	delete[] vnormals;
	delete[] vpoints;
	if (delTriObj) {
		triObj->DeleteThis();
	}

	return expmesh;
}


