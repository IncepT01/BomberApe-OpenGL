#include "ObjParser.h"
#include <array>
#include <list>
#include <string>
#include <charconv>
#include <algorithm>

#include <glm/gtx/norm.hpp>
#include <glm/gtc/constants.hpp>

using namespace std;

class InMemoryTokenizer
{
public:
	InMemoryTokenizer() = default;
	void SetData( const char* ptr, size_t Length ) noexcept;
	std::string_view NextToken( bool onlySameLine = false ) noexcept;
	void ToNextLine() noexcept;
	operator bool() const noexcept;
private:
	const char* currentPtr = nullptr;
	const char* endPtr = nullptr;
};

void InMemoryTokenizer::SetData( const char* ptr, size_t Length ) noexcept
{
	this->currentPtr = ptr;
	this->endPtr = ptr + Length;
}

std::string_view InMemoryTokenizer::NextToken( bool onlySameLine ) noexcept
{
	for ( ;currentPtr < endPtr 
			&& std::isspace(static_cast<unsigned char>(*currentPtr) ); ++currentPtr )
	{
		if ( onlySameLine && *currentPtr == '\n' )
		{
			return std::string_view();
		}
	}
	const char* tPtr = currentPtr;
	std::size_t tLength = 0;

	while ( currentPtr < endPtr 
			&& !std::isspace(static_cast<unsigned char>(*currentPtr) ) )
	{
		currentPtr++;
		tLength++;
	}

	return std::string_view( tPtr, tLength );
}

void InMemoryTokenizer::ToNextLine() noexcept
{
	while ( currentPtr < endPtr && *currentPtr != '\n' )
		currentPtr++;
	currentPtr++;
}

InMemoryTokenizer::operator bool() const noexcept
{
	return currentPtr < endPtr;
}

constexpr unsigned short From2Char( const char ch1, const char ch2)
{
	unsigned short sh = static_cast<unsigned short>(ch2) << 8 | static_cast<unsigned short>(ch1);

	return sh;
}

static std::vector<unsigned int> triangulatePolygon( const std::vector<glm::vec2>& );

ObjParser::Mesh ObjParser::parse(const std::filesystem::path& fileName)
{
	Mesh resultMesh;

	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texcoords;

	std::vector<IndexedVert> face_vertIds;
	face_vertIds.reserve( 4 );
	bool needsNormalComputation = false;
	std::unordered_map<IndexedVert, unsigned int, IndexedVertHash> vertexIndices;

	std::error_code ec;
	std::size_t fileSize = std::filesystem::file_size( fileName, ec );

	if ( ec ) throw(EXC_FILENOTFOUND);

	std::vector<char> objRawData( fileSize );

	std::ifstream objFileStrm( fileName, std::ios::binary );

	if ( !objFileStrm ) throw(EXC_FILENOTFOUND);

	objFileStrm.read( objRawData.data(), fileSize );

	InMemoryTokenizer tokenizer;

	tokenizer.SetData( objRawData.data(), fileSize );

	unsigned int nIndexedVerts = 0;

	while ( tokenizer )
	{
		std::string_view token = tokenizer.NextToken();

		if ( token[ 0 ] == '#' )
		{
			tokenizer.ToNextLine();
			continue;
		}

		switch ( *reinterpret_cast<const unsigned short*>( token.data() ) )
		{
			case From2Char('m','t'): //mtllib <.mtl file>
			{
				auto mtlFile = tokenizer.NextToken();
			} break;

			case From2Char('u','s'): // usemtl <material name>
			{
				auto mtlName = tokenizer.NextToken();
			}break;

			case From2Char('o',' '):
			case From2Char('o','\t'): // o <object name>
			{
				auto objectName = tokenizer.NextToken();
			}break;

			case From2Char('g',' '):
			case From2Char('g','\t'): // g <group name>
			{
				auto groupName = tokenizer.NextToken();
			}break;
			case From2Char('v',' '):
			case From2Char('v','\t'): // v <x> <y> <z> [<w>]
			{
				positions.emplace_back(glm::vec3());

				float& x = positions.back().x;
				float& y = positions.back().y;
				float& z = positions.back().z;

				std::string_view coordT = tokenizer.NextToken();
				std::from_chars( coordT.data(), coordT.data() + coordT.size(), x );
				coordT = tokenizer.NextToken();
				std::from_chars( coordT.data(), coordT.data() + coordT.size(), y );
				coordT = tokenizer.NextToken();
				std::from_chars( coordT.data(), coordT.data() + coordT.size(), z );
				coordT = tokenizer.NextToken(true);

				if ( !coordT.empty() )
				{
					float w;
					std::from_chars( coordT.data(), coordT.data() + coordT.size(), w );
					x /= w;
					y /= w;
					z /= w;
				}
			}break;
			case From2Char('v','n'): // vn <nx> <ny> <nz>
			{
				normals.emplace_back(glm::vec3());

				float& x = normals.back().x;
				float& y = normals.back().y;
				float& z = normals.back().z;

				std::string_view coordT = tokenizer.NextToken();
				std::from_chars( coordT.data(), coordT.data() + coordT.size(), x );
				coordT = tokenizer.NextToken();
				std::from_chars( coordT.data(), coordT.data() + coordT.size(), y );
				coordT = tokenizer.NextToken();
				std::from_chars( coordT.data(), coordT.data() + coordT.size(), z );
				
			}break;
			case From2Char('v','t'): // vt <s> <t>
			{
				texcoords.emplace_back(glm::vec2());

				float& s = texcoords.back().x;
				float& t = texcoords.back().y;

				std::string_view coordT = tokenizer.NextToken();
				std::from_chars( coordT.data(), coordT.data() + coordT.size(), s );
				coordT = tokenizer.NextToken();
				std::from_chars( coordT.data(), coordT.data() + coordT.size(), t );
	
			}break;
			case From2Char('f',' '):
			case From2Char('f','\t'): // f (<pi>[/<ti>][/<ni>])3+
			{
				face_vertIds.clear();
				needsNormalComputation = false;

				std::string_view faceVertT = tokenizer.NextToken( true );
				while ( !faceVertT.empty() )
				{
					face_vertIds.emplace_back( IndexedVert{} );
					IndexedVert& idxVert = face_vertIds.back();

					size_t posEndOffs = faceVertT.find_first_of( '/', 0 );
					if ( posEndOffs == std::string_view::npos ) posEndOffs = faceVertT.size();

					std::from_chars( faceVertT.data(), faceVertT.data() + posEndOffs, idxVert.v );
					idxVert.v--;

					size_t texStartOffs = posEndOffs + 1;
					size_t texEndOffs = faceVertT.find_first_of( '/', texStartOffs );
					if ( texEndOffs == std::string_view::npos ) texEndOffs = faceVertT.size();
					if ( texEndOffs > texStartOffs ) std::from_chars( faceVertT.data() + texStartOffs, faceVertT.data() + texEndOffs, idxVert.vt);
					if ( idxVert.vt ) idxVert.vt--; 
					size_t normStartOffs = texEndOffs + 1;

					if ( faceVertT.size() > normStartOffs )
					{
						std::from_chars( faceVertT.data() + normStartOffs, faceVertT.data() + faceVertT.size(), idxVert.vn );
						idxVert.vn--;
					}
					else needsNormalComputation = true;
					
					faceVertT = tokenizer.NextToken( true );
				}

				if ( 3 < face_vertIds.size() )
				{
					std::vector<IndexedVert> face_vertIdsFace2Tris;
					if ( 4 == face_vertIds.size() )
					{
						glm::vec3 v10 = positions[ face_vertIds[ 0 ].v ] - positions[ face_vertIds[ 1 ].v ];
						glm::vec3 v12 = positions[ face_vertIds[ 2 ].v ] - positions[ face_vertIds[ 1 ].v ];

						glm::vec3 v32 = positions[ face_vertIds[ 2 ].v ] - positions[ face_vertIds[ 3 ].v ];
						glm::vec3 v30 = positions[ face_vertIds[ 0 ].v ] - positions[ face_vertIds[ 3 ].v ];

						float angle_012 = ::acosf( glm::dot(v10,v12) / sqrtf( glm::dot(v10,v10) * glm::dot(v12,v12) ) );
						float angle_230 = ::acosf( glm::dot(v32,v30) / sqrtf( glm::dot(v32,v32) * glm::dot(v30,v30) ) );
						
						if ( ( angle_012 + angle_230 ) <= glm::pi<float>() )
						{
							face_vertIdsFace2Tris =
							{ face_vertIds[ 0 ], face_vertIds[ 1 ], face_vertIds[ 2 ],
							  face_vertIds[ 0 ], face_vertIds[ 2 ], face_vertIds[ 3 ] };
						}
						else
						{
							face_vertIdsFace2Tris =
							{ face_vertIds[ 0 ], face_vertIds[ 1 ], face_vertIds[ 3 ],
							  face_vertIds[ 1 ], face_vertIds[ 2 ], face_vertIds[ 3 ] };
						}
					}
					else 
					{
						// Calculate the best fitting plane
						glm::vec3 MidPoint( 0.0 );
						for ( const auto& vertex : face_vertIds )
						{
							MidPoint += positions[ vertex.v ];
						}
						MidPoint /= float( face_vertIds.size() );

						std::vector<glm::vec3> centeredPoints( face_vertIds.size() );

						std::transform( face_vertIds.cbegin(), face_vertIds.cend(), centeredPoints.begin(),
										[&positions,MidPoint]( const IndexedVert& faceV )->glm::vec3
										{ return positions[ faceV.v ] - MidPoint;}
										);

						float cov_xx = 0.0f, cov_xy = 0.0f;
						float cov_yy = 0.0f, cov_yz = 0.0f;
						float cov_xz = 0.0f, cov_zz = 0.0f;

						for ( const glm::vec3& centeredP : centeredPoints )
						{
							cov_xx += centeredP.x * centeredP.x;
							cov_xy += centeredP.x * centeredP.y;
							
							cov_yy += centeredP.y * centeredP.y;
							cov_yz += centeredP.y * centeredP.z;

							cov_xz += centeredP.x * centeredP.z;
							cov_zz += centeredP.z * centeredP.z;
						}

						// viktor-vad: Very strange, but the pca.hpp and pca.inc disappeared from glm/gtx.
						// Did not find any explanation for this.
						// Instead of some header file copy-hacking, I implemented a 3x3 verion of eigen decomposition.
						// It was not intended, but most likely it is faster than the original glm pca, since that is a general method with Housholder and QR.
						// https://dl.acm.org/doi/epdf/10.1145/355578.366316
						// https://en.wikipedia.org/wiki/Eigenvalue_algorithm#2%C3%972_matrices
						glm::vec3 eigenVectors[2];
						{
							glm::vec3 eigenVectors_[3];
							float p1 = cov_xy * cov_xy + cov_xz * cov_xz + cov_yz * cov_yz;
							float trC = cov_xx + cov_yy + cov_zz;
							float eig1 = 0.0f, eig2 = 0.0f, eig3 = 0.0f;

							// normal case
							if ( p1 > 1e-15f )
							{
								float q = trC / 3.0f;
								float p2 = ( cov_xx - q ) * ( cov_xx - q ) + ( cov_yy - q ) * ( cov_yy - q ) + ( cov_zz - q ) * ( cov_zz - q ) + 2.0f * p1;
								float p = std::sqrt( p2 / 6.0f );

								float cov_xx_q = cov_xx - q;
								float cov_yy_q = cov_yy - q;
								float cov_zz_q = cov_zz - q;

								float r = glm::clamp( ( cov_xx_q * cov_yy_q * cov_zz_q + 2.0f * cov_xy * cov_yz * cov_xz - cov_xx_q * cov_yz * cov_yz - cov_yy_q * cov_xz * cov_xz - cov_zz_q * cov_xy * cov_xy ) / ( 2.0f * p * p * p ),
													  -1.0f, 1.0f );

								float phi = ::acosf( r ) / 3.0f;

								eig1 = q + 2.0f * p * std::cos( phi );
								eig2 = q + 2.0f * p * std::cos( phi + ( 2.0f * glm::pi<float>() / 3.0f ) );
								eig3 = trC - eig1 - eig2;
							}
							else // covariance matrix is numericaly diagonal. We assume eigen values are the diagonal values.
							{
								eig1 = std::max( { cov_xx, cov_yy, cov_zz } );
								eig3 = std::min( { cov_xx, cov_yy, cov_zz } );
								eig2 = trC - eig1 - eig2;
							}

							eigenVectors_[ 0 ] = glm::vec3( cov_xy * cov_xy + cov_xz * cov_xz + ( cov_xx - eig2 ) * ( cov_xx - eig3 ),
														   cov_xy * ( ( cov_xx - eig3 ) + ( cov_yy - eig2 ) ) + cov_xz * cov_yz,
														   cov_xz * ( ( cov_xx - eig3 ) + ( cov_zz - eig2 ) ) + cov_xy * cov_yz );

							eigenVectors_[ 1 ] = glm::vec3( cov_xy * ( ( cov_xx - eig1 ) + ( cov_yy - eig3 ) ) + cov_xz * cov_yz,
														   cov_yz * cov_yz + cov_xy * cov_xy + ( cov_yy - eig1 ) * ( cov_yy - eig3 ),
														   cov_yz * ( ( cov_yy - eig3 ) + ( cov_zz - eig1 ) ) + cov_xy * cov_xz );

							eigenVectors_[ 2 ] = glm::vec3( cov_xz * ( ( cov_xx - eig1 ) + ( cov_zz - eig2 ) ) + cov_xy * cov_yz,
														   cov_yz * ( ( cov_yy - eig1 ) + ( cov_zz - eig2 ) ) + cov_xy * cov_xz,
														   cov_yz * cov_yz + cov_xz * cov_xz + ( cov_zz - eig1 ) * ( cov_zz - eig2 ) );
							
							// Simplification of original method.
							// We only need the first 2 eigen vectors for 2D projection.
							// Therefor we are not intereted, which is bigger, but in leaving the smallest out.
							float minEig = std::min( { eig1, eig2, eig3 } );

							if ( eig3 == minEig )
							{
								eigenVectors[ 0 ] = glm::normalize( eigenVectors_[ 0 ] );
								eigenVectors[ 1 ] = glm::normalize( eigenVectors_[ 1 ] );
							}
							else if ( eig2 == minEig )
							{
                                eigenVectors[ 0 ] = glm::normalize( eigenVectors_[ 0 ] );
                                eigenVectors[ 1 ] = glm::normalize( eigenVectors_[ 2 ] );
                            }
							else //if ( eig1 == minEig ) most unlikly case
							{
                                eigenVectors[ 0 ] = glm::normalize( eigenVectors_[ 1 ] );
                                eigenVectors[ 1 ] = glm::normalize( eigenVectors_[ 2 ] );
                            }
						}

						std::vector<glm::vec2> facePointsProjected( face_vertIds.size() );
						

						std::transform(centeredPoints.cbegin(),centeredPoints.cend(),facePointsProjected.begin(),
										[ &eigenVectors ]( const glm::vec3& cp )->glm::vec2
										{
											return glm::vec2(
												glm::dot( cp, eigenVectors[0] ),
												glm::dot( cp, eigenVectors[1] )
											);
										} );

						// checking the orientation. CCW should be kept
						float sum = 0.0;
						for ( int i = 0; i < facePointsProjected.size() - 1; ++i )
						{
							sum += ( facePointsProjected[ i + 1 ].x - facePointsProjected[ i ].x ) *
								( facePointsProjected[ i + 1 ].y + facePointsProjected[ i ].y );
						}
						sum += ( facePointsProjected.front().x - facePointsProjected.back().x ) *
							( facePointsProjected.front().y + facePointsProjected.back().y );

						if ( sum > 0.0f )
						{
							for ( int i = 0; i < facePointsProjected.size(); ++i )
								facePointsProjected[ i ].y *= -1.0f;
						}

						std::vector<unsigned int> triIndices = triangulatePolygon( facePointsProjected );
						
						face_vertIdsFace2Tris.resize( triIndices.size() );
						std::transform( triIndices.cbegin(), triIndices.cend(), face_vertIdsFace2Tris.begin(),
										[ &face_vertIds ]( const unsigned int fTriId )->IndexedVert
										{
											return face_vertIds[ fTriId ];
										} );

					}
					face_vertIds = std::move( face_vertIdsFace2Tris );
				}
				
				if ( texcoords.empty() ) texcoords.emplace_back( glm::vec2( 0.0 ) );
				
				if ( needsNormalComputation )
				{
					for ( int i = 0; i < face_vertIds.size(); i += 3 )
					{
						glm::vec3 n = glm::normalize( glm::cross(
							positions[face_vertIds[i + 1].v] - positions[face_vertIds[i].v],
							positions[face_vertIds[i + 2].v] - positions[face_vertIds[i].v]
						) );

						unsigned int n_idx = static_cast<unsigned int>( normals.size() );
						normals.push_back( n );
						face_vertIds[ i ].vn = face_vertIds[ i + 1 ].vn = face_vertIds[ i + 2 ].vn = n_idx;
					}
				}


				for ( const auto& vertex : face_vertIds )
				{
					unsigned int& vIndex = vertexIndices[ vertex ];
					if (vIndex == 0) // new vertex
					{
						Vertex v;
						v.position = positions[vertex.v];
						v.texcoord = texcoords[vertex.vt];
						v.normal = normals[vertex.vn];

						resultMesh.vertexArray.push_back(v);
						resultMesh.indexArray.push_back(nIndexedVerts++);
						vIndex = nIndexedVerts;	
					} else {
						resultMesh.indexArray.push_back(vIndex-1);
					}
				}
			}break;
		}

		tokenizer.ToNextLine();
	}

	return resultMesh;
}

// Hash function for IndexedVert
// version of fasthash64 https://github.com/ztanml/fast-hash
// simplified for using only for 1 64 bit data (seed is the other one).

static inline constexpr uint64_t fasthash64_mix(uint64_t h)
{
	h ^= h >> 23;
	h *= 0x2127599bf4325c37ULL;
	h ^= h >> 47;
	return h;
}

static inline constexpr uint64_t fasthash64( uint64_t v, uint64_t seed )
{
	constexpr uint64_t    m = 0x880355f21e6d1965ULL;
	constexpr uint64_t    m_size = m * sizeof(uint64_t);
	constexpr uint64_t    m_p2 = m * m;

	uint64_t h = seed ^ m_size;
	h ^= fasthash64_mix(v);
	h *= m_p2;

	return fasthash64_mix(h);
}

std::size_t ObjParser::IndexedVertHash::operator()( const IndexedVert& iv ) const noexcept
{
	return fasthash64( iv.vt, iv.vn_64 );
}

static std::vector<unsigned int> triangulatePolygon( const std::vector<glm::vec2>& polygon )
{
	constexpr float M_2PI = glm::two_pi<float>();
	using Edge = std::array<unsigned int, 2>;

	struct
	{
		std::vector<unsigned int> triIdxList;
		
		void AppendTriangle( unsigned int i0, unsigned int i1, unsigned int i2 )
		{
			triIdxList.push_back( i0 );
			triIdxList.push_back( i1 );
			triIdxList.push_back( i2 );
		}

		void SetTriangle( unsigned int triIdx, unsigned int i0, unsigned int i1, unsigned int i2 )
		{
			triIdxList[ triIdx * 3    ] = i0;
			triIdxList[ triIdx * 3 + 1] = i1;
			triIdxList[ triIdx * 3 + 2] = i2;
		}

		bool findTri4Edge( const Edge& edge, unsigned int& triIdx, unsigned int& oppositeIdx ) const noexcept
		{
			for ( triIdx = static_cast<unsigned int>(triIdxList.size()) / 3 - 1;
				  static_cast<int>( triIdx ) >= 0;
				  triIdx-- ) // usually the neighbouring triangles are pushed later
			{
				if ( ( triIdxList[ 3 * triIdx ] == edge[ 0 ] ) && ( triIdxList[ 3 * triIdx + 1 ] == edge[ 1 ] ) )
				{
					oppositeIdx = triIdxList[ 3 * triIdx + 2 ];
					return true;
				}
				if ( ( triIdxList[ 3 * triIdx + 1] == edge[ 0 ] ) && ( triIdxList[ 3 * triIdx + 2 ] == edge[ 1 ] ) )
				{
					oppositeIdx = triIdxList[ 3 * triIdx ];
					return true;
				}
				if ( ( triIdxList[ 3 * triIdx + 2] == edge[ 0 ] ) && ( triIdxList[ 3 * triIdx ] == edge[ 1 ] ) )
				{
					oppositeIdx = triIdxList[ 3 * triIdx + 1 ];
					return true;
				}
			}

			return false;
		}

	} triangulation;
	triangulation.triIdxList.reserve( polygon.size() * 3 - 6 );

	struct PolygonNode
	{
		unsigned int id;
		float angle;
	};

	std::vector<PolygonNode> polygonNodes( polygon.size() );
	std::list< std::array<unsigned int, 2> > edges2check;

	auto advNode = [ &polygonNodes ]( const size_t i,const long int di )->size_t
	{
		 return ( polygonNodes.size() + i + di ) % polygonNodes.size();
	};

	auto computeAngle = [ & ]( const size_t i )->void
	{
		glm::vec2 prevP = polygon[ polygonNodes[advNode(i,-1)].id ];
		glm::vec2     P = polygon[ polygonNodes[        i    ].id ];
		glm::vec2 postP = polygon[ polygonNodes[advNode(i, 1)].id ];

		float angle1 = atan2f( prevP.y - P.y, prevP.x - P.x );
		float angle2 = atan2f( postP.y - P.y, postP.x - P.x );

		polygonNodes[ i ].angle = angle1 - angle2;
		if ( polygonNodes[ i ].angle < 0.0f )
		{
			polygonNodes[ i ].angle += M_2PI;
		}

	};

	for ( unsigned int i = 0; i < polygonNodes.size(); ++i )
	{
		polygonNodes[ i ].id = i;
	}
	for ( unsigned int i = 0; i < polygonNodes.size(); ++i )
	{
		computeAngle( i );
	}

	while ( polygonNodes.size() > 2 )
	{
		unsigned int minAngleIdx = static_cast<unsigned int>( 
			std::distance( polygonNodes.cbegin(), 
						   std::min_element( polygonNodes.cbegin(), polygonNodes.cend(),
						[]( const PolygonNode& n1, const PolygonNode& n2 ) { return n1.angle < n2.angle; } 
						) ) );

		const unsigned int i0 = polygonNodes[ advNode( minAngleIdx, -1 ) ].id;
		const unsigned int i1 = polygonNodes[          minAngleIdx       ].id;
		const unsigned int i2 = polygonNodes[ advNode( minAngleIdx,  1 ) ].id;

		triangulation.AppendTriangle( i0, i1, i2 );

		edges2check.push_back( { i0, i1 });
		edges2check.push_back( { i1, i2 });
		edges2check.push_back( { i2, i0 });

		for ( ; !edges2check.empty(); edges2check.pop_front() )
		{
			const Edge& edge2check = edges2check.front();
			const unsigned int _idx0 = edge2check[ 0 ];
			const unsigned int _idx2 = edge2check[ 1 ];

			unsigned int leftTriIdx, _idx3;
			bool leftFound = triangulation.findTri4Edge( edge2check, leftTriIdx, _idx3 );
			unsigned int rightTriIdx, _idx1;
			bool rightFound = triangulation.findTri4Edge( { edge2check[ 1 ], edge2check[ 0 ] }, rightTriIdx, _idx1 );

			if ( leftFound && rightFound )
			{
				const glm::vec2& P0 = polygon[ _idx0 ];
				const glm::vec2& P1 = polygon[ _idx1 ];
				const glm::vec2& P2 = polygon[ _idx2 ];
				const glm::vec2& P3 = polygon[ _idx3 ];

				const glm::vec2 P0mP3 = P0 - P3;
				const glm::vec2 P1mP3 = P1 - P3;
				const glm::vec2 P2mP3 = P2 - P3;

				glm::mat3 D( P0mP3.x, P0mP3.y, glm::dot( P0mP3, P0mP3 ),
							 P1mP3.x, P1mP3.y, glm::dot( P1mP3, P1mP3 ),
							 P2mP3.x, P2mP3.y, glm::dot( P2mP3, P2mP3 )
				);

				if ( glm::determinant( D ) > 0.0 )
				{
					triangulation.SetTriangle( leftTriIdx,  _idx0, _idx1, _idx3 );
					triangulation.SetTriangle( rightTriIdx, _idx1, _idx2, _idx3 );

					edges2check.push_back( { _idx0, _idx1 } );
					edges2check.push_back( { _idx1, _idx2 } );
					edges2check.push_back( { _idx2, _idx3 } );
					edges2check.push_back( { _idx3, _idx0 } );
				}
			}
		}

		polygonNodes.erase( polygonNodes.cbegin() + minAngleIdx );
		computeAngle( advNode( minAngleIdx, -1 ) );
		computeAngle( advNode( minAngleIdx,  0 ) );
	}
	return triangulation.triIdxList;
}