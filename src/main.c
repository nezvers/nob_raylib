#include "raylib.h" // www.raylib.com
#include "adjust.h" // learn about features -> https://github.com/bi3mer/adjust.h
#include "load_library.h"
#include "os/executable_directory.h"
#include "plug_host.h"

void test_dll();
void test_plug();


int main(void) {
    // Initialization
    adjust_init();

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib template");

    SetTargetFPS(60);

    test_dll();
    test_plug();

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


void test_dll() {
    // DLL TEST
    const char *test_lib_path;
#if defined(_WIN32)
    test_lib_path = "test_dll.dll";
#else
    test_lib_path = "test_dll.so";
    char exe_dir[1024];
    char so_path[1024];
    if (GetExecutableDirectory(exe_dir, sizeof(exe_dir)) == 0) {
        snprintf(so_path, sizeof(so_path), "%s/%s", exe_dir, test_lib_path);
        test_lib_path = so_path;
    }
#endif
    void *test_lib = LibLoad(test_lib_path);
    if (!LibIsValid(test_lib)) {
        printf("Failed to load test_dll.dll\n");
    }
    else{
        void (*print_hello)() = LibGetSymbol(test_lib, "print_hello");
        print_hello();
    }
}


void test_plug() {
    // Plug Test
    const char *plug_path;
#if defined(_WIN32)
    plug_path = "plug_template.dll";
#else
    plug_path = "plug_template.so";
#endif
    PlugApi plug_api;
    if (!PlugLoad(plug_path, &plug_api)) {
        printf("Failed to load plug_template\n");
    } else {
        plug_api.init();
        plug_api.plug_state = plug_api.save_state();
        plug_api.load_state(plug_api.plug_state);
        plug_api.update(NULL);
        plug_api.reset();
        plug_api.free_state();
    }
}
