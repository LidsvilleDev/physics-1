#include "raylib.h"
#include "raymath.h"
#include "raygui.h"
#include "Physics.h"
#include <vector>
#include <cassert>

void rayguiTest();

enum SlingshotState
{
    SLING_IDLE,
    SLING_DRAG
};

int main()
{
    PhysicsWorld world;
    Init(world);

    const float slingshot_radius = 10.0f;
    const Vector2 slingshot_position = { 110.0f, 575.0f };
    Vector2 bird_position = slingshot_position;
    SlingshotState slingshot_state = SLING_IDLE;
    bool birdIsCircle = true;

    InitWindow(800, 800, "Physics-1");
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        Vector2 mouse_position = GetMousePosition();
        Update(world);

        BeginDrawing();
            ClearBackground(WHITE);
            Draw(world);

            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && CheckCollisionPointCircle(mouse_position, slingshot_position, slingshot_radius))
                slingshot_state = SLING_DRAG;
            if (IsKeyPressed(KEY_ONE)) { birdIsCircle = true; }
            if (IsKeyPressed(KEY_TWO)) { birdIsCircle = false; }

            if (slingshot_state == SLING_DRAG)
            {
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                    bird_position = mouse_position;
                else
                {
                    PhysicsBody bird;
                    bird.position = slingshot_position;

                    float launchAngle = -atan2f(mouse_position.y - slingshot_position.y, mouse_position.x - slingshot_position.x);
                    launchAngle = (180 * DEG2RAD) - launchAngle;

                    float launchSpeed = Vector2Distance(mouse_position, slingshot_position);
                    launchSpeed *= 3.0f;

                    bird.velocity = Vector2Rotate(Vector2UnitX, launchAngle) * launchSpeed;
                    bird.gravity_scale = 20.0f;
                    bird.toughness = 75.0f;
                    if (birdIsCircle) {
                        bird.collider_type = COLLIDER_TYPE_CIRCLE;
                        bird.collider.circle.radius = 10.0f;
                    }
                    else {
                        bird.collider_type = COLLIDER_TYPE_BOX;
                        bird.collider.box.extents = { 10.0f, 10.0f };
                    }
                    world.entities.push_back(bird);

                    slingshot_state = SLING_IDLE;
                }
            }
            
            if (slingshot_state == SLING_IDLE)
                DrawCircleV(slingshot_position, slingshot_radius, GREEN);
            else
                DrawCircleV(bird_position, slingshot_radius, ORANGE);
            
            DrawRectangle(100, 600, 20, 80, BROWN); // Sling-shot
            DrawRectanglePro({ 95, 580, 50, 10 }, { 25, 5 }, 70.0f, BROWN);
            DrawRectanglePro({ 125, 580, 50, 10 }, { 25, 5 }, -70.0f, BROWN);
            //DrawRectangle(0, 680, GetScreenWidth(), 20, DARKGREEN); // Ground
            //DrawRectangle(0, 700, GetScreenWidth(), 100, DARKBLUE); // Underground

        EndDrawing();
    }

    Quit(world);
    CloseWindow();
    return 0;
}

// Example taken from https://github.com/raysan5/raygui, read documentation to understand how required raygui wigets work!
void rayguiTest()
{
    static bool showMessageBox = false;
    //if (GuiButton({ 24, 24, 120, 30 }, "#191#Show Message")) showMessageBox = true;

    if (showMessageBox)
    {
        //int result = GuiMessageBox({ 85, 70, 250, 100 },
        //    "#191#Message Box", "Hi! This is a message!", "Nice;Cool");

       // if (result >= 0) showMessageBox = false;
    }
}
