#pragma once

// Main
#include <Registries/ResourceManager.h>

// Types to hold
#include <GPU/Context.h>
#include <Resources/Mesh.h>
#include <Resources/Image.h>
#include <Resources/Material.h>


// @TODO: Possibly templatify this into 4 functions? Get, Add, GetMap, DoesExist?


namespace bs
{
	class AssetManager
	{
	public:
		AssetManager();

		void addTexture(bs::vk::Texture& texture, short&& id) noexcept;

		void addModel(bs::vk::Model& model, std::string&& id) noexcept;

		void addBuffer(bs::vk::Buffer* buffer, std::string&& id) noexcept;

		void addImg(bs::Image& img, std::string&& id) noexcept;

		const bs::vk::Texture& getTexture(short&& id);

		bs::vk::Texture& getTextureMutable(short&& id);

		size_t getNumTextures() noexcept;
		
		//List of textures for pushing data to the gpu
		const std::vector<bs::vk::texture_t>& getTextures();

		bs::vk::Model& getModel(std::string&& id);

		bs::vk::Buffer* getBuffer(std::string&& id);

		bs::Image& getImage(std::string&& id);

		
		//Store globally necessary pointers
		VkDescriptorSet*	pDescsetglobal;
		
		bool				loaded = false;
		bool				loadedarray[8] = { false, false, false, false, false, false, false, false };

	private:

		ResourceManager<bs::Image> m_images;
		ResourceManager<bs::Mesh> m_meshes;

		ResourceManager<bs::vk::Texture, short> m_textures;
		ResourceManager<bs::vk::Model> m_models;
		ResourceManager<bs::Material> m_materials;
		ResourceManager<std::shared_ptr<bs::vk::Buffer>> m_buffers;

	};

	extern AssetManager* asset_manager;
}