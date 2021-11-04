#include "AssetManager.h"

namespace bs
{
    AssetManager* asset_manager;

    AssetManager::AssetManager()
	{

	}

	void AssetManager::addTexture(bs::vk::Texture& texture, short&& id) noexcept
	{
		m_textures.addAsset(texture, id);
	}

	void AssetManager::addModel(bs::vk::Model& model, std::string&& id) noexcept
	{
		m_models.addAsset(model, id);
	}

	void AssetManager::addBuffer(bs::vk::Buffer* buffer, std::string&& id) noexcept
	{
		auto bufferptr = std::shared_ptr<bs::vk::Buffer>(buffer);
		m_buffers.addAsset(bufferptr, id);
	}

	void AssetManager::addImg(bs::Image& img, std::string&& id) noexcept
	{
		m_images.addAsset(img, id);
	}

	const bs::vk::Texture& AssetManager::getTexture(short&& id)
	{
		return m_textures.getAsset(std::forward<short>(id));
	}

	bs::vk::Texture& AssetManager::getTextureMutable(short&& id)
	{
		return m_textures.getAsset(std::forward<short>(id));
	}

	size_t AssetManager::getNumTextures() noexcept
	{
		return m_textures.getMap().size();
	}
	
	//List of textures for pushing data to the gpu
	const std::vector<bs::vk::texture_t>& AssetManager::getTextures()
	{
		static std::vector<bs::vk::texture_t> textures;
		static bool ran = false;
		if(!ran)
		{
			textures.reserve(m_textures.getMap().size());
			for(auto& [key, value] : m_textures.getMap())
			{
				textures.emplace_back(value.getAPITextureInfo());
			}
			
			ran = true;
		}

		/// OR ALTERNATIVE IMPL:
		/*
		textures.resize(getNumTextures());
		for(auto& [key, value] : m_textures.getMap())
		{
			textures[key] = value.getAPITextureInfo();
		}
		*/

		return textures;
	}

	bs::vk::Model& AssetManager::getModel(std::string&& id)
	{
		return m_models.getAsset(std::forward<std::string>(id));
	}

	bs::vk::Buffer* AssetManager::getBuffer(std::string&& id)
	{
		return m_buffers.getAsset(std::forward<std::string>(id)).get();
	}

	bs::Image& AssetManager::getImage(std::string&& id)
	{
		return m_images.getAsset(std::forward<std::string>(id));
	}
}