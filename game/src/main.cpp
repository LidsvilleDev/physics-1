#include "raylib.h"
#include "raymath.h"
#include <vector>

struct PhysicsBody
{
    int id = -1;
    float time = 0.0f;
    Vector2 position = Vector2Zeros;
    Vector2 velocity = Vector2Zeros;
};

struct PhysicsWorld
{
    Vector2 gravity = { 0.0f, 1.6f };
    std::vector<PhysicsBody> entities;
};

constexpr float PROJECTILE_RADIUS = 10.0f;
constexpr float LAUNCH_HEIGHT = 600.0f;

int main()
{
    // An example of using C file libraries + raylib (which also uses C files internally)!
    char* testData = new char[8192];
    int byteCount = 0;

    PhysicsWorld world;

    std::vector<float> launchAngles = { 0.0f, 3.189f, 6.42f, 9.736f, 13.194f,
        16.874f, 20.905f, 25.529f, 31.367f, 45.0f, 58.633f, 64.471f, 69.095f, 
        73.126f, 76.806f, 80.264f, 83.58f, 86.81f, 90.0f };
    
    for (int i = 0; i < launchAngles.size(); i++)
    {
        float launchAngle = launchAngles[i] * DEG2RAD;
        float launchSpeed = 120.0f;

        PhysicsBody entity{};
        entity.id = i;
        entity.position = { 0.0f, LAUNCH_HEIGHT - 1.0f };
        entity.velocity = Vector2Rotate(Vector2UnitX, -launchAngle) * launchSpeed;
        world.entities.push_back(entity);
    }

    InitWindow(800, 800, "Physics-1");
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Motion loop
        for (PhysicsBody& e : world.entities)
        {
            Vector2 acc = world.gravity;

            e.velocity += acc * dt;             // v = a * t
            e.position += e.velocity * dt;      // p = v * t

            e.time += dt;
        }

        // Collision loop
        world.entities.erase(std::remove_if(world.entities.begin(), world.entities.end(),
            [&](PhysicsBody& entity)
            {
                bool remove = entity.position.y >= LAUNCH_HEIGHT;
                if (remove)
                {
                    byteCount += sprintf(testData + byteCount, "Test #%i Range: %f units, Time: %f seconds.\n", entity.id, entity.position.x, entity.time);
                }
                return remove;
            }
        ), world.entities.end());
        
        if (world.entities.size() == 0)
        {
            const char* fileName = "test_output.txt";
            SaveFileText(fileName, testData);
            delete[] testData;
            break;
        }

        // Render loop
        BeginDrawing();
            ClearBackground(WHITE);
            for (const PhysicsBody& e : world.entities)
            {
                DrawCircleV(e.position, PROJECTILE_RADIUS, LIGHTGRAY);
            }
            DrawRectangle(0, LAUNCH_HEIGHT, GetScreenWidth(), 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
