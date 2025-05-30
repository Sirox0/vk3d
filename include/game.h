#ifndef GAME_H
#define GAME_H

#include <vulkan/vulkan.h>
#include <cglm/cglm.h>

#include "numtypes.h"

#define FT_ASSERT(expression) \
    { \
        FT_Error error = expression; \
        if (error != 0) { \
            printf("freetype error: %s\n", FT_Error_String(error)); \
            exit(1); \
        } \
    }

typedef struct {
    VkImage image;
    VkImageView view;
    VkSampler sampler;
} texture_t;

typedef struct {
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout uboDescriptorSetLayout;

    VkBuffer cubeVertexBuffer;
    VkBuffer cubeIndexBuffer;
    VkDeviceSize cubeIndexBufferOffset;
    VkDeviceMemory cubeVertexIndexBuffersMemory;

    VkBuffer cubeUniformBuffer;
    VkDeviceMemory cubeUniformBufferMemory;
    void* cubeUniformBufferMemoryRaw;

    VkDescriptorSet cubeUboDescriptorSet;
    VkPipelineLayout cubePipelineLayout;
    VkPipeline cubePipeline;

    VkSemaphore swapchainReadySemaphore;
    VkSemaphore renderingDoneSemaphore;
    VkFence frameFence;

    VkRenderPass renderpass;
} game_globals_t;

#define MAX_STAR_SCALE 1.1f
#define LERP_STAR_INVERSE_SPEED 100.0f

void gameInit();
void gameRender();
void gameQuit();

extern game_globals_t gameglobals;

#endif