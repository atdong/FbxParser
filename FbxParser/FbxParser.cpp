#include "FbxParser.h"
#include <assert.h>

FbxParser::FbxParser(FbxString fbxFile) :
pManager(nullptr), pScene(nullptr), pMesh(nullptr), fbxFile(fbxFile), controlPoints()
{
	initFbxObjects();
}

void FbxParser::initFbxObjects()
{
	//create the FBX manager which is the object allocator for almost all the classes in the SDK
	pManager = FbxManager::Create();
	if (!pManager) {
		FBXSDK_printf("error: unable to create FBX manager!\n");
		exit(1);
	}
	else
		FBXSDK_printf("Autodesk FBX SDK version:%s\n", pManager->GetVersion());

	//create an IOSettings object. This object holds all import/export settings
	FbxIOSettings *ios = FbxIOSettings::Create(pManager, IOSROOT);
	pManager->SetIOSettings(ios);

	//load plugins from the executable directory
	FbxString path = FbxGetApplicationDirectory();
	FBXSDK_printf("application path: %s\n", path.Buffer());
	pManager->LoadPluginsDirectory(path.Buffer());

	//create an FBX scene. This object holds most objects imported/exported from/to files
	pScene = FbxScene::Create(pManager, "my scene");
	if (!pScene) {
		FBXSDK_printf("error: unable to create FBX scene\n");
		exit(1);
	}
}

bool FbxParser::loadScene()
{
	assert(pManager != nullptr && pScene != nullptr);

	int animStackCount = 0;
	bool status = false;
	//create an importer
	FbxImporter *importer = FbxImporter::Create(pManager, "");

	//initialize the importer by providing a file name
	const bool imorterStatus = importer->Initialize(fbxFile, -1, pManager->GetIOSettings());
	if (!imorterStatus) {
		FBXSDK_printf("error: initialize importer failed\n");
		return status;
	}

	if (importer->IsFBX()) {
		FBXSDK_printf("\n\n-------Animation Stack Information-------\n\n");

		animStackCount = importer->GetAnimStackCount();
		FBXSDK_printf("number of animation stacks: %d\n", animStackCount);
		FBXSDK_printf("current animation stack: %s\n", importer->GetActiveAnimStackName().Buffer());

		for (int i = 0; i != animStackCount; ++i) {
			FbxTakeInfo *takeInfo = importer->GetTakeInfo(i);

			FBXSDK_printf("animation stack %d\n", i);
			FBXSDK_printf("	name: %s\n", takeInfo->mName.Buffer());
			FBXSDK_printf("	description: %s\n", takeInfo->mDescription.Buffer());
			FBXSDK_printf("	import name: %s\n", takeInfo->mImportName.Buffer());
			FBXSDK_printf("	import state: %s\n\n", takeInfo->mSelect ? "true" : "false");

			// Set the import states. By default, the import states are always set to 
			// true. The code below shows how to change these states.
			IOS_REF.SetBoolProp(IMP_FBX_MATERIAL, true);
			IOS_REF.SetBoolProp(IMP_FBX_TEXTURE, true);
			IOS_REF.SetBoolProp(IMP_FBX_LINK, true);
			IOS_REF.SetBoolProp(IMP_FBX_SHAPE, true);
			IOS_REF.SetBoolProp(IMP_FBX_GOBO, true);
			IOS_REF.SetBoolProp(IMP_FBX_ANIMATION, true);
			IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
		}

		//import the scene
		status = importer->Import(pScene);
		char password[1024];
		if (status == false && importer->GetStatus().GetCode() == FbxStatus::ePasswordError)
		{
			FBXSDK_printf("Please enter password: ");
			password[0] = '\0';

			FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
				scanf("%s", password);
			FBXSDK_CRT_SECURE_NO_WARNING_END

				FbxString pwd(password);

			IOS_REF.SetStringProp(IMP_FBX_PASSWORD, pwd);
			IOS_REF.SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

			status = importer->Import(pScene);

			if (status == false && importer->GetStatus().GetCode() == FbxStatus::ePasswordError)
			{
				FBXSDK_printf("\nPassword is wrong, import aborted.\n");
			}
		}
	}

	//destroy the importer
	importer->Destroy();

	return status;
}

void FbxParser::displayMetaData(FbxScene *pScene)
{
	FBXSDK_printf("\n\n--------------------------Meta Data------------------------------\n\n");
	FbxDocumentInfo *sceneInfo = pScene->GetSceneInfo();
	if (sceneInfo) {
		FBXSDK_printf("title: %s\n", sceneInfo->mTitle.Buffer());
		FBXSDK_printf("author: %s\n", sceneInfo->mAuthor.Buffer());
		FBXSDK_printf("comment: %s\n", sceneInfo->mComment.Buffer());
		FBXSDK_printf("keywords: %s\n", sceneInfo->mKeywords.Buffer());
		FBXSDK_printf("revision: %s\n", sceneInfo->mRevision.Buffer());
		FBXSDK_printf("subject: %s\n", sceneInfo->mSubject.Buffer());

		FbxThumbnail *thumbnail = sceneInfo->GetSceneThumbnail();
		if (thumbnail) {
			FBXSDK_printf("Thumbnail:\n");

			switch (thumbnail->GetDataFormat())
			{
			case FbxThumbnail::eRGB_24:
				FBXSDK_printf("RGB\n");
				break;
			case FbxThumbnail::eRGBA_32:
				FBXSDK_printf("RGBA\n");
				break;
			default:
				break;
			}

			switch (thumbnail->GetSize())
			{
			case FbxThumbnail::eNotSet:
				FBXSDK_printf("Size: no dimensions specified (%ld bytes)\n", thumbnail->GetSizeInBytes());
				break;
			case FbxThumbnail::e64x64:
				FBXSDK_printf("Size: 64 x 64 pixels (%ld bytes)\n", thumbnail->GetSizeInBytes());
				break;
			case FbxThumbnail::e128x128:
				FBXSDK_printf("Size: 128 x 128 pixels (%ld bytes)\n", thumbnail->GetSizeInBytes());
			default:
				break;
			}
		}
		else {
			FBXSDK_printf("error: get thumbnail failed.\n");
		}

	}
}

void FbxParser::displayGlobalLightSettings(FbxGlobalSettings *pGlobalSettings)
{
	FBXSDK_printf("\n\n---------------------Global Light Settings---------------------\n\n");

	FbxColor color = pGlobalSettings->GetAmbientColor();
	FBXSDK_printf("ambient color: \n(red)%lf, (green)%lf, (blue)%lf\n\n", color.mRed, color.mGreen, color.mBlue);
}

void FbxParser::displayHierarchy(FbxNode *node, int depth)
{
	FbxString nodeName("");
	for (int i = 0; i != depth; ++i) {
		nodeName += "   ";
	}
	nodeName += node->GetName();
	nodeName += "\n";

	FBXSDK_printf(nodeName.Buffer());

	for (int i = 0; i != node->GetChildCount(); ++i) {
		displayHierarchy(node->GetChild(i), depth + 1);
	}
}

void FbxParser::displayHierarchy(FbxScene *pScene)
{
	FBXSDK_printf("\n\n---------------------------Hierarchy-------------------------------\n\n");

	FbxNode *node = pScene->GetRootNode();
	for (int i = 0; i != node->GetChildCount(); ++i) {
		displayHierarchy(node->GetChild(i), 0);
	}
}

void FbxParser::displayContent(FbxScene *pScene)
{
	FBXSDK_printf("\n\n------------------------Node Content---------------------------\n\n");

	FbxNode *node = pScene->GetRootNode();
	if (node) {
		for (int i = 0; i != node->GetChildCount(); ++i) {
			displayContent(node->GetChild(i));
		}
	}
	else {
		FBXSDK_printf("null node!\n");
	}
}

void FbxParser::displayContent(FbxNode *node)
{
	displayTexture(node);

	FbxNodeAttribute::EType attributeType;
	if (!node->GetNodeAttribute()) {
		FBXSDK_printf("null node attribute.\n");
	}
	else {
		attributeType = node->GetNodeAttribute()->GetAttributeType();

		switch (attributeType)
		{
		case fbxsdk::FbxNodeAttribute::eMarker:
			displayMarker(node);
			break;
		case fbxsdk::FbxNodeAttribute::eSkeleton:
			displaySkeleton(node);
			break;
		case fbxsdk::FbxNodeAttribute::eMesh:
			displayMesh(node);
			break;
		case fbxsdk::FbxNodeAttribute::eCamera:
			FBXSDK_printf("eCamera\n");
			break;
		default:
			FBXSDK_printf("default\n");
			break;
		}
	}
}

void FbxParser::displayTexture(FbxNode *node)
{
	FbxMesh *nodeMesh = node->GetMesh();
	if (nodeMesh) {
		int materialCount = node->GetSrcObjectCount<FbxSurfaceMaterial>();
		for (int index = 0; index != materialCount; ++index) {
			FbxSurfaceMaterial *material = (FbxSurfaceMaterial*)node->GetSrcObject<FbxSurfaceMaterial>(index);
			if (material) {
				FBXSDK_printf("material name: %s\n", material->GetName());
				FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);

				int layeredTextureCount = prop.GetSrcObjectCount<FbxLayeredTexture>();
				if (layeredTextureCount > 0) {
					FBXSDK_printf("layeredTextureCount\n");
					for (int layerIndex = 0; layerIndex != layeredTextureCount; ++layerIndex) {
						FbxLayeredTexture *layeredTexture = prop.GetSrcObject<FbxLayeredTexture>(layerIndex);
						int count = layeredTexture->GetSrcObjectCount<FbxTexture>();
						for (int c = 0; c != count; ++c) {
							FbxFileTexture *fileTexture = FbxCast<FbxFileTexture>(layeredTexture->GetSrcObject<FbxFileTexture>(c));
							const char *textureName = fileTexture->GetFileName();
							FBXSDK_printf("texture name: %s\n", textureName);
						}
					}
				}
				else {
					FBXSDK_printf("textureName\n");
					int textureCount = prop.GetSrcObjectCount<FbxTexture>();
					for (int i = 0; i != textureCount; ++i) {
						FbxFileTexture *fileTexture = FbxCast<FbxFileTexture>(prop.GetSrcObject<FbxFileTexture>(i));
						const char *textureName = fileTexture->GetFileName();
						FBXSDK_printf("texture name: %s\n", textureName);
					}
				}
			}
		}
	}
}

void FbxParser::displayMarker(FbxNode *node)
{
	FbxMarker *marker = (FbxMarker*)node->GetNodeAttribute();
	FbxString markerInfo("");
	markerInfo += "marker name: " + FbxString(node->GetName()) + "\n";
	FBXSDK_printf(markerInfo.Buffer());

	//type
	markerInfo = "marker type:\n";
	switch (marker->GetType())
	{
	case FbxMarker::eStandard:
		markerInfo += "standard\n";
		break;
	case FbxMarker::eOptical:
		markerInfo += "optical\n";
		break;
	case FbxMarker::eEffectorIK:
		markerInfo += "IK Effector\n";
		break;
	case FbxMarker::eEffectorFK:
		markerInfo += "FK Effector\n";
		break;
	default:
		break;
	}
	FBXSDK_printf(markerInfo.Buffer());

	//look
	markerInfo = "marker look:\n";
	switch (marker->Look.Get())
	{
	case FbxMarker::eCube:
		markerInfo += "Cube\n";
		break;
	case FbxMarker::eHardCross:
		markerInfo += "Hard Cross\n";
		break;
	case FbxMarker::eLightCross:
		markerInfo += "Light Cross\n";
		break;
	case FbxMarker::eSphere:
		markerInfo += "Sphere\n";
		break;
	default:
		break;
	}
	FBXSDK_printf(markerInfo.Buffer());

	//size
	markerInfo = "size: " + FbxString(marker->Size.Get());
	FBXSDK_printf(markerInfo.Buffer());

	//color
	FbxDouble3 c = marker->Color.Get();
	FbxColor color(c[0], c[1], c[2]);
	FBXSDK_printf("red: %lf,    green: %lf,    blue: %lf", color.mRed, color.mGreen, color.mBlue);

	//IKPivot
	FbxDouble3 pivot = marker->IKPivot.Get();
	FBXSDK_printf("IKPivot:    %lf, %lf, %lf", pivot.mData[0], pivot.mData[1], pivot.mData[2]);
}

void FbxParser::displayMesh(FbxNode *node)
{
	pMesh = (FbxMesh*)node->GetNodeAttribute();

	FbxString meshInfo("");
	meshInfo += "mesh name: " + FbxString(node->GetName()) + "\n";
	FBXSDK_printf(meshInfo.Buffer());

	//Meta Data
	int metaDataCount = ((FbxObject*)pMesh)->GetSrcObjectCount<FbxObjectMetaData>();
	if (metaDataCount > 0) {
		for (int i = 0; i != metaDataCount; ++i) {
			FbxObjectMetaData *metaData = ((FbxObject*)pMesh)->GetSrcObject<FbxObjectMetaData>(i);
			FBXSDK_printf("name: %s\n", metaData->GetName());
		}
	}
	else    FBXSDK_printf("get meta data failed.\n\n");

	//Control Points
	int controlPointsCount = pMesh->GetControlPointsCount();
	controlPoints = pMesh->GetControlPoints();

	FBXSDK_printf("\n\n---------------Control Points---------------------\n\n");
	for (int i = 0; i != controlPointsCount; ++i) {
		//FBXSDK_printf("    Control Point %d", i + 1);
		//display3DVector("        Coordinates: ", controlPoints[i]);

		for (int j = 0; j != pMesh->GetElementNormalCount(); ++j) {
			FbxGeometryElementNormal *geoEleNormal = pMesh->GetElementNormal(j);
			switch (geoEleNormal->GetMappingMode())
			{
			case FbxGeometryElement::eByControlPoint:
				switch (geoEleNormal->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
					FBXSDK_printf("eDirect\n\n");
					break;
				case FbxGeometryElement::eIndexToDirect:
					FBXSDK_printf("eIndexToDirect\n\n");
					break;
				default:
					FBXSDK_printf("default element\n\n");
					break;
				}
				break;
			case FbxGeometryElement::eByPolygonVertex:
				//FBXSDK_printf("eByPolygonVertex\n");
				switch (geoEleNormal->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
					/*FBXSDK_printf("eDirect\n");
					display3DVector("direct array:\n", geoEleNormal->GetDirectArray().GetAt(i));
					FBXSDK_printf("\n");*/
					break;
				case FbxGeometryElement::eIndexToDirect:
					FBXSDK_printf("eIndexToDirect\n\n");
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
			/*if (geoEleNormal->GetMappingMode() == FbxGeometryElement::eByControlPoint) {
			FBXSDK_printf("eByControlPoint\n");
			char header[100];
			FBXSDK_sprintf(header, 100, "        normal vector:\n");
			if (geoEleNormal->GetReferenceMode() == FbxGeometryElement::eDirect) {
			display3DVector(header, geoEleNormal->GetDirectArray().GetAt(i));
			}
			}
			else if (geoEleNormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
			FBXSDK_printf("eByPolygonVertex\n");


			}*/
		}
	}
	//glutDisplayFunc(displayModel);
	FBXSDK_printf("\n\n");
}

void FbxParser::displaySkeleton(FbxNode *node)
{
	FBXSDK_printf("\n\n");
	FbxSkeleton *skeleton = (FbxSkeleton*)node->GetNodeAttribute();

	FbxString skeletonInfo("");
	skeletonInfo += "skeleton name: " + FbxString(node->GetName()) + "\n";
	FBXSDK_printf(skeletonInfo.Buffer());

	int srcObjCount = ((FbxObject*)skeleton)->GetSrcObjectCount<FbxObjectMetaData>();
	if (srcObjCount > 0)
		FBXSDK_printf("meta data connections\n");
	else
		FBXSDK_printf("meta data connections failed\n");

	for (int i = 0; i != srcObjCount; ++i) {
		FbxObjectMetaData *metaData = ((FbxObject*)skeleton)->GetSrcObject<FbxObjectMetaData>(i);
		FBXSDK_printf("name: %s\n", metaData->GetName());
	}

	const char* skeletonTypes[] = { "Root", "Limb", "Limb Node", "Effector" };
	FBXSDK_printf("skeleton type: %s\n", skeletonTypes[skeleton->GetSkeletonType()]);

	if (skeleton->GetSkeletonType() == FbxSkeleton::eLimb) {
		FBXSDK_printf("limb type: %lf\n", skeleton->LimbLength.Get());
	}
	else if (skeleton->GetSkeletonType() == FbxSkeleton::eLimbNode) {
		FBXSDK_printf("limb node size: %lf\n", skeleton->Size.Get());
	}
	else if (skeleton->GetSkeletonType() == FbxSkeleton::eRoot) {
		FBXSDK_printf("limb root size: %lf\n", skeleton->Size.Get());
	}
	else {
		FBXSDK_printf("undefined skeleton type: %s\n", skeleton->GetSkeletonType());
	}

	FbxColor color = skeleton->GetLimbNodeColor();
	FBXSDK_printf("color:	red: %lf, green: %lf, blue: %lf\n", color[0], color[1], color[2]);
}