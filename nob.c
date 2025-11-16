#define NOB_IMPLEMENTATION
// #define NOB_STRIP_PREFIX
#define NOB_WARN_DEPRECATED
#include "nob.h" // https://github.com/tsoding/nob.h
#include <stdio.h>
#include <string.h>

#define PROJECT_NAME "NoBuild_Raylib"

#define BUILD_FOLDER "build/"
#define SOURCE_FOLDER "src/"
#define DOWNLOAD_FOLDER "download/"
#define DEPENDENCY_FOLDER "dependencies/"

#define RAYLIB_TAG "5.5"
#define RAYLIB_TAR_FILE "raylib.tar.gz"
#define RAYLIB_DIR_NAME "raylib/"
#define RAYLIB_PLATFORM "PLATFORM_DESKTOP"
#define RAYLIB_ARCHIVE DOWNLOAD_FOLDER RAYLIB_TAR_FILE
#define RAYLIB_SRC_DIR DEPENDENCY_FOLDER RAYLIB_DIR_NAME "src/"


#define DEBUG
#define WINDOWS
// #define LINUX

#define USE_MINGW_MAKE
// #define USE_MAKE

#define USE_GCC gcc
// #define USE_CLANG clang

void source_files(Nob_Cmd *cmd){
	nob_cc_inputs(cmd,
		SOURCE_FOLDER "main.c",
	);

	nob_cmd_append(cmd,
		"-I", "include/",
	);
}

void download_file(const char *url, const char *dest) {
	char cmd[2048];
	snprintf(cmd, sizeof(cmd), "curl -fsSL \"%s\" -o \"%s\"", url, dest);
	if (system(cmd) != 0) {
		snprintf(cmd, sizeof(cmd), "wget -q \"%s\" -O \"%s\"", url, dest);
		if (system(cmd) != 0) {
			fprintf(stderr, "Failed to download %s\n", url);
		}
	}
}

int setup_raylib(Nob_Cmd *cmd){
	const char *raylib_url = "https://github.com/raysan5/raylib/archive/refs/tags/" RAYLIB_TAG ".tar.gz";
	
	// Download
	if (!nob_file_exists(RAYLIB_ARCHIVE)){
		download_file(raylib_url, RAYLIB_ARCHIVE);

		if (!nob_file_exists(RAYLIB_ARCHIVE)){
			return 1;
		}
	}

	// Extract
	if (!nob_mkdir_if_not_exists(DEPENDENCY_FOLDER RAYLIB_DIR_NAME)) return 1;
	if (!nob_file_exists(RAYLIB_SRC_DIR "raylib.h")){
		const char *extract_cmd = "tar -xzf \"" RAYLIB_ARCHIVE "\" -C \"" DEPENDENCY_FOLDER RAYLIB_DIR_NAME "\" --strip-components=1";
		system(extract_cmd);
	}

	// Compile
	if (!nob_file_exists(RAYLIB_SRC_DIR "raylib.a")){
		nob_log(NOB_INFO, "[INFO] Changing directory: %s ", RAYLIB_SRC_DIR);
		if (!nob_set_current_dir(nob_temp_sprintf("./%s", RAYLIB_SRC_DIR))) return 1;
		
		// TODO: match used compiler make
	#if defined USE_MINGW_MAKE
		nob_cmd_append(cmd, "mingw32-make");
	#else
		nob_cmd_append(cmd, "make");
	#endif
		nob_cmd_append(cmd, "PLATFORM=" RAYLIB_PLATFORM);
		if (!nob_cmd_run(cmd)) return 1;
		
		nob_log(NOB_INFO, "[INFO] Changing directory: %s ", "../../../");
		if (!nob_set_current_dir("../../../")) return 1;
	}

	if (!nob_file_exists(RAYLIB_SRC_DIR "libraylib.a")){
		nob_log(NOB_ERROR, "[ERROR] Failed to compile raylib");
		return 1;
	}

	return 0;
}

void link_raylib(Nob_Cmd *cmd){
	nob_cmd_append(cmd, "-L", RAYLIB_SRC_DIR, "-lraylib", "-I", RAYLIB_SRC_DIR);

#if defined(WINDOWS)
	nob_cmd_append(cmd, "-lgdi32", "-lwinmm", "-lopengl32");
#elif defined(LINUX)
	nob_cmd_append(cmd, "-lm", "-ldl", "-lpthread", "-lGL", "-lX11");
#endif
}

int main(int argc, char **argv){
	NOB_GO_REBUILD_URSELF(argc, argv);

	if (!nob_mkdir_if_not_exists(DOWNLOAD_FOLDER)) return 1;
	if (!nob_mkdir_if_not_exists(DEPENDENCY_FOLDER)) return 1;

	Nob_Cmd cmd = {0};
	if (setup_raylib(&cmd) != 0){
		return 1;
	}

	if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;

	// Compile project
#if defined(USE_GCC)
	nob_cmd_append(&cmd, "gcc");
#else
	nob_cmd_append(&cmd, "cc");
#endif

	nob_cc_output(&cmd, BUILD_FOLDER PROJECT_NAME);
#if defined(DEBUG)
	nob_cmd_append(&cmd, "-g");
#endif
	source_files(&cmd);
	link_raylib(&cmd);
	nob_cc_flags(&cmd);

#if defined(DEBUG)
	nob_cmd_append(&cmd, "-D", nob_temp_sprintf("RESOURCES_PATH=%s", "../resources/"));
#else
	nob_cmd_append(&cmd, "-D", nob_temp_sprintf("RESOURCES_PATH=%s", "resources/"));
#endif

	if (!nob_cmd_run(&cmd)) return 1;

	return 0;
}