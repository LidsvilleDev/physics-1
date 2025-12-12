#pragma once
#include "raylib.h"
#include "raymath.h"
#include <iostream>
#include <vector>

enum ColliderType
{
    COLLIDER_TYPE_INVALID,
    COLLIDER_TYPE_CIRCLE,
    COLLIDER_TYPE_HALF_SPACE,
    COLLIDER_TYPE_BOX //<-- You must implement for LE8 & A3
};

union Collider
{
    struct
    {
        float radius;
    } circle;

    struct
    {
        Vector2 normal;
    } half_space;

    struct
    {
        Vector2 extents;
    } box;
};

struct PhysicsBody
{
    Vector2 position = Vector2Zeros;
    Vector2 velocity = Vector2Zeros;
    Vector2 net_force = Vector2Zeros;

    float drag = 1.0f;
    float inv_mass = 1.0f;
    float gravity_scale = 1.0f;

    float friction_coeff = 1.0f;        // Minimum friction --> objects "roll" off each other (no energy lost on-collision due to friction)
    float restitution_coeff = 0.0f;     // Maximum restitution --> objects "bounce" off each other (no energy lost on-collision due to restitution)

    float toughness = 0.0f;

    bool should_delete = false;

    ColliderType collider_type = COLLIDER_TYPE_INVALID;
    Collider collider{};
    bool collision = false;
    Color color = MAGENTA; // Now colored based on collision status
};

struct HitPair
{
    //int a = -1;
    //int b = -1;
    PhysicsBody* a = nullptr;
    PhysicsBody* b = nullptr;
    Vector2 mtv = Vector2Zeros;
};

struct PhysicsWorld
{
    Vector2 gravity = { 0.0f, 9.81f };
    std::vector<PhysicsBody> entities;
};

void Init(PhysicsWorld& world);
void Update(PhysicsWorld& world);
void Draw(const PhysicsWorld& world);
void Quit(PhysicsWorld& world);

// Inverse-Mass of 0.0 means "infinitely heavy" --> 1.0f / 0.0f --> "infinity" for our purposes
inline bool IsMassInfinite(const PhysicsBody& entity)
{
    return entity.inv_mass <= FLT_EPSILON;
}

inline Vector2 GetBoxMin(Vector2 pos, Vector2 ext) {
    Vector2 p1 = pos + ext;
    Vector2 p2 = pos - ext;

    return { fminf(p1.x, p2.x), fminf(p1.y, p2.y) };
}

inline Vector2 GetBoxMax(Vector2 pos, Vector2 ext) {
    Vector2 p1 = pos + ext;
    Vector2 p2 = pos - ext;

    return { fmaxf(p1.x, p2.x), fmaxf(p1.y, p2.y) };
}

inline bool CheckForBreak(PhysicsBody& a, Vector2 incomingVelocity)
{
    if (a.toughness == 0.0f) { return false; }
    float mag = Vector2Length(incomingVelocity);
    bool shouldBreak = false;
    if (mag > a.toughness) { shouldBreak = true; }
    return shouldBreak;
}

// MTV points FROM 2 TO 1
inline bool CircleCircle(Vector2 pos1, float rad1, Vector2 pos2, float rad2, Vector2* mtv = nullptr)
{
    float radii_sum = rad1 + rad2;
    float distance = Vector2Distance(pos1, pos2);
    bool collision = distance <= radii_sum;

    // AB = B - A
    // 21 = 1 - 2
    if (collision && mtv != nullptr)
    {
        float mtv_magnitude = radii_sum - distance;
        Vector2 mtv_direction = Vector2Normalize(pos1 - pos2);
        *mtv = mtv_direction * mtv_magnitude;
    }

    return collision;
}

// MTV points FROM half-space TO circle
inline bool CircleHalfSpace(Vector2 pos_circle, float rad, Vector2 pos_half_space, Vector2 normal, Vector2* mtv = nullptr)
{
    Vector2 to_circle = pos_circle - pos_half_space;
    float proj = Vector2DotProduct(to_circle, normal);
    bool collision = proj <= rad;

    if (collision && mtv != nullptr)
    {
        float mtv_magnitude = rad - proj;
        Vector2 mtv_direction = normal;
        *mtv = mtv_direction * mtv_magnitude;
    }

    return collision;
}

inline bool BoxBox(Vector2 pos1, Vector2 ext1, Vector2 pos2, Vector2 ext2, Vector2* mtv = nullptr)
{
    Vector2 aMin = GetBoxMin(pos1, ext1);
    Vector2 aMax = GetBoxMax(pos1, ext1);
    Vector2 bMin = GetBoxMin(pos2, ext2);
    Vector2 bMax = GetBoxMax(pos2, ext2);

    bool collision = (aMin.x <= bMax.x && aMax.x >= bMin.x) && (aMin.y <= bMax.y && aMax.y >= bMin.y); 
    
    Vector2 delta = pos2 - pos1;
    Vector2 overlapSum = ext1 + ext2;
    overlapSum -= delta;

    float overlapUsed = overlapSum.x;
    if (overlapSum.y < overlapSum.x) { overlapUsed = ext1.y + ext2.y; }


    if (collision && mtv != nullptr)
    {
        static const Vector2 faces[4] =
        {
            { -1, 0 },
            { 1, 0 },
            { 0, -1 },
            { 0, 1 },
        };

        float distances[4] =
        {
            (bMax.x - aMin.x),
            (aMax.x - bMin.x),
            (bMax.y - aMin.y),
            (aMax.y - bMin.y)
        };

        float penetration = FLT_MAX;
        Vector2 bestAxis;

        for (int i = 0; i < 4; i++)
        {
            if (distances[i] < penetration) {
                penetration = distances[i];
                bestAxis = faces[i];
            }
        }

        float mtv_magnitude = penetration;
        Vector2 mtv_direction = bestAxis;
        *mtv = mtv_direction * mtv_magnitude;
    }

    return collision;
}

inline bool CircleBox(Vector2 pos1, float rad, Vector2 pos2, Vector2 ext, Vector2* mtv = nullptr)
{
    Vector2 min = GetBoxMin(pos2, ext);
    Vector2 max = GetBoxMax(pos2, ext);
    Vector2 closest = pos1;
    closest.x = (closest.x < min.x) ? min.x : (closest.x > max.x) ? max.x : closest.x;
    closest.y = (closest.y < min.y) ? min.y : (closest.y > max.y) ? max.y : closest.y;

    Vector2 diff = closest - pos1;
    bool collision = Vector2Length(diff) <= rad;

    if (collision && mtv != nullptr)
    {
        float mtv_magnitude = rad - Vector2Length(diff);
        Vector2 mtv_direction = Vector2Normalize(pos1 - pos2);
        *mtv = mtv_direction * mtv_magnitude;
    }
    return collision;
}

inline bool BoxHalfspace(Vector2 pos_box, Vector2 ext, Vector2 pos_half_space, Vector2 normal, Vector2* mtv = nullptr)
{
    Vector2 box_half_ext = { ext.x / 2.0f, ext.y / 2.0f };
    Vector2 box_center = { pos_box.x + box_half_ext.x, pos_box.y + box_half_ext.y };
    Vector2 diff = pos_half_space - box_center;

    Vector2 to_box = pos_box - pos_half_space;
    float proj = Vector2DotProduct(to_box, normal);
    bool collision = proj - ext.y <= 0;
    
    if (collision && mtv != nullptr)
    {
        float mtv_magnitude = proj - ext.y;
        Vector2 mtv_direction = normal;
        *mtv = mtv_direction * mtv_magnitude;
    }

    return collision;
}
