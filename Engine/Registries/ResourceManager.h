#pragma once

#include <unordered_map>
#include <map>
#include <string>
#include <mutex>
#include <iostream>

namespace bs
{
	template <typename T, typename keytype = std::string>
	class ResourceManager
	{
	public:
		ResourceManager() = default;
		~ResourceManager() = default;

		void addAsset(T& a, keytype& key) noexcept
		{
			std::lock_guard<std::mutex> mutguard(m_lock);
			m_assetMap.emplace(key, a);
		}

		const bool doesAssetExist(keytype&& key) const noexcept
		{
			return m_assetMap.contains(key);
		}

		T& getAsset(keytype&& key)
		{
			std::lock_guard<std::mutex> mutguard(m_lock);
			if(doesAssetExist(std::forward<keytype>(key)))
			{
				return m_assetMap.at(key);
			}
			else
			{
				// error!!!
				puts("Tried to retrieve an asset that does not exist!\n");
				
				std::cout << "Type of asset is: " << typeid(T).name() << " | and the value of the key is: " << key << "\n";
				throw;
			}   
		}

		const T& getAsset(keytype&& key) const
		{
			//std::lock_guard<std::mutex> mutguard(m_lock);
			if(doesAssetExist(std::forward<keytype>(key)))
			{
				return m_assetMap.at(key);
			}
			else
			{
				// error!!!
				puts("Tried to retrieve an asset that does not exist!\n");
				
				std::cout << "Type of asset is: " << typeid(T).name() << " | and the value of the key is: " << key << "\n";
				throw;
			}   
		}

		void removeAsset(keytype&& key)
		{
			std::lock_guard<std::mutex> mutguard(m_lock);
			m_assetMap.erase(key);
		}

		std::unordered_map<keytype, T>& getMap() noexcept
		{
			return m_assetMap;
		}

		const std::unordered_map<keytype, T>& getMap() const noexcept
		{
			return m_assetMap;
		}

	private:
		std::unordered_map<keytype, T> m_assetMap;
		std::mutex m_lock;
	};

	template <typename T, typename keytype = short>
	class ResourceMap
	{
	public:
		ResourceMap() = default;
		~ResourceMap() = default;

		void addAsset(T& a, keytype& key) noexcept
		{
			std::lock_guard<std::mutex> mutguard(m_lock);
			m_assetMap.emplace(key, a);
		}

		const bool doesAssetExist(keytype&& key) const noexcept
		{
			return m_assetMap.contains(key);
		}

		T& getAsset(keytype&& key)
		{
			std::lock_guard<std::mutex> mutguard(m_lock);
			if(doesAssetExist(std::forward<keytype>(key)))
			{
				return m_assetMap.at(key);
			}
			else
			{
				// error!!!
				puts("Tried to retrieve an asset that does not exist!\n");
				
				std::cout << "Type of asset is: " << typeid(T).name() << " | and the value of the key is: " << key << "\n";
				throw;
			}   
		}

		const T& getAsset(keytype&& key) const
		{
			//std::lock_guard<std::mutex> mutguard(m_lock);
			if(doesAssetExist(std::forward<keytype>(key)))
			{
				return m_assetMap.at(key);
			}
			else
			{
				// error!!!
				puts("Tried to retrieve an asset that does not exist!\n");
				
				std::cout << "Type of asset is: " << typeid(T).name() << " | and the value of the key is: " << key << "\n";
				throw;
			}   
		}

		void removeAsset(keytype&& key)
		{
			std::lock_guard<std::mutex> mutguard(m_lock);
			m_assetMap.erase(key);
		}

		std::map<keytype, T>& getMap() noexcept
		{
			return m_assetMap;
		}

		const std::map<keytype, T>& getMap() const noexcept
		{
			return m_assetMap;
		}

	private:
		std::map<keytype, T> m_assetMap;
		std::mutex m_lock;
	};
}