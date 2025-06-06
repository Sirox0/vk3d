#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Math/Vec3.h>
#include <cglm/cglm.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "numtypes.h"
#include "game.h"


JPH_SUPPRESS_WARNING_PUSH


#define JPH_SHAPE_ASSERT(expression, var) \
    JPH::ShapeRefC var; \
    { \
        JPH::ShapeSettings::ShapeResult res = expression; \
        if (res.HasError()) { \
            printf("jolt shape creation error: %s\n", res.GetError().c_str()); \
            exit(1); \
        } \
        var = res.Get(); \
    }

inline const JPH::ObjectLayer LAYER_STATIC = 0;
inline const JPH::ObjectLayer LAYER_DYNAMIC = 1;

inline const JPH::BroadPhaseLayer BP_LAYER_STATIC = JPH::BroadPhaseLayer(0);
inline const JPH::BroadPhaseLayer BP_LAYER_DYNAMIC = JPH::BroadPhaseLayer(1);

// hope that those classes will get inlined

class ObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter {
public:
	constexpr inline bool ShouldCollide(JPH::ObjectLayer layer1, JPH::ObjectLayer layer2) const override final {
		switch (layer1) {
            case LAYER_STATIC:
                return layer2 == LAYER_DYNAMIC;
            case LAYER_DYNAMIC:
                return true;
            default:
                return false;
		}
	}
};

class BroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface {
public:
	constexpr inline u32 GetNumBroadPhaseLayers() const override final {
		return 2;
	}

	constexpr inline JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override final {
		switch (layer) {
            case LAYER_STATIC:
                return BP_LAYER_STATIC;
            case LAYER_DYNAMIC:
                return BP_LAYER_DYNAMIC;
            default:
                return JPH::BroadPhaseLayer(-1);
        }
	}
};

class ObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
	constexpr inline bool ShouldCollide(JPH::ObjectLayer layer1, JPH::BroadPhaseLayer layer2) const override final {
		switch (layer1) {
            case LAYER_STATIC:
                return layer2 == BP_LAYER_DYNAMIC;
            case LAYER_DYNAMIC:
                return true;
            default:
                return false;
		}
	}
};

typedef struct {
    JPH::Factory factory;
    JPH::TempAllocatorImpl* tempAllocator;
    JPH::JobSystemThreadPool* jobSystem;

    ObjectLayerPairFilter ovoInterface;
    BroadPhaseLayerInterface bplInterface;
    ObjectVsBroadPhaseLayerFilter ovbInterface;

    JPH::PhysicsSystem physicsSystem;
    JPH::BodyInterface* bodyInterface;

    JPH::Body* floor;
    JPH::Body* cube;
} jolt_globals_t;

jolt_globals_t joltglobals;

extern "C" void joltInit() {
    JPH::RegisterDefaultAllocator();

    joltglobals.factory = JPH::Factory();
    JPH::Factory::sInstance = &joltglobals.factory;
    JPH::RegisterTypes();

    joltglobals.tempAllocator = new JPH::TempAllocatorImpl(10 * 256 * 1024);
    joltglobals.jobSystem = new JPH::JobSystemThreadPool(256, 8, 8);

    joltglobals.physicsSystem.Init(256, 0, 1024, 1024, joltglobals.bplInterface, joltglobals.ovbInterface, joltglobals.ovoInterface);

    joltglobals.bodyInterface = &joltglobals.physicsSystem.GetBodyInterface();

    JPH::BoxShapeSettings boxShapeSettings = {};
    boxShapeSettings.mUserData = 0;
    boxShapeSettings.mHalfExtent = JPH::Vec3(100.0f, 1.0f, 100.0f);
    boxShapeSettings.mDensity = 500.0f;

    JPH_SHAPE_ASSERT(boxShapeSettings.Create(), floorShape);

    JPH::BodyCreationSettings boxSettings = {};
    boxSettings.mPosition = JPH::Vec3(0.0f, 3.0f, 0.0f);
    boxSettings.mMotionType = JPH::EMotionType::Static;
    boxSettings.mObjectLayer = LAYER_STATIC;
    boxSettings.SetShape(floorShape);

    joltglobals.floor = joltglobals.bodyInterface->CreateBody(boxSettings);

    joltglobals.bodyInterface->AddBody(joltglobals.floor->GetID(), JPH::EActivation::DontActivate);

    boxShapeSettings.mHalfExtent = JPH::Vec3(1.0f, 1.0f, 1.0f);
    boxShapeSettings.mDensity = 0.01f;

    JPH_SHAPE_ASSERT(boxShapeSettings.Create(), cubeShape);

    boxSettings.mPosition = JPH::Vec3(0.0f, -5.0f, 3.0f);
    boxSettings.mMotionType = JPH::EMotionType::Dynamic;
    boxSettings.mObjectLayer = LAYER_DYNAMIC;
    boxSettings.mGravityFactor *= -1;
    boxSettings.SetShape(cubeShape);

    joltglobals.cube = joltglobals.bodyInterface->CreateBody(boxSettings);

    joltglobals.bodyInterface->AddBody(joltglobals.cube->GetID(), JPH::EActivation::Activate);

    joltglobals.physicsSystem.OptimizeBroadPhase();
}

extern "C" void joltUpdate(f32 deltaTime, mat4 m) {
    joltglobals.physicsSystem.Update(deltaTime, 1, joltglobals.tempAllocator, joltglobals.jobSystem);

    JPH::Mat44 transform = joltglobals.bodyInterface->GetWorldTransform(joltglobals.cube->GetID());

    for (u32 i = 0; i < 4; i++) {
        for (u32 j = 0; j < 4; j++) {
            m[i][j] = transform(j, i);
        }
    }
}

extern "C" void joltFireCube(vec3 pos, vec3 dir) {
    glm_vec3_scale(pos, 2.0f, pos);

    joltglobals.bodyInterface->SetPosition(joltglobals.cube->GetID(), JPH::Vec3(pos[0], pos[1], pos[2]), JPH::EActivation::Activate);
    joltglobals.bodyInterface->SetLinearVelocity(joltglobals.cube->GetID(), JPH::Vec3(dir[0], dir[1], dir[2]));
}

extern "C" void joltQuit() {
    joltglobals.bodyInterface->RemoveBody(joltglobals.cube->GetID());
    joltglobals.bodyInterface->DeactivateBody(joltglobals.cube->GetID());
    joltglobals.bodyInterface->RemoveBody(joltglobals.floor->GetID());
    joltglobals.bodyInterface->DeactivateBody(joltglobals.floor->GetID());

    delete joltglobals.jobSystem;
    delete joltglobals.tempAllocator;

    JPH::UnregisterTypes();
    JPH::Factory::sInstance = NULL;
}

JPH_SUPPRESS_WARNING_POP