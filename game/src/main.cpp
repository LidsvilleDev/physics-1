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
#include <iostream>

Vector2 screenSize = { 800, 800 };

float dt = 0.0f;

enum PhysicsShape {
    CIRCLE,
    RECTANGLE,
    HALF_SPACE
};

class PhysicsBody 
{
public:
    bool isStatic;
    Vector2 velocity;
    Vector2 position;
    float drag;
    float mass;
    Color color;

    PhysicsBody(Vector2 vel, Vector2 pos, float d, float m, Color c, bool s) : velocity(vel), position(pos), drag(d), mass(m), color(c), isStatic(s) {}
    PhysicsBody(Vector2 vel, Vector2 pos, float d, float m, bool s) : velocity(vel), position(pos), drag(d), mass(m), isStatic(s) { color = BLACK; }

    virtual void draw() { return; }

    virtual PhysicsShape Shape() = 0;
};

class PhysicsRectangle : public PhysicsBody 
{
public:
    Vector2 size;

    PhysicsRectangle(Vector2 vel, Vector2 pos, float d, float m, Color c, Vector2 si, bool s) : PhysicsBody(vel, pos, d, m, c, s) { size = si; };

    void draw() override
    {
        DrawRectangle(position.x, position.y, size.x, size.y, color);
    }

    PhysicsShape Shape() override { return RECTANGLE; };
};

class PhysicsCircle : public PhysicsBody
{
public:
    float radius;
    
    PhysicsCircle(Vector2 vel, Vector2 pos, float d, float m, Color c, float r, bool s) : PhysicsBody(vel, pos, d, m, c, s) { radius = r; };

    void draw() override
    {
        DrawCircle(position.x, position.y, radius, color);
    }

    PhysicsShape Shape() override { return CIRCLE; };
};


class PhysicsHalfspace : public PhysicsBody
{
private:
    float rotation = 0;
    Vector2 normal = { 0, -1 };

public:
    PhysicsHalfspace(Vector2 vel, Vector2 pos, float d, float m, Color c, bool s, float rot, Vector2 nor) : PhysicsBody(vel, pos, d, m, c, s) { rotation = rot, normal = nor; };

    void setRotationDegrees(float rotationInDegrees)
    {
        rotation = rotationInDegrees;
        normal = Vector2Rotate({ 0, -1 }, rotation * DEG2RAD);
    }

    float getRotation() { return rotation; }

    Vector2 getNormal() { return normal; }

    void draw() override
    {
        //Draw arbitrary point on the line
        DrawCircle(position.x, position.y, 8, color);

        //Draw normal vector, perpendicular to the surface
        DrawLineEx(position, position + normal * 30, 1, color);

        //Draw the line/surface
        //Rotate function takes radians. 360 degrees = 2PI radians 
        Vector2 parallelToSurface = Vector2Rotate(normal, PI * 0.5f);
        DrawLineEx(position - parallelToSurface * 4000, position + parallelToSurface * 4000, 1, color);
    }

    PhysicsShape Shape() override { return HALF_SPACE; }
};

bool CircleCircleOverlap(PhysicsCircle* circleA, PhysicsCircle* circleB) {
    Vector2 displacement = circleB->position - circleA->position;
    float distance = Vector2Length(displacement);
    float overlap = circleA->radius + circleB->radius - distance;
    if (overlap >= 0) 
    {
        Vector2 normalAtoB;
        
        if (abs(distance) < 0.0001f) { normalAtoB = { 0, 1 }; }
        else { normalAtoB = displacement / distance; }

        Vector2 mtv = normalAtoB * overlap;

        circleA->position -= mtv * 0.5f;
        circleB->position += mtv * 0.5f;

        return true; 
    }
    else { return false; }
}

bool CircleHalfspaceOverlap(PhysicsCircle* circle, PhysicsHalfspace* halfspace)
{
    Vector2 displacementToCircle = circle->position - halfspace->position;

    float dot = Vector2DotProduct(displacementToCircle, halfspace->getNormal());
    Vector2 projectionDisplacementOntoNormal = halfspace->getNormal() * dot;

    DrawLineEx(circle->position, circle->position - projectionDisplacementOntoNormal, 1, GRAY);
    Vector2 midpoint = circle->position - projectionDisplacementOntoNormal * 0.5f;
    DrawText(TextFormat("D: %6.0f", dot), midpoint.x, midpoint.y, 30, GRAY);

    float overlap = dot < circle->radius;

    if (overlap > 0)
    {
        Vector2 mtv = halfspace->getNormal() * overlap;
        circle->position += mtv;
    }

    return overlap;
}

bool CircleOffTop(PhysicsCircle* circle) {
    if (circle->position.y <= 0 + circle->radius) { return true; }
    else { return false; }
}

bool CircleOffSide(PhysicsCircle* circle) {
    if (circle->position.x >= screenSize.x - circle->radius || circle->position.x <= 0 + circle->radius) { return true; }
    else { return false; }
}

bool CircleOffBottom(PhysicsCircle* circle) {
    if (circle->position.y >= screenSize.y - circle->radius) { return true; }
    else { return false; }
}

class PhysicsSimulation
{
public:
    Vector2 gravity = { 0.0f, 9.81f };

    std::vector<PhysicsBody*> bodies;

    void add(PhysicsBody* newBody) {
        bodies.push_back(newBody);
    }

    void check_collisions(size_t index) {
        PhysicsBody* bodyPointerA = bodies[index];
        PhysicsShape shapeOfA = bodyPointerA->Shape();
        bodyPointerA->color = GREEN;
            
        for (size_t j = 0; j < bodies.size(); j++) {
            if (j != index) {
                PhysicsBody* bodyPointerB = bodies[j];
                PhysicsShape shapeOfB = bodyPointerB->Shape();
                PhysicsCircle* birdPointerB = (PhysicsCircle*)bodyPointerB;

                bool didOverlap = false;
                if (shapeOfA == CIRCLE && shapeOfB == CIRCLE) {  didOverlap = CircleCircleOverlap((PhysicsCircle*)bodyPointerA, (PhysicsCircle*)bodyPointerB); }
                else if (shapeOfA == CIRCLE && shapeOfB == HALF_SPACE) { didOverlap = CircleHalfspaceOverlap((PhysicsCircle*)bodyPointerA, (PhysicsHalfspace*)bodyPointerB); }
                else if (shapeOfA == HALF_SPACE && shapeOfB == CIRCLE) { didOverlap = CircleHalfspaceOverlap((PhysicsCircle*)bodyPointerB, (PhysicsHalfspace*)bodyPointerA); }

                if (didOverlap) { bodyPointerA->color = RED; }
            }
        }
    }

    void simulate_body(size_t index) {
        PhysicsBody* b = bodies[index];

        if (!b->isStatic)
        {
            b->velocity += gravity * dt * 2.5f;
            b->position += b->velocity * dt * 2.5f;

            if (dynamic_cast<PhysicsCircle*>(b) != nullptr)
            {
                PhysicsCircle* birdPointer = (PhysicsCircle*)b;
                if (CircleOffBottom(birdPointer))
                {
                    // Making it appear as if the bird gradually slows down upon landing
                    birdPointer->position.y = screenSize.y - birdPointer->radius;
                    birdPointer->velocity.x -= birdPointer->velocity.x / birdPointer->drag * dt;
                }

                // Bouncing the bird if it hits the top of the screen or either side of the screen
                if (CircleOffSide(birdPointer)) { birdPointer->velocity.x *= -1; }
                if (CircleOffTop(birdPointer)) { birdPointer->velocity.y *= -1; }
            }
        }

        check_collisions(index);
    }

    void simulate_world() {
        for (size_t i = 0; i < bodies.size(); i++) {
            simulate_body(i);
        }
    }

    void draw_world() {
        for (size_t i = 0; i < bodies.size(); i++) {
            bodies[i]->draw();
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

PhysicsCircle bird(launchVelocity, launchPosition, 0.93f, 0.0f, GREEN, birdRadius, false);

PhysicsHalfspace halfspace({ 0, 0 }, { 0, 0 }, 1, 1, ORANGE, true, 0, { 0, -1 });
PhysicsSimulation simulation;

void update()
{
    dt = GetFrameTime();

    launchVelocity = Vector2Rotate(Vector2UnitX, launchAngle) * launchSpeed;

    simulation.simulate_world();

    if (IsKeyPressed(KEY_SPACE))
    {
        bird.velocity = launchVelocity;
        bird.position = launchPosition;
        //PhysicsCircle* newBird = new PhysicsCircle(launchVelocity, launchPosition, 0.93f, 0.0, birdRadius, GREEN);
        PhysicsCircle* newBird = new PhysicsCircle(bird);
        newBird->radius = rand() % 10 + 10;

        simulation.add(newBird);
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
    simulation.draw_world();

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

    //Controls for halfspace
    GuiSliderBar(Rectangle{ 20, 200, 240, 30 }, "X", TextFormat("%.0f", halfspace.position.x), &halfspace.position.x, 0, GetScreenWidth());
    GuiSliderBar(Rectangle{ screenSize.x/3+20, 200, 240, 30 }, "Y", TextFormat("%.0f", halfspace.position.y), &halfspace.position.y, 0, GetScreenHeight());

    float halfspaceRotation = halfspace.getRotation();
    GuiSliderBar(Rectangle{ screenSize.x/3*2+20, 200, 200, 30 }, "Rotation", TextFormat("%.0f", halfspace.getRotation()), &halfspaceRotation, -360, 360);
    halfspace.setRotationDegrees(halfspaceRotation);

    EndDrawing();
}

int main()
{
    InitWindow(screenSize.x, screenSize.y, "Physics-1");
    halfspace.isStatic = true;
    halfspace.position = { 500, 700 };
    simulation.add(&halfspace);

    while (!WindowShouldClose())
    {
        update();
        draw();
    }

    CloseWindow();
    return 0;
}
