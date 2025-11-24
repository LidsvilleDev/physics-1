#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <vector>
#include <cassert>

enum ColliderType
{
    COLLIDER_TYPE_INVALID,
    COLLIDER_TYPE_CIRCLE,
    COLLIDER_TYPE_HALF_SPACE
    //COLLDER_BOX <-- not implemented yet
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
        float rotation;
        float getRotation() { return rotation; }
        void setRotation(float rot) {
            rotation = rot;
            normal = Vector2Rotate(Vector2UnitX, rotation * DEG2RAD);
        }
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

    float friction_coeff = 1.0f;        // How easily an object moves along a surface --> 0 = easy, 1 = hard
    float restitution_coeff = 1.0f;     // "Bounciness" --> how much energy is lost on-collision (0 = all energy lost, 1 = no energy lost)

    ColliderType collider_type = COLLIDER_TYPE_INVALID;
    Collider collider{};
    bool collision = false;
    Color color = MAGENTA;
};

struct HitPair
{
    //int a = -1;
    //int b = -1;
    PhysicsBody* a = nullptr;
    PhysicsBody* b = nullptr;
    Vector2 mtv = Vector2Zeros;
};

// Physics Simulation
struct PhysicsWorld
{
    Vector2 gravity = { 0.0f, 9.81f };
    std::vector<PhysicsBody> entities;
};

// MTV points FROM 2 TO 1
bool CircleCircle(Vector2 pos1, float rad1, Vector2 pos2, float rad2, Vector2* mtv = nullptr)
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
bool CircleHalfSpace(Vector2 pos_circle, float rad, Vector2 pos_half_space, Vector2 normal, Vector2* mtv = nullptr)
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

void UpdateMotion(PhysicsWorld& world);
std::vector<HitPair> DetectCollisions(PhysicsWorld& world);
void ValidateResolutionVectors(std::vector<HitPair>& collisions);
void ResolveCollisions(std::vector<HitPair> collisions);

void Update(PhysicsWorld& world);
void Draw(const PhysicsWorld& world);
void DrawForces(std::vector<HitPair> collisions, Vector2 gravity);

void DrawProjCircleHalfSpace(Vector2 pos_circle, float rad, Vector2 pos_half_space, Vector2 normal);

// Inverse-Mass of 0.0 means "infinitely heavy" --> 1.0f / 0.0f --> "infinity" for our purposes
bool IsMassInfinite(const PhysicsBody& entity)
{
    return entity.inv_mass <= FLT_EPSILON;
}

int main()
{
    PhysicsWorld world;
    {
        PhysicsBody* entity = nullptr;

        Vector2 test_force = Vector2UnitY * -1000.0f;

        // Static half-space
        world.entities.push_back({});
        entity = &world.entities.back();
        entity->position = { 400.0f, 600.0f };
        entity->gravity_scale = 0.0f;
        entity->collider_type = COLLIDER_TYPE_HALF_SPACE;
        entity->collider.half_space.normal = Vector2Rotate(Vector2UnitX, -45.0f * DEG2RAD);
        entity->inv_mass = 0.0f;

        // Static circle
        //world.entities.push_back({});
        //entity = &world.entities.back();
        //entity->position = { 400.0f, 500.0f };
        //entity->gravity_scale = 0.0f;
        //entity->collider_type = COLLIDER_TYPE_CIRCLE;
        //entity->collider.circle.radius = 20.0f;
        //entity->inv_mass = 0.0f;

        world.entities.push_back({});
        entity = &world.entities.back();
        entity->position = { 100.0f, 100.0f };
        entity->gravity_scale = 1.0f;
        entity->collider_type = COLLIDER_TYPE_CIRCLE;
        entity->collider.circle.radius = 20.0f;
        entity->inv_mass = 1.0f / 2.0f;
        entity->net_force += test_force;
        entity->friction_coeff = 0.1f;
        entity->color = ORANGE;

        world.entities.push_back({});
        entity = &world.entities.back();
        entity->position = { 300.0f, 100.0f };
        entity->gravity_scale = 1.0f;
        entity->collider_type = COLLIDER_TYPE_CIRCLE;
        entity->collider.circle.radius = 20.0f;
        entity->inv_mass = 1.0f / 2.0f;
        entity->net_force += test_force;
        entity->friction_coeff = 0.8f;
        entity->color = GREEN;

        world.entities.push_back({});
        entity = &world.entities.back();
        entity->position = { 500.0f, 100.0f };
        entity->gravity_scale = 1.0f;
        entity->collider_type = COLLIDER_TYPE_CIRCLE;
        entity->collider.circle.radius = 20.0f;
        entity->inv_mass = 1.0f / 8.0f;
        entity->net_force += test_force;
        entity->friction_coeff = 0.1f;
        entity->color = BLUE;

        world.entities.push_back({});
        entity = &world.entities.back();
        entity->position = { 700.0f, 100.0f };
        entity->gravity_scale = 8.0f;
        entity->collider_type = COLLIDER_TYPE_CIRCLE;
        entity->collider.circle.radius = 20.0f;
        entity->inv_mass = 1.0f / 8.0f;
        entity->net_force += test_force;
        entity->friction_coeff = 0.8f;
        entity->color = YELLOW;
    }

    // Ensure all half-space's have infinite mass (good habit to validate your entities after creation but before physics-loop)
    for (const PhysicsBody& e : world.entities)
    {
        if (e.collider_type == COLLIDER_TYPE_HALF_SPACE)
        {
            assert(IsMassInfinite(e));
        }
    }

    // Delta-time is 0 on the frame 1, which will zero any forces applied before frame 1 (fix by skipping frame 1)
    bool is_first_frame = true;
    InitWindow(800, 800, "Physics-1");
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        if (!is_first_frame)
            Update(world);
        else
            is_first_frame = false;

        BeginDrawing();
            ClearBackground(WHITE);

            float i = 1;

            for (PhysicsBody& e : world.entities)
            {
                if (e.collider_type == COLLIDER_TYPE_HALF_SPACE)
                {
                    float halfspace_rotation = e.collider.half_space.getRotation();
                    GuiSliderBar(Rectangle{ 120, 240, 540, 45 }, "Halfspace Rotation", TextFormat("%.0f", e.collider.half_space.getRotation()), &halfspace_rotation, -120, -45);
                    e.collider.half_space.setRotation(halfspace_rotation);
                }
                if (e.collider_type == COLLIDER_TYPE_CIRCLE)
                {
                    float coeff = e.friction_coeff;
                    GuiSliderBar(Rectangle{ 80 + (180 * (i-1)), 40, 100, 60 }, TextFormat("%i Coeff", int(i)), TextFormat("%.0f", e.friction_coeff), &coeff, 0, 2);
                    e.friction_coeff = coeff;
                    float mass = 1.0f / e.inv_mass;
                    GuiSliderBar(Rectangle{ 80 + (180 * (i-1)), 160, 100, 60 }, TextFormat("%i Mass", int(i)), TextFormat("%.0f", mass), &mass, 0.1, 20);
                    e.inv_mass = 1.0f / mass;

                    i = i + 1.0f;
                }
            }

            Draw(world);

            // Less efficient re-calculating collisions during render loop, but its easier than hard-coding a test-case ;)
            std::vector<HitPair> collisions = DetectCollisions(world);
            ValidateResolutionVectors(collisions);
            DrawForces(collisions, world.gravity);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

void DrawProjCircleHalfSpace(Vector2 pos_circle, float rad, Vector2 pos_half_space, Vector2 normal)
{
    Vector2 to_circle = pos_circle - pos_half_space;
    float proj = Vector2DotProduct(to_circle, normal);

    // TODO -- customize line thickness and colours *if necessary*
    DrawLineEx(pos_half_space, pos_half_space + to_circle, 5.0f, MAGENTA);
    DrawCircleV(pos_half_space + normal * proj, rad, PINK);
    DrawLineEx(pos_half_space, pos_half_space + normal * proj, 4.0f, PINK);
}

// Motion loop
void UpdateMotion(PhysicsWorld& world)
{
    float dt = GetFrameTime();

    for (size_t i = 0; i < world.entities.size(); i++)
    {
        PhysicsBody& e = world.entities[i];

        // F = ma
        // a = F / m        --> possible divide-by-zero error
        // a = F * (1 / m)  --> prevent by multiplying by inverse-mass
        Vector2 acc = e.net_force * e.inv_mass;

        // Acceleration due to gravity is always 9.81 (independent of mass)
        acc += world.gravity * e.gravity_scale;

        e.velocity += acc * dt;             // v = a * t
        e.position += e.velocity * dt;      // p = v * t

        // Reset net force and collision render status
        e.net_force = Vector2Zeros;
        e.collision = false;
    }
}

// Collision loop (Test every object against all other objects)
std::vector<HitPair> DetectCollisions(PhysicsWorld& world)
{
    std::vector<HitPair> collisions;

    for (size_t i = 0; i < world.entities.size(); i++)
    {
        for (size_t j = i + 1; j < world.entities.size(); j++)
        {
            PhysicsBody& a = world.entities[i];
            PhysicsBody& b = world.entities[j];
            assert(a.collider_type != COLLIDER_TYPE_INVALID && b.collider_type != COLLIDER_TYPE_INVALID);
            bool collision = false;
            Vector2 mtv = Vector2Zeros;

            if (a.collider_type == COLLIDER_TYPE_CIRCLE &&
                b.collider_type == COLLIDER_TYPE_CIRCLE)
            {
                collision = CircleCircle(
                    a.position, a.collider.circle.radius,
                    b.position, b.collider.circle.radius,
                    &mtv);
            }
            else if (
                a.collider_type == COLLIDER_TYPE_CIRCLE &&
                b.collider_type == COLLIDER_TYPE_HALF_SPACE)
            {
                collision = CircleHalfSpace(
                    a.position, a.collider.circle.radius,
                    b.position, b.collider.half_space.normal,
                    &mtv);
            }
            else if (
                a.collider_type == COLLIDER_TYPE_HALF_SPACE &&
                b.collider_type == COLLIDER_TYPE_CIRCLE)
            {
                collision = CircleHalfSpace(
                    b.position, b.collider.circle.radius,
                    a.position, a.collider.half_space.normal,
                    &mtv);
            }

            a.collision |= collision;
            b.collision |= collision;

            if (collision)
            {
                HitPair collision;
                collision.a = &a;
                collision.b = &b;
                collision.mtv = mtv;
                collisions.push_back(collision);
            }
        }
    }

    return collisions;
}

void ValidateResolutionVectors(std::vector<HitPair>& collisions)
{
    for (HitPair& collision : collisions)
    {
        PhysicsBody*& a = collision.a;
        PhysicsBody*& b = collision.b;
        Vector2& mtv = collision.mtv;

        // Ensure at least one entity can move (otherwise we shouldn't be resolving collision)
        assert(!(IsMassInfinite(*a) && IsMassInfinite(*b)));

        // Ensure entity A is *ALWAYS* dynamic, and entity B is either static or dynamic
        if (IsMassInfinite(*a))
        {
            // Swap A and B if A is static
            PhysicsBody* temp = b;
            b = a;
            a = temp;
        }

        // Ensure that mtv points FROM B TO A
        Vector2 direction_BA = Vector2Normalize(a->position - b->position);
        float dot = Vector2DotProduct(direction_BA, collision.mtv);
        if (dot < 0.0f)
        {
            mtv *= -1.0f;
        }
    }
}

void ResolveCollisions(std::vector<HitPair> collisions)
{
    for (HitPair collision : collisions)
    {
        PhysicsBody& a = *collision.a;
        PhysicsBody& b = *collision.b;
        Vector2 mtv = collision.mtv;

        if (IsMassInfinite(b))
        {
            a.position += mtv;
        }
        else
        {
            a.position += mtv * 0.5f;
            b.position -= mtv * 0.5f;
        }
    }
}

void Update(PhysicsWorld& world)
{
    UpdateMotion(world);
    std::vector<HitPair> collisions = DetectCollisions(world);
    ValidateResolutionVectors(collisions);
    ResolveCollisions(collisions);
}

void Draw(const PhysicsWorld& world)
{
    for (const PhysicsBody& e : world.entities)
    {
        Color color = e.color;
        if (e.collider_type == COLLIDER_TYPE_CIRCLE)
        {
            DrawCircleV(e.position, e.collider.circle.radius, color);

            float magnitude = world.gravity.y / e.inv_mass;
            Vector2 force_gravity = Vector2Normalize(world.gravity) * magnitude;

            DrawLineEx(e.position, e.position + force_gravity, 2.0f, PURPLE);
            DrawLineEx(e.position, e.position + e.velocity, 2.0f, RED);
        }
        else if (e.collider_type == COLLIDER_TYPE_HALF_SPACE)
        {
            // "Flip" the normal to determine the direction of the half-space
            Vector2 direction = { -e.collider.half_space.normal.y, e.collider.half_space.normal.x };
            Vector2 p0 = e.position + direction * 1000.0f;
            Vector2 p1 = e.position - direction * 1000.0f;
            DrawLineEx(p0, p1, 5.0f, color);
            DrawLineEx(e.position, e.position + e.collider.half_space.normal * 50.0f, 5.0f, GOLD);
        }
    }

    // Removed hard-coded test to free us of our entity-order constraint!
    //DrawProjCircleHalfSpace(
    //    world.entities[2].position, world.entities[2].collider.circle.radius,
    //    world.entities[1].position, world.entities[1].collider.half_space.normal);
}

void DrawForces(std::vector<HitPair> collisions, Vector2 gravity)
{
    for (HitPair collision : collisions)
    {
        PhysicsBody& a = *collision.a;
        PhysicsBody& b = *collision.b;

        if (a.collider_type == COLLIDER_TYPE_CIRCLE && b.collider_type == COLLIDER_TYPE_HALF_SPACE)
        {
            // Stop circle once it touches the half-space so we can visualize forces
            //a.gravity_scale = 0.0f;
            //a.velocity = Vector2Zeros;

            // Lab Exercise 6 TODO -- correct the friction direction by doing the following:
            // Read pages 386-392 of the Game Physics Cookbook (by Gabor Szauer)
            // It explains how to properly calculate the friction direction.
            // Use the textbook's formula to correct force_normal and force_friction

            float magnitude = gravity.y / a.inv_mass;
            Vector2 force_gravity = Vector2Normalize(gravity) * magnitude;
            //Vector2 force_normal = b.collider.half_space.normal * magnitude;

            Vector2 fg_perp = b.collider.half_space.normal * Vector2DotProduct(force_gravity, b.collider.half_space.normal);
            Vector2 force_normal = fg_perp * -1;

            Vector2 fg_para = force_gravity - fg_perp;
            Vector2 force_friction = fg_para * -1;

            // This friction force "just happens" to be in the right direction.
            // The textbook's formula is much more involved than this!
            //Vector2 force_friction = { force_normal.y, -force_normal.x };
            //force_friction *= a.friction_coeff;

            DrawLineEx(a.position, a.position + force_normal, 2.0f, GREEN);
            DrawLineEx(a.position, a.position + force_friction, 2.0f, ORANGE);
        }
    }
}
