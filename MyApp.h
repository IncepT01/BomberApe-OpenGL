#pragma once

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

// GLEW
#include <GL/glew.h>

// SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

// Utils
#include "GLUtils.hpp"
#include "Camera.h"
#include "CameraManipulator.h"

struct SUpdateInfo
{
	float ElapsedTimeInSec = 0.0f; // Program indulása óta eltelt idő
	float DeltaTimeInSec   = 0.0f; // Előző Update óta eltelt idő
};

class CMyApp
{
public:
	CMyApp();
	~CMyApp();

	bool Init();
	void Clean();

	void Update( const SUpdateInfo& );
	void Render();
	void RenderGUI();

	void KeyboardDown(const SDL_KeyboardEvent&);
	void KeyboardUp(const SDL_KeyboardEvent&);
	void MouseMove(const SDL_MouseMotionEvent&);
	void MouseDown(const SDL_MouseButtonEvent&);
	void MouseUp(const SDL_MouseButtonEvent&);
	void MouseWheel(const SDL_MouseWheelEvent&);
	void Resize(int, int);

	void OtherEvent( const SDL_Event& );
	void DrawWall(glm::mat4 world);
	void DrawDynamit(glm::mat4 world);
	void DrawExplosion(glm::mat4 world);
	glm::vec3 EvaluatePathPosition() const;
	glm::vec3 EvaluatePathTangent() const;
protected:
	void SetupDebugCallback();

	//
	// Adat változók
	//

	float m_ElapsedTimeInSec = 0.0f;
	float m_DeltaTimeInSec = 0.0f;
	float m_TimeScale = 1.0f;

	int wallList[18][2];
	bool m_explosionsOn = true;

	std::vector<glm::vec3> m_controlPoints;

	float m_currentParam = 0.0;
	static constexpr int MAX_POINT_COUNT = 20;

	int m_guiCurrentItem = -1;


	// Kamera
	Camera m_camera;
	CameraManipulator m_cameraManipulator;

	//
	// OpenGL-es dolgok
	//
	
	// uniform location lekérdezése
	static GLint ul( const char* uniformName ) noexcept;

	// shaderekhez szükséges változók
	GLuint m_programID = 0;		  // shaderek programja
	GLuint m_programSkyboxID = 0; // skybox programja


	// Fényforrás- ...
	glm::vec4 m_lightPos = glm::vec4( 0.3f, 0.3f, 0.3f, 0.0f );

	glm::vec3 m_La = glm::vec3(0.0, 0.0, 0.0 );
	glm::vec3 m_Ld = glm::vec3(1.0, 1.0, 1.0 );
	glm::vec3 m_Ls = glm::vec3(1.0, 1.0, 1.0 );

	float m_lightConstantAttenuation    = 1.0;
	float m_lightLinearAttenuation      = 0.0;
	float m_lightQuadraticAttenuation   = 0.0;

	// ... és anyagjellemzők
	glm::vec3 m_Ka = glm::vec3( 1.0 );
	glm::vec3 m_Kd = glm::vec3( 1.0 );
	glm::vec3 m_Ks = glm::vec3( 1.0 );

	float m_Shininess = 1.0;

	// Shaderek inicializálása, és törtlése
	void InitShaders();
	void CleanShaders();
	void InitSkyboxShaders();
	void CleanSkyboxShaders();

	// Geometriával kapcsolatos változók

	OGLObject m_SuzanneGPU = {};
	OGLObject m_surfaceGPU = {};
	OGLObject m_SkyboxGPU = {};
	OGLObject m_TileGPU = {};
	OGLObject m_HardhatGPU = {};
	OGLObject m_HengerGPU = {};
	OGLObject m_WallGPU = {};

	// Geometria inicializálása, és törtlése
	void InitGeometry();
	void CleanGeometry();
	void InitSkyboxGeometry();
	void CleanSkyboxGeometry();

	// Textúrázás, és változói

	GLuint m_SuzanneTextureID = 0;
	GLuint m_surfaceTextureID = 0;
	GLuint m_skyboxTextureID = 0;

	GLuint m_tileTextureID = 0;
	GLuint m_wallTextureID = 0;
	GLuint m_hardhatTextureID = 0;
	GLuint m_dynamitTextureID = 0;
	GLuint m_explosionTextureID = 0;

	void InitTextures();
	void CleanTextures();
	void InitSkyboxTextures();
	void CleanSkyboxTextures();
};

