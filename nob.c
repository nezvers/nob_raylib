#define NOB_IMPLEMENTATION
// #define NOB_STRIP_PREFIX
#define NOB_WARN_DEPRECATED
#include "nob.h" // https://github.com/tsoding/nob.h
#include <stdio.h>
#include <string.h>

#define PROJECT_NAME "nob_raylib"

#define SOURCE_FOLDER "src/"
#define INCLUDE_FOLDER "include/"
#define BUILD_FOLDER "build/"
#define BIN_FOLDER BUILD_FOLDER "bin/"
#define DOWNLOAD_FOLDER "download/"
#define DEPENDENCY_FOLDER "dependencies/"

#define RAYLIB_TAG "5.5"
#define RAYLIB_TAR_FILE "raylib.tar.gz"
#define RAYLIB_DIR_NAME "raylib/"
#define RAYLIB_PLATFORM "PLATFORM_DESKTOP"
#define RAYLIB_ARCHIVE DOWNLOAD_FOLDER RAYLIB_TAR_FILE
#define RAYLIB_SRC_DIR DEPENDENCY_FOLDER RAYLIB_DIR_NAME "src/"

static bool is_debug;
static const char *project_name = PROJECT_NAME;

// REDEFINE nob_cc - https://web.archive.org/web/20160308010351/https://beefchunk.com/documentation/lang/c/pre-defined-c/precomp.html
#undef nob_cc
#if _WIN32
#	define WINDOWS
#	if defined(__MINGW32__)
#		define nob_cc(cmd) nob_cmd_append(cmd, "gcc")
#	elif defined(__GNUC__)
#		define nob_cc(cmd) nob_cmd_append(cmd, "cc")
#	elif defined(__clang__)
#		define nob_cc(cmd) nob_cmd_append(cmd, "clang")
#	elif defined(_MSC_VER)
#		define nob_cc(cmd) nob_cmd_append(cmd, "cl.exe")
#	endif
#else
#	define LINUX
#	define nob_cc(cmd) nob_cmd_append(cmd, "cc")
#endif

void nob_make(Nob_Cmd *cmd){
#if defined(__MINGW32__)
	nob_cmd_append(cmd, "mingw32-make");
#else
	nob_cmd_append(cmd, "make");
#endif
}

// Downloads file using curl or fallbacks to wget
void download_file(const char *url, const char *dest) {
	char download_cmd[2048] = {0};
	snprintf(download_cmd, sizeof(download_cmd), "curl -fsSL \"%s\" -o \"%s\"", url, dest);
	if (system(download_cmd) != 0) {
		snprintf(download_cmd, sizeof(download_cmd), "wget -q \"%s\" -O \"%s\"", url, dest);
		if (system(download_cmd) != 0) {
			fprintf(stderr, "Failed to download %s\n", url);
		}
	}
}

int extract_tar_archive(const char *archive_path, const char *target_dir){
	char tar_cmd[2048] = {0};
	snprintf(tar_cmd, sizeof(tar_cmd), "tar -xzf \"%s\" -C \"%s\" --strip-components=1", archive_path, target_dir);

	if (system(tar_cmd)){
		nob_log(NOB_ERROR, "[ERROR] Failed to extract: %s -> %s\nCMD: %s", archive_path, target_dir, tar_cmd);
		return 1;
	}
	return 0;
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
		if(extract_tar_archive(RAYLIB_ARCHIVE, DEPENDENCY_FOLDER RAYLIB_DIR_NAME)){
			return 1;
		}
	}

	// Compile
	if (!nob_file_exists(RAYLIB_SRC_DIR "libraylib.a")){
		nob_log(NOB_INFO, "[INFO] Changing directory: %s ", RAYLIB_SRC_DIR);
		if (!nob_set_current_dir(nob_temp_sprintf("./%s", RAYLIB_SRC_DIR))) return 1;

		nob_make(cmd);
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

void get_include_raylib(Nob_Cmd *cmd){
	nob_cmd_append(cmd, "-I", RAYLIB_SRC_DIR);
}

void link_raylib(Nob_Cmd *cmd){
	nob_cmd_append(cmd, "-L", RAYLIB_SRC_DIR, "-lraylib");

#if defined(WINDOWS)
	nob_cmd_append(cmd, "-lgdi32", "-lwinmm", "-lopengl32");
#elif defined(LINUX)
	nob_cmd_append(cmd, "-lm", "-ldl", "-lpthread", "-lGL", "-lX11");
#endif
}

void get_include_directories(Nob_Cmd *cmd){
	nob_cmd_append(cmd, "-I", INCLUDE_FOLDER);
}

void get_defines(Nob_Cmd *cmd){

	// RESOURCES_PATH macro definition
	if (is_debug){
		nob_cmd_append(cmd, "-D", nob_temp_sprintf("RESOURCES_PATH=%s", "../resources/"));
	}
	else{
		nob_cmd_append(cmd, "-D", nob_temp_sprintf("RESOURCES_PATH=%s", "resources/"));
	}

	// Debug symbols
	if (is_debug) nob_cmd_append(cmd, "-g");
}

int get_source_files(Nob_Cmd *cmd){
	// Build source objects in parallel
	Nob_Cmd obj_cmd = {0};
	Nob_Procs procs = {0};
	static struct {
		const char *bin_path;
		const char *src_path;
	} targets[] = {
		{ .bin_path = BIN_FOLDER"main.o", .src_path = SOURCE_FOLDER"main.c" },
	};

	// TODO: Add check if object files are changed
	for (size_t i = 0; i < NOB_ARRAY_LEN(targets); ++i){
		nob_cc(&obj_cmd);
		nob_da_append(&obj_cmd, "-c");
		nob_da_append(&obj_cmd, targets[i].src_path);
		nob_da_append(&obj_cmd, "-o");
		nob_da_append(&obj_cmd, targets[i].bin_path);
		nob_cc_flags(&obj_cmd);
		get_defines(&obj_cmd);
		get_include_raylib(&obj_cmd);
		get_include_directories(&obj_cmd);
		if (!nob_cmd_run(&obj_cmd, .async = &procs)) return 1;
	}

	// Wait on all the async processes to finish and reset procs dynamic array to 0
	if (!nob_procs_flush(&procs)) return 1;

	for (size_t i = 0; i < NOB_ARRAY_LEN(targets); ++i){
		nob_cc_inputs(cmd, targets[i].bin_path);
	}
	return 0;
}

int main(int argc, char **argv){
	NOB_GO_REBUILD_URSELF(argc, argv);

	const char *program_name = nob_shift(argv, argc);
	while (argc > 0){
		const char *command_name = nob_shift(argv, argc);
		if (strcmp(command_name, "-debug") == 0){
			is_debug = true;
		}
		else if (strcmp(command_name, "-name") == 0){
			if (!(argc > 0)){
				nob_log(NOB_ERROR, "[ERROR] No project name provided after `project`");
				return 1;
			}
			project_name = nob_shift(argv, argc);
		}
	}

	if (!nob_mkdir_if_not_exists(DOWNLOAD_FOLDER)) return 1;
	if (!nob_mkdir_if_not_exists(DEPENDENCY_FOLDER)) return 1;
	if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;
	if (!nob_mkdir_if_not_exists(BIN_FOLDER)) return 1;

	Nob_Cmd cmd = {0};
	if (setup_raylib(&cmd) != 0){
		return 1;
	}

	// Compile project
	nob_cc(&cmd);
	if (is_debug){
		// Place inside build folder
		nob_cc_output(&cmd, nob_temp_sprintf("%s%s", BUILD_FOLDER, project_name));
	}
	else{
		// Place in root folder, next to resources folder
		nob_cc_output(&cmd, project_name);
	}
	if (get_source_files(&cmd)) return 1;
	link_raylib(&cmd);

	if (!nob_cmd_run(&cmd)) return 1;

	return 0;
}
