/*
* Vulkan Example - Geometry shader (vertex normal debugging)
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanExampleBase.h"


// Vertex layout for this example
std::vector<vkx::VertexLayout> vertexLayout =
{
    vkx::VertexLayout::VERTEX_LAYOUT_POSITION,
    vkx::VertexLayout::VERTEX_LAYOUT_NORMAL,
    vkx::VertexLayout::VERTEX_LAYOUT_COLOR,
    vkx::VertexLayout::VERTEX_LAYOUT_UV
};

class VulkanExample : public vkx::ExampleBase {
public:
    struct {
        vk::PipelineVertexInputStateCreateInfo inputState;
        std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
    } vertices;

    struct {
        vkx::MeshBuffer object;
    } meshes;

    struct {
        glm::mat4 projection;
        glm::mat4 model;
    } uboVS;

    struct {
        glm::mat4 projection;
        glm::mat4 model;
    } uboGS;

    struct {
        vkx::UniformData VS;
        vkx::UniformData GS;
    } uniformData;

    struct {
        vk::Pipeline solid;
        vk::Pipeline normals;
    } pipelines;

    vk::PipelineLayout pipelineLayout;
    vk::DescriptorSet descriptorSet;
    vk::DescriptorSetLayout descriptorSetLayout;

    VulkanExample() : vkx::ExampleBase(ENABLE_VALIDATION) {
        camera.setZoom(-8.0f);
        camera.setRotation({ 0.0f, -25.0f, 0.0f });
        title = "Vulkan Example - Geometry shader";
    }

    ~VulkanExample() {
        // Clean up used Vulkan resources 
        // Note : Inherited destructor cleans up resources stored in base class
        device.destroyPipeline(pipelines.solid);
        device.destroyPipeline(pipelines.normals);

        device.destroyPipelineLayout(pipelineLayout);
        device.destroyDescriptorSetLayout(descriptorSetLayout);

        meshes.object.destroy();

        uniformData.VS.destroy();
        uniformData.GS.destroy();
    }

    void updateDrawCommandBuffer(const vk::CommandBuffer& cmdBuffer) override {
        cmdBuffer.setViewport(0, vkx::viewport(size));
        cmdBuffer.setScissor(0, vkx::rect2D(size));
        cmdBuffer.setLineWidth(1.0f);
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);
        cmdBuffer.bindVertexBuffers(VERTEX_BUFFER_BIND_ID, meshes.object.vertices.buffer, { 0 });
        cmdBuffer.bindIndexBuffer(meshes.object.indices.buffer, 0, vk::IndexType::eUint32);
        // Solid shading
        cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.solid);
        cmdBuffer.drawIndexed(meshes.object.indexCount, 1, 0, 0, 0);
        // Normal debugging
        cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.normals);
        cmdBuffer.drawIndexed(meshes.object.indexCount, 1, 0, 0, 0);
    }

    void loadMeshes() {
        meshes.object = loadMesh(getAssetPath() + "models/suzanne.obj", vertexLayout, 0.25f);
    }

    void setupVertexDescriptions() {
        // Binding description
        vertices.bindingDescriptions.resize(1);
        vertices.bindingDescriptions[0] =
            vkx::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, vkx::vertexSize(vertexLayout), vk::VertexInputRate::eVertex);

        // Attribute descriptions
        // Describes memory layout and shader positions
        vertices.attributeDescriptions.resize(vertexLayout.size());

        // Location 0 : Position
        vertices.attributeDescriptions[0] =
            vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, vk::Format::eR32G32B32Sfloat, 0);

        // Location 1 : Normals
        vertices.attributeDescriptions[1] =
            vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, vk::Format::eR32G32B32Sfloat, sizeof(float) * 3);

        // Location 2 : Texture coordinates
        vertices.attributeDescriptions[2] =
            vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, vk::Format::eR32G32Sfloat, sizeof(float) * 6);

        // Location 3 : Texture coordinates
        vertices.attributeDescriptions[3] =
            vkx::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, vk::Format::eR32G32Sfloat, sizeof(float) * 8);

        // Assign to vertex buffer
        vertices.inputState = vk::PipelineVertexInputStateCreateInfo();
        vertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
        vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
        vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
        vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
    }

    void setupDescriptorPool() {
        // Example uses two ubos
        std::vector<vk::DescriptorPoolSize> poolSizes =
        {
            vkx::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 2),
        };

        vk::DescriptorPoolCreateInfo descriptorPoolInfo =
            vkx::descriptorPoolCreateInfo(poolSizes.size(), poolSizes.data(), 2);

        descriptorPool = device.createDescriptorPool(descriptorPoolInfo);
    }

    void setupDescriptorSetLayout() {
        std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings =
        {
            // Binding 0 : Vertex shader ubo
            vkx::descriptorSetLayoutBinding(
                vk::DescriptorType::eUniformBuffer,
                vk::ShaderStageFlagBits::eVertex,
                0),
            // Binding 1 : Geometry shader ubo
            vkx::descriptorSetLayoutBinding(
                vk::DescriptorType::eUniformBuffer,
                vk::ShaderStageFlagBits::eGeometry,
                1)
        };

        vk::DescriptorSetLayoutCreateInfo descriptorLayout =
            vkx::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), setLayoutBindings.size());

        descriptorSetLayout = device.createDescriptorSetLayout(descriptorLayout);


        vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
            vkx::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);

        pipelineLayout = device.createPipelineLayout(pPipelineLayoutCreateInfo);

    }

    void setupDescriptorSet() {
        vk::DescriptorSetAllocateInfo allocInfo =
            vkx::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
        descriptorSet = device.allocateDescriptorSets(allocInfo)[0];

        std::vector<vk::WriteDescriptorSet> writeDescriptorSets =
        {
            // Binding 0 : Vertex shader shader ubo
            vkx::writeDescriptorSet(
                descriptorSet,
                vk::DescriptorType::eUniformBuffer,
                0,
                &uniformData.VS.descriptor),
            // Binding 1 : Geometry shader ubo
            vkx::writeDescriptorSet(
                descriptorSet,
                vk::DescriptorType::eUniformBuffer,
                1,
                &uniformData.GS.descriptor)
        };

        device.updateDescriptorSets(writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
    }

    void preparePipelines() {
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState =
            vkx::pipelineInputAssemblyStateCreateInfo(vk::PrimitiveTopology::eTriangleList, vk::PipelineInputAssemblyStateCreateFlags(), VK_FALSE);

        vk::PipelineRasterizationStateCreateInfo rasterizationState =
            vkx::pipelineRasterizationStateCreateInfo(vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise);

        vk::PipelineColorBlendAttachmentState blendAttachmentState =
            vkx::pipelineColorBlendAttachmentState();

        vk::PipelineColorBlendStateCreateInfo colorBlendState =
            vkx::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

        vk::PipelineDepthStencilStateCreateInfo depthStencilState =
            vkx::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, vk::CompareOp::eLessOrEqual);

        vk::PipelineViewportStateCreateInfo viewportState =
            vkx::pipelineViewportStateCreateInfo(1, 1);

        vk::PipelineMultisampleStateCreateInfo multisampleState =
            vkx::pipelineMultisampleStateCreateInfo(vk::SampleCountFlagBits::e1);

        std::vector<vk::DynamicState> dynamicStateEnables = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
            vk::DynamicState::eLineWidth
        };
        vk::PipelineDynamicStateCreateInfo dynamicState =
            vkx::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size());


        // Tessellation pipeline
        // Load shaders
        std::array<vk::PipelineShaderStageCreateInfo, 3> shaderStages;

        shaderStages[0] = loadShader(getAssetPath() + "shaders/geometryshader/base.vert.spv", vk::ShaderStageFlagBits::eVertex);
        shaderStages[1] = loadShader(getAssetPath() + "shaders/geometryshader/base.frag.spv", vk::ShaderStageFlagBits::eFragment);
        shaderStages[2] = loadShader(getAssetPath() + "shaders/geometryshader/normaldebug.geom.spv", vk::ShaderStageFlagBits::eGeometry);

        vk::GraphicsPipelineCreateInfo pipelineCreateInfo =
            vkx::pipelineCreateInfo(pipelineLayout, renderPass);

        pipelineCreateInfo.pVertexInputState = &vertices.inputState;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineCreateInfo.pRasterizationState = &rasterizationState;
        pipelineCreateInfo.pColorBlendState = &colorBlendState;
        pipelineCreateInfo.pMultisampleState = &multisampleState;
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pDepthStencilState = &depthStencilState;
        pipelineCreateInfo.pDynamicState = &dynamicState;
        pipelineCreateInfo.stageCount = shaderStages.size();
        pipelineCreateInfo.pStages = shaderStages.data();
        pipelineCreateInfo.renderPass = renderPass;

        // Normal debugging pipeline
        pipelines.normals = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];


        // Solid rendering pipeline
        // Shader
        shaderStages[0] = loadShader(getAssetPath() + "shaders/geometryshader/mesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
        shaderStages[1] = loadShader(getAssetPath() + "shaders/geometryshader/mesh.frag.spv", vk::ShaderStageFlagBits::eFragment);
        pipelineCreateInfo.stageCount = 2;
        pipelines.solid = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];


    }

    // Prepare and initialize uniform buffer containing shader uniforms
    void prepareUniformBuffers() {
        // Vertex shader uniform buffer block
        uniformData.VS = createUniformBuffer(uboVS);
        // Geometry shader uniform buffer block
        uniformData.GS = createUniformBuffer(uboGS);
        updateUniformBuffers();
    }

    void updateUniformBuffers() {
        // Vertex shader
        uboVS.projection = getProjection();
        uboVS.model = camera.matrices.view;
        uniformData.VS.copy(uboVS);

        // Geometry shader
        uboGS.model = uboVS.model;
        uboGS.projection = uboVS.projection;
        uniformData.GS.copy(uboGS);
    }

    void prepare() {
        ExampleBase::prepare();
        loadMeshes();
        setupVertexDescriptions();
        prepareUniformBuffers();
        setupDescriptorSetLayout();
        preparePipelines();
        setupDescriptorPool();
        setupDescriptorSet();
        updateDrawCommandBuffers();
        prepared = true;
    }

    virtual void render() {
        if (!prepared)
            return;
        draw();
    }

    virtual void viewChanged() {
        updateUniformBuffers();
    }
};

RUN_EXAMPLE(VulkanExample)
