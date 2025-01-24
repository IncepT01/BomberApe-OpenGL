#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <functional>

#include "GLUtils.hpp"


class ObjParser
{
public:

	typedef MeshObject<Vertex> Mesh;

	static Mesh parse(const std::filesystem::path& fileName);

	enum Exception { EXC_FILENOTFOUND };

private:
	struct IndexedVert
	{
		union
		{
			struct
			{
				uint32_t v,vt;
			};
			uint64_t v_vt = 0Ul;
		}; 

		union
		{
			struct
			{
				uint32_t dummy;
				uint32_t vn;
			};
			uint64_t vn_64 = 0Ul;
		}; 
		
		inline bool operator==( const IndexedVert& other ) const
		{
			return this->v_vt == other.v_vt && this->vn == other.vn;
		}

	};

	struct IndexedVertHash
	{
		std::size_t operator()( const IndexedVert& iv ) const noexcept;
	};
};
