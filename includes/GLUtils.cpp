#include "GLUtils.hpp"

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include <SDL2/SDL_image.h>

/* 

Az http://www.opengl-tutorial.org/ oldal alapján.

*/
void loadShader( const GLuint loadedShader, const std::filesystem::path& _fileName )
{
	// ha nem sikerult hibauzenet es -1 visszaadasa
	if ( loadedShader == 0 )
	{
		SDL_LogMessage( SDL_LOG_CATEGORY_ERROR,
						SDL_LOG_PRIORITY_ERROR,
						"Shader needs to be inited before loading %s !", _fileName.string().c_str());
		return;
	}

	// shaderkod betoltese _fileName fajlbol
	std::string shaderCode = "";

	// _fileName megnyitasa
	std::ifstream shaderStream( _fileName );
	if ( !shaderStream.is_open() )
	{
		SDL_LogMessage( SDL_LOG_CATEGORY_ERROR,
						SDL_LOG_PRIORITY_ERROR,
						"Error while opening shader file %s!", _fileName.string().c_str());
		return;
	}

	// file tartalmanak betoltese a shaderCode string-be
	std::string line = "";
	while ( std::getline( shaderStream, line ) )
	{
		shaderCode += line + "\n";
	}

	shaderStream.close();

	compileShaderFromSource( loadedShader, shaderCode );
}

void compileShaderFromSource( const GLuint loadedShader, std::string_view shaderCode )
{
	// kod hozzarendelese a shader-hez
	const char* sourcePointer = shaderCode.data();
	GLint sourceLength = static_cast<GLint>( shaderCode.length() );

	glShaderSource( loadedShader, 1, &sourcePointer, &sourceLength );

	// shader leforditasa
	glCompileShader( loadedShader );

	// ellenorizzuk, h minden rendben van-e
	GLint result = GL_FALSE;
	int infoLogLength;

	// forditas statuszanak lekerdezese
	glGetShaderiv(loadedShader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(loadedShader, GL_INFO_LOG_LENGTH, &infoLogLength);

	if ( GL_FALSE == result || infoLogLength != 0 )
	{
		// hibauzenet elkerese es kiirasa
		std::string ErrorMessage(infoLogLength,'\0');
		glGetShaderInfoLog(loadedShader, infoLogLength, NULL, ErrorMessage.data());

		SDL_LogMessage( SDL_LOG_CATEGORY_ERROR, 
						( result ) ? SDL_LOG_PRIORITY_WARN : SDL_LOG_PRIORITY_ERROR,
						"[glLinkProgram] Shader compile error: %s" , ErrorMessage.data() );
	}
}


void AssembleProgram( const GLuint programID, const std::filesystem::path& vs_filename, const std::filesystem::path& fs_filename )
{
	//
	// shaderek betöltése
	//

	if ( programID == 0 ) return;

	GLuint vs_ID = glCreateShader( GL_VERTEX_SHADER   );
	GLuint fs_ID = glCreateShader( GL_FRAGMENT_SHADER );

	if ( vs_ID == 0 || fs_ID == 0 )
	{
		SDL_SetError("Error while initing shaders (glCreateShader)!");
	}

	loadShader(vs_ID, vs_filename );
	loadShader(fs_ID, fs_filename );

	// adjuk hozzá a programhoz a shadereket
	glAttachShader(programID, vs_ID);
	glAttachShader(programID, fs_ID);

	// illesszük össze a shadereket (kimenő-bemenő változók összerendelése stb.)
	glLinkProgram(programID);

	// linkeles ellenorzese
	GLint infoLogLength = 0, result = 0;

	glGetProgramiv(programID, GL_LINK_STATUS, &result);
	glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (GL_FALSE == result || infoLogLength != 0 )
	{
		std::string ErrorMessage(infoLogLength, '\0');
		glGetProgramInfoLog(programID, infoLogLength, nullptr, ErrorMessage.data() );
		SDL_LogMessage( SDL_LOG_CATEGORY_ERROR, 
						( result ) ? SDL_LOG_PRIORITY_WARN : SDL_LOG_PRIORITY_ERROR,
						"[glLinkProgram] Shader linking error: %s" , ErrorMessage.data() );
	}

	// mar nincs ezekre szukseg
	glDeleteShader( vs_ID );
	glDeleteShader( fs_ID );
}

static void invert_image_RGBA(int pitchInPixels, int height, Uint32* image_pixels)
{
	int height_div_2 = height / 2;
	Uint32* lower_data  =image_pixels;
	Uint32* higher_data =image_pixels + ( height - 1 ) * pitchInPixels;

	for ( int index = 0; index < height_div_2; index++ )
	{
		for ( int rowIndex = 0; rowIndex < pitchInPixels; rowIndex++ )
		{
			*lower_data ^= higher_data[ rowIndex ];
			higher_data[ rowIndex ] ^= *lower_data;
			*lower_data ^= higher_data[ rowIndex ];

			lower_data++;
		}
		higher_data -= pitchInPixels;
	}
}

void TextureFromFile( const GLuint tex, const std::filesystem::path& fileName, GLenum Type, GLenum Role )
{
	if ( tex == 0 )
	{
		SDL_LogMessage( SDL_LOG_CATEGORY_ERROR, 
						SDL_LOG_PRIORITY_ERROR,
						"Texture object needs to be inited before loading %s !", fileName.string().c_str());
		return;
	}

	// Kép betöltése
	SDL_Surface* loaded_img = IMG_Load(fileName.string().c_str());

	if (loaded_img == nullptr)
	{
		SDL_LogMessage( SDL_LOG_CATEGORY_ERROR, 
						SDL_LOG_PRIORITY_ERROR,
						"[TextureFromFile] Error while loading texture: %s", fileName.string().c_str());
		return;
	}

	// Uint32-ben tárolja az SDL a színeket, ezért számít a bájtsorrend
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	Uint32 format = SDL_PIXELFORMAT_ABGR8888;
#else
	Uint32 format = SDL_PIXELFORMAT_RGBA8888;
#endif

	// Átalakítás 32bit RGBA formátumra, ha nem abban volt
	SDL_Surface* formattedSurf = SDL_ConvertSurfaceFormat(loaded_img, format, 0);
	SDL_FreeSurface(loaded_img);
	if (formattedSurf == nullptr)
	{
		SDL_LogMessage( SDL_LOG_CATEGORY_ERROR, 
						SDL_LOG_PRIORITY_ERROR,
						"[TextureFromFile] Error while processing texture");
		return;
	}

	// Áttérés SDL koordinátarendszerről ( (0,0) balfent ) OpenGL textúra-koordinátarendszerre ( (0,0) ballent )
	if ( Type != GL_TEXTURE_CUBE_MAP && Type != GL_TEXTURE_CUBE_MAP_ARRAY )
		invert_image_RGBA( formattedSurf->pitch / sizeof( Uint32 ), formattedSurf->h, reinterpret_cast<Uint32*>( formattedSurf->pixels ) );

	glBindTexture(Type, tex);
	glTexImage2D(
		Role, 						// melyik binding point-on van a textúra erőforrás, amihez tárolást rendelünk
		0, 							// melyik részletességi szint adatait határozzuk meg
		GL_RGBA, 					// textúra belső tárolási formátuma (GPU-n)
		formattedSurf->w, 			// szélesség
		formattedSurf->h, 			// magasság
		0, 							// nulla kell, hogy legyen ( https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml )
		GL_RGBA, 					// forrás (=CPU-n) formátuma
		GL_UNSIGNED_BYTE, 			// forrás egy pixelének egy csatornáját hogyan tároljuk
		formattedSurf->pixels);		// forráshoz pointer

	glBindTexture(Type, 0);

	// Használt SDL_Surface-k felszabadítása
	SDL_FreeSurface(formattedSurf);
}

void SetupTextureSampling( GLenum Target, GLuint textureID, bool generateMipMap )
{
	// mintavételezés beállításai
	glBindTexture( Target, textureID );
	if ( generateMipMap ) glGenerateMipmap( Target ); // Mipmap generálása
	glTexParameteri( Target, GL_TEXTURE_MAG_FILTER, GL_LINEAR ); // bilineáris szürés nagyításkor (ez az alapértelmezett)
	glTexParameteri( Target, GL_TEXTURE_MIN_FILTER, generateMipMap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR ); // trilineáris szűrés a mipmap-ekböl kicsinyítéskor
	// mi legyen az eredmény, ha a textúrán kívülröl próbálunk mintát venni?
	glTexParameteri( Target, GL_TEXTURE_WRAP_S, GL_REPEAT ); // vízszintesen
	glTexParameteri( Target, GL_TEXTURE_WRAP_T, GL_REPEAT ); // függölegesen
	glBindTexture( Target, 0 );
}

void CleanOGLObject( OGLObject& ObjectGPU )
{
	glDeleteBuffers(1,      &ObjectGPU.vboID);
	ObjectGPU.vboID = 0;
	glDeleteBuffers(1,      &ObjectGPU.iboID);
	ObjectGPU.iboID = 0;
	glDeleteVertexArrays(1, &ObjectGPU.vaoID);
	ObjectGPU.vaoID = 0;
}