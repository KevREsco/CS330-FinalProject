///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = CreateGLTexture(
		"../../Utilities/textures/WoodTable.jpg",
		"planeTexture");
	if (!bReturn) {
		std::cout << "Failed to load plane texture." << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../Utilities/textures/greenmetal.jpg",
		"phoneTexture");
	if (!bReturn) {
		std::cout << "Failed to load phone texture." << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../Utilities/textures/camera.jpg",
		"cameraTexture");
	if (!bReturn) {
		std::cout << "Failed to load camera texture." << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../Utilities/textures/stainless.jpg",
		"silverTexture");
	if (!bReturn) {
		std::cout << "Failed to load silver ring texture." << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../Utilities/textures/gold-seamless-texture.jpg",
		"conductorTexture");
	if (!bReturn) {
		std::cout << "Failed to load camera texture." << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../Utilities/textures/trav.jpg",
		"pyramidTexture");
	if (!bReturn) {
		std::cout << "Failed to load phone texture." << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../Utilities/textures/plastic.jpg",
		"plasticTexture");
	if (!bReturn) {
		std::cout << "Failed to load phone texture." << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../Utilities/textures/sphere.jpg",
		"sphereTexture");
	if (!bReturn) {
		std::cout << "Failed to load phone texture." << std::endl;
	}

	BindGLTextures();
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadPyramid4Mesh();

	// -------------------------------------------------------
	// Define materials for plane, phone, etc.
	// -------------------------------------------------------
	{
		OBJECT_MATERIAL planeMat;
		planeMat.tag = "planeMaterial";
		// A bit reflective
		planeMat.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
		planeMat.ambientStrength = 0.2f;
		planeMat.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
		planeMat.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
		planeMat.shininess = 32.0f;
		m_objectMaterials.push_back(planeMat);
	}

	{
		OBJECT_MATERIAL phoneMat;
		phoneMat.tag = "phoneMaterial";
		phoneMat.ambientColor = glm::vec3(0.7f, 0.7f, 0.7f);
		phoneMat.ambientStrength = 0.2f;
		phoneMat.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
		phoneMat.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
		phoneMat.shininess = 0.0f;
		m_objectMaterials.push_back(phoneMat);
	}

	{
		OBJECT_MATERIAL silverMat;
		silverMat.tag = "silverMaterial";
		silverMat.ambientColor = glm::vec3(0.7f, 0.7f, 0.7f);
		silverMat.ambientStrength = 0.1f;
		silverMat.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
		silverMat.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
		silverMat.shininess = 64.0f;
		m_objectMaterials.push_back(silverMat);
	}

	{
		OBJECT_MATERIAL cameraMat;
		cameraMat.tag = "cameraMaterial";
		cameraMat.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
		cameraMat.ambientStrength = 0.2f;
		cameraMat.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
		cameraMat.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
		cameraMat.shininess = 0.0f;
		m_objectMaterials.push_back(cameraMat);
	}

	{
		OBJECT_MATERIAL pyramidMat;
		pyramidMat.tag = "pyramidMaterial";
		pyramidMat.ambientColor = glm::vec3(0.5f, 0.5f, 0.2f);
		pyramidMat.ambientStrength = 0.2f;
		pyramidMat.diffuseColor = glm::vec3(0.5f, 0.5f, 0.2f);
		pyramidMat.specularColor = glm::vec3(0.8f, 0.8f, 0.5f);
		pyramidMat.shininess = 60.0f;
		m_objectMaterials.push_back(pyramidMat);
	}

	{
		OBJECT_MATERIAL sphereMat;
		sphereMat.tag = "sphereMaterial";
		sphereMat.ambientColor = glm::vec3(0.8f, 0.3f, 0.3f);
		sphereMat.ambientStrength = 0.3f;
		sphereMat.diffuseColor = glm::vec3(0.8f, 0.3f, 0.3f);
		sphereMat.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
		sphereMat.shininess = 32.0f;
		m_objectMaterials.push_back(sphereMat);
	}

	{
		OBJECT_MATERIAL batteryMat;
		batteryMat.tag = "batteryMaterial";
		batteryMat.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
		batteryMat.ambientStrength = 0.2f;
		batteryMat.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
		batteryMat.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
		batteryMat.shininess = 32.0f;
		m_objectMaterials.push_back(batteryMat);
	}

	{
		OBJECT_MATERIAL goldConductorMat;
		goldConductorMat.tag = "goldConductorMaterial";
		goldConductorMat.ambientColor = glm::vec3(1.0f, 0.84f, 0.0f);
		goldConductorMat.ambientStrength = 0.3f;
		goldConductorMat.diffuseColor = glm::vec3(1.0f, 0.84f, 0.0f);
		goldConductorMat.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
		goldConductorMat.shininess = 120.0f;
		m_objectMaterials.push_back(goldConductorMat);
	}
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{

	//Enable lighting in the shader
	if (m_pShaderManager != NULL)
	{
		// Enable Phong calculations
		m_pShaderManager->setIntValue(g_UseLightingName, true);

		// Set up a main light
		m_pShaderManager->setVec3Value("lightSources[0].position", 1.0f, 15.0f, 0.0f);
		m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.01f, 0.01f, 0.01f);
		m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.4f, 1.0f, 0.4f);
		m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.5f, 0.5f, 0.5f);
		m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 25.0f);
		m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.2f);

		// LIGHT 1 (Slightly colored side light)
		m_pShaderManager->setVec3Value("lightSources[1].position", -5.0f, 5.0f, 5.0f);
		// Give it a dimmer ambient
		m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.0f, 0.0f, 0.05f);
		m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.2f, 0.2f, 0.8f);
		m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.4f, 0.4f, 0.9f);
		m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
		m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.15f);
	}

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("planeMaterial");
	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("planeTexture");
	SetTextureUVScale(1.0f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	 // 2. Render the phone as a flatter box
	glm::vec3 phoneScale = glm::vec3(2.0f, 0.05f, 4.0f);
	float     phoneXRot = 0.0f;
	float     phoneYRot = -5.0f;
	float     phoneZRot = 0.0f;
	glm::vec3 phonePosition = glm::vec3(0.0f, 0.03f, 0.0f);

	SetTransformations(phoneScale, phoneXRot, phoneYRot, phoneZRot, phonePosition);

	SetShaderMaterial("phoneMaterial");
	//SetShaderColor(0.855f, 0.831f, 0.792f, 1.0f);
	SetShaderTexture("phoneTexture");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	//3. Render the three camera bumps plus its ring to give a real phone look
	{
		// Half the phone’s dimensions for positioning reference
		float phoneHalfWidth = 0.5f * phoneScale.x;  // 1.0f
		float phoneHalfHeight = 0.5f * phoneScale.z;  // 2.0f

		// Base offset above phone
		float cameraOffsetY = 0.02f;

		// -- Shared scale values: ring vs. lens
		// The ring is bigger in x,z, shorter in y
		glm::vec3 ringScale = glm::vec3(0.145f, 0.06f, 0.145f);
		// The lens is smaller in x,z, taller in y
		glm::vec3 lensScale = glm::vec3(0.12f, 0.065f, 0.12f);

		// Common rotations
		float camXRot = 0.0f;
		float camYRot = 0.0f;
		float camZRot = 0.0f;

		/****************************************************************
		 * CAMERA #1
		 ****************************************************************/
		glm::vec3 cam1Pos(
			-phoneHalfWidth + 0.32f,  // X offset
			cameraOffsetY,           // Y offset from phone
			phoneHalfHeight - 3.37f   // Z offset
		);

		// 1A) Draw the silver ring (lower + flatter)
		SetTransformations(ringScale, camXRot, camYRot, camZRot, cam1Pos);
		SetShaderMaterial("silverMaterial");
		SetShaderTexture("silverTexture");//Texture
		SetTextureUVScale(1.0f, 1.0f);//UV scaling
		m_basicMeshes->DrawCylinderMesh();//Drawing

		// 1B) Draw the lens (slightly higher in Y so it protrudes above ring)
		glm::vec3 cam1LensPos = cam1Pos;
		cam1LensPos.y += 0.01f; // Raise lens by 0.01 above ring
		SetTransformations(lensScale, camXRot, camYRot, camZRot, cam1LensPos);
		SetShaderMaterial("cameraMaterial");
		SetShaderTexture("cameraTexture");
		SetTextureUVScale(1.0f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		/****************************************************************
		 * CAMERA #2
		 ****************************************************************/
		glm::vec3 cam2Pos(
			-phoneHalfWidth + 0.60f,
			cameraOffsetY,
			phoneHalfHeight - 3.52f
		);

		// 2A) Silver ring
		SetTransformations(ringScale, camXRot, camYRot, camZRot, cam2Pos);
		SetShaderMaterial("silverMaterial");
		SetShaderTexture("silverTexture");
		SetTextureUVScale(1.0f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// 2B) Lens
		glm::vec3 cam2LensPos = cam2Pos;
		cam2LensPos.y += 0.01f;
		SetTransformations(lensScale, camXRot, camYRot, camZRot, cam2LensPos);
		SetShaderMaterial("cameraMaterial");
		SetShaderTexture("cameraTexture");
		SetTextureUVScale(1.0f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		/****************************************************************
		 * CAMERA #3
		 ****************************************************************/
		glm::vec3 cam3Pos(
			-phoneHalfWidth + 0.35f,
			cameraOffsetY,
			phoneHalfHeight - 3.74f
		);

		// 3A) Silver ring
		SetTransformations(ringScale, camXRot, camYRot, camZRot, cam3Pos);
		SetShaderMaterial("silverMaterial");
		SetShaderTexture("silverTexture");
		SetTextureUVScale(1.0f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// 3B) Lens
		glm::vec3 cam3LensPos = cam3Pos;
		cam3LensPos.y += 0.01f;
		SetTransformations(lensScale, camXRot, camYRot, camZRot, cam3LensPos);
		SetShaderMaterial("cameraMaterial");
		SetShaderTexture("cameraTexture");
		SetTextureUVScale(1.0f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		/****************************************************************
		 * PYRAMID MESH
		 ****************************************************************/
		glm::vec3 pyramidScale = glm::vec3(1.2f, 1.0f, 1.2f);
		float     pyramidXRot = 0.0f;
		float     pyramidYRot = 25.0f;
		float     pyramidZRot = 0.0f;
		glm::vec3 pyramidPosition = glm::vec3(-2.7f, 0.52f, 0.5f);

		SetTransformations(pyramidScale, pyramidXRot, pyramidYRot, pyramidZRot, pyramidPosition);

		SetShaderMaterial("pyramidMaterial");
		SetShaderColor(2.855f, 0.831f, 0.792f, 1.0f);
		SetShaderTexture("pyramidTexture");
		SetTextureUVScale(1.0f, 1.0f);
		m_basicMeshes->DrawPyramid4Mesh();

		/****************************************************************
		 * SPHERE MESH
		 ****************************************************************/
		glm::vec3 sphereScale = glm::vec3(0.40f, 0.45f, 0.40f);
		float     sphereXRot = 0.0f;
		float     sphereYRot = 25.0f;
		float     sphereZRot = 0.0f;
		glm::vec3 spherePosition = glm::vec3(-4.5f, 0.50f, 1.3f);

		SetTransformations(sphereScale, sphereXRot, sphereYRot, sphereZRot, spherePosition);

		SetShaderMaterial("sphereMaterial");
		SetShaderColor(0.855f, 0.831f, 0.792f, 1.0f);
		SetShaderTexture("sphereTexture");
		SetTextureUVScale(1.0f, 1.0f);
		m_basicMeshes->DrawSphereMesh();

		/****************************************************************
		 * BATTERY MESH
		 ****************************************************************/
		glm::vec3 batteryScale = glm::vec3(1.3f, 0.4f, 0.9f);
		float     batteryXRot = 0.0f;
		float     batteryYRot = 20.0f;
		float     batteryZRot = 0.0f;
		glm::vec3 batteryPosition = glm::vec3(-3.7f, 0.21f, -0.8f);

		SetTransformations(batteryScale, batteryXRot, batteryYRot, batteryZRot, batteryPosition);

		SetShaderMaterial("batteryMaterial");
		//SetShaderColor(0.855f, 0.831f, 0.792f, 1.0f);
		SetShaderTexture("plasticTexture");
		SetTextureUVScale(1.0f, 1.0f);
		m_basicMeshes->DrawBoxMesh();

		/****************************************************************
		 * GOLD CONDUCTORS
		 ****************************************************************/
		glm::vec3 conductorScale = glm::vec3(0.25f, 0.16f, 0.1f);
		float     conductorXRot = 0.0f;
		float     conductorYRot = 20.0f;
		float     conductorZRot = 0.0f;

		//first conductor
		glm::vec3 conductorPosition1 = glm::vec3(-4.26f, 0.08f, -0.8f);
		SetTransformations(conductorScale, conductorXRot, conductorYRot, conductorZRot, conductorPosition1);
		SetShaderMaterial("goldConductorMaterial");
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("conductorTexture");
		m_basicMeshes->DrawBoxMesh();

		//second conductor
		glm::vec3 conductorPosition2 = glm::vec3(-4.188f, 0.08f, -0.6f);
		SetTransformations(conductorScale, conductorXRot, conductorYRot, conductorZRot, conductorPosition2);
		SetShaderMaterial("goldConductorMaterial");
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("conductorTexture");
		m_basicMeshes->DrawBoxMesh();

		//third conductor
		glm::vec3 conductorPosition3 = glm::vec3(-4.115f, 0.08f, -0.4f);
		SetTransformations(conductorScale, conductorXRot, conductorYRot, conductorZRot, conductorPosition3);
		SetShaderMaterial("goldConductorMaterial");
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("conductorTexture");
		m_basicMeshes->DrawBoxMesh();

	}
}