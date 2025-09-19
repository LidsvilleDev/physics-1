/*
This project uses the Raylib framework to provide us functionality for math, graphics, GUI, input etc.
See documentation here: https://www.raylib.com/, and examples here: https://www.raylib.com/examples.html
*/

#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "game.h"

Vector2 screenSize = { 800, 800 };

Rectangle platform = { 0.0f, 450.0f, 125.0f, 20.0f };

float birdRadius = 10.0f;
Vector2 birdPosition = { platform.x + platform.width - birdRadius, platform.y - (platform.height - birdRadius) };

float friction = 0.93f;
float launchSpeed = 100.0f;
float launchAngle = 0.0f;

Vector2 launchPosition = birdPosition;
Vector2 launchVelocity = Vector2Rotate(Vector2UnitX, (launchAngle * DEG2RAD)) * launchSpeed;
Vector2 predictedLaunchVelocity;

Vector2 birdVelocity = launchVelocity;
Vector2 birdAcceleration = { 0.0f, 98.1f };

float timeHeldW, timeHeldS = 1.0f;

float dt = 0.0f;

void update()
{
    dt = GetFrameTime();

    predictedLaunchVelocity.x = 1.0f * cosf(launchAngle) - 0.0f * sinf(launchAngle);
    predictedLaunchVelocity.y = 1.0f * sinf(launchAngle) + 0.0f * cosf(launchAngle);
    predictedLaunchVelocity *= launchSpeed;

    birdVelocity += birdAcceleration * dt;
    birdPosition += birdVelocity * dt;

    if (birdPosition.y >= screenSize.y - birdRadius)
    {
        birdPosition.y = screenSize.y - birdRadius;
        // Making it appear as if the bird gradually slows down upon landing
        birdVelocity.x -= birdVelocity.x / friction * dt;
    }

    // Bouncing the bird if it hits the top of the screen or either side of the screen
    if (birdPosition.x >= screenSize.x - birdRadius || birdPosition.x <= 0 - birdRadius)
    {
        birdVelocity.x *= -1;
    }
    if (birdPosition.y <= 0 - birdRadius)
    {
        birdVelocity.y *= -1;
    }

    if (IsKeyPressed(KEY_SPACE))
    {
        birdPosition = launchPosition;
        birdVelocity = predictedLaunchVelocity;
    }

    // Input for increasing/decreasing friction (1/2)
    if (IsKeyDown(KEY_ONE))
    {
        friction += 0.1f * dt;
    }
    if (IsKeyDown(KEY_TWO))
    {
        friction -= 0.1f * dt;
    }

    // Input for moving the start around (arrow keys)
    if (IsKeyDown(KEY_LEFT))
    {
        launchPosition.x -= 75.0f * dt;
    }
    if (IsKeyDown(KEY_RIGHT))
    {
        launchPosition.x += 75.0f * dt;
    }
    if (IsKeyDown(KEY_UP))
    {
        launchPosition.y -= 75.0f * dt;
    }
    if (IsKeyDown(KEY_DOWN))
    {
        launchPosition.y += 75.0f * dt;
    }

    // Input for altering the launch angle (WASD)
    if (IsKeyDown(KEY_A))
    {
        launchAngle -= 50.0f * DEG2RAD * dt;
    }
    if (IsKeyDown(KEY_D))
    {
        launchAngle += 50.0f * DEG2RAD * dt;
    }
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
}

void draw()
{
    BeginDrawing();
    ClearBackground(WHITE);

    // Where our bird should launch from when we press space
    DrawCircleV(launchPosition, birdRadius, ORANGE);

    // Our bird's current position
    DrawCircleV(birdPosition, birdRadius, RED);

    // Ground at the bottom of the screen, and a platform 3/4's the way down the screen
    DrawRectangleRec(platform, BLACK);

    // Show the result of user-defined launch-angle * launch-speed
    DrawLineEx(launchPosition, launchPosition + predictedLaunchVelocity, 2.0f, RED);

    // Labels for user-defined launch values
    DrawText(TextFormat("Launch Position: %f %f", launchPosition.x, launchPosition.y), 10, 10, 20, LIME);
    DrawText(TextFormat("Launch Angle: %f", launchAngle * RAD2DEG), 10, 40, 20, ORANGE);
    DrawText(TextFormat("Launch Speed: %f ", launchSpeed), 10, 70, 20, DARKGREEN);
    DrawText(TextFormat("Bird Velocity: %f %f", birdVelocity.x, birdVelocity.y), 10, 100, 20, GOLD);
    DrawText(TextFormat("Ground Friction: %f", friction), 10, 130, 20, VIOLET);
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
