#define NOB_IMPLEMENTATION
// #define NOB_STRIP_PREFIX
#define NOB_WARN_DEPRECATED
#include "include/nob.h" // https://github.com/tsoding/nob.h
#include <stdio.h>
#include <string.h>
#include "include/nob_utils.h"

#define PROJECT_NAME "nob_raylib"

#define SOURCE_FOLDER "src/"
#define INCLUDE_FOLDER "include/"
#define BUILD_FOLDER "build/"
#define BIN_FOLDER BUILD_FOLDER "bin/"
#define WEB_FOLDER "web_build/"
#define RESOURCES_FOLDER "resources/"
#define DOWNLOAD_FOLDER "download/"
#define DEPENDENCY_FOLDER "dependencies/"
#define CONFIG_FILE_NAME ".config"

#define RAYLIB_TAG "5.5"
#define RAYLIB_TAR_FILE "raylib.tar.gz"
#define RAYLIB_DIR_NAME "raylib/"
#define RAYLIB_PLATFORM "PLATFORM_DESKTOP"
#define RAYLIB_ARCHIVE DOWNLOAD_FOLDER RAYLIB_TAR_FILE
#define RAYLIB_SRC_DIR DEPENDENCY_FOLDER RAYLIB_DIR_NAME "src/"

#define EMSCRIPTEN_TAG "4.0.20"
#define EMSCRIPTEN_TAR_FILE "emscripten.tar.gz"
#define EMSCRIPTEN_DIR_NAME "emscripten/"
#define EMSCRIPTEN_ARCHIVE DOWNLOAD_FOLDER EMSCRIPTEN_TAR_FILE
#define EMSCRIPTEN_SRC_DIR DEPENDENCY_FOLDER EMSCRIPTEN_DIR_NAME

static const char *project_name = PROJECT_NAME;

// For tracking last build settings that needs to be rebuild
static int config_version = 1;

// It is set by build input arguments
static struct SavedConfig current_config = {
	false,
	PLATFORM_DESKTOP,
	false,
};
// If successfully loaded config it will point to the data
static struct SavedConfig *previous_config = NULL;

//---------------------------------------------------------------------------------
// Raylib
const char *get_raylib_platform(enum PLATFORM_TARGET platform){
	switch (platform){
		case PLATFORM_DESKTOP_GLFW: return "PLATFORM=PLATFORM_DESKTOP_GLFW";
		case PLATFORM_DESKTOP_SDL: return "PLATFORM=PLATFORM_DESKTOP_SDL";
		case PLATFORM_DESKTOP_RGFW: return "PLATFORM=PLATFORM_DESKTOP_RGFW";
		case PLATFORM_WEB: return "PLATFORM=PLATFORM_WEB";
		case PLATFORM_DRM: return "PLATFORM=PLATFORM_DRM";
		case PLATFORM_ANDROID: return "PLATFORM=PLATFORM_ANDROID";
		default: return "PLATFORM=PLATFORM_DESKTOP";
	}
}

enum RESULT setup_raylib(Nob_Cmd *cmd){
	const char *raylib_url = "https://github.com/raysan5/raylib/archive/refs/tags/" RAYLIB_TAG ".tar.gz";

	// Download
	if (!nob_file_exists(RAYLIB_ARCHIVE)){
		if (download_file(raylib_url, RAYLIB_ARCHIVE) == FAILED){
			return FAILED;
		}

		if (!nob_file_exists(RAYLIB_ARCHIVE)){
			nob_log(NOB_ERROR, "Just downloaded file dissapeared");
			return FAILED;
		}
	}

	// Extract
	if (!nob_mkdir_if_not_exists(DEPENDENCY_FOLDER RAYLIB_DIR_NAME)) return FAILED;
	if (!nob_file_exists(RAYLIB_SRC_DIR "raylib.h")){
		if(extract_tar_archive(RAYLIB_ARCHIVE, DEPENDENCY_FOLDER RAYLIB_DIR_NAME, 1)){
			return FAILED;
		}
	}

	// Compile
	bool need_rebuild = previous_config != NULL && previous_config->platform != current_config.platform;
	if (!nob_file_exists(RAYLIB_SRC_DIR "libraylib.a") || need_rebuild){
		nob_log(NOB_INFO, "Changing directory: %s ", RAYLIB_SRC_DIR);
		if (!nob_set_current_dir(nob_temp_sprintf("./%s", RAYLIB_SRC_DIR))){
			nob_log(NOB_ERROR, "Failed to change to directory: %s", RAYLIB_SRC_DIR);
			return FAILED;
		}

		nob_make(cmd);
		const char *raylib_platform = get_raylib_platform(current_config.platform);
		if (current_config.platform == PLATFORM_WEB){
			Nob_Cmd emsdk_cmd = {0};
			nob_cmd_append(&emsdk_cmd, "emsdk", "activate", "latest");
			if (!nob_cmd_run(&emsdk_cmd)){
				nob_log(NOB_ERROR, "Failed to activate latest emscriptem");
				return FAILED;
			}
			nob_cmd_append(&emsdk_cmd, "emsdk_env");
			if (!nob_cmd_run(&emsdk_cmd)){
				nob_log(NOB_ERROR, "Failed to initiate emscriptem environment");
				return FAILED;
			}
		}
		nob_cmd_append(cmd, raylib_platform);
		if (!nob_cmd_run(cmd)){
			nob_log(NOB_ERROR, "Failed to compile raylib");
			return FAILED;
		}

		nob_log(NOB_INFO, "Changing directory: %s ", "../../../");
		if (!nob_set_current_dir("../../../")){
			nob_log(NOB_ERROR, "Failed to move bact to root directory");
			return FAILED;
		}
	}

	if (!nob_file_exists(RAYLIB_SRC_DIR "libraylib.a")){
		nob_log(NOB_ERROR, "libraylib.a disappeared!!!");
		return FAILED;
	}

	return SUCCESS;
}

void get_include_raylib(Nob_Cmd *cmd){
	nob_cmd_append(cmd, "-I", RAYLIB_SRC_DIR);
}

void link_raylib(Nob_Cmd *cmd){
	nob_cmd_append(cmd, "-L", RAYLIB_SRC_DIR, "-lraylib");

	switch (current_config.platform){
		case (PLATFORM_DESKTOP):
		case (PLATFORM_DESKTOP_GLFW):
		case (PLATFORM_DESKTOP_RGFW):
	#if defined(WINDOWS)
			nob_cmd_append(cmd, "-lgdi32", "-lwinmm", "-lopengl32");
	#elif defined(LINUX)
			nob_cmd_append(cmd, "-lm", "-ldl", "-lpthread", "-lGL", "-lX11");
	#endif
			break;
	}
}
//---------------------------------------------------------------------------------
// Emscripten
enum RESULT setup_emscripten(Nob_Cmd *cmd){
	const char *emscripten_url = "https://github.com/emscripten-core/emscripten/archive/refs/tags/" EMSCRIPTEN_TAG ".tar.gz";
	// EMSCRIPTEN_TAG
	// EMSCRIPTEN_TAR_FILE
	// EMSCRIPTEN_DIR_NAME
	// EMSCRIPTEN_ARCHIVE
	// EMSCRIPTEN_SRC_DIR
	// Download
	if (!nob_file_exists(EMSCRIPTEN_ARCHIVE)){
		if (download_file(emscripten_url, EMSCRIPTEN_ARCHIVE) == FAILED){
			return FAILED;
		}

		if (!nob_file_exists(EMSCRIPTEN_ARCHIVE)){
			nob_log(NOB_ERROR, "Just downloaded file dissapeared");
			return FAILED;
		}
	}

	// Extract
	if (!nob_mkdir_if_not_exists(DEPENDENCY_FOLDER EMSCRIPTEN_DIR_NAME)) return FAILED;
	if (!nob_file_exists(EMSCRIPTEN_SRC_DIR "README.md")){
		if(extract_tar_archive(EMSCRIPTEN_ARCHIVE, DEPENDENCY_FOLDER EMSCRIPTEN_DIR_NAME, 1)){
			return FAILED;
		}
	}

	if (!nob_file_exists(EMSCRIPTEN_SRC_DIR ".emscripten")){
#if defined(WINDOWS)
		system(EMSCRIPTEN_SRC_DIR ".emsdk.bat install latest");
#elif defined(LINUX)
		system(EMSCRIPTEN_SRC_DIR ".emsdk install latest");
#endif
	}

#if defined(WINDOWS)
	if (system(EMSCRIPTEN_SRC_DIR ".emsdk.bat activate latest") != 0){return FAILED;}
	if (system(EMSCRIPTEN_SRC_DIR "emsdk_env.bat") != 0){return FAILED;}
#elif defined(LINUX)
	if (system(EMSCRIPTEN_SRC_DIR "emsdk activate latest") != 0){return FAILED;}
	if (system("source " EMSCRIPTEN_SRC_DIR "emsdk_env.sh") != 0){return FAILED;}
#endif

	return SUCCESS;
}

enum RESULT setup_web(Nob_Cmd *cmd){
	if (setup_emscripten(cmd) == FAILED){
		return FAILED;
	}

	if (!nob_mkdir_if_not_exists(WEB_FOLDER)) return FAILED;
	if (nob_file_exists(WEB_FOLDER RESOURCES_FOLDER)){
		if (delete_directory(WEB_FOLDER RESOURCES_FOLDER) == FAILED){
			return FAILED;
		}
	}
	if (!nob_mkdir_if_not_exists(WEB_FOLDER RESOURCES_FOLDER)) return FAILED;
	if (!nob_copy_directory_recursively(RESOURCES_FOLDER, WEB_FOLDER RESOURCES_FOLDER)){
		return FAILED;
	}

	return SUCCESS;
}
//---------------------------------------------------------------------------------
// Source
void get_include_directories(Nob_Cmd *cmd){
	nob_cmd_append(cmd, "-I", INCLUDE_FOLDER);
}

void get_defines(Nob_Cmd *cmd){
	if (current_config.is_debug){
		// Debug symbols
		nob_cmd_append(cmd, "-g");
		nob_cmd_append(cmd, "-O0", "-DDEBUG");
		switch (current_config.platform){
			case (PLATFORM_DESKTOP):
			case (PLATFORM_DESKTOP_GLFW):
			case (PLATFORM_DESKTOP_RGFW):
				nob_cmd_append(cmd, "-D", nob_temp_sprintf("RESOURCES_PATH=%s", "../" RESOURCES_FOLDER));
				break;
			default:
				nob_cmd_append(cmd, "-D", nob_temp_sprintf("RESOURCES_PATH=%s", "../" RESOURCES_FOLDER));	
		}
	}
	else{
		nob_cmd_append(cmd, "-D", nob_temp_sprintf("RESOURCES_PATH=%s", RESOURCES_FOLDER));
	}

	if (current_config.platform == PLATFORM_WEB){
		nob_cmd_append(cmd, "-Os", "-Wall");
		nob_cmd_append(cmd, "-s", "USE_GLFW=3");
		nob_cmd_append(cmd, "-s", "ASSERTIONS=1");
		nob_cmd_append(cmd, "-s", "WASM=1");
		nob_cmd_append(cmd, "-s", "TOTAL_MEMORY=67108864");
		nob_cmd_append(cmd, "-s", "FORCE_FILESYSTEM=1");
		nob_cmd_append(cmd, "--preload-file", RESOURCES_FOLDER);
		nob_cmd_append(cmd, "--shell-file", "../minshell.html");
	}
}

enum RESULT get_source_files(Nob_Cmd *cmd){
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
		nob_cmd_append(&obj_cmd, "-c", targets[i].src_path);
		nob_cmd_append(&obj_cmd, "-o", targets[i].bin_path);
		nob_cc_flags(&obj_cmd);
		get_defines(&obj_cmd);
		get_include_raylib(&obj_cmd);
		get_include_directories(&obj_cmd);
		if (!nob_cmd_run(&obj_cmd, .async = &procs)){
			nob_log(NOB_ERROR, "Appending build to queue failed");
			return FAILED;
		}
	}

	// Wait on all the async processes to finish and reset procs dynamic array to 0
	if (!nob_procs_flush(&procs)){
		nob_log(NOB_ERROR, "Parallel source build failed");
		return FAILED;
	}

	for (size_t i = 0; i < NOB_ARRAY_LEN(targets); ++i){
		nob_cc_inputs(cmd, targets[i].bin_path);
	}
	return SUCCESS;
}


int main(int argc, char **argv){
	NOB_GO_REBUILD_URSELF(argc, argv);

	const char *program_name = nob_shift(argv, argc);
	while (argc > 0){
		const char *command_name = nob_shift(argv, argc);
		if (strcmp(command_name, "-debug") == 0){
			current_config.is_debug = true;
		}
		else if (strcmp(command_name, "-name") == 0){
			if (!(argc > 0)){
				nob_log(NOB_ERROR, "No project name provided after `project`");
				return FAILED;
			}
			project_name = nob_shift(argv, argc);
		}
		else if (strcmp(command_name, "-platform") == 0){
			if (!(argc > 0)){
				nob_log(NOB_ERROR, "No project name provided after `project`");
				return FAILED;
			}
			const char *platform = nob_shift(argv, argc);
			if (strcmp(platform, "web") == 0){
				current_config.platform = PLATFORM_WEB;
			}
		}
	}

	if (!nob_mkdir_if_not_exists(DOWNLOAD_FOLDER)) return FAILED;
	if (!nob_mkdir_if_not_exists(DEPENDENCY_FOLDER)) return FAILED;
	if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return FAILED;
	if (!nob_mkdir_if_not_exists(BIN_FOLDER)) return FAILED;

	struct SavedConfig saved_config = {0};
	if (load_binary(&saved_config, sizeof(saved_config), BUILD_FOLDER CONFIG_FILE_NAME, config_version) == 0){
		previous_config = &saved_config;
	}
	
	Nob_Cmd cmd = {0};

	if (current_config.platform == PLATFORM_WEB && setup_web(&cmd) == FAILED){
		return FAILED;
	}

	if (setup_raylib(&cmd) == FAILED) return FAILED;

	// Compile project
	nob_cc(&cmd);
	if (current_config.is_debug){
		// Place inside build folder
		nob_cc_output(&cmd, nob_temp_sprintf("%s%s", BUILD_FOLDER, project_name));
	}
	else{
		// Place in root folder, next to resources folder
		nob_cc_output(&cmd, project_name);
	}
	if (get_source_files(&cmd)) return FAILED;
	link_raylib(&cmd);

	if (!nob_cmd_run(&cmd)) return FAILED;

	if (save_binary(&current_config, sizeof(current_config), BUILD_FOLDER CONFIG_FILE_NAME, config_version) == FAILED){
		return FAILED;
	}

	return SUCCESS;
}
