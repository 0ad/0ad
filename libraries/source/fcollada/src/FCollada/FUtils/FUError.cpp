/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUError.h"

//
// FUError
//
FUCriticalSection FUError::criticalSection;
FUEvent3<FUError::Level, uint32, uint32> FUError::onErrorEvent;
FUEvent3<FUError::Level, uint32, uint32> FUError::onWarningEvent;
FUEvent3<FUError::Level, uint32, uint32> FUError::onDebugEvent;
fm::string FUError::customErrorString;
FUError::Level FUError::fatalLevel = FUError::ERROR_LEVEL;

FUError::FUError()
{
}

FUError::~FUError()
{
}

bool FUError::Error(FUError::Level errorLevel, uint32 errorCode, uint32 errorArgument)
{
	criticalSection.Enter();

	switch (errorLevel)
	{
	case FUError::WARNING_LEVEL: onWarningEvent(errorLevel, errorCode, errorArgument); break;
	case FUError::ERROR_LEVEL: onErrorEvent(errorLevel, errorCode, errorArgument); break;
	case FUError::DEBUG_LEVEL: onDebugEvent(errorLevel, errorCode, errorArgument); break;
	case FUError::LEVEL_COUNT: default: FUBreak;
	}

	criticalSection.Leave();
	return errorLevel >= fatalLevel;
}

void FUError::AddErrorCallback(FUError::Level errorLevel, FUError::FUErrorFunctor* callback)
{ 
	criticalSection.Enter();

	switch (errorLevel)
	{
	case FUError::WARNING_LEVEL: onWarningEvent.InsertHandler(callback); break;
	case FUError::ERROR_LEVEL: onErrorEvent.InsertHandler(callback); break;
	case FUError::DEBUG_LEVEL: onDebugEvent.InsertHandler(callback); break;
	case FUError::LEVEL_COUNT: default: FUBreak;
	}

	criticalSection.Leave();
}

void FUError::RemoveErrorCallback(FUError::Level errorLevel, void* object, void* function)
{ 
	criticalSection.Enter();

	switch (errorLevel)
	{
	case FUError::WARNING_LEVEL: onWarningEvent.ReleaseHandler(object, function); break;
	case FUError::ERROR_LEVEL: onErrorEvent.ReleaseHandler(object, function); break;
	case FUError::DEBUG_LEVEL: onDebugEvent.ReleaseHandler(object, function); break;
	case FUError::LEVEL_COUNT: default: FUBreak;
	}

	criticalSection.Leave();
}

const char* FUError::GetErrorString(FUError::Code errorCode)
{
	switch (errorCode)
	{
	case ERROR_DEFAULT_ERROR: return "Generic Error."; 
	case ERROR_MALFORMED_XML: return "Corrupted COLLADA document: malformed XML."; 
	case ERROR_PARSING_FAILED: return "Exception caught while parsing a COLLADA document from file."; 
	case ERROR_INVALID_ELEMENT: return "Invalid or unexpected XML element."; 
	case ERROR_MISSING_ELEMENT: return "Missing, necessary XML element."; 
	case ERROR_UNKNOWN_ELEMENT: return "Unknown element: parsing error."; 
	case ERROR_MISSING_INPUT: return "Missing necessary COLLADA <input>."; 
	case ERROR_INVALID_URI: return "Incomplete or invalid URI fragment."; 
	case ERROR_WRITE_FILE: return "Unable to write COLLADA document to file."; 
	case ERROR_MISSING_PROPERTY: return "Missing necessary XML property."; 
	case NO_MATCHING_PLUGIN: return "No plug-in available for this task."; 

	case ERROR_ANIM_CURVE_DRIVER_MISSING: return "Unable to find animation curve driver."; 
	case ERROR_SOURCE_SIZE: return "Expecting sources to be the same size."; 

	case ERROR_IB_MATRIX_MISSING: return "No inverted bind matrix input in controller."; 
	case ERROR_VCOUNT_MISSING: return "Expecting <vcount> element in combiner for controller."; 
	case ERROR_V_ELEMENT_MISSING: return "Expecting <v> element after <vcount> element in combiner for controller."; 
	case ERROR_JC_BPMC_NOT_EQUAL: return "Joint count and bind pose matrix count aren't equal for controller."; 
	case ERROR_INVALID_VCOUNT: return "The <vcount> element list should contains the number of values determined by the <vertex_weights>'s 'count' attribute."; 
	case ERROR_PARSING_PROG_ERROR: return "Parsing programming error in controller."; 
	case ERROR_UNKNOWN_CHILD: return "Unknown child in <geometry> with id."; 
	case ERROR_UNKNOWN_GEO_CH: return "Unknown geometry for creation of convex hull of."; 
	case ERROR_UNKNOWN_MESH_ID: return "Mesh has source with an unknown id."; 
	case ERROR_INVALID_U_KNOT: return "Found non-ascending U knot vector"; 
	case ERROR_INVALID_V_KNOT: return "Found non-ascending V knot vector"; 
	case ERROR_NOT_ENOUGHT_U_KNOT: return "Not enough elements in the U knot vector."; 
	case ERROR_NOT_ENOUGHT_V_KNOT: return "Not enough elements in the V knot vector."; 
	case ERROR_INVALID_CONTROL_VERTICES: return "Found unexpected number of control vertices."; 
	case ERROR_NO_CONTROL_VERTICES: return "No <control_vertices> element in NURBS surface."; 
	case ERROR_UNKNOWN_POLYGONS: return "Unknown polygons element in geometry."; 
	case WARNING_NO_POLYGON: return "No polygon <p>/<vcount> element found in geometry."; 
	case ERROR_NO_VERTEX_INPUT: return "Cannot find 'VERTEX' polygons' input within geometry."; 
	case ERROR_NO_VCOUNT: return "No or empty <vcount> element found in geometry." ; 
	case ERROR_MISPLACED_VCOUNT: return "<vcount> is only expected with the <polylist> element in geometry."; 
	case ERROR_UNKNOWN_PH_ELEMENT: return "Unknown element found in <ph> element for geometry."; 
	case ERROR_INVALID_FACE_COUNT: return "Face count for polygons node doesn't match actual number of faces found in <p> element(s) in geometry."; 
	case ERROR_DUPLICATE_ID: return "Geometry source has duplicate 'id'."; 
	case ERROR_INVALID_CVS_WEIGHTS: return "Numbers of CVs and weights are different in NURB spline."; 
	case ERROR_INVALID_SPLINE: return "Invalid spline. Equation \"n = k - d - 1\" is not respected."; 
	case ERROR_UNKNOWN_EFFECT_CODE: return "Unknown effect code type."; 
	case ERROR_BAD_FLOAT_VALUE: return "Bad value for float parameter in integer parameter."; 
	case ERROR_BAD_BOOLEAN_VALUE: return "Bad value for boolean parameter in effect."; 
	case ERROR_BAD_FLOAT_PARAM: return "Bad float value for float parameter."; 
	case ERROR_BAD_FLOAT_PARAM2: return "Bad value for float2 parameter."; 
	case ERROR_BAD_FLOAT_PARAM3: return "Bad value for float3 parameter."; 
	case ERROR_BAD_FLOAT_PARAM4: return "Bad value for float4 parameter."; 
	case ERROR_BAD_MATRIX: return "Bad value for matrix parameter."; 
	case ERROR_PROG_NODE_MISSING: return "Unable to find the program node for standard effect."; 
	case ERROR_INVALID_TEXTURE_SAMPLER: return "Unexpected texture sampler on some parameters for material."; 
	case ERROR_PARAM_NODE_MISSING: return "Cannot find parameter node referenced by."; 
	case ERROR_INVALID_IMAGE_FILENAME: return "Invalid filename for image: "; 
	case ERROR_UNKNOWN_TEXTURE_SAMPLER: return "Unknown texture sampler element."; 
	case ERROR_COMMON_TECHNIQUE_MISSING: return "Unable to find common technique for physics material."; 
	case ERROR_TECHNIQUE_NODE_MISSING: return "Technique node not specified."; 
	case ERROR_PHYSICS_MODEL_CYCLE_DETECTED: return "A cycle was found in the physics model."; 

	case ERROR_MAX_CANNOT_RESIZE_MUT_LIST: return "Cannot Resize the ParticleMutationsList"; 
	case WARNING_UNSUPPORTED_TEXTURE_UVS: return "3D Studio Max does not support both UV Offset and UV Frame Translate";

	case WARNING_MISSING_URI_TARGET: return "Missing or invalid URI target."; 
	case WARNING_UNKNOWN_CHILD_ELEMENT: return "Unknown <asset> child element"; 
	case WARNING_UNKNOWN_AC_CHILD_ELEMENT: return "Unknown <asset><contributor> child element."; 
	case WARNING_BASE_NODE_TYPE: return "Unknown base node type."; 
	case WARNING_INST_ENTITY_MISSING: return "Unable to find instantiated entity."; 
	case WARNING_INVALID_MATERIAL_BINDING: return "Invalid material binding in geometry instantiation."; 
	case WARNING_UNKNOWN_MAT_ID: return "Unknown material id or semantic."; 
	case WARNING_RIGID_CONSTRAINT_MISSING: return "Couldn't find rigid constraint for instantiation."; 
	case WARNING_INVALID_ANIM_LIB: return "Animation library contains unknown element."; 
	case WARNING_INVALID_ANIM_TARGET: return "Animation Channel target is invalid"; 
	case WARNING_UNKNOWN_ANIM_LIB_ELEMENT: return "Unknown element in animation clip library."; 
	case WARNING_INVALID_SE_PAIR: return "Invalid start/end pair for animation clip."; 
	case WARNING_CURVES_MISSING: return "No curves instantiated by animation."; 
	case WARNING_EMPTY_ANIM_CLIP: return "Empty animation clip."; 
	case WARNING_UNKNOWN_CAM_ELEMENT: return "Camera library contains unknown element."; 
	case WARNING_NO_STD_PROG_TYPE: return "No standard program type for camera."; 
	case WARNING_PARAM_ROOT_MISSING: return "Cannot find parameter root node for camera."; 
	case WARNING_UNKNOWN_CAM_PROG_TYPE: return "Unknown program type for camera."; 
	case WARNING_UNKNOWN_CAM_PARAM: return "Unknown parameter for camera."; 
	case WARNING_UNKNOWN_LIGHT_LIB_ELEMENT: return "Light library contains unknown element."; 
	case WARNING_UNKNOWN_LIGHT_TYPE_VALUE: return "Unknown light type value for light."; 
	case WARNING_UNKNOWN_LT_ELEMENT: return "Unknown element under <light><technique_common> for light."; 
	case WARNING_UNKNOWN_LIGHT_PROG_PARAM: return "Unknown program parameter for light."; 
	case WARNING_INVALID_CONTROLLER_LIB_NODE: return "Unexpected node in controller library."; 
	case WARNING_CONTROLLER_TYPE_CONFLICT: return "A controller cannot be both a skin and a morpher."; 
	case WARNING_SM_BASE_MISSING: return "No base type element, <skin> or <morph>, found for controller."; 
	case WARNING_UNKNOWN_MC_PROC_METHOD: return "Unknown processing method from morph controller."; 
	case WARNING_UNKNOWN_MC_BASE_TARGET_MISSING: return "Cannot find base target for morph controller."; 
	case WARNING_UNKNOWN_MORPH_TARGET_TYPE: return "Unknown morph targets input type in morph controller."; 
	case WARNING_TARGET_GEOMETRY_MISSING: return "Unable to find target geometry."; 
	case WARNING_CONTROLLER_TARGET_MISSING: return "Target not found for controller."; 
	case WARNING_UNKNOWN_SC_VERTEX_INPUT: return "Unknown vertex input in skin controller."; 
	case WARNING_INVALID_TARGET_GEOMETRY_OP: return "Unable to clone/find the target geometry for controller."; 
	case WARNING_INVALID_JOINT_INDEX: return "Joint index out of bounds in combiner for controller."; 
	case WARNING_INVALID_WEIGHT_INDEX: return "Weight index out of bounds in combiner for controller."; 
	case WARNING_UNKNOWN_JOINT: return "Unknown joint."; 
	case WARNING_UNKNOWN_GL_ELEMENT: return "Geometry library contains unknown element."; 
	case WARNING_EMPTY_GEOMETRY: return "No mesh, spline or NURBS surfaces found within geometry."; 
	case WARNING_MESH_VERTICES_MISSING: return "No <vertices> element in mesh."; 
	case WARNING_VP_INPUT_NODE_MISSING: return "No vertex position input node in mesh."; 
	case WARNING_GEOMETRY_VERTICES_MISSING: return "Empty <vertices> element in geometry."; 
	case WARNING_MESH_TESSELLATION_MISSING: return "No tessellation found for mesh."; 
	case WARNING_INVALID_POLYGON_MAT_SYMBOL: return "Unknown or missing polygonal material symbol in geometry."; 
	case WARNING_EXTRA_VERTEX_INPUT: return "There should never be more than one 'VERTEX' input in a mesh: skipping extra 'VERTEX' inputs."; 
	case WARNING_UNKNOWN_POLYGONS_INPUT: return "Unknown polygons set input."; 
	case WARNING_UNKNOWN_POLYGON_CHILD: return "Unknown polygon child element in geometry."; 
	case WARNING_INVALID_PRIMITIVE_COUNT: return "Primitive count for mesh node doesn't match actual number of primitives found in <p> element(s) in geometry."; 
	case WARNING_INVALID_GEOMETRY_SOURCE_ID: return "Geometry source with no 'id' is unusable."; 
	case WARNING_EMPTY_SOURCE: return "Geometry has source with no data."; 
	case WARNING_EMPTY_POLYGONS: return "Polygons is empty.";
	case WARNING_SPLINE_CONTROL_INPUT_MISSING: return "No control vertice input in spline."; 
	case WARNING_CONTROL_VERTICES_MISSING: return "No <control_vertices> element in spline."; 
	case WARNING_VARYING_SPLINE_TYPE: return "Geometry contains different kinds of splines."; 
	case WARNING_UNKNOWN_EFFECT_ELEMENT: return "Unknown element in effect library."; 
	case WARNING_UNSUPPORTED_PROFILE: return "Unsupported profile or unknown element in effect."; 
	case WARNING_SID_MISSING: return "<code>/<include> nodes must have an 'sid' attribute to identify them."; 
	case WARNING_INVALID_ANNO_TYPE: return "Annotation has none-supported type."; 
	case WARNING_GEN_REF_ATTRIBUTE_MISSING: return "No reference attribute on generator parameter."; 
	case WARNING_MOD_REF_ATTRIBUTE_MISSING: return "No reference attribute on modifier parameter."; 
	case WARNING_SAMPLER_NODE_MISSING: return "Unable to find sampler node for sampler parameter."; 
	case WARNING_EMPTY_SURFACE_SOURCE: return "Empty surface source value for sampler parameter."; 
	case WARNING_EMPTY_INIT_FROM: return "<init_from> element is empty in surface parameter."; 
	case WARNING_EMPTY_IMAGE_NAME: return "Empty image name for surface parameter."; 
	case WARNING_UNKNOWN_PASS_ELEMENT: return "Pass contains unknown element."; 
	case WARNING_UNKNOWN_PASS_SHADER_ELEMENT: return "Pass shader contains unknown element."; 
	case WARNING_UNAMED_EFFECT_PASS_SHADER: return "Unnamed effect pass shader found."; 
	case WARNING_UNKNOWN_EPS_STAGE: return "Unknown stage for effect pass shader."; 
	case WARNING_INVALID_PROFILE_INPUT_NODE: return "Invalid profile input node for effect"; 
	case WARNING_UNKNOWN_STD_MAT_BASE_ELEMENT: return "Unknown element as standard material base."; 
	case WARNING_TECHNIQUE_MISSING: return "Expecting <technique> within the <profile_COMMON> element for effect."; 
	case WARNING_UNKNOWN_MAT_INPUT_SEMANTIC: return "Unknown input semantic in material."; 
	case WARNING_UNKNOWN_INPUT_TEXTURE: return "Unknown input texture."; 
	case WARNING_UNSUPPORTED_SHADER_TYPE: return "Unsupported shader program type."; 
	case WARNING_UNKNOWN_MAT_PARAM_NAME: return "Unknown parameter name for material."; 
	case WARNING_UNKNOWN_TECHNIQUE_ELEMENT: return "Technique contains unknown element."; 
	case WARNING_UNKNOWN_IMAGE_LIB_ELEMENT: return "Image library contains unknown element."; 
	case WARNING_UNKNOWN_TEX_LIB_ELEMENT: return "Texture library contains unknown element."; 
	case WARNING_UNKNOWN_CHANNEL_USAGE: return "Unknown channel usage for texture."; 
	case WARNING_UNKNOWN_INPUTE_SEMANTIC: return "Unknown input semantic for texture."; 
	case WARNING_UNKNOWN_IMAGE_SOURCE: return "Unknown or external image source for texture."; 
	case WARNING_UNKNOWN_MAT_LIB_ELEMENT: return "Unknown element in material library."; 
	case WARNING_UNSUPPORTED_REF_EFFECTS: return "Externally referenced effects are not supported. Material."; 
	case WARNING_EMPTY_INSTANCE_EFFECT: return "Empty material's <instance_effect> definition. Should instantiate an effect from the effect's library. Material."; 
	case WARNING_EFFECT_MISSING: return "Unable to find effect for material."; 
	case WARNING_UNKNOWN_FORCE_FIELD_ELEMENT: return "Force field library contains unknown element."; 
	case WARNING_UNKNOWN_ELEMENT: return "Unknown element."; 
	case WARNING_INVALID_BOX_TYPE: return "Box is not of the right type."; 
	case WARNING_INVALID_PLANE_TYPE: return "Plane is not of the right type."; 
	case WARNING_INVALID_SPHERE_TYPE: return "Sphere is not of the right type."; 
	case WARNING_INVALID_CAPSULE_TYPE: return "Capsule is not of the right type."; 
	case WARNING_INVALID_TCAPSULE_TYPE: return "Tapered Capsule is not of the right type."; 
	case WARNING_INVALID_TCYLINDER_TYPE: return "Tapered cylinder is not of the right type."; 
	case WARNING_UNKNOWN_PHYS_MAT_LIB_ELEMENT: return "Unknown element in physics material library."; 

	case WARNING_UNKNOWN_PHYS_LIB_ELEMENT: return "PhysicsModel library contains unknown element."; 
	case WARNING_CORRUPTED_INSTANCE: return "Unable to retrieve instance for scene node."; 
	case WARNING_UNKNOWN_PRB_LIB_ELEMENT: return "PhysicsRigidBody library contains unknown element."; 
	case WARNING_PHYS_MAT_INST_MISSING: return "Error: Instantiated physics material in rigid body was not found."; 
	case WARNING_PHYS_MAT_DEF_MISSING: return "No physics material defined in rigid body."; 
	case WARNING_UNKNOWN_RGC_LIB_ELEMENT: return "PhysicsRigidConstraint library contains unknown element."; 
	case WARNING_INVALID_NODE_TRANSFORM: return "Invalid node transform."; 
	case WARNING_RF_NODE_MISSING: return "Reference-frame rigid body/scene node not defined in rigid constraint."; 
	case WARNING_RF_REF_NODE_MISSING: return "Reference-frame rigid body/scene node specified in rigid_constraint not found in physics model."; 
	case WARNING_TARGET_BS_NODE_MISSING: return "Target rigid body/scene node not defined in rigid constraint."; 
	case WARNING_TARGE_BS_REF_NODE_MISSING: return "Target rigid body/scene node specified in rigid_constraint not found in physics model."; 
	case WARNING_UNKNOW_PS_LIB_ELEMENT: return "PhysicsShape library contains unknown element."; 
	case WARNING_FCDGEOMETRY_INST_MISSING: return "Unable to retrieve FCDGeometry instance for scene node. "; 
	case WARNING_INVALID_SHAPE_NODE: return "Invalid shape."; 
	case WARNING_UNKNOW_NODE_ELEMENT_TYPE: return "Unknown node type for scene's <node> element."; 
	case WARNING_CYCLE_DETECTED: return "A cycle was found in the visual scene at node."; 
	case WARNING_INVALID_NODE_INST: return "Unable to retrieve node instance for scene node."; 
	case WARNING_INVALID_WEAK_NODE_INST: return "Unable to retrieve weakly-typed instance for scene node."; 
	case WARNING_UNSUPPORTED_EXTERN_REF: return "FCollada does not support external references for this element/entity."; 
	case WARNING_UNEXPECTED_ASSET: return "Found more than one asset present in scene node."; 
	case WARNING_INVALID_TRANSFORM: return "Unknown element or bad transform in scene node."; 
	case WARNING_TARGET_SCENE_NODE_MISSING: return "Unable to find target scene node for object."; 
	case WARNING_XREF_UNASSIGNED: return "XRef imported but not instanciated."; 
	case WARNING_UNSUPPORTED_EXTERN_REF_NODE: return "Unsupported external reference node."; 

	case WARNING_SHAPE_NODE_MISSING: return "Shape node missing."; 
	case WARNING_MASS_AND_DENSITY_MISSING: return "Mass and density missing."; 
	
	case DEBUG_LOAD_SUCCESSFUL: return "COLLADA document loaded successfully."; 
	case DEBUG_WRITE_SUCCESSFUL: return "COLLADA document written successfully."; 

	case ERROR_CUSTOM_STRING: return customErrorString.c_str(); 
	default: return "Unknown error code.";
	}
}

void FUError::SetCustomErrorString(const char* _customErrorString)
{
	customErrorString = _customErrorString;
}

//
// FUErrorSimpleHandler
//

FUErrorSimpleHandler::FUErrorSimpleHandler(FUError::Level fatalLevel)
:	localFatalityLevel(fatalLevel), fails(false)
{
	FUError::AddErrorCallback(FUError::DEBUG_LEVEL, this, &FUErrorSimpleHandler::OnError);
	FUError::AddErrorCallback(FUError::WARNING_LEVEL, this, &FUErrorSimpleHandler::OnError);
	FUError::AddErrorCallback(FUError::ERROR_LEVEL, this, &FUErrorSimpleHandler::OnError);
}

FUErrorSimpleHandler::~FUErrorSimpleHandler()
{
	FUError::RemoveErrorCallback(FUError::DEBUG_LEVEL, this, &FUErrorSimpleHandler::OnError);
	FUError::RemoveErrorCallback(FUError::WARNING_LEVEL, this, &FUErrorSimpleHandler::OnError);
	FUError::RemoveErrorCallback(FUError::ERROR_LEVEL, this, &FUErrorSimpleHandler::OnError);
}

void FUErrorSimpleHandler::OnError(FUError::Level errorLevel, uint32 errorCode, uint32 lineNumber)
{
	FUSStringBuilder newLine(256);
	newLine.append('['); newLine.append(lineNumber); newLine.append("] ");
	if (errorLevel == FUError::WARNING_LEVEL) newLine.append("Warning: ");
	else if (errorLevel == FUError::ERROR_LEVEL) newLine.append("ERROR: ");
	const char* errorString = FUError::GetErrorString((FUError::Code) errorCode);
	if (errorString != NULL) newLine.append(errorString);
	else
	{
		newLine.append("Unknown error code: ");
		newLine.append(errorCode);
	}

	if (message.length() > 0) message.append('\n');
	message.append(newLine);

	fails |= errorLevel >= localFatalityLevel; 
}
