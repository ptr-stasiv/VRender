#pragma once
#include "vrender.h"

#include <vector>

namespace asset
{
	struct MeshInfo
	{
		std::string Name;

		std::vector<glm::vec3> Positions;
		std::vector<glm::vec3> Normals;
		std::vector<glm::vec2> UVs;

		std::vector<glm::vec3> Tangents;
		std::vector<glm::vec3> Bitangents;
	};

	struct ImageInfo
	{
		uint16_t Width;
		uint16_t Height;

		std::vector<uint8_t> PixelsData;
	};

	using AssetId = size_t;
}

namespace manager
{
	class AssetManager
	{
	private:
		std::unordered_map<asset::AssetId, asset::MeshInfo> MeshesLookup;
		std::unordered_map<asset::AssetId, asset::ImageInfo> ImagesLookup;
	public:
		asset::AssetId LoadMeshInfo(const std::string& filepath);
		asset::AssetId LoadImageInfo(const std::string& filepath);

		inline void UnloadAll()
		{
			MeshesLookup.clear();
			ImagesLookup.clear();
		}

		inline asset::MeshInfo GetMeshInfo(const asset::AssetId id)
		{
			auto findRes = MeshesLookup.find(id);
			if (findRes == MeshesLookup.end())
			{
				LOG("Error getting mesh under id: %d", id)
				return asset::MeshInfo{};
			}

			return findRes->second;
		}

		inline asset::ImageInfo GetImageInfo(const asset::AssetId id)
		{
			auto findRes = ImagesLookup.find(id);
			if (findRes == ImagesLookup.end())
			{
				LOG("Error getting image under id: %d", id)
				return asset::ImageInfo{};
			}

			return findRes->second;
		}
	};
}