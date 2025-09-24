/*
This project uses the Raylib framework to provide us functionality for math, graphics, GUI, input etc.
See documentation here: https://www.raylib.com/, and examples here: https://www.raylib.com/examples.html
*/

#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "game.h"
#include <vector>

Vector2 screenSize = { 800, 800 };

class PhysicsBody 
{
public:
    Vector2 velocity;
    Vector2 position;
    float drag;
    float mass;
    float radius;

    PhysicsBody(Vector2 vel, Vector2 pos, float d, float m, float r) {
        velocity = vel;
        position = pos;
        drag = d;
        mass = m;
        radius = r;
    }
};

class PhysicsSimulation
{
public:
    float dt;
    Vector2 gravity = { 0.0f, 9.81f };

    std::vector<PhysicsBody> bodies;

    void simulate_world() {
        dt = GetFrameTime();
        for (PhysicsBody& b : bodies) {

            b.velocity += gravity * dt;
            b.position += b.velocity * dt;

            if (b.position.y >= screenSize.y - b.radius)
            {
                b.position.y = screenSize.y - b.radius;
                // Making it appear as if the bird gradually slows down upon landing
                b.velocity.x -= b.velocity.x / b.drag * dt;
            }

            // Bouncing the bird if it hits the top of the screen or either side of the screen
            if (b.position.x >= screenSize.x - b.radius || b.position.x <= 0 + b.radius) {
                b.velocity.x *= -1;
            }
            if (b.position.y <= 0 + b.radius) {
                b.velocity.y *= -1;
            }
        }
    }
};

Rectangle platform = { 0.0f, 450.0f, 125.0f, 20.0f };

float timeHeldW, timeHeldS = 1.0f;

float birdRadius = 10.0f;

float launchSpeed = 100.0f;
float launchAngle = 0.0f;
Vector2 launchPosition = { platform.x + platform.width - birdRadius, platform.y - (platform.height - birdRadius) };
Vector2 launchVelocity = Vector2Rotate(Vector2UnitX, launchAngle) * launchSpeed;

PhysicsBody bird(launchVelocity, launchPosition, 0.93f, 0.0f, birdRadius);

PhysicsSimulation simulation;

float dt = 0.0f;

void update()
{
    dt = GetFrameTime();

    launchVelocity = Vector2Rotate(Vector2UnitX, launchAngle) * launchSpeed;

    simulation.simulate_world();

    if (IsKeyPressed(KEY_SPACE))
    {
        bird.position = launchPosition;
        bird.velocity = launchVelocity;
        simulation.bodies.push_back(bird);
    }

    // Input for increasing/decreasing bird.drag (1/2)
    if (IsKeyDown(KEY_ONE)) { bird.drag += 0.1f * dt; }
    if (IsKeyDown(KEY_TWO)) { bird.drag -= 0.1f * dt; }

    // Input for moving the start around (arrow keys)
    if (IsKeyDown(KEY_LEFT))  { launchPosition.x -= 75.0f * dt; }
    if (IsKeyDown(KEY_RIGHT)) { launchPosition.x += 75.0f * dt; }
    if (IsKeyDown(KEY_UP))    { launchPosition.y -= 75.0f * dt; }
    if (IsKeyDown(KEY_DOWN))  { launchPosition.y += 75.0f * dt; }

    // Input for altering the launch angle (WASD)
    if (IsKeyDown(KEY_A)) { launchAngle -= 50.0f * DEG2RAD * dt; }
    if (IsKeyDown(KEY_D)) { launchAngle += 50.0f * DEG2RAD * dt; }
    if (IsKeyDown(KEY_W)) 
    {
        launchSpeed += 125.0f * timeHeldW * dt;
        timeHeldW += dt;
    }
    else { timeHeldW = 1.0f; }
    if (IsKeyDown(KEY_S)) 
    {
        launchSpeed -= 125.0f * timeHeldS * dt;
        timeHeldS += dt;
    }
    else { timeHeldS = 1.0f; }

    // Input for altering gravity strength and angle (IJKL)?

    if (IsKeyDown(KEY_I)) { simulation.gravity += simulation.gravity * 1.05f * dt; }
    if (IsKeyDown(KEY_K)) { simulation.gravity -= simulation.gravity * 1.05f * dt; }
    if (IsKeyDown(KEY_J)) { simulation.gravity = Vector2Rotate(simulation.gravity, (-45.0f * DEG2RAD * dt)); }
    if (IsKeyDown(KEY_L)) { simulation.gravity = Vector2Rotate(simulation.gravity, (45.0f * DEG2RAD * dt)); }
}

void draw()
{
    BeginDrawing();
    ClearBackground(WHITE);

    // Where our bird should launch from when we press space
    DrawCircleV(launchPosition, bird.radius, ORANGE);

    // All of our birds current position(s)
    for (PhysicsBody& b : simulation.bodies)
    {
        DrawCircleV(b.position, bird.radius, RED);
    }

    // Ground at the bottom of the screen, and a platform 3/4's the way down the screen
    DrawRectangleRec(platform, BLACK);

    // Show the result of user-defined launch-angle * launch-speed
    DrawLineEx(launchPosition, launchPosition + launchVelocity, 2.0f, RED);
    DrawLineEx(screenSize/2, (screenSize/2)+simulation.gravity, 2.0f, DARKGREEN);

    // Labels for user-defined launch values
    DrawText(TextFormat("Launch Position: %f %f", launchPosition.x, launchPosition.y), 10, 10, 20, LIME);
    DrawText(TextFormat("Launch Angle: %f", launchAngle * RAD2DEG), 10, 40, 20, ORANGE);
    DrawText(TextFormat("Launch Speed: %f ", launchSpeed), 10, 70, 20, DARKGREEN);
    DrawText(TextFormat("Bird Velocity: %f %f", bird.velocity.x, bird.velocity.y), 10, 100, 20, GOLD);
    DrawText(TextFormat("Bird Drag: %f", bird.drag), 10, 130, 20, VIOLET);
    DrawText(TextFormat("Gravity: %f %f", simulation.gravity.x, simulation.gravity.y), 10, 160, 20, VIOLET);
    EndDrawing();
}

int main()
{
    InitWindow(screenSize.x, screenSize.y, "Physics-1");

    while (!WindowShouldClose())
    {
        update();
        draw();
    }

    CloseWindow();
    return 0;
}
