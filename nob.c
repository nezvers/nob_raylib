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
#define OBJ_FOLDER BUILD_FOLDER "obj/"
#define LIB_FOLDER BUILD_FOLDER "lib/"
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

void link_raylib(Nob_Cmd *cmd){
#if defined(_MSC_VER)
	// TODO: fix reverse directory slash
	nob_cmd_append(cmd, RAYLIB_SRC_DIR, "raylib.lib");
#else
	nob_cmd_append(cmd, "-L", RAYLIB_SRC_DIR, "-lraylib");
#endif

	switch (current_config.platform){
		case (PLATFORM_DESKTOP_SDL):
		case (PLATFORM_DESKTOP_GLFW):
		case (PLATFORM_DESKTOP_RGFW):
	    case (PLATFORM_DESKTOP):
		// TODO: refactor to use the ones that are needed in context
#if defined(WINDOWS)
	#if defined(_MSC_VER)
			nob_cmd_append(cmd, "opengl32.lib", "gdi32.lib", "winmm.lib", "user32.lib", "kernel32.lib", "shell32.lib", "msvcrt.lib");//, "Ole32.lib", "comdlg32.lib");
	#else
			nob_cmd_append(cmd, "-lopengl32", "-lgdi32", "-lwinmm");//, "-lole32");
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

enum RESULT setup_raylib(bool force_rebuild, Nob_Cmd *link_cmd){
	enum RESULT result = SUCCESS;
	Nob_Cmd raylib_cmd = {0};
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
	bool need_rebuild = force_rebuild || (previous_config != NULL && previous_config->platform != current_config.platform);
	// TODO: match check in context of compiler (*.a doesn't work for msvc)
	if (!nob_file_exists(RAYLIB_SRC_DIR "libraylib.a") || need_rebuild){
		nob_log(NOB_INFO, "Changing directory: %s ", RAYLIB_SRC_DIR);
		if (!nob_set_current_dir(nob_temp_sprintf("./%s", RAYLIB_SRC_DIR))){
			nob_log(NOB_ERROR, "Failed to change to directory: %s", RAYLIB_SRC_DIR);
			assert(false);
			nob_return_defer(FAILED);
		}

		nob_cmd_make(&raylib_cmd);
		const char *raylib_platform = get_raylib_platform(current_config.platform);
		nob_cmd_append(&raylib_cmd, raylib_platform, "-j4");
		bool compile_success = nob_cmd_run(&raylib_cmd);

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

	// link_raylib(link_cmd);

defer:
	nob_temp_rewind(temp_checkpoint);
	return result;
}

void get_include_raylib(Nob_Cmd *cmd){
	nob_cmd_append(cmd, "-I", RAYLIB_SRC_DIR);
}

//---------------------------------------------------------------------------------
// Emscripten
enum RESULT setup_emscripten(){
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

enum RESULT setup_web(){
	enum RESULT result = SUCCESS;
	if (setup_emscripten() == FAILED) nob_return_defer(FAILED);

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
#if defined(_MSC_VER)
	nob_cmd_append(cmd, "/I" INCLUDE_FOLDER);
#else
	nob_cmd_append(cmd, "-I", INCLUDE_FOLDER);
#endif
}

void get_defines(Nob_Cmd *cmd){
	// Apple example - https://github.com/ImplodedPotato/C-nob-raylib-template
	// web example & hotreload - https://github.com/angelcaru/raylib-template/blob/master/nob.c
	// reference - https://github.com/OleksiiBulba/c-nob.h-raylib-template
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
		return;
	}
	if (current_config.is_debug){
		// Debug symbols
		nob_cmd_debug(cmd);
		nob_cmd_append(cmd, "-DDEBUG");
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
		nob_cmd_append(cmd, "-DRELEASE");
		// nob_cmd_append(cmd, "-DMODE_PRODUCTION"); // disable adjust.h "passive" hotreload
		nob_cmd_append(cmd, "-D", "RESOURCES_PATH=" RESOURCES_FOLDER);
	}
	nob_cmd_optimize(cmd, current_config.optimize);
}

// TODO: NOT USED
void link_platform(Nob_Cmd *cmd){
	if (current_config.platform == PLATFORM_WEB){

	}
	else{
#if defined(WINDOWS)
	nob_cmd_append(cmd, "-lwinmm", "-lgdi32", "-lole32");
#elif defined(LINUX)
	nob_cmd_append(cmd, "-lm", "-ldl", "-flto=auto", "-lpthread");
#endif
	}
}

enum RESULT compile_plug_template(bool force_rebuild){
	enum RESULT result = SUCCESS;
	size_t temp_checkpoint = nob_temp_save();
	Nob_File_Paths file_list = {0};
	Nob_Cmd plug_obj_cmd = {0};
	Nob_Cmd plug_lib_cmd = {0};

	bool is_shared = true;
	nob_cc_flags(&plug_obj_cmd);
	nob_cmd_optimize(&plug_obj_cmd, current_config.optimize);
	get_include_directories(&plug_obj_cmd);
	if (nob_cmd_process_source_dir(&plug_obj_cmd, "plug_template/", OBJ_FOLDER "plug_template/", ".c", current_config.is_debug, is_shared, force_rebuild) == FAILED){
		nob_log(NOB_ERROR, "Failed building load_library.o");
		assert(false);
		nob_return_defer(FAILED);
	}
	nob_temp_rewind(temp_checkpoint);

	
	temp_checkpoint = nob_temp_save();
	nob_cc(&plug_lib_cmd);
	nob_cmd_input_objects_dir(&plug_lib_cmd, OBJ_FOLDER  "plug_template/", &file_list);
	nob_cmd_output_shared_library(&plug_lib_cmd, "plug_template", BUILD_FOLDER, current_config.is_debug);
	
	#if defined(WINDOWS)
	nob_cmd_append(&plug_lib_cmd, "-Wl,--out-implib,"LIB_FOLDER"/libplug_template.a");
	nob_cmd_append(&plug_lib_cmd, "-lkernel32");
#else

#endif

	if (!nob_cmd_run(&plug_lib_cmd)){
		nob_log(NOB_ERROR, "Failed building load_library.a");
		assert(false);
		nob_return_defer(FAILED);
	}


defer:
	nob_cmd_free(plug_obj_cmd);
	nob_cmd_free(plug_lib_cmd);
	nob_da_free(file_list);
	nob_temp_rewind(temp_checkpoint);
	return result;
}


enum RESULT compile_load_library(bool force_rebuild, Nob_Cmd *link_cmd){
	enum RESULT result = SUCCESS;
	size_t temp_checkpoint = nob_temp_save();
	Nob_File_Paths file_list = {0};
	Nob_Cmd load_lib_obj_cmd = {0};
	Nob_Cmd load_lib_cmd = {0};

	bool is_shared = false;

	// Object files
	nob_cc_flags(&load_lib_obj_cmd);
	nob_cmd_optimize(&load_lib_obj_cmd, current_config.optimize);
	get_include_directories(&load_lib_obj_cmd);
	if (nob_cmd_process_source_dir(&load_lib_obj_cmd, SOURCE_FOLDER "load_library/", OBJ_FOLDER "load_library/", ".c", current_config.is_debug, is_shared, force_rebuild) == FAILED){
		nob_log(NOB_ERROR, "Failed building load_library.o");
		assert(false);
		nob_return_defer(FAILED);
	}
	nob_temp_rewind(temp_checkpoint);

	// static lib
	temp_checkpoint = nob_temp_save();
	nob_cmd_new_static_library(&load_lib_cmd, "load_library", LIB_FOLDER);
	nob_cmd_input_objects_dir(&load_lib_cmd, OBJ_FOLDER  "load_library/", &file_list);
	if (!nob_cmd_run(&load_lib_cmd)){
		nob_log(NOB_ERROR, "Failed building load_library.a");
		assert(false);
		nob_return_defer(FAILED);
	}
	
	// NOTE: Can't use temp strings
#if _MSC_VER
	nob_cmd_append(link_cmd, LIB_FOLDER "libload_library.lib");
	nob_cmd_append(link_cmd, "Kernel32.lib");
#else
	nob_cmd_append(link_cmd, "-L" LIB_FOLDER);
	nob_cmd_append(link_cmd, "-lload_library");
	#if defined(WINDOWS)
		nob_cmd_append(link_cmd, "-lkernel32");
	#else
		nob_cmd_append(link_cmd, "-ldl");
	#endif
#endif

defer:
	nob_cmd_free(load_lib_obj_cmd);
	nob_cmd_free(load_lib_cmd);
	nob_da_free(file_list);
	nob_temp_rewind(temp_checkpoint);
	return result;
}

enum RESULT compile_main(bool force_rebuild, Nob_Cmd *link_cmd){
	enum RESULT result = SUCCESS;
	size_t temp_checkpoint = nob_temp_save();
	Nob_File_Paths file_list = {0};
	Nob_Cmd main_obj_cmd = {0};
	Nob_Cmd main_cmd = {0};
	
	bool is_shared = false;
	
	nob_cc_flags(&main_obj_cmd);
	get_defines(&main_obj_cmd);
	get_include_raylib(&main_obj_cmd);
	get_include_directories(&main_obj_cmd);
	nob_cmd_optimize(&main_obj_cmd, current_config.optimize);
	if (nob_cmd_process_source_dir(&main_obj_cmd, SOURCE_FOLDER, OBJ_FOLDER "main/", ".c", current_config.is_debug, is_shared, force_rebuild) == FAILED){
		nob_log(NOB_ERROR, "Failed building main objects");
		assert(false);
		nob_return_defer(FAILED);
	}
	nob_temp_rewind(temp_checkpoint);
	

	nob_cc(&main_cmd);
	// Place inside build folder
	nob_cc_output(&main_cmd, nob_temp_sprintf("%s%s", BUILD_FOLDER, project_name));

	nob_cmd_input_objects_dir(&main_cmd, OBJ_FOLDER  "main/", &file_list);
	nob_cmd_append_cmd(&main_cmd, link_cmd);
	link_raylib(&main_cmd);
	
	if (!nob_cmd_run(&main_cmd)){
		nob_log(NOB_ERROR, "Failed to compile app");
		assert(false);
		nob_return_defer(FAILED);
	}

defer:
	nob_cmd_free(main_obj_cmd);
	nob_cmd_free(main_cmd);
	nob_da_free(file_list);
	nob_temp_rewind(temp_checkpoint);
	return result;
}

enum RESULT compile_project(){
	enum RESULT result = SUCCESS;
	size_t temp_checkpoint = nob_temp_save();
	// Append only constant commands used at the end for main executable compilation
	Nob_Cmd link_cmd = {0};
	
	// TODO: force_rebuild for specific modules through nob arguments
	bool force_rebuild = false;
	if (compile_load_library(force_rebuild, &link_cmd) == FAILED){
		nob_log(NOB_ERROR, "Failed to compile load_library.");
		assert(false);
		nob_return_defer(FAILED);
	}
	
	if (setup_raylib(force_rebuild, &link_cmd) == FAILED){
		nob_log(NOB_ERROR, "Failed to setup RAYLIB.");
		assert(false);
		nob_return_defer(FAILED);
	}

	if (compile_main(force_rebuild, &link_cmd) == FAILED){
		nob_log(NOB_ERROR, "Failed to compile main module.");
		assert(false);
		nob_return_defer(FAILED);
	}

	if (compile_plug_template(force_rebuild) == FAILED){
		nob_log(NOB_ERROR, "Failed to compile plug template.");
		assert(false);
		nob_return_defer(FAILED);
	}

defer:
	nob_temp_rewind(temp_checkpoint);
	nob_cmd_free(link_cmd);
	return result;
}

int main(int argc, char **argv){
	NOB_GO_REBUILD_URSELF(argc, argv);
	enum RESULT result = SUCCESS;
	size_t temp_checkpoint = nob_temp_save();
	char root_dir[1024] = {0};
	snprintf(starting_cwd, sizeof(starting_cwd), "%s", nob_get_current_dir_temp());
	nob_temp_rewind(temp_checkpoint);

	// Set CWD to the project's root directory
	const char *program_path = nob_shift(argv, argc);
	get_directory_path(root_dir, sizeof(root_dir), program_path);
	if (!nob_set_current_dir(root_dir)){
		nob_return_defer(FAILED);
	}

	// CLI
	while (argc > 0){
		const char *command_name = nob_shift(argv, argc);
		if (strcmp(command_name, "-debug") == 0){
			current_config.is_debug = true;
		}
		else if (strcmp(command_name, "-name") == 0){
			if (!(argc > 0)){
				nob_log(NOB_ERROR, "No project name provided after `-name`");
				assert(false);
				nob_return_defer(FAILED);
			}
			project_name = nob_shift(argv, argc);
		}
		else if (strcmp(command_name, "-platform") == 0){
			if (!(argc > 0)){
				nob_log(NOB_ERROR, "No target platform provided after `-platform`");
				assert(false);
				nob_return_defer(FAILED);
			}
			const char *platform = nob_shift(argv, argc);
			if (strcmp(platform, "web") == 0){
				current_config.platform = PLATFORM_WEB;
			}
		}
		else if (strcmp(command_name, "-optimize") == 0){
			if (!(argc > 0)){
				nob_log(NOB_ERROR, "No optimization option provided after `-optimize`");
				assert(false);
				nob_return_defer(FAILED);
			}
			const char *optimize = nob_shift(argv, argc);
			if (strcmp(optimize, "debug") == 0){
				current_config.optimize = OPTIMIZATION_DEBUG;
			}
			else if (strcmp(optimize, "release") == 0){
				current_config.optimize = OPTIMIZATION_RELEASE;
			}
			else if (strcmp(optimize, "size") == 0){
				current_config.optimize = OPTIMIZATION_SIZE;
			}
			else if (strcmp(optimize, "speed") == 0){
				current_config.optimize = OPTIMIZATION_SPEED;
			}
			else if (strcmp(optimize, "aggressive") == 0){
				current_config.optimize = OPTIMIZATION_AGGRESSIVE;
				// NOTE: MSVC needs to link "/LTCG"
			}
		}
	}

	if (!nob_mkdir_if_not_exists(DOWNLOAD_FOLDER)) nob_return_defer(FAILED);
	if (!nob_mkdir_if_not_exists(DEPENDENCY_FOLDER)) nob_return_defer(FAILED);
	if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) nob_return_defer(FAILED);
	if (!nob_mkdir_if_not_exists(OBJ_FOLDER)) nob_return_defer(FAILED);
	if (!nob_mkdir_if_not_exists(LIB_FOLDER)) nob_return_defer(FAILED);

	struct SavedConfig saved_config = {0};
	if (load_binary(&saved_config, sizeof(saved_config), BUILD_FOLDER CONFIG_FILE_NAME, config_version) == 0){
		previous_config = &saved_config;
	}
	// TODO: move to compile project
	if (current_config.platform == PLATFORM_WEB && setup_web() == FAILED){
		nob_log(NOB_ERROR, "Failed to setup web.");
		assert(false);
		nob_return_defer(FAILED);
	}

	// Compile project
	if (compile_project()){
		nob_log(NOB_ERROR, "Failed to get source files");
		assert(false);
		nob_return_defer(FAILED);
	}

	if (save_binary(&current_config, sizeof(current_config), BUILD_FOLDER CONFIG_FILE_NAME, config_version) == FAILED){
		nob_log(NOB_ERROR, "Failed to save config");
		assert(false);
		nob_return_defer(FAILED);
	}

defer:
	nob_set_current_dir(starting_cwd);
	return result;
}
