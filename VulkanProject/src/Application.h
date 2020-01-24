#pragma once
#include "Vulkan/VulkanContext.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

struct Vertex
{
    glm::vec2 Position;
    glm::vec3 Color;

    static VkVertexInputBindingDescription GetBindingDescription() 
    {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding   = 0;
        bindingDescription.stride    = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static VkVertexInputAttributeDescription* GetAttributeDescriptions() 
    {
        static VkVertexInputAttributeDescription attributeDescriptions[2];

        attributeDescriptions[0].binding    = 0;
        attributeDescriptions[0].location   = 0;
        attributeDescriptions[0].format     = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset     = offsetof(Vertex, Position);

        attributeDescriptions[1].binding    = 0;
        attributeDescriptions[1].location   = 1;
        attributeDescriptions[1].format     = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset     = offsetof(Vertex, Color);

        return attributeDescriptions;
    }
};

class Application
{
public:
    DECL_NO_COPY(Application);

    bool IsRunning() { return m_bIsRunning; } 
    void Init();
    void Run();
    void Release();
    
    VulkanContext* GetVulkanContext() const { return m_pContext; }

    static Application* Create();
    static Application& Get() { return *s_pInstance; }
private:
    Application();
    ~Application();

    void CreateWindow();
    void CreateFramebuffers();

    void ReleaseFramebuffers();

    void OnWindowResize(uint32 width, uint32 height);
    void OnWindowClose();
private:
    GLFWwindow* m_pWindow;
    VulkanContext* m_pContext;
    VulkanRenderPass* m_pRenderPass;
    VulkanGraphicsPipelineState* m_PipelineState;
    VulkanCommandBuffer* m_pCurrentCommandBuffer;
    VulkanBuffer* m_pVertexBuffer;
    VulkanBuffer* m_pIndexBuffer;
    VulkanDeviceAllocator* m_pDeviceAllocator;
    std::vector<VulkanFramebuffer*> m_Framebuffers;
    std::vector<VulkanCommandBuffer*> m_CommandBuffers;
    uint32 m_Width;
    uint32 m_Height;
    bool m_bIsRunning;

    static Application* s_pInstance;
};
