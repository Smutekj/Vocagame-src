#pragma once

#define OBJ_LIST(X)      \
    X(Trigger)            \
    X(Bullet)            \
    X(CharSeq)           \
    X(Spawner)           \
    X(Pickup)            \
    X(GunBlock)          \
    X(BoomBox)           \
    X(Wall)              \
    X(Planet)              \
    X(TextBubble)        \
    X(TextRect)     \
    X(TextPlatform)      \
    X(ElectroWall)       \
    X(Meteor)       \
    X(AnimatedSprite)    \
    X(VisualObject)      \
    X(ParticleObject)    \
    X(ParticleTexObject) \
    X(PathingWall)       \
    X(Box)

#define TYPE_LIST   Meteor, Trigger, Bullet, CharSeq, Spawner, Pickup, GunBlock, BoomBox, \
                    Wall, Planet, TextBubble, TextRect, TextPlatform, ElectroWall, AnimatedSprite,  \
                    VisualObject, ParticleObject, ParticleTexObject, PathingWall, Box
#define STRINGIFY(x) #x

/*-----------------*/
enum class TypeId
{
    DungeonPlayer = 0,
    TYPE_LIST,
    Snake,
    Player,
    Count
};
using ObjectType = TypeId;
/*------------------- */
#include "describe.hpp"
BOOST_DESCRIBE_ENUM(TypeId, TYPE_LIST)
/*------------------- */