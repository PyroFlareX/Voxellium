#include "Buffer.h"

#include <stdexcept>
#include <memory>

namespace bs::vk
{
	Buffer::Buffer(const BufferDescription bufdesc) : m_device(bufdesc.dev), m_size(bufdesc.size),
		m_stride(bufdesc.stride), m_buffer_type(bufdesc.bufferType), m_buffer_alloc_prop(bufdesc.usage)
	{
		m_stride = std::max(m_stride, (u64)1);
		memcpy(&m_tptr, &bufdesc.bufferData, sizeof(m_tptr));

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = m_buffer_alloc_prop;

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = getSize();
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		// To get the buffer type
		if (m_buffer_type & bs::vk::BufferUsage::VERTEX_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}
		if (m_buffer_type & bs::vk::BufferUsage::INDEX_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}
		if (m_buffer_type & bs::vk::BufferUsage::UNIFORM_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}
		if (m_buffer_type & bs::vk::BufferUsage::STORAGE_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}
		if (m_buffer_type & bs::vk::BufferUsage::INDIRECT_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		}
		if (m_buffer_type & bs::vk::BufferUsage::TRANSFER_BUFFER)
		{
			if(m_buffer_alloc_prop == VMA_MEMORY_USAGE_GPU_ONLY)
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			}
			else if(m_buffer_alloc_prop == VMA_MEMORY_USAGE_CPU_ONLY)
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			}
			else
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			}
		}
		
		vmaCreateBuffer(m_device->getAllocator(), &bufferInfo, &allocInfo, &m_buffer, &m_allocation, nullptr);

		if((m_tptr != nullptr) && (m_buffer_alloc_prop != VMA_MEMORY_USAGE_GPU_ONLY))
		{
			writeBuffer(m_tptr);
			m_tptr = nullptr;
		}
	}

	Buffer::Buffer(bs::Device* dev, const BufferUsage buf, const u64 size, const void* data, VmaMemoryUsage usage)
		: m_device(dev), m_size(size), m_stride(1), m_buffer_type(buf), m_buffer_alloc_prop(usage)
	{
		memcpy(&m_tptr, &data, sizeof(m_tptr));

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = m_buffer_alloc_prop;

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = getSize();
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		// To get the buffer type
		if (m_buffer_type & bs::vk::BufferUsage::VERTEX_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}
		if (m_buffer_type & bs::vk::BufferUsage::INDEX_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}
		if (m_buffer_type & bs::vk::BufferUsage::UNIFORM_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}
		if (m_buffer_type & bs::vk::BufferUsage::STORAGE_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}
		if (m_buffer_type & bs::vk::BufferUsage::INDIRECT_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		}
		if (m_buffer_type & bs::vk::BufferUsage::TRANSFER_BUFFER)
		{
			if(m_buffer_alloc_prop == VMA_MEMORY_USAGE_GPU_ONLY)
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			}
			else if(m_buffer_alloc_prop == VMA_MEMORY_USAGE_CPU_ONLY)
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			}
			else
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			}
		}
		
		if(m_size == 0) { return; }
		vmaCreateBuffer(m_device->getAllocator(), &bufferInfo, &allocInfo, &m_buffer, &m_allocation, nullptr);

		if((m_tptr != nullptr) && (m_buffer_alloc_prop != VMA_MEMORY_USAGE_GPU_ONLY))
		{
			writeBuffer(m_tptr);
			m_tptr = nullptr;
		}
	}

	Buffer::Buffer(Buffer&& rhs) noexcept 
		: m_device(std::move(rhs.m_device)), m_size(std::move(rhs.m_size)), m_stride(std::move(rhs.m_stride)),
			m_buffer_type(std::move(rhs.m_buffer_type)), m_buffer_alloc_prop(std::move(rhs.m_buffer_alloc_prop))
	{
		assert(false); //Not yet implemented!

		std::swap(this->m_buffer, rhs.m_buffer);
		std::swap(this->m_allocation, rhs.m_allocation);

		this->m_device;
		this->m_size;
		this->m_stride;
		this->m_buffer_type;
		this->m_buffer_alloc_prop;
	}

	Buffer& Buffer::operator=(Buffer&& rhs) noexcept
	{
		assert(false); //Not yet implemented!

		std::swap(this->m_buffer, rhs.m_buffer);
		std::swap(this->m_allocation, rhs.m_allocation);

		m_device = std::move(rhs.m_device);
		m_size = std::move(rhs.m_size);
		m_stride = std::move(rhs.m_stride);
		m_buffer_type = std::move(rhs.m_buffer_type);
		m_buffer_alloc_prop = std::move(rhs.m_buffer_alloc_prop);

		return *this;
	}

	Buffer::~Buffer() noexcept
	{
		deleteBuffer();
	}

	u64 Buffer::getStride() const
	{
		return m_stride;
	}

	u64 Buffer::getSize() const
	{
		return m_size;
	}

	u64 Buffer::getNumElements() const
	{
		return m_size / m_stride;
	}

	void Buffer::setMaxElements(u64 numElements)
	{
		setAllocationSize(m_stride * numElements);
	}

	void Buffer::setAllocationSize(const u64 numBytes)
	{
		m_size = numBytes;
	}
	
	void Buffer::allocateBuffer()
	{
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = m_buffer_alloc_prop;

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = getSize();
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		// To get the buffer type
		if (m_buffer_type & bs::vk::BufferUsage::VERTEX_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}
		if (m_buffer_type & bs::vk::BufferUsage::INDEX_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}
		if (m_buffer_type & bs::vk::BufferUsage::UNIFORM_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}
		if (m_buffer_type & bs::vk::BufferUsage::STORAGE_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}
		if (m_buffer_type & bs::vk::BufferUsage::INDIRECT_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		}
		if (m_buffer_type & bs::vk::BufferUsage::TRANSFER_BUFFER)
		{
			if(m_buffer_alloc_prop == VMA_MEMORY_USAGE_GPU_ONLY)
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			}
			else if(m_buffer_alloc_prop == VMA_MEMORY_USAGE_CPU_ONLY)
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			}
			else
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			}
		}
		
		deleteBuffer();
		vmaCreateBuffer(m_device->getAllocator(), &bufferInfo, &allocInfo, &m_buffer, &m_allocation, nullptr);

		if(m_tptr != nullptr && m_buffer_alloc_prop != VMA_MEMORY_USAGE_GPU_ONLY)
		{
			writeBuffer(m_tptr);
			m_tptr = nullptr;
		}
	}

	void Buffer::writeBuffer(const void* data, u64 size, u64 offset)
	{
		if(size == 0)
		{
			size = getSize();
		}

		//Maps GPU memory to CPU visible address
		void* bufferdata = nullptr;
		vmaMapMemory(m_device->getAllocator(), m_allocation, &bufferdata);
		//Add the offset
		bufferdata = (u8*)bufferdata + offset;
		memcpy(bufferdata, data, size);

		//Unmaps the buffer
		vmaUnmapMemory(m_device->getAllocator(), m_allocation);
	}

	void Buffer::setAPIResource(VkBuffer& buffer)
	{	//This is PROBABLY a bad idea to keep this function around, bc the device
		//	and allcoation aren't changed, leading to double frees or leaks
		deleteBuffer();
		m_buffer = buffer;
	}

	VkBuffer& Buffer::getAPIResource()
	{
		return m_buffer;
	}

	VmaAllocation& Buffer::getAllocation()
	{
		return m_allocation;
	}

	void Buffer::deleteBuffer()
	{
		vmaDestroyBuffer(m_device->getAllocator(), m_buffer, m_allocation);
	}
}