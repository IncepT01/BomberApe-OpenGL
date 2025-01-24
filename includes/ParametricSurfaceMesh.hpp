#pragma once
#include "GLUtils.hpp"

template <typename SurfT>
[[nodiscard]] MeshObject<Vertex> GetParamSurfMesh( const SurfT& surf, const std::size_t N = 80, const std::size_t M = 40 )
{
    MeshObject<Vertex> outputMesh;

	// NxM darab négyszöggel közelítjük a parametrikus felületünket => (N+1)x(M+1) pontban kell kiértékelni
	outputMesh.vertexArray.resize((N + 1) * (M + 1));


	for (std::size_t j = 0; j <= M; ++j)
	{
		for (std::size_t i = 0; i <= N; ++i)
		{
			float u = i / (float)N;
			float v = j / (float)M;

			std::size_t index = i + j * (N + 1);
			outputMesh.vertexArray[index].position = surf.GetPos(u, v);
			outputMesh.vertexArray[index].normal   = surf.GetNorm(u, v);
			outputMesh.vertexArray[index].texcoord = surf.GetTex(u, v);
		}
	}



	// indexpuffer adatai: NxM négyszög = 2xNxM háromszög = háromszöglista esetén 3x2xNxM index
	outputMesh.indexArray.resize(3 * 2 * (N) * (M));

	for (std::size_t j = 0; j < M; ++j)
	{
		for (std::size_t i = 0; i < N; ++i)
		{
			// minden négyszögre csináljunk kettő háromszöget, amelyek a következő 
			// (i,j) indexeknél született (u_i, v_j) paraméterértékekhez tartozó
			// pontokat kötik össze:
			// 
			// (i,j+1) C-----D (i+1,j+1)
			//         |\    |				A = p(u_i, v_j)
			//         | \   |				B = p(u_{i+1}, v_j)
			//         |  \  |				C = p(u_i, v_{j+1})
			//         |   \ |				D = p(u_{i+1}, v_{j+1})
			//         |    \|
			//   (i,j) A-----B (i+1,j)
			//
			// - az (i,j)-hez tartózó 1D-s index a VBO-ban: i+j*(N+1)
			// - az (i,j)-hez tartózó 1D-s index az IB-ben: i*6+j*6*N
			//		(mert minden négyszöghöz 2db háromszög = 6 index tartozik)
			//
			std::size_t index = i * 6 + j * (6 * N);
			outputMesh.indexArray[ index + 0 ] = static_cast<GLuint>( ( i     ) + ( j     ) * ( N + 1 ) );
			outputMesh.indexArray[ index + 1 ] = static_cast<GLuint>( ( i + 1 ) + ( j     ) * ( N + 1 ) );
			outputMesh.indexArray[ index + 2 ] = static_cast<GLuint>( ( i     ) + ( j + 1 ) * ( N + 1 ) );
			outputMesh.indexArray[ index + 3 ] = static_cast<GLuint>( ( i + 1 ) + ( j     ) * ( N + 1 ) );
			outputMesh.indexArray[ index + 4 ] = static_cast<GLuint>( ( i + 1 ) + ( j + 1 ) * ( N + 1 ) );
			outputMesh.indexArray[ index + 5 ] = static_cast<GLuint>( ( i     ) + ( j + 1 ) * ( N + 1 ) );
		}
	}
    
        return outputMesh;
}