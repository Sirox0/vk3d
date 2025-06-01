#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <cglm/cglm.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "numtypes.h"
#include "vkFunctions.h"
#include "vkInit.h"
#include "pipeline.h"
#include "game.h"
#include "util.h"
#include "garbage.h"
#include "config.h"
#include "mathext.h"

game_globals_t gameglobals = {};

#define GARBAGE_CMD_BUFFER_NUM 1
#define GARBAGE_BUFFER_NUM 1
#define GARBAGE_BUFFER_MEMORY_NUM 1
#define GARBAGE_FENCE_NUM 1

void gameInit() {
    VkCommandBuffer garbageCmdBuffers[GARBAGE_CMD_BUFFER_NUM];
    VkBuffer garbageBuffers[GARBAGE_BUFFER_NUM];
    VkDeviceMemory garbageBuffersMem[GARBAGE_BUFFER_MEMORY_NUM];
    VkFence garbageFences[GARBAGE_FENCE_NUM];

    garbageCreate(GARBAGE_CMD_BUFFER_NUM, garbageCmdBuffers, GARBAGE_FENCE_NUM, garbageFences);

    {
        vertex_input_cube_t vertexbuf[] = {
            {{-0.1f, 0.1f, 0.1f}, {1.0f, 0.0f, 0.0f}},
            {{-0.1f, 0.1f, -0.1f}, {0.0f, 1.0f, 0.0f}},
            {{0.1f, 0.1f, -0.1f}, {0.0f, 0.0f, 1.0f}},
            {{0.1f, 0.1f, 0.1f}, {1.0f, 1.0f, 0.0f}},
            {{-0.1f, -0.1f, 0.1f}, {0.0f, 1.0f, 1.0f}},
            {{-0.1f, -0.1f, -0.1f}, {1.0f, 0.0f, 1.0f}},
            {{0.1f, -0.1f, -0.1f}, {1.0f, 1.0f, 1.0f}},
            {{0.1f, -0.1f, 0.1f}, {0.5f, 0.5f, 0.5f}},
        };

        u16 indexbuf[] = {
            1, 0, 2,
            2, 0, 3,

            5, 6, 4,
            6, 7, 4,

            4, 0, 5,
            5, 0, 1,

            7, 6, 3,
            6, 2, 3,

            6, 5, 2,
            5, 1, 2,

            4, 7, 3,
            4, 3, 0
        };

        createBuffer(&gameglobals.cubeVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(vertexbuf));
        createBuffer(&gameglobals.cubeIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(indexbuf));

        VkMemoryRequirements vertexBufferMemReq;
        vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.cubeVertexBuffer, &vertexBufferMemReq);
        VkMemoryRequirements indexBufferMemReq;
        vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.cubeIndexBuffer, &indexBufferMemReq);

        u32 alignCoefficient = getAlignCooficient(vertexBufferMemReq.size, indexBufferMemReq.alignment);
        gameglobals.cubeIndexBufferOffset = vertexBufferMemReq.size + alignCoefficient;

        allocateMemory(&gameglobals.cubeVertexIndexBuffersMemory, vertexBufferMemReq.size + alignCoefficient + indexBufferMemReq.size, getMemoryTypeIndex(vertexBufferMemReq.memoryTypeBits & indexBufferMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

        VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.cubeVertexBuffer, gameglobals.cubeVertexIndexBuffersMemory, 0), "failed to bind buffer memory\n");
        VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.cubeIndexBuffer, gameglobals.cubeVertexIndexBuffersMemory, gameglobals.cubeIndexBufferOffset), "failed to bind buffer memory\n");

        {
            createBuffer(&garbageBuffers[0], VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sizeof(vertexbuf) + sizeof(indexbuf));

            VkMemoryRequirements garbageBufferMemReq;
            vkGetBufferMemoryRequirements(vkglobals.device, garbageBuffers[0], &garbageBufferMemReq);

            allocateMemory(&garbageBuffersMem[0], garbageBufferMemReq.size, getMemoryTypeIndex(garbageBufferMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));

            VK_ASSERT(vkBindBufferMemory(vkglobals.device, garbageBuffers[0], garbageBuffersMem[0], 0), "failed to bind buffer memory\n");

            void* garbageBuffer0MemRaw;
            VK_ASSERT(vkMapMemory(vkglobals.device, garbageBuffersMem[0], 0, VK_WHOLE_SIZE, 0, &garbageBuffer0MemRaw), "failed to map memory\n");

            memcpy(garbageBuffer0MemRaw, vertexbuf, sizeof(vertexbuf));
            memcpy(garbageBuffer0MemRaw + sizeof(vertexbuf), indexbuf, sizeof(indexbuf));

            VkMappedMemoryRange memoryRange = {};
            memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memoryRange.offset = 0;
            memoryRange.size = VK_WHOLE_SIZE;
            memoryRange.memory = garbageBuffersMem[0];

            VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 1, &memoryRange), "failed to flush device memory\n");
            
            vkUnmapMemory(vkglobals.device, garbageBuffersMem[0]);

            VkBufferCopy copyInfo[2] = {};
            copyInfo[0].srcOffset = 0;
            copyInfo[0].dstOffset = 0;
            copyInfo[0].size = sizeof(vertexbuf);
            copyInfo[1].srcOffset = sizeof(vertexbuf);
            copyInfo[1].dstOffset = 0;
            copyInfo[1].size = sizeof(indexbuf);

            vkCmdCopyBuffer(garbageCmdBuffers[0], garbageBuffers[0], gameglobals.cubeVertexBuffer, 1, &copyInfo[0]);
            vkCmdCopyBuffer(garbageCmdBuffers[0], garbageBuffers[0], gameglobals.cubeIndexBuffer, 1, &copyInfo[1]);
        }
    }

    {
        VK_ASSERT(vkEndCommandBuffer(garbageCmdBuffers[0]), "failed to end command buffer\n");

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &garbageCmdBuffers[0];

        VK_ASSERT(vkQueueSubmit(vkglobals.queue, 1, &submitInfo, garbageFences[0]), "failed to submit command buffer\n");
    }

    {
        createBuffer(&gameglobals.cubeUniformBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(mat4) * 3);

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.cubeUniformBuffer, &memReq);

        allocateMemory(&gameglobals.cubeUniformBufferMemory, memReq.size, getMemoryTypeIndex(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));

        VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.cubeUniformBuffer, gameglobals.cubeUniformBufferMemory, 0), "failed to bind buffer memory\n");

        VK_ASSERT(vkMapMemory(vkglobals.device, gameglobals.cubeUniformBufferMemory, 0, VK_WHOLE_SIZE, 0, &gameglobals.cubeUniformBufferMemoryRaw), "failed to map memory\n");
    }

    {
        VkAttachmentDescription colorAttachmentDesc = {};
        colorAttachmentDesc.format = vkglobals.surfaceFormat.format;
        colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.inputAttachmentCount = 0;
        subpass.preserveAttachmentCount = 0;
        subpass.pDepthStencilAttachment = NULL;
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        VkRenderPassCreateInfo renderpassInfo = {};
        renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderpassInfo.dependencyCount = 0;
        renderpassInfo.pDependencies = NULL;
        renderpassInfo.attachmentCount = 1;
        renderpassInfo.pAttachments = &colorAttachmentDesc;
        renderpassInfo.subpassCount = 1;
        renderpassInfo.pSubpasses = &subpass;

        VK_ASSERT(vkCreateRenderPass(vkglobals.device, &renderpassInfo, NULL, &gameglobals.renderpass), "failed to create renderpass\n");
    }

    for (u32 i = 0; i < vkglobals.swapchainImageCount; i++) {
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = gameglobals.renderpass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &vkglobals.swapchainImageViews[i];
        framebufferInfo.width = vkglobals.swapchainExtent.width;
        framebufferInfo.height = vkglobals.swapchainExtent.height;
        framebufferInfo.layers = 1;

        VK_ASSERT(vkCreateFramebuffer(vkglobals.device, &framebufferInfo, NULL, &vkglobals.swapchainFramebuffers[i]), "failed to create framebuffer\n");
    }

    {
        VkDescriptorPoolSize uboPoolSize = {};
        uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboPoolSize.descriptorCount = 2;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &uboPoolSize;
        poolInfo.maxSets = 1;

        VK_ASSERT(vkCreateDescriptorPool(vkglobals.device, &poolInfo, NULL, &gameglobals.descriptorPool), "failed to create descriptor pool\n");
    }

    {
        VkDescriptorSetLayoutBinding uboBinding = {};
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboBinding.binding = 0;
        uboBinding.descriptorCount = 1;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = 1;
        descriptorSetLayoutInfo.pBindings = &uboBinding;

        VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &descriptorSetLayoutInfo, NULL, &gameglobals.uboDescriptorSetLayout), "failed to create descriptor set layout\n");
    }

    {
        VkDescriptorSetAllocateInfo descriptorSetsInfo = {};
        descriptorSetsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetsInfo.descriptorPool = gameglobals.descriptorPool;
        descriptorSetsInfo.descriptorSetCount = 1;
        descriptorSetsInfo.pSetLayouts = (VkDescriptorSetLayout[]){gameglobals.uboDescriptorSetLayout};

        VK_ASSERT(vkAllocateDescriptorSets(vkglobals.device, &descriptorSetsInfo, &gameglobals.cubeUboDescriptorSet), "failed to allocate descriptor sets\n");

        VkDescriptorBufferInfo descriptorBufferInfo = {};
        descriptorBufferInfo.buffer = gameglobals.cubeUniformBuffer;
        descriptorBufferInfo.offset = 0;
        descriptorBufferInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet starTextureDescriptorWrite = {};
        starTextureDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        starTextureDescriptorWrite.descriptorCount = 1;
        starTextureDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        starTextureDescriptorWrite.dstBinding = 0;
        starTextureDescriptorWrite.dstArrayElement = 0;
        starTextureDescriptorWrite.dstSet = gameglobals.cubeUboDescriptorSet;
        starTextureDescriptorWrite.pBufferInfo = &descriptorBufferInfo;

        vkUpdateDescriptorSets(vkglobals.device, 1, &starTextureDescriptorWrite, 0, NULL);
    }

    {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &gameglobals.uboDescriptorSetLayout;

        VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, NULL, &gameglobals.cubePipelineLayout), "failed to create pipeline layout\n");

        graphics_pipeline_info_t pipelineInfo = {};
        pipelineFillDefaultGraphicsPipeline(&pipelineInfo);
        pipelineInfo.stageCount = 2;
        pipelineInfo.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfo.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfo.stages[0].module = createShaderModuleFromFile("assets/shaders/cube.vert.spv");
        pipelineInfo.stages[1].module = createShaderModuleFromFile("assets/shaders/cube.frag.spv");
        pipelineInfo.layout = gameglobals.cubePipelineLayout;
        pipelineInfo.renderpass = gameglobals.renderpass;
        pipelineInfo.subpass = 0;

        VkVertexInputBindingDescription bindingDesc = {};
        bindingDesc.binding = 0;
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDesc.stride = sizeof(vertex_input_cube_t);

        VkVertexInputAttributeDescription attributeDescs[2] = {};
        attributeDescs[0].binding = 0;
        attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[0].location = 0;
        attributeDescs[0].offset = offsetof(vertex_input_cube_t, pos);
        attributeDescs[1].binding = 0;
        attributeDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[1].location = 1;
        attributeDescs[1].offset = offsetof(vertex_input_cube_t, color);

        pipelineInfo.vertexInputState.vertexAttributeDescriptionCount = 2;
        pipelineInfo.vertexInputState.pVertexAttributeDescriptions = attributeDescs;
        pipelineInfo.vertexInputState.vertexBindingDescriptionCount = 1;
        pipelineInfo.vertexInputState.pVertexBindingDescriptions = &bindingDesc;

        pipelineCreateGraphicsPipelines(NULL, 1, &pipelineInfo, &gameglobals.cubePipeline);

        vkDestroyShaderModule(vkglobals.device, pipelineInfo.stages[0].module, NULL);
        vkDestroyShaderModule(vkglobals.device, pipelineInfo.stages[1].module, NULL);
    }

    {
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VK_ASSERT(vkCreateSemaphore(vkglobals.device, &semaphoreInfo, NULL, &gameglobals.swapchainReadySemaphore), "failed to create semaphore\n");
        VK_ASSERT(vkCreateSemaphore(vkglobals.device, &semaphoreInfo, NULL, &gameglobals.renderingDoneSemaphore), "failed to create semaphore\n");

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VK_ASSERT(vkCreateFence(vkglobals.device, &fenceInfo, NULL, &gameglobals.frameFence), "failed to create fence\n");
    }

    glm_mat4_identity(gameglobals.cubeUniformBufferMemoryRaw);

    glm_perspective(glm_rad(45.0f), (f32)vkglobals.swapchainExtent.width / vkglobals.swapchainExtent.height, 0.0f, 1.0f, gameglobals.cubeUniformBufferMemoryRaw + sizeof(mat4) * 2);
    (*((mat4*)(gameglobals.cubeUniformBufferMemoryRaw + sizeof(mat4) * 2)))[1][1] *= -1;

    {
        VkMappedMemoryRange memoryRange = {};
        memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memoryRange.offset = 0;
        memoryRange.size = VK_WHOLE_SIZE;
        memoryRange.memory = gameglobals.cubeUniformBufferMemory;

        VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 1, &memoryRange), "failed to flush device memory\n");
    }

    gameglobals.loopActive = 1;
    gameglobals.deltaTime = 0;

    garbageWaitAndDestroy(GARBAGE_CMD_BUFFER_NUM, garbageCmdBuffers, GARBAGE_BUFFER_NUM, garbageBuffers, GARBAGE_BUFFER_MEMORY_NUM, garbageBuffersMem, GARBAGE_FENCE_NUM, garbageFences);
}

void gameEvent(SDL_Event* e) {
    if (e->type == SDL_EVENT_KEY_DOWN) {
        if (e->key.key == SDLK_W) gameglobals.cam.velocity[2] = 1.0f / 2000.0f;
        else if (e->key.key == SDLK_S) gameglobals.cam.velocity[2] = -1.0f / 2000.0f;
        else if (e->key.key == SDLK_A) gameglobals.cam.velocity[0] = -1.0f / 2000.0f;
        else if (e->key.key == SDLK_D) gameglobals.cam.velocity[0] = 1.0f / 2000.0f;
        else if (e->key.key == SDLK_SPACE) gameglobals.cam.velocity[1] = 1.0f / 2000.0f;
        else if (e->key.key == SDLK_LCTRL) gameglobals.cam.velocity[1] = -1.0f / 2000.0f;
    } else if (e->type == SDL_EVENT_KEY_UP) {
        if (e->key.key == SDLK_W) gameglobals.cam.velocity[2] = 0;
        else if (e->key.key == SDLK_S) gameglobals.cam.velocity[2] = 0;
        else if (e->key.key == SDLK_A) gameglobals.cam.velocity[0] = 0;
        else if (e->key.key == SDLK_D) gameglobals.cam.velocity[0] = 0;
        else if (e->key.key == SDLK_SPACE) gameglobals.cam.velocity[1] = 0;
        else if (e->key.key == SDLK_LCTRL) gameglobals.cam.velocity[1] = 0;
    } else if (e->type == SDL_EVENT_MOUSE_MOTION) {
        gameglobals.cam.yaw += (f32)e->motion.xrel / 400.0f;
        gameglobals.cam.pitch -= (f32)e->motion.yrel / 400.0f;
    }
}

void updateCubeUbo() {
    versor y, p;
    glm_quatv(y, gameglobals.cam.yaw, (vec3){0.0f, -1.0f, 0.0f});
    glm_quatv(p, gameglobals.cam.pitch, (vec3){1.0f, 0.0f, 0.0f});
    glm_quat_mul(y, p, y);
    
    {
        mat4 rot;
        glm_quat_mat4(y, rot);
        vec3 vel;
        glm_mat4_mulv3(rot, (vec3){gameglobals.cam.velocity[0] * gameglobals.deltaTime, gameglobals.cam.velocity[1] * gameglobals.deltaTime, gameglobals.cam.velocity[2] * gameglobals.deltaTime}, 0.0f, vel);
        glm_vec3_add(gameglobals.cam.position, vel, gameglobals.cam.position);
    }

    {
        glm_quat_look(gameglobals.cam.position, y, gameglobals.cubeUniformBufferMemoryRaw + sizeof(mat4));
        glm_translate(gameglobals.cubeUniformBufferMemoryRaw + sizeof(mat4), (vec3){-gameglobals.cam.position[0], -gameglobals.cam.position[1], -gameglobals.cam.position[2]});
    }

    glm_rotate(gameglobals.cubeUniformBufferMemoryRaw, glm_rad(90.0f) * gameglobals.deltaTime / 1000.0f, (vec3){0.0f, -1.0f, 0.0f});

    VkDeviceSize alignedSize = sizeof(mat4) * 2 + getAlignCooficient(sizeof(mat4) * 2, vkglobals.deviceProperties.limits.nonCoherentAtomSize);
    VkMappedMemoryRange memoryRange = {};
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.offset = 0;
    memoryRange.size = alignedSize > (sizeof(mat4) * 3) ? VK_WHOLE_SIZE : alignedSize;
    memoryRange.memory = gameglobals.cubeUniformBufferMemory;

    VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 1, &memoryRange), "failed to flush device memory\n");
}

void gameRender() {
    updateCubeUbo();

    VK_ASSERT(vkWaitForFences(vkglobals.device, 1, &gameglobals.frameFence, VK_FALSE, 0xFFFFFFFFFFFFFFFF), "failed to wait for fences\n");
    VK_ASSERT(vkResetFences(vkglobals.device, 1, &gameglobals.frameFence), "failed to reset fences\n");

    u32 imageIndex;
    VK_ASSERT(vkAcquireNextImageKHR(vkglobals.device, vkglobals.swapchain, 0xFFFFFFFFFFFFFFFF, gameglobals.swapchainReadySemaphore, NULL, &imageIndex), "failed tp acquire swapchain image\n");

    {
        VkCommandBufferBeginInfo cmdBeginInfo = {};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_ASSERT(vkBeginCommandBuffer(vkglobals.cmdBuffer, &cmdBeginInfo), "failed to begin command buffer\n");

        VkClearValue clearValue = {};
        clearValue.color = (VkClearColorValue){{0.0f, 0.0f, 0.0f, 0.0f}};

        VkRenderPassBeginInfo renderpassBeginInfo = {};
        renderpassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderpassBeginInfo.renderPass = gameglobals.renderpass;
        renderpassBeginInfo.renderArea = (VkRect2D){(VkOffset2D){0, 0}, vkglobals.swapchainExtent};
        renderpassBeginInfo.framebuffer = vkglobals.swapchainFramebuffers[imageIndex];
        renderpassBeginInfo.clearValueCount = 1;
        renderpassBeginInfo.pClearValues = &clearValue;

        vkCmdBeginRenderPass(vkglobals.cmdBuffer, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkDeviceSize vertexBufferOffsets[1] = {};
        vkCmdBindPipeline(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.cubePipeline);
        vkCmdBindVertexBuffers(vkglobals.cmdBuffer, 0, 1, &gameglobals.cubeVertexBuffer, vertexBufferOffsets);
        vkCmdBindIndexBuffer(vkglobals.cmdBuffer, gameglobals.cubeIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.cubePipelineLayout, 0, 1, &gameglobals.cubeUboDescriptorSet, 0, NULL);

        vkCmdDrawIndexed(vkglobals.cmdBuffer, 36, 1, 0, 0, 0);

        vkCmdEndRenderPass(vkglobals.cmdBuffer);

        VK_ASSERT(vkEndCommandBuffer(vkglobals.cmdBuffer), "failed to end command buffer\n");
    }

    VkPipelineStageFlags semaphoreSignalStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &gameglobals.swapchainReadySemaphore;
    submitInfo.pWaitDstStageMask = &semaphoreSignalStage;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &gameglobals.renderingDoneSemaphore;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkglobals.cmdBuffer;

    VK_ASSERT(vkQueueSubmit(vkglobals.queue, 1, &submitInfo, gameglobals.frameFence), "failed to submit command buffer\n");

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vkglobals.swapchain;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &gameglobals.renderingDoneSemaphore;
    presentInfo.pImageIndices = &imageIndex;

    VK_ASSERT(vkQueuePresentKHR(vkglobals.queue, &presentInfo), "failed to present swapchain image\n");
}

void gameQuit() {
    vkDestroyFence(vkglobals.device, gameglobals.frameFence, NULL);
    vkDestroySemaphore(vkglobals.device, gameglobals.renderingDoneSemaphore, NULL);
    vkDestroySemaphore(vkglobals.device, gameglobals.swapchainReadySemaphore, NULL);
    vkDestroyPipeline(vkglobals.device, gameglobals.cubePipeline, NULL);
    vkDestroyPipelineLayout(vkglobals.device, gameglobals.cubePipelineLayout, NULL);
    vkUnmapMemory(vkglobals.device, gameglobals.cubeUniformBufferMemory);
    vkDestroyBuffer(vkglobals.device, gameglobals.cubeUniformBuffer, NULL);
    vkFreeMemory(vkglobals.device, gameglobals.cubeUniformBufferMemory, NULL);
    vkDestroyBuffer(vkglobals.device, gameglobals.cubeIndexBuffer, NULL);
    vkDestroyBuffer(vkglobals.device, gameglobals.cubeVertexBuffer, NULL);
    vkFreeMemory(vkglobals.device, gameglobals.cubeVertexIndexBuffersMemory, NULL);
    vkDestroyDescriptorSetLayout(vkglobals.device, gameglobals.uboDescriptorSetLayout, NULL);
    vkDestroyDescriptorPool(vkglobals.device, gameglobals.descriptorPool, NULL);
    for (u32 i = 0; i < vkglobals.swapchainImageCount; i++) {
        vkDestroyFramebuffer(vkglobals.device, vkglobals.swapchainFramebuffers[i], NULL);
    }
    vkDestroyRenderPass(vkglobals.device, gameglobals.renderpass, NULL);
}