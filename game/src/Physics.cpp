#include "Physics.h"
#include <cassert>

void UpdateMotion(PhysicsWorld& world);
std::vector<HitPair> DetectCollisions(PhysicsWorld& world);
void ValidateResolutionVectors(std::vector<HitPair>& collisions);
void ResolveCollisions(std::vector<HitPair> collisions);
void ResolveVelocity(PhysicsBody& a, PhysicsBody& b, Vector2 mtv);
void ResolvePosition(PhysicsBody& a, PhysicsBody& b, Vector2 mtv);

void DeleteBodies(PhysicsWorld& world)
{
    for (size_t i = 0; i < world.entities.size(); i++)
    {
        if (world.entities[i].should_delete)
        {
            std::vector<PhysicsBody>::iterator iterator = (world.entities.begin() + i);
            world.entities.erase(iterator);
            i--;
        }
    }
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
            else if (
                a.collider_type == COLLIDER_TYPE_CIRCLE &&
                b.collider_type == COLLIDER_TYPE_BOX)
            {
                collision = CircleBox(
                    a.position, a.collider.circle.radius,
                    b.position, b.collider.box.extents,
                    &mtv);
            }
            else if (
                a.collider_type == COLLIDER_TYPE_BOX &&
                b.collider_type == COLLIDER_TYPE_CIRCLE)
            {
                collision = CircleBox(
                    b.position, b.collider.circle.radius,
                    a.position, a.collider.box.extents,
                    &mtv);
            }
            else if (
                a.collider_type == COLLIDER_TYPE_BOX &&
                b.collider_type == COLLIDER_TYPE_HALF_SPACE)
            {
                collision = BoxHalfspace(
                    a.position, a.collider.box.extents,
                    b.position, b.collider.half_space.normal,
                    &mtv);
            }
            else if (
                a.collider_type == COLLIDER_TYPE_HALF_SPACE &&
                b.collider_type == COLLIDER_TYPE_BOX)
            {
                collision = BoxHalfspace(
                    b.position, b.collider.box.extents,
                    a.position, a.collider.half_space.normal,
                    &mtv);
            }
            else if (
                a.collider_type == COLLIDER_TYPE_BOX &&
                b.collider_type == COLLIDER_TYPE_BOX)
            {
                collision = BoxBox(
                    a.position, a.collider.box.extents,
                    b.position, b.collider.box.extents,
                    &mtv);
            }

            if (collision)
            {
                bool shouldBreakA = CheckForBreak(a, b.velocity);
                if (shouldBreakA) {
                    a.should_delete = true;
                }
                bool shouldBreakB = CheckForBreak(b, a.velocity);
                if (shouldBreakB) {
                    b.should_delete = true;
                }
                if (shouldBreakA || shouldBreakB) {
                    break;
                }
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

        ResolveVelocity(a, b, mtv);
        ResolvePosition(a, b, mtv);
    }
}

void ResolveVelocity(PhysicsBody& a, PhysicsBody& b, Vector2 mtv)
{
    Vector2 normal = Vector2Normalize(mtv);

    // Resolve velocity
    float inv_mass_sum = a.inv_mass + b.inv_mass;
    assert(inv_mass_sum >= EPSILON);

    // Velocity from B to A (must go from B to A since we asserted that mtv points from B to A)
    Vector2 vel_rel = a.velocity - b.velocity;
    float dot_vel_norm = Vector2DotProduct(vel_rel, normal);

    // Objects are already moving away from each other if relative angle is < 90 degrees!
    if (dot_vel_norm >= EPSILON) return;

    // Use the lower of the two "bounciness" coefficients (restitution = "how much energy is lost on-collision")
    float e = fminf(a.restitution_coeff, b.restitution_coeff);

    // Impulse magnitude
    float j = (-(1.0f + e) * dot_vel_norm) / inv_mass_sum;

    // Magnitude * direction of impulse
    Vector2 impulse = normal * j;

    // Apply impulse to both bodies
    a.velocity = a.velocity + impulse * a.inv_mass;
    b.velocity = b.velocity - impulse * b.inv_mass;

    // Friction direction is tangent to impulse
    Vector2 tangent = vel_rel - normal * dot_vel_norm;
    if (FloatEquals(Vector2LengthSqr(tangent), 0.0f)) return;
    tangent = Vector2Normalize(tangent);

    float jt = (-Vector2DotProduct(vel_rel, tangent)) / inv_mass_sum;
    if (FloatEquals(jt, 0.0f)) return;

    // Coulomb's law (https://en.wikipedia.org/wiki/Coulomb%27s_law)
    float friction = sqrtf(a.friction_coeff * b.friction_coeff);
    if (jt > j * friction)
        jt = j * friction;
    else if (jt < -j * friction)
        jt = -j * friction;

    // Friction = friction direction * friction magnitude
    Vector2 tangent_impulse = tangent * jt;

    // Apply friction to both bodies
    a.velocity = a.velocity + tangent_impulse * a.inv_mass;
    b.velocity = b.velocity - tangent_impulse * b.inv_mass;
}

void ResolvePosition(PhysicsBody& a, PhysicsBody& b, Vector2 mtv)
{
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

void Init(PhysicsWorld& world)
{
    PhysicsBody* entity = nullptr;
    Vector2 test_force = Vector2UnitX * 1000.0f;

    world.entities.push_back({});
    entity = &world.entities.back();
    entity->position = { 400.0f, 680.0f };
    entity->gravity_scale = 0.0f;
    entity->collider_type = COLLIDER_TYPE_HALF_SPACE;
    entity->collider.half_space.normal = Vector2UnitY * -1.0f;
    entity->inv_mass = 0.0f;

    // BLOCKS START
    float block_mass = 1.0f / 3.0f;
    float block_grav_scale = 8.0f;

    world.entities.push_back({});
    entity = &world.entities.back();
    entity->position = { 418.0f, 650.0f };
    entity->gravity_scale = block_grav_scale;
    entity->collider_type = COLLIDER_TYPE_BOX;
    entity->collider.box.extents = { 100.0f / 2.0f, 70.0 / 2.0f };
    entity->inv_mass = block_mass;

    world.entities.push_back({});
    entity = &world.entities.back();
    entity->position = { 398.0f, 580.0f };
    entity->gravity_scale = block_grav_scale;
    entity->collider_type = COLLIDER_TYPE_BOX;
    entity->collider.box.extents = { 100.0f / 2.0f, 70.0 / 2.0f };
    entity->inv_mass = block_mass;

    world.entities.push_back({});
    entity = &world.entities.back();
    entity->position = { 418.0f, 510.0f };
    entity->gravity_scale = block_grav_scale;
    entity->collider_type = COLLIDER_TYPE_BOX;
    entity->collider.box.extents = { 100.0f / 2.0f, 70.0 / 2.0f };
    entity->inv_mass = block_mass;

    // Ensure all half-space's have infinite mass (good habit to validate your entities after creation but before physics-loop)
    for (const PhysicsBody& e : world.entities)
    {
        if (e.collider_type == COLLIDER_TYPE_HALF_SPACE)
        {
            assert(IsMassInfinite(e));
        }
    }
}

void Update(PhysicsWorld& world)
{
    // Delta-time is 0 on the frame 1, which will zero any forces applied before frame 1 (fix by skipping frame 1)
    static bool is_first_frame = true;
    if (is_first_frame)
    {
        is_first_frame = false;
        return;
    }

    DeleteBodies(world);
    UpdateMotion(world);
    std::vector<HitPair> collisions = DetectCollisions(world);
    ValidateResolutionVectors(collisions);
    ResolveCollisions(collisions);
}

void Draw(const PhysicsWorld& world)
{
    for (const PhysicsBody& e : world.entities)
    {
        //Color color = e.collision ? RED : GREEN;
        if (e.collider_type == COLLIDER_TYPE_CIRCLE)
        {
            DrawCircleV(e.position, e.collider.circle.radius, e.color);
        }
        else if (e.collider_type == COLLIDER_TYPE_HALF_SPACE)
        {
            // "Flip" the normal to determine the direction of the half-space
            Vector2 direction = { -e.collider.half_space.normal.y, e.collider.half_space.normal.x };
            Vector2 p0 = e.position + direction * 1000.0f;
            Vector2 p1 = e.position - direction * 1000.0f;
            DrawLineEx(p0, p1, 5.0f, e.color);
            DrawLineEx(e.position, e.position + e.collider.half_space.normal * 50.0f, 5.0f, GOLD);
        }
        else if (e.collider_type == COLLIDER_TYPE_BOX)
        {
            DrawRectangleV(e.position - e.collider.box.extents, e.collider.box.extents * 2.0f, e.color);
        }
    }
}

void Quit(PhysicsWorld& world)
{
    world.entities.resize(0);
    world.gravity = { 0.0f, 9.81f };
}
