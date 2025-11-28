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
#define RAYLIB_DIR_NAME "raylib/"
#define RAYLIB_PLATFORM "PLATFORM_DESKTOP"
#define RAYLIB_TAR_FILE "raylib.tar.gz"
#define RAYLIB_ARCHIVE DOWNLOAD_FOLDER RAYLIB_TAR_FILE
#define RAYLIB_SRC_DIR DEPENDENCY_FOLDER RAYLIB_DIR_NAME "src/"

#define EMSCRIPTEN_TAG "4.0.20"
#define EMSCRIPTEN_DIR_NAME "emsdk/"
#define EMSCRIPTEN_TAR_FILE "emsdk.tar.gz"
#define EMSCRIPTEN_ARCHIVE DOWNLOAD_FOLDER EMSCRIPTEN_TAR_FILE
#define EMSCRIPTEN_SRC_DIR DEPENDENCY_FOLDER EMSCRIPTEN_DIR_NAME

static const char *project_name = PROJECT_NAME;

// For tracking last build settings that needs to be rebuild
static int config_version = 1;

// It is set by build input arguments
static struct SavedConfig current_config = {
	false,				// -debug
	PLATFORM_DESKTOP,	// -platform <target>
	false,				// -wayland
};
// If successfully loaded config it will point to the data
static struct SavedConfig *previous_config = NULL;
static char starting_cwd[1024] = {0};

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
	enum RESULT result = SUCCESS;
	size_t temp_checkpoint = nob_temp_save();
	const char *raylib_url = "https://github.com/raysan5/raylib/archive/refs/tags/" RAYLIB_TAG ".tar.gz";

	// Download
	if (!nob_file_exists(RAYLIB_ARCHIVE)){
		if (download_file(raylib_url, RAYLIB_ARCHIVE) == FAILED){
			assert(false);
			nob_return_defer(FAILED);
		}
		if (!nob_file_exists(RAYLIB_ARCHIVE)){
			nob_log(NOB_ERROR, "Just downloaded file dissapeared");
			assert(false);
			nob_return_defer(FAILED);
		}
	}

	// Extract
	if (!nob_mkdir_if_not_exists(DEPENDENCY_FOLDER RAYLIB_DIR_NAME)) nob_return_defer(FAILED);
	if (!nob_file_exists(DEPENDENCY_FOLDER RAYLIB_DIR_NAME "README.md")){
		if(extract_tar_archive(RAYLIB_ARCHIVE, DEPENDENCY_FOLDER RAYLIB_DIR_NAME, 1)){
			assert(false);
			nob_return_defer(FAILED);
		}
	}

	// Compile
	bool need_rebuild = previous_config != NULL && previous_config->platform != current_config.platform;
	if (!nob_file_exists(RAYLIB_SRC_DIR "libraylib.a") || need_rebuild){
		nob_log(NOB_INFO, "Changing directory: %s ", RAYLIB_SRC_DIR);
		if (!nob_set_current_dir(nob_temp_sprintf("./%s", RAYLIB_SRC_DIR))){
			nob_log(NOB_ERROR, "Failed to change to directory: %s", RAYLIB_SRC_DIR);
			assert(false);
			nob_return_defer(FAILED);
		}

		nob_cmd_make(cmd);
		const char *raylib_platform = get_raylib_platform(current_config.platform);
		nob_cmd_append(cmd, raylib_platform);
		// defer failure after redurning current working directory
		bool compile_success = nob_cmd_run(cmd);

		nob_log(NOB_INFO, "Changing directory: %s ", "../../../");
		if (!nob_set_current_dir("../../../")){
			nob_log(NOB_ERROR, "Failed to move back to root directory");
			assert(false);
			nob_return_defer(FAILED);
		}
		if (!compile_success){
			nob_log(NOB_ERROR, "Failed to compile raylib");
			assert(false);
			nob_return_defer(FAILED);
		}
	}

	if (!nob_file_exists(RAYLIB_SRC_DIR "libraylib.a")){
		nob_log(NOB_ERROR, "libraylib.a disappeared!!!");
		assert(false);
		nob_return_defer(FAILED);
	}

defer:
	nob_temp_rewind(temp_checkpoint);
	return result;
}

void get_include_raylib(Nob_Cmd *cmd){
	nob_cmd_append(cmd, "-I", RAYLIB_SRC_DIR);
}

void link_raylib(Nob_Cmd *cmd){
	nob_cmd_append(cmd, "-L", RAYLIB_SRC_DIR, "-lraylib");

	switch (current_config.platform){
		case (PLATFORM_DESKTOP_SDL):
		case (PLATFORM_DESKTOP_GLFW):
		case (PLATFORM_DESKTOP_RGFW):
	    case (PLATFORM_DESKTOP):
		// TODO: refactor to use the ones that are needed in context
#if defined(WINDOWS)
	#if defined(_MSC_VER)
			nob_cmd_append(cmd, "Winmm.lib", "gdi32.lib", "User32.lib", "Shell32.lib", "Ole32.lib", "comdlg32.lib");
	#else
			nob_cmd_append(cmd, "-lgdi32", "-lwinmm", "-lopengl32", "-lole32");
	#endif
#elif defined(LINUX)
			nob_cmd_append(cmd, "-lm", "-ldl", "-lpthread", "-lGL", "-lX11");
#endif
			break;
		case (PLATFORM_WEB):
		    // TODO:
			break;
		case (PLATFORM_ANDROID):
		    // TODO:
			break;
		case (PLATFORM_DRM):
		    // TODO:
			break;
	}
}
//---------------------------------------------------------------------------------
// Emscripten
enum RESULT setup_emscripten(Nob_Cmd *cmd){
	enum RESULT result = SUCCESS;
	size_t temp_checkpoint = nob_temp_save();
	const char *emscripten_git = "https://github.com/emscripten-core/emsdk.git";
	const char *emsdk_tar = "https://github.com/emscripten-core/emsdk/archive/refs/tags/" EMSCRIPTEN_TAG ".tar.gz";
	// EMSCRIPTEN_TAG
	// EMSCRIPTEN_DIR_NAME
	// EMSCRIPTEN_SRC_DIR
	// EMSCRIPTEN_TAR_FILE
	// EMSCRIPTEN_ARCHIVE

	// Download
	if (!nob_file_exists(EMSCRIPTEN_ARCHIVE)){
		if (download_file(emsdk_tar, EMSCRIPTEN_ARCHIVE) == FAILED){
			assert(false);
			nob_return_defer(FAILED);
		}
		if (!nob_file_exists(EMSCRIPTEN_ARCHIVE)){
			nob_log(NOB_ERROR, "Just downloaded file dissapeared");
			assert(false);
			nob_return_defer(FAILED);
		}
	}

	// Extract
	if (!nob_mkdir_if_not_exists(DEPENDENCY_FOLDER EMSCRIPTEN_DIR_NAME)) nob_return_defer(FAILED);
	if (!nob_file_exists(DEPENDENCY_FOLDER EMSCRIPTEN_DIR_NAME "README.md")){
		if(extract_tar_archive(EMSCRIPTEN_ARCHIVE, DEPENDENCY_FOLDER EMSCRIPTEN_DIR_NAME, 1)){
			assert(false);
			nob_return_defer(FAILED);
		}
	}

	if (!nob_file_exists(EMSCRIPTEN_SRC_DIR ".emscripten")){
		nob_log(NOB_INFO, "Changing directory: %s ", EMSCRIPTEN_SRC_DIR);
		if (!nob_set_current_dir(EMSCRIPTEN_SRC_DIR)){
			nob_log(NOB_ERROR, "Failed to change to directory: %s", EMSCRIPTEN_SRC_DIR);
			assert(false);
			nob_return_defer(FAILED);
		}

		nob_log(NOB_INFO, "EMSDK first time installation - latest");
		bool install_success;
#if defined(WINDOWS)
		Nob_Cmd emsdk_cmd = {0};
		nob_cmd_append(&emsdk_cmd, ".\\emsdk.bat", "install", "latest");
		install_success = nob_cmd_run(&emsdk_cmd);
		nob_cmd_free(emsdk_cmd);
#elif defined(LINUX)
		install_success = system("./emsdk install latest") == 0;
#endif
		nob_log(NOB_INFO, "Changing directory: %s ", "../../");
		if (!nob_set_current_dir("../../")){
			nob_log(NOB_ERROR, "Failed to move back to root directory");
			assert(false);
			nob_return_defer(FAILED);
		}
		if (!install_success) nob_return_defer(FAILED);
	}

	nob_log(NOB_INFO, "EMSDK activate environment");
#if defined(WINDOWS)
	char emsdk_dir[128] = EMSCRIPTEN_SRC_DIR;
	swap_dir_slashes(emsdk_dir, sizeof(emsdk_dir));
	if (system(nob_temp_sprintf("%semsdk.bat activate latest", emsdk_dir)) != 0){nob_return_defer(FAILED);}
	if (system(nob_temp_sprintf("%semsdk_env.bat", emsdk_dir)) != 0){nob_return_defer(FAILED);}
#elif defined(LINUX)
	if (system(EMSCRIPTEN_SRC_DIR "emsdk activate latest") != 0){nob_return_defer(FAILED);}
	if (system("source " EMSCRIPTEN_SRC_DIR "emsdk_env.sh") != 0){nob_return_defer(FAILED);}
#endif

defer:
	nob_temp_rewind(temp_checkpoint);
	return result;
}

enum RESULT setup_web(Nob_Cmd *cmd){
	enum RESULT result = SUCCESS;
	if (setup_emscripten(cmd) == FAILED) nob_return_defer(FAILED);

	if (!nob_mkdir_if_not_exists(WEB_FOLDER)) nob_return_defer(FAILED);
	if (nob_file_exists(WEB_FOLDER RESOURCES_FOLDER)){
		if (delete_directory(WEB_FOLDER RESOURCES_FOLDER) == FAILED){
			assert(false);
			nob_return_defer(FAILED);
		}
	}
	if (!nob_mkdir_if_not_exists(WEB_FOLDER RESOURCES_FOLDER)) nob_return_defer(FAILED);

	if (!nob_copy_directory_recursively(RESOURCES_FOLDER, WEB_FOLDER RESOURCES_FOLDER)){
		assert(false);
		nob_return_defer(FAILED);
	}

defer:
	return result;
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
				nob_cmd_append(cmd, "-D", "RESOURCES_PATH=../" RESOURCES_FOLDER);
				break;
			default:
				nob_cmd_append(cmd, "-D", "RESOURCES_PATH=../" RESOURCES_FOLDER);
		}
	}
	else{
		nob_cmd_append(cmd, "-D", "RESOURCES_PATH=" RESOURCES_FOLDER);
	}

	if (current_config.platform == PLATFORM_WEB){
		// TODO: This is more like placeholder
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

void link_platform(Nob_Cmd *cmd){
#if defined(WINDOWS)
	nob_cmd_append(cmd, "-lwinmm", "-lgdi32", "-lole32");
#elif defined(LINUX)
	nob_cmd_append(cmd, "-lm", "-ldl", "-flto=auto", "-lpthread");
#endif
}

enum RESULT get_source_files(Nob_Cmd *cmd){
	Nob_Cmd main_obj_cmd = {0};
	enum RESULT result = SUCCESS;
	size_t temp_checkpoint = nob_temp_save();
	bool force_rebuild = false;

	// commands for each obj file compilation
	nob_cc_flags(&main_obj_cmd);
	get_defines(&main_obj_cmd);
	get_include_raylib(&main_obj_cmd);
	get_include_directories(&main_obj_cmd);

	if (nob_cmd_process_source_dir(cmd, &main_obj_cmd, SOURCE_FOLDER, BIN_FOLDER, ".c", force_rebuild) == FAILED){
		nob_log(NOB_ERROR, "Failed building main.o");
		assert(false);
		nob_return_defer(FAILED);
	}

defer:
	nob_temp_rewind(temp_checkpoint);
	nob_cmd_free(main_obj_cmd);
	return result;
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
				assert(false);
				return FAILED;
			}
			project_name = nob_shift(argv, argc);
		}
		else if (strcmp(command_name, "-platform") == 0){
			if (!(argc > 0)){
				nob_log(NOB_ERROR, "No project name provided after `project`");
				assert(false);
				return FAILED;
			}
			const char *platform = nob_shift(argv, argc);
			if (strcmp(platform, "web") == 0){
				current_config.platform = PLATFORM_WEB;
			}
		}
	}

	snprintf(starting_cwd, sizeof(starting_cwd), "%s", nob_get_current_dir_temp());

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
		nob_log(NOB_ERROR, "Failed to setup web.");
		assert(false);
		return FAILED;
	}

	if (setup_raylib(&cmd) == FAILED){
		nob_log(NOB_ERROR, "Failed to setup RAYLIB.");
		assert(false);
		return FAILED;
	}

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
	if (get_source_files(&cmd)){
		nob_log(NOB_ERROR, "Failed to get source files");
		assert(false);
		return FAILED;
	}
	link_raylib(&cmd);

	if (!nob_cmd_run(&cmd)){
		nob_log(NOB_ERROR, "Failed to compile app");
		assert(false);
		return FAILED;
	}

	if (save_binary(&current_config, sizeof(current_config), BUILD_FOLDER CONFIG_FILE_NAME, config_version) == FAILED){
		nob_log(NOB_ERROR, "Failed to save config");
		assert(false);
		return FAILED;
	}

	nob_cmd_free(cmd);

	return SUCCESS;
}
