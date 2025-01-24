#pragma once

#include <filesystem>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

/* 

Az http://www.opengl-tutorial.org/ oldal alapján.

*/

// Segéd osztályok

struct VertexPosColor
{
    glm::vec3 position;
    glm::vec3 color;
};

struct VertexPosTex
{
    glm::vec3 position;
    glm::vec2 texcoord;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
};

// Segéd függvények

void loadShader( const GLuint loadedShader, const std::filesystem::path& _fileName );
void compileShaderFromSource( const GLuint loadedShader, std::string_view shaderCode );

void AssembleProgram( const GLuint programID, const std::filesystem::path& vs_filename, const std::filesystem::path& fs_filename );

void TextureFromFile( const GLuint tex, const std::filesystem::path& fileName, GLenum Type, GLenum Role );

inline void TextureFromFile( const GLuint tex, const std::filesystem::path& fileName, GLenum Type = GL_TEXTURE_2D ) { TextureFromFile( tex, fileName, Type, Type ); }

void SetupTextureSampling( GLenum Target, GLuint textureID, bool generateMipMap = true );

template<typename VertexT>
struct MeshObject
{
    std::vector<VertexT> vertexArray;
    std::vector<GLuint>  indexArray;
};

struct OGLObject
{
    GLuint  vaoID = 0; // vertex array object erőforrás azonosító
    GLuint  vboID = 0; // vertex buffer object erőforrás azonosító
    GLuint  iboID = 0; // index buffer object erőforrás azonosító
    GLsizei count = 0; // mennyi indexet/vertexet kell rajzolnunk
};


struct VertexAttributeDescriptor
{
	GLuint         index = -1;
	std::uintptr_t strideInBytes = 0;
	GLint          numberOfComponents = 0;
	GLenum         glType = GL_NONE;
};

template <typename VertexT>
[[nodiscard]] OGLObject CreateGLObjectFromMesh( const MeshObject<VertexT>& mesh, std::initializer_list<VertexAttributeDescriptor> vertexAttrDescList )
{
	OGLObject meshGPU = { 0 };

	// 1 db VAO foglalasa
	glGenVertexArrays(1, &meshGPU.vaoID);
	// a frissen generált VAO beallitasa aktívnak
	glBindVertexArray(meshGPU.vaoID);

	// hozzunk létre egy új VBO erőforrás nevet
	glGenBuffers(1, &meshGPU.vboID);
	glBindBuffer(GL_ARRAY_BUFFER, meshGPU.vboID); // tegyük "aktívvá" a létrehozott VBO-t

	// töltsük fel adatokkal az aktív VBO-t
	glBufferData(GL_ARRAY_BUFFER,	// az aktív VBO-ba töltsünk adatokat
				  mesh.vertexArray.size() * sizeof(VertexT),		// ennyi bájt nagyságban
				  mesh.vertexArray.data(),	// erről a rendszermemóriabeli címről olvasva
				  GL_STATIC_DRAW);	// úgy, hogy a VBO-nkba nem tervezünk ezután írni és minden kirajzoláskor felhasnzáljuk a benne lévő adatokat

	// index puffer létrehozása
	glGenBuffers(1, &meshGPU.iboID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshGPU.iboID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indexArray.size() * sizeof(GLuint), mesh.indexArray.data(), GL_STATIC_DRAW);

	meshGPU.count = static_cast<GLsizei>(mesh.indexArray.size());

	for ( const auto& vertexAttrDesc: vertexAttrDescList )
	{
		glEnableVertexAttribArray(vertexAttrDesc.index); // ez lesz majd a pozíció
		glVertexAttribPointer(
			vertexAttrDesc.index,				  // a VB-ben található adatok közül a 0. "indexű" attribútumait állítjuk be
			vertexAttrDesc.numberOfComponents,	  // komponens szam
			vertexAttrDesc.glType,				  // adatok tipusa
			GL_FALSE,							  // normalizalt legyen-e
			sizeof(VertexT),					  // stride (0=egymas utan)
			reinterpret_cast<const void*>(vertexAttrDesc.strideInBytes) // a 0. indexű attribútum hol kezdődik a sizeof(Vertex)-nyi területen belül
		);
	}

	glBindVertexArray(0); // feltöltüttük a VAO-t, kapcsoljuk le
	glBindBuffer(GL_ARRAY_BUFFER, 0); // feltöltöttük a VBO-t is, ezt is vegyük le
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return meshGPU;
}

void CleanOGLObject( OGLObject& ObjectGPU );

