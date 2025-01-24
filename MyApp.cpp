#include "MyApp.h"
#include "SDL_GLDebugMessageCallback.h"
#include "ObjParser.h"
#include "ParametricSurfaceMesh.hpp"

#include <imgui.h>
#include <string>

CMyApp::CMyApp()
{
}

CMyApp::~CMyApp()
{
}

void CMyApp::SetupDebugCallback()
{
	// engedélyezzük és állítsuk be a debug callback függvényt ha debug context-ben vagyunk 
	GLint context_flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
	if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		glDebugMessageCallback(SDL_GLDebugMessageCallback, nullptr);
	}
}

void CMyApp::InitShaders()
{
	m_programID = glCreateProgram();
	AssembleProgram( m_programID, "Shaders/Vert_PosNormTex.vert", "Shaders/Frag_ZH.frag" );
	InitSkyboxShaders();
}

void CMyApp::InitSkyboxShaders()
{
	m_programSkyboxID = glCreateProgram();
	AssembleProgram( m_programSkyboxID, "Shaders/Vert_skybox.vert", "Shaders/Frag_skybox.frag" );
}

void CMyApp::CleanShaders()
{
	glDeleteProgram( m_programID );
	CleanSkyboxShaders();
}

void CMyApp::CleanSkyboxShaders()
{
	glDeleteProgram( m_programSkyboxID );
}

// Nyers parameterek
struct Param
{
	glm::vec3 GetPos( float u, float v ) const noexcept
	{
		return glm::vec3( u, v, 0.0f );
	}
	glm::vec3 GetNorm( float u, float v ) const noexcept
	{
		return glm::vec3( 0.0,0.0,1.0 );
	}
	glm::vec2 GetTex( float u, float v ) const noexcept
	{
		return glm::vec2( u, v );
	}
};

struct Henger
{
	float r;
	float h;
	Henger(float _r = 1.0f, float _h = 1.0f) : r(_r), h(_h) {}

	glm::vec3 GetPos(float u, float v) const noexcept
	{
		u *= glm::two_pi<float>();

		return glm::vec3(
			r * cosf(u),
			h * (v - 1.f/2.f),
			-r * sinf(u));
	}
	glm::vec3 GetNorm(float u, float v) const noexcept
	{
		//u *= glm::two_pi<float>();

		glm::vec3 du = GetPos(u + 0.01f, v) - GetPos(u - 0.01f, v);
		glm::vec3 dv = GetPos(u, v + 0.01f) - GetPos(u, v - 0.01f);

		return glm::normalize(glm::cross(du, dv));
	}
	glm::vec2 GetTex(float u, float v) const noexcept
	{
		return glm::vec2(u, v);
	}
};

void CMyApp::InitGeometry()
{

	const std::initializer_list<VertexAttributeDescriptor> vertexAttribList =
	{
		{ 0, offsetof( Vertex, position ), 3, GL_FLOAT },
		{ 1, offsetof( Vertex, normal   ), 3, GL_FLOAT },
		{ 2, offsetof( Vertex, texcoord ), 2, GL_FLOAT },
	};

	// Suzanne

	MeshObject<Vertex> suzanneMeshCPU = ObjParser::parse("Assets/Suzanne.obj");
	m_SuzanneGPU = CreateGLObjectFromMesh( suzanneMeshCPU, vertexAttribList );

	//Hardhat
	MeshObject<Vertex> hardhatMeshCPU = ObjParser::parse("Assets/hardhat.obj");
	m_HardhatGPU = CreateGLObjectFromMesh(hardhatMeshCPU, vertexAttribList);

	// Parametrikus felület
	MeshObject<Vertex> hengerMeshCPU = GetParamSurfMesh( Henger() );
	m_HengerGPU = CreateGLObjectFromMesh( hengerMeshCPU, vertexAttribList );

	MeshObject<Vertex> tileCPU;
	tileCPU.vertexArray = {
		{glm::vec3(-1,-1,0), glm::vec3(0,0,1), glm::vec2(0,0)},
		{glm::vec3(0,-1,0), glm::vec3(0,0,1), glm::vec2(7,0)},
		{glm::vec3(-1,0,0), glm::vec3(0,0,1), glm::vec2(0,7)},
		{glm::vec3(0,0,0), glm::vec3(0,0,1), glm::vec2(7,7)}
	};
	tileCPU.indexArray = {
		0, 1, 2,
		2, 1, 3,
	};

	m_TileGPU = CreateGLObjectFromMesh(tileCPU, vertexAttribList);

	MeshObject<Vertex> wallCPU;
	wallCPU.vertexArray = {
		{glm::vec3(-1,-1,0), glm::vec3(0,0,1), glm::vec2(0,0)},
		{glm::vec3(0,-1,0), glm::vec3(0,0,1), glm::vec2(1,0)},
		{glm::vec3(-1,0,0), glm::vec3(0,0,1), glm::vec2(0,1)},
		{glm::vec3(0,0,0), glm::vec3(0,0,1), glm::vec2(1,1)}
	};
	wallCPU.indexArray = {
		0, 1, 2,
		2, 1, 3,
	};

	m_WallGPU = CreateGLObjectFromMesh(wallCPU, vertexAttribList);

	// Skybox
	InitSkyboxGeometry();
}

void CMyApp::CleanGeometry()
{
	CleanOGLObject( m_SuzanneGPU );
	CleanOGLObject( m_surfaceGPU );
	CleanOGLObject(m_HardhatGPU);
	CleanOGLObject(m_HengerGPU);
	CleanOGLObject(m_TileGPU);
	CleanOGLObject(m_WallGPU);
	CleanSkyboxGeometry();
}

void CMyApp::InitSkyboxGeometry()
{
	// skybox geo
	MeshObject<glm::vec3> skyboxCPU =
	{
		std::vector<glm::vec3>
		{
			// hátsó lap
			glm::vec3(-1, -1, -1),
			glm::vec3( 1, -1, -1),
			glm::vec3( 1,  1, -1),
			glm::vec3(-1,  1, -1),
			// elülső lap
			glm::vec3(-1, -1, 1),
			glm::vec3( 1, -1, 1),
			glm::vec3( 1,  1, 1),
			glm::vec3(-1,  1, 1),
		},

		std::vector<GLuint>
		{
			// hátsó lap
			0, 1, 2,
			2, 3, 0,
			// elülső lap
			4, 6, 5,
			6, 4, 7,
			// bal
			0, 3, 4,
			4, 3, 7,
			// jobb
			1, 5, 2,
			5, 6, 2,
			// alsó
			1, 0, 4,
			1, 4, 5,
			// felső
			3, 2, 6,
			3, 6, 7,
		}
	};

	m_SkyboxGPU = CreateGLObjectFromMesh( skyboxCPU, { { 0, offsetof( glm::vec3,x ), 3, GL_FLOAT } } );
}

void CMyApp::CleanSkyboxGeometry()
{
	CleanOGLObject( m_SkyboxGPU );
}

void CMyApp::InitTextures()
{
	// diffuse textures

	glGenTextures( 1, &m_SuzanneTextureID );
	TextureFromFile( m_SuzanneTextureID, "Assets/wood.jpg" );
	SetupTextureSampling( GL_TEXTURE_2D, m_SuzanneTextureID );

	glGenTextures(1, &m_tileTextureID);
	TextureFromFile(m_tileTextureID, "Assets/grid.png");
	SetupTextureSampling(GL_TEXTURE_2D, m_tileTextureID);

	glGenTextures(1, &m_wallTextureID);
	TextureFromFile(m_wallTextureID, "Assets/wall.png");
	SetupTextureSampling(GL_TEXTURE_2D, m_wallTextureID);

	glGenTextures(1, &m_hardhatTextureID);
	TextureFromFile(m_hardhatTextureID, "Assets/hat.png");
	SetupTextureSampling(GL_TEXTURE_2D, m_hardhatTextureID);

	glGenTextures(1, &m_dynamitTextureID);
	TextureFromFile(m_dynamitTextureID, "Assets/dynamit.png");
	SetupTextureSampling(GL_TEXTURE_2D, m_dynamitTextureID);

	glGenTextures(1, &m_explosionTextureID);
	TextureFromFile(m_explosionTextureID, "Assets/flames.png");
	SetupTextureSampling(GL_TEXTURE_2D, m_explosionTextureID);

	// skybox texture

	InitSkyboxTextures();
}

void CMyApp::CleanTextures()
{
	// diffuse textures

	glDeleteTextures( 1, &m_SuzanneTextureID );
	glDeleteTextures(1, &m_tileTextureID);
	glDeleteTextures(1, &m_wallTextureID);
	glDeleteTextures(1, &m_hardhatTextureID);
	glDeleteTextures(1, &m_dynamitTextureID);
	glDeleteTextures(1, &m_explosionTextureID);

	// skybox texture

	CleanSkyboxTextures();
}

void CMyApp::InitSkyboxTextures()
{
	// skybox texture

	glGenTextures( 1, &m_skyboxTextureID );
	TextureFromFile( m_skyboxTextureID, "Assets/bunker_xpos.png", GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_X );
	TextureFromFile( m_skyboxTextureID, "Assets/bunker_xneg.png", GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_NEGATIVE_X );
	TextureFromFile( m_skyboxTextureID, "Assets/bunker_ypos.png", GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_Y );
	TextureFromFile( m_skyboxTextureID, "Assets/bunker_yneg.png", GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y );
	TextureFromFile( m_skyboxTextureID, "Assets/bunker_zpos.png", GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_Z );
	TextureFromFile( m_skyboxTextureID, "Assets/bunker_zneg.png", GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z );
	SetupTextureSampling( GL_TEXTURE_CUBE_MAP, m_skyboxTextureID, false );

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void CMyApp::CleanSkyboxTextures()
{
	glDeleteTextures( 1, &m_skyboxTextureID );
}

bool CMyApp::Init()
{
	SetupDebugCallback();

	// törlési szín legyen kékes
	glClearColor(0.125f, 0.25f, 0.5f, 1.0f);

	InitShaders();
	InitGeometry();
	InitTextures();

	//
	// egyéb inicializálás
	//

	glEnable(GL_CULL_FACE); // kapcsoljuk be a hátrafelé néző lapok eldobását
	glCullFace(GL_BACK);    // GL_BACK: a kamerától "elfelé" néző lapok, GL_FRONT: a kamera felé néző lapok

	glEnable(GL_DEPTH_TEST); // mélységi teszt bekapcsolása (takarás)

	// kamera
	m_camera.SetView(
		glm::vec3(4.0, 10.0, 14),// honnan nézzük a színteret	   - eye
		glm::vec3(4.0, 0.0, 4.0),   // a színtér melyik pontját nézzük - at
		glm::vec3(0.0, 1.0, 0.0));  // felfelé mutató irány a világban - up

	m_cameraManipulator.SetCamera( &m_camera );

#pragma region WallList init
	wallList[0][0] = 1;
	wallList[0][1] = 1;

	wallList[1][0] = 2;
	wallList[1][1] = 1;

	wallList[2][0] = 6;
	wallList[2][1] = 1;

	wallList[3][0] = 1;
	wallList[3][1] = 2;

	wallList[4][0] = 6;
	wallList[4][1] = 2;

	wallList[5][0] = 7;
	wallList[5][1] = 2;

	wallList[6][0] = 7;
	wallList[6][1] = 3;

	wallList[7][0] = 7;
	wallList[7][1] = 4;

	wallList[8][0] = 1;
	wallList[8][1] = 5;

	wallList[9][0] = 2;
	wallList[9][1] = 5;

	wallList[10][0] = 3;
	wallList[10][1] = 5;

	wallList[11][0] = 7;
	wallList[11][1] = 5;

	wallList[12][0] = 1;
	wallList[12][1] = 6;

	wallList[13][0] = 5;
	wallList[13][1] = 6;

	wallList[14][0] = 7;
	wallList[14][1] = 6;

	wallList[15][0] = 1;
	wallList[15][1] = 7;

	wallList[16][0] = 3;
	wallList[16][1] = 7;

	wallList[17][0] = 5;
	wallList[17][1] = 7;

	for (int i = 0; i < 18; i++) {
		wallList[i][0] = wallList[i][0] +  1;
		//wallList[i][1] = wallList[i][1] - 1;
	}

#pragma endregion


	m_controlPoints.push_back(glm::vec3(4.f, 0.0, 1.f));
	m_controlPoints.push_back(glm::vec3(4.f, 0.0, 2.f));
	m_controlPoints.push_back(glm::vec3(4.f, 0.0, 3.f));
	m_controlPoints.push_back(glm::vec3(5.f, 0.0, 3.f));
	m_controlPoints.push_back(glm::vec3(5.f, 0.0, 4.f));
	m_controlPoints.push_back(glm::vec3(6.f, 0.0, 4.f));
	m_controlPoints.push_back(glm::vec3(6.f, 0.0, 5.f));
	m_controlPoints.push_back(glm::vec3(6.f, 0.0, 6.f));
	m_controlPoints.push_back(glm::vec3(6.f, 0.0, 7.f));

	return true;
}

void CMyApp::Clean()
{
	CleanShaders();
	CleanGeometry();
	CleanTextures();
}

void CMyApp::Update( const SUpdateInfo& updateInfo )
{
	m_ElapsedTimeInSec = updateInfo.ElapsedTimeInSec;
	m_DeltaTimeInSec = updateInfo.DeltaTimeInSec;

	m_cameraManipulator.Update( updateInfo.DeltaTimeInSec );
}

void CMyApp::Render()
{
	// töröljük a frampuffert (GL_COLOR_BUFFER_BIT)...
	// ... és a mélységi Z puffert (GL_DEPTH_BUFFER_BIT)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_currentParam += (m_DeltaTimeInSec * m_TimeScale);
	if (m_currentParam > m_controlPoints.size() - 1) {
		m_currentParam = 0;
	}

	glm::mat4 matWorld;
	if (!m_explosionsOn || m_currentParam <= 5.0 || m_currentParam >= 7.0) {
		m_lightPos = glm::vec4(0.3f, 0.3f, 0.3f, 0.0f);
		m_Ld = glm::vec3(1.f, 1.f, 1.f);
		m_Ls = glm::vec3(1.f, 1.f, 1.f);
		m_lightLinearAttenuation = 0.3f;
		m_lightQuadraticAttenuation = 0.3f;
	}

	// Suzanne

	glBindVertexArray( m_SuzanneGPU.vaoID );

	// - Textúrák beállítása, minden egységre külön
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, m_SuzanneTextureID );

	glUseProgram( m_programID );

	// - Uniform paraméterek

	// view és projekciós mátrix
	glUniformMatrix4fv( ul("viewProj"), 1, GL_FALSE, glm::value_ptr( m_camera.GetViewProj() ) );


	

	// Transformációs mátrixok
	glm::vec3 suzanneForward = EvaluatePathTangent(); // Merre nézzen a Suzanne?
	glm::vec3 suzanneWorldUp = glm::vec3(0.0, 1.0, 0.0); // Milyen irány a felfelé?
	if (fabsf(suzanneForward.y) > 0.99) // Ha a Suzanne felfelé néz, akkor a worldUp irányt nem tudjuk használni, mert akkor a jobbra vektor null vektor lesz
	{
		suzanneWorldUp = glm::vec3(-1.0, 0.0, 0.0); // Ezért ha felfelé néz, akkor a worldUp legyen egy tetszőleges [0,1,0] vektorra merőleges irány
	}

	glm::vec3 suzanneRight = glm::normalize(glm::cross(suzanneForward, suzanneWorldUp)); // Jobbra nézése
	glm::vec3 suzanneUp = glm::cross(suzanneRight, suzanneForward); // Felfelé nézése

	// A három vektorból álló bázisvektorokat egy mátrixba rendezzük, hogy tudjuk velük forgatni a Suzanne-t
	// valamint eltoljuk a megfelelő helyre a pályán. A pálya pozícióját a EvaluatePathPosition() függvény adja meg.
	// Így kapunk egy affin transzformációs mátrixot.
	glm::mat4 suzanneTrans(0.0f);
	suzanneTrans[0] = glm::vec4(suzanneForward, 0.0f);
	suzanneTrans[1] = glm::vec4(suzanneUp, 0.0f);
	suzanneTrans[2] = glm::vec4(suzanneRight, 0.0f);
	suzanneTrans[3] = glm::vec4(EvaluatePathPosition(), 1.0f);


	matWorld = suzanneTrans * glm::rotate(glm::pi<float>() / 2, glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.35f, 0.35f, 0.35f));

	



	// Transzformációs mátrixok
	//matWorld = glm::translate(EvaluatePathPosition()) * glm::rotate(glm::pi<float>()/2, glm::vec3(0,1,0)) * glm::scale(glm::vec3(0.35f, 0.35f, 0.35f));

	glUniformMatrix4fv( ul( "world" ),    1, GL_FALSE, glm::value_ptr( matWorld ) );
	glUniformMatrix4fv( ul( "worldIT" ),  1, GL_FALSE, glm::value_ptr( glm::transpose( glm::inverse( matWorld ) ) ) );

	// - Fényforrások beállítása
	glUniform3fv( ul( "cameraPos" ), 1, glm::value_ptr( m_camera.GetEye() ) );
	glUniform4fv( ul( "lightPos" ),  1, glm::value_ptr( m_lightPos ) );

	glUniform3fv( ul( "La" ),		 1, glm::value_ptr( m_La ) );
	glUniform3fv( ul( "Ld" ),		 1, glm::value_ptr( m_Ld ) );
	glUniform3fv( ul( "Ls" ),		 1, glm::value_ptr( m_Ls ) );

	glUniform1f( ul( "lightConstantAttenuation"	 ), m_lightConstantAttenuation );
	glUniform1f( ul( "lightLinearAttenuation"	 ), m_lightLinearAttenuation   );
	glUniform1f( ul( "lightQuadraticAttenuation" ), m_lightQuadraticAttenuation);

	// - Anyagjellemzők beállítása
	glUniform3fv( ul( "Ka" ),		 1, glm::value_ptr( m_Ka ) );
	glUniform3fv( ul( "Kd" ),		 1, glm::value_ptr( m_Kd ) );
	glUniform3fv( ul( "Ks" ),		 1, glm::value_ptr( m_Ks ) );

	glUniform1f( ul( "Shininess" ),	m_Shininess );


	// - textúraegységek beállítása
	glUniform1i( ul( "texImage" ), 0 );


	
	// Rajzolási parancs kiadása
	glDrawElements( GL_TRIANGLES,    
					m_SuzanneGPU.count,			 
					GL_UNSIGNED_INT,
					nullptr );
	

	// Hardhat

	glBindVertexArray(m_HardhatGPU.vaoID);

	// - Textúrák beállítása, minden egységre külön
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_hardhatTextureID);

	glUseProgram(m_programID);

	// - Uniform paraméterek

	// view és projekciós mátrix
	glUniformMatrix4fv(ul("viewProj"), 1, GL_FALSE, glm::value_ptr(m_camera.GetViewProj()));


	// Transformációs mátrixok
	glm::vec3 hardhatForward = EvaluatePathTangent(); // Merre nézzen a hardhat?
	glm::vec3 hardhatWorldUp = glm::vec3(0.0, 1.0, 0.0); // Milyen irány a felfelé?
	if (fabsf(hardhatForward.y) > 0.99) // Ha a hardhat felfelé néz, akkor a worldUp irányt nem tudjuk használni, mert akkor a jobbra vektor null vektor lesz
	{
		hardhatWorldUp = glm::vec3(-1.0, 0.0, 0.0); // Ezért ha felfelé néz, akkor a worldUp legyen egy tetszőleges [0,1,0] vektorra merőleges irány
	}

	glm::vec3 hardhatRight = glm::normalize(glm::cross(hardhatForward, hardhatWorldUp)); // Jobbra nézése
	glm::vec3 hardhatUp = glm::cross(hardhatRight, hardhatForward); // Felfelé nézése

	// A három vektorból álló bázisvektorokat egy mátrixba rendezzük, hogy tudjuk velük forgatni a hardhat-t
	// valamint eltoljuk a megfelelő helyre a pályán. A pálya pozícióját a EvaluatePathPosition() függvény adja meg.
	// Így kapunk egy affin transzformációs mátrixot.
	glm::mat4 hardhatTrans(0.0f);
	hardhatTrans[0] = glm::vec4(hardhatForward, 0.0f);
	hardhatTrans[1] = glm::vec4(hardhatUp, 0.0f);
	hardhatTrans[2] = glm::vec4(hardhatRight, 0.0f);
	hardhatTrans[3] = glm::vec4(EvaluatePathPosition(), 1.0f);


	matWorld = hardhatTrans * glm::translate(glm::vec3(-0.075f, 0.25f, 0.5f)) * glm::rotate(-glm::pi<float>() / 2, glm::vec3(1, 0, 0)) * glm::scale(glm::vec3(0.025f, 0.025f, 0.025f));


	// Transzformációs mátrixok
	//matWorld = glm::translate(EvaluatePathPosition())* glm::translate(glm::vec3(-0.075f, 0.25f, 0.5f)) *  glm::rotate(-glm::pi<float>() / 2, glm::vec3(1, 0, 0)) * glm::scale(glm::vec3(0.025f, 0.025f, 0.025f));

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	// - Fényforrások beállítása
	glUniform3fv(ul("cameraPos"), 1, glm::value_ptr(m_camera.GetEye()));
	glUniform4fv(ul("lightPos"), 1, glm::value_ptr(m_lightPos));

	glUniform3fv(ul("La"), 1, glm::value_ptr(m_La));
	glUniform3fv(ul("Ld"), 1, glm::value_ptr(m_Ld));
	glUniform3fv(ul("Ls"), 1, glm::value_ptr(m_Ls));

	glUniform1f(ul("lightConstantAttenuation"), m_lightConstantAttenuation);
	glUniform1f(ul("lightLinearAttenuation"), m_lightLinearAttenuation);
	glUniform1f(ul("lightQuadraticAttenuation"), m_lightQuadraticAttenuation);

	// - Anyagjellemzők beállítása
	glUniform3fv(ul("Ka"), 1, glm::value_ptr(m_Ka));
	glUniform3fv(ul("Kd"), 1, glm::value_ptr(m_Kd));
	glUniform3fv(ul("Ks"), 1, glm::value_ptr(m_Ks));

	glUniform1f(ul("Shininess"), m_Shininess);


	// - textúraegységek beállítása
	glUniform1i(ul("texImage"), 0);



	// Rajzolási parancs kiadása
	glDrawElements(GL_TRIANGLES,
		m_HardhatGPU.count,
		GL_UNSIGNED_INT,
		nullptr);



	//Tiles
	glBindVertexArray(m_TileGPU.vaoID);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_tileTextureID);

	matWorld = glm::translate(glm::vec3(7 + 0.5f, -0.5f, 0 + 0.5f)) * glm::rotate(-glm::pi<float>() / 2, glm::vec3(1, 0, 0)) * glm::scale(glm::vec3(7, 7, 7));

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_TileGPU.count,
		GL_UNSIGNED_INT,
		nullptr);

	


	//Walls
	for (int i = 0; i < 18; i++) {
		matWorld = glm::translate(glm::vec3(wallList[i][0] - 0.5f, 0.0f, wallList[i][1] + 0.5f));
		DrawWall(matWorld);
	}

	//Dynamit

	if (m_currentParam >= 3.0 && m_currentParam <= 5.0) {
		matWorld = glm::translate(glm::vec3(4, 0, 3));
		DrawDynamit(matWorld);
	}


	//
	// skybox
	//

	// - VAO
	glBindVertexArray( m_SkyboxGPU.vaoID );

	// - Textura
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_CUBE_MAP, m_skyboxTextureID );

	// - Program
	glUseProgram( m_programSkyboxID );

	// - uniform parameterek
	glUniformMatrix4fv( ul("viewProj"), 1, GL_FALSE, glm::value_ptr( m_camera.GetViewProj() ) );
	glUniformMatrix4fv( ul("world"),    1, GL_FALSE, glm::value_ptr( glm::translate( m_camera.GetEye() ) ) );

	// - textúraegységek beállítása
	glUniform1i( ul( "skyboxTexture" ), 1 );

	// mentsük el az előző Z-test eredményt, azaz azt a relációt, ami alapján update-eljük a pixelt.
	GLint prevDepthFnc;
	glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFnc);

	// most kisebb-egyenlőt használjunk, mert mindent kitolunk a távoli vágósíkokra
	glDepthFunc(GL_LEQUAL);

	// Rajzolási parancs kiadása
	glDrawElements( GL_TRIANGLES, m_SkyboxGPU.count, GL_UNSIGNED_INT, nullptr );

	glDepthFunc(prevDepthFnc);


	//Explosion
	if (m_explosionsOn && m_currentParam >= 5.0 && m_currentParam <= 7.0) {
		matWorld = glm::translate(glm::vec3(4, 0, 3));
		DrawExplosion(matWorld);
	}


	// shader kikapcsolasa
	glUseProgram( 0 );

	// - Textúrák kikapcsolása, minden egységre külön
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, 0 );
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );


	// VAO kikapcsolása
	glBindVertexArray( 0 );

}

void CMyApp::RenderGUI()
{
	// ImGui::ShowDemoWindow();
	if (ImGui::Begin("Settings"))
	{

		ImGui::Checkbox("Toggle Explosions", &m_explosionsOn);

		//Time Scale
		ImGui::SliderFloat("Time Scale", &m_TimeScale, 0, 20);

		// A paramétert szabályozó csúszka
		ImGui::SliderFloat("Contorl point", &m_currentParam, 0, (float)(m_controlPoints.size() - 1));

		ImGui::SeparatorText("Control points");

		// A listboxban megjelenítjük a pontokat
		// Legyen a magasssága annyi, hogy MAX_POINT_COUNT elem férjen bele
		// ImGui::GetTextLineHeightWithSpacing segítségével lekérhető egy sor magassága
		if (ImGui::BeginListBox("Control Points", ImVec2(0.0, MAX_POINT_COUNT * ImGui::GetTextLineHeightWithSpacing())))
		{
			for (int i = 0; i < static_cast<const int>(m_controlPoints.size()); ++i)
			{
				const bool is_seleceted = (m_guiCurrentItem == i); // épp ki van-e jelölve?
				if (ImGui::Selectable(std::to_string(i).c_str(), is_seleceted))
				{
					if (i == m_guiCurrentItem) m_guiCurrentItem = -1; // Ha rákattintottunk, akkor szedjük le a kijelölést
					else m_guiCurrentItem = i; // Különben jelöljük ki
				}

				// technikai apróság, nem baj ha lemarad.
				if (is_seleceted)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
		}
	}
	ImGui::End();
	
}

GLint CMyApp::ul( const char* uniformName ) noexcept
{
	GLuint programID = 0;

	// Kérdezzük le az aktuális programot!
	// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGet.xhtml
	glGetIntegerv( GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>( &programID ) );
	// A program és a uniform név ismeretében kérdezzük le a location-t!
	// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
	return glGetUniformLocation( programID, uniformName );
}

// https://wiki.libsdl.org/SDL2/SDL_KeyboardEvent
// https://wiki.libsdl.org/SDL2/SDL_Keysym
// https://wiki.libsdl.org/SDL2/SDL_Keycode
// https://wiki.libsdl.org/SDL2/SDL_Keymod

void CMyApp::KeyboardDown(const SDL_KeyboardEvent& key)
{	
	if ( key.repeat == 0 ) // Először lett megnyomva
	{
		if ( key.keysym.sym == SDLK_F5 && key.keysym.mod & KMOD_CTRL )
		{
			CleanShaders();
			InitShaders();
		}
		if ( key.keysym.sym == SDLK_F1 )
		{
			GLint polygonModeFrontAndBack[ 2 ] = {};
			// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGet.xhtml
			glGetIntegerv( GL_POLYGON_MODE, polygonModeFrontAndBack ); // Kérdezzük le a jelenlegi polygon módot! Külön adja a front és back módokat.
			GLenum polygonMode = ( polygonModeFrontAndBack[ 0 ] != GL_FILL ? GL_FILL : GL_LINE ); // Váltogassuk FILL és LINE között!
			// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glPolygonMode.xhtml
			glPolygonMode( GL_FRONT_AND_BACK, polygonMode ); // Állítsuk be az újat!
		}
	}
	m_cameraManipulator.KeyboardDown( key );
}

void CMyApp::KeyboardUp(const SDL_KeyboardEvent& key)
{
	m_cameraManipulator.KeyboardUp( key );
}

// https://wiki.libsdl.org/SDL2/SDL_MouseMotionEvent

void CMyApp::MouseMove(const SDL_MouseMotionEvent& mouse)
{
	m_cameraManipulator.MouseMove( mouse );
}

// https://wiki.libsdl.org/SDL2/SDL_MouseButtonEvent

void CMyApp::MouseDown(const SDL_MouseButtonEvent& mouse)
{
}

void CMyApp::MouseUp(const SDL_MouseButtonEvent& mouse)
{
}

// https://wiki.libsdl.org/SDL2/SDL_MouseWheelEvent

void CMyApp::MouseWheel(const SDL_MouseWheelEvent& wheel)
{
	m_cameraManipulator.MouseWheel( wheel );
}


// a két paraméterben az új ablakméret szélessége (_w) és magassága (_h) található
void CMyApp::Resize(int _w, int _h)
{
	glViewport(0, 0, _w, _h);
	m_camera.SetAspect( static_cast<float>(_w) / _h );
}

// Le nem kezelt, egzotikus esemény kezelése
// https://wiki.libsdl.org/SDL2/SDL_Event

void CMyApp::OtherEvent( const SDL_Event& ev )
{

}


void CMyApp::DrawWall(glm::mat4 world) {

	glBindVertexArray(m_WallGPU.vaoID);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_wallTextureID);

	world = world * glm::translate(glm::vec3(0.f, -0.5f, 0.f));
	
	//felső
	glm::mat4 matWorld = world * glm::translate(glm::vec3(0.f, 1.f, -1.f)) * glm::rotate(-glm::pi<float>() / 2, glm::vec3(1, 0, 0));

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_TileGPU.count,
		GL_UNSIGNED_INT,
		nullptr);

	//szemben
	matWorld = world * glm::translate(glm::vec3(0.f, 0.f, 0.f)) * glm::rotate(glm::pi<float>() / 2, glm::vec3(1, 0, 0));

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_TileGPU.count,
		GL_UNSIGNED_INT,
		nullptr);

	//hátul
	matWorld = world * glm::translate(glm::vec3(-1.f, 1.f, -1.f)) * glm::rotate(-glm::pi<float>(), glm::vec3(0, 1, 0));

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_TileGPU.count,
		GL_UNSIGNED_INT,
		nullptr);


	//jobb
	matWorld = world * glm::translate(glm::vec3(0.f, 1.f, -1.f)) * glm::rotate(glm::pi<float>() / 2, glm::vec3(0, 1, 0));

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_TileGPU.count,
		GL_UNSIGNED_INT,
		nullptr);

	//bal
	matWorld = world * glm::translate(glm::vec3(-1.f, 1.f, 0.f)) * glm::rotate(-glm::pi<float>() / 2, glm::vec3(0, 1, 0));

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_TileGPU.count,
		GL_UNSIGNED_INT,
		nullptr);


	//alsó
	matWorld = world * glm::translate(glm::vec3(0.f, 1.f, 0.f));

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_TileGPU.count,
		GL_UNSIGNED_INT,
		nullptr);
}

void CMyApp::DrawDynamit(glm::mat4 world) {

	glm::mat4 matWorld = glm::identity<glm::mat4>();

	glBindVertexArray(m_HengerGPU.vaoID);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_dynamitTextureID);

	glDisable(GL_CULL_FACE);

	float radius = 0.1;
	float height = 1.f;
	matWorld = world * glm::translate(glm::vec3(1.f / 16.f, 0, sqrtf(3) / 16.f)) * glm::scale(glm::vec3(radius, height, radius));


	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_HengerGPU.count,
		GL_UNSIGNED_INT,
		nullptr);



	matWorld = glm::identity<glm::mat4>();

	matWorld = world * glm::translate(glm::vec3(1.f / 16.f, 0, -sqrtf(3) / 16.f)) * glm::scale(glm::vec3(radius, height, radius));

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_HengerGPU.count,
		GL_UNSIGNED_INT,
		nullptr);



	matWorld = glm::identity<glm::mat4>();

	matWorld = world * glm::translate(glm::vec3(-1.f / 8.f, 0, 0))  * glm::scale(glm::vec3(radius, height, radius));

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_HengerGPU.count,
		GL_UNSIGNED_INT,
		nullptr);

	glEnable(GL_CULL_FACE);
}

void CMyApp::DrawExplosion(glm::mat4 world) {

	glUseProgram(m_programID);

	glm::mat4 matWorld = glm::identity<glm::mat4>();

	glBindVertexArray(m_HengerGPU.vaoID);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_explosionTextureID);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);

	float radius = 0.35;
	float height = 5.f;
	matWorld = world * glm::rotate(glm::pi<float>()/2, glm::vec3(1,0,0)) * glm::scale(glm::vec3(radius, height, radius));


	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_HengerGPU.count,
		GL_UNSIGNED_INT,
		nullptr);



	matWorld = glm::identity<glm::mat4>();

	//Z tengely mentén kell forgarni nem y!
	matWorld = world * glm::rotate(glm::pi<float>() / 2, glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(radius, height, radius));

	m_lightPos = glm::vec4(world[3][0], world[3][1], world[3][2], 1.0f);
	glUniform4fv(ul("lightPos"), 1, glm::value_ptr(m_lightPos));

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	m_Ld = glm::vec3(1.f, 0.6f, 0.f);
	m_Ls = glm::vec3(1.f, 0.6f, 0.f);
	glUniform3fv(ul("Ld"), 1, glm::value_ptr(m_Ld));
	glUniform3fv(ul("Ls"), 1, glm::value_ptr(m_Ls));

	m_lightLinearAttenuation = 0.3f;
	m_lightQuadraticAttenuation = 0.3f;
	glUniform1f(ul("lightLinearAttenuation"), m_lightLinearAttenuation);
	glUniform1f(ul("lightQuadraticAttenuation"), m_lightQuadraticAttenuation);

	glDrawElements(GL_TRIANGLES,
		m_HengerGPU.count,
		GL_UNSIGNED_INT,
		nullptr);

	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);

}

glm::vec3 CMyApp::EvaluatePathPosition() const
{
	if (m_controlPoints.size() == 0) // Ha nincs pont, akkor visszaadjuk az origót
		return glm::vec3(0);

	const int interval = (const int)m_currentParam; // Melyik két pont között vagyunk?

	if (interval < 0) // Ha a paraméter negatív, akkor a kezdőpontot adjuk vissza
		return m_controlPoints[0];

	if (interval >= m_controlPoints.size() - 1) // Ha a paraméter nagyobb, mint a pontok száma, akkor az utolsó pontot adjuk vissza
		return m_controlPoints[m_controlPoints.size() - 1];

	float localT = m_currentParam - interval; // A paramétert normalizáljuk az aktuális intervallumra

	return glm::mix(m_controlPoints[interval], m_controlPoints[interval + 1], localT); // Lineárisan interpolálunk a két kontrollpont között
}


// Tangens kiszámítása a kontrollpontok alapján
glm::vec3 CMyApp::EvaluatePathTangent() const
{
	if (m_controlPoints.size() < 2) // Ha nincs elég pont az interpolációhoy, akkor visszaadjuk az x tengelyt
		return glm::vec3(1.0, 0.0, 0.0);

	int interval = (int)m_currentParam; // Melyik két pont között vagyunk?

	if (interval < 0) // Ha a paraméter negatív, akkor a kezdő intervallumot adjuk vissza
		interval = 0;

	if (interval >= m_controlPoints.size() - 1) // Ha a paraméter nagyobb, mint az intervallumok száma, akkor az utolsót adjuk vissza
		interval = static_cast<int>(m_controlPoints.size() - 2);

	return glm::normalize(m_controlPoints[interval + 1] - m_controlPoints[interval]);
}
