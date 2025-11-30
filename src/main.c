#include "raylib.h" // www.raylib.com
#include "adjust.h" // learn about features -> https://github.com/bi3mer/adjust.h
#include "load_library.h"

int main(void)
{
    // Initialization
    adjust_init();

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib template");

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        // Update
        adjust_update();

        // Draw
        BeginDrawing();

            ClearBackground(RAYWHITE);

            // NOTE: Keep each ADJUST on it's own line
            DrawText(
                ADJUST_STRING("ADJUST! You can change this while app is running!"), 
                ADJUST_INT(170),
                ADJUST_INT(200),
                ADJUST_INT(20),
                LIGHTGRAY);

        EndDrawing();
    }

    // De-Initialization
    adjust_cleanup();
    CloseWindow();

    return 0;
}
