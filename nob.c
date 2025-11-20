#define NOB_IMPLEMENTATION
// #define NOB_STRIP_PREFIX
#define NOB_WARN_DEPRECATED
#include "include/nob.h" // https://github.com/tsoding/nob.h
#include <stdio.h>
#include <string.h>

#define PROJECT_NAME "nob_raylib"

#define SOURCE_FOLDER "src/"
#define INCLUDE_FOLDER "include/"
#define BUILD_FOLDER "build/"
#define BIN_FOLDER BUILD_FOLDER "bin/"
#define WEB_FOLDER "web_build"
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

static bool is_debug;
static const char *project_name = PROJECT_NAME;

enum PLATFORM_TARGET {
	PLATFORM_DESKTOP,
	PLATFORM_DESKTOP_GLFW,
	PLATFORM_DESKTOP_SDL,
	PLATFORM_DESKTOP_RGFW,
	PLATFORM_WEB,
	PLATFORM_DRM,
	PLATFORM_ANDROID,
};

// For tracking last build settings that needs to be rebuild
static int config_version = 1;
struct SavedConfig {
	enum PLATFORM_TARGET platform;
	bool enable_wayland;
};

// It is set by build input arguments
static struct SavedConfig current_config = {
	PLATFORM_DESKTOP,
	false,
};
// If successfully loaded config it will point to the data
static struct SavedConfig *previous_config = NULL;

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

int save_binary(const void *buffer, size_t size, const char *file_path, int bin_version){
	FILE *file = fopen(file_path, "wb");
    if (!file) {
        nob_log(NOB_ERROR, "[ERROR] Open file to save: %s ", file_path);
    	fclose(file);
        return 1;
    }

	if (fwrite(&bin_version, sizeof(bin_version), 1, file) == 0){
        nob_log(NOB_ERROR, "[ERROR] Couldn't save file bin version: %s ", file_path);
    	fclose(file);
		return 1;
	}

	if (fwrite(buffer, size, 1, file) == 0){
        nob_log(NOB_ERROR, "[ERROR] Couldn't save file: %s ", file_path);
   		fclose(file);
		return 1;
	}
    fclose(file);

	nob_log(NOB_INFO, "[INFO] Successful save file: %s ", file_path);
	return 0;
}

int load_binary(void *buffer, size_t size, const char *file_path, int bin_version){
	if (!nob_file_exists(file_path)){
		nob_log(NOB_INFO, "[INFO] File to load doesn't exist: %s", file_path);
		return 1;
	}
	FILE *file = fopen(file_path, "rb");
    if (!file) {
        nob_log(NOB_ERROR, "[ERROR] Open file to load: %s ", file_path);
        return 1;
    }
	
	int saved_version;
	if (fread(&saved_version, sizeof(saved_version), 1, file) == 0){
		nob_log(NOB_ERROR, "[ERROR] Couldn't load file bin version: %s ", file_path);
    	fclose(file);
        return 1;
	}

	if (saved_version != bin_version){
		nob_log(NOB_INFO, "[INFO] File bin version missmatch (skip loading): %s (%d != %d)", file_path, saved_version, bin_version);
    	fclose(file);
        return 1;
	}

	if (fread(buffer, size, 1, file) == 0){
		nob_log(NOB_ERROR, "[ERROR] Couldn't load file: %s ", file_path);
    	fclose(file);
        return 1;
	}
    fclose(file);

	nob_log(NOB_INFO, "[INFO] Successful load file: %s ", file_path);
	return 0;
}

// Downloads file using curl or fallbacks to wget
void download_file(const char *url, const char *dest) {
	char download_cmd[2048] = {0};
	snprintf(download_cmd, sizeof(download_cmd), "curl -fsSL \"%s\" -o \"%s\"", url, dest);
	if (system(download_cmd) != 0) {
		snprintf(download_cmd, sizeof(download_cmd), "wget -q \"%s\" -O \"%s\"", url, dest);
		if (system(download_cmd) != 0) {
			nob_log(NOB_ERROR, "Failed to download %s\n", url);
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
	bool need_rebuild = previous_config != NULL && previous_config->platform != current_config.platform;
	if (!nob_file_exists(RAYLIB_SRC_DIR "libraylib.a") || need_rebuild){
		nob_log(NOB_INFO, "Changing directory: %s ", RAYLIB_SRC_DIR);
		if (!nob_set_current_dir(nob_temp_sprintf("./%s", RAYLIB_SRC_DIR))){
			nob_log(NOB_ERROR, "Failed to change to directory: %s", RAYLIB_SRC_DIR);
			return 1;
		}

		nob_make(cmd);
		const char *raylib_platform = get_raylib_platform(current_config.platform);
		if (current_config.platform == PLATFORM_WEB){
			Nob_Cmd emsdk_cmd = {0};
			nob_cmd_append(&emsdk_cmd, "emsdk", "activate", "latest");
			if (!nob_cmd_run(&emsdk_cmd)){
				nob_log(NOB_ERROR, "Failed to activate latest emscriptem");
				return 1;
			}
			nob_cmd_append(&emsdk_cmd, "emsdk_env");
			if (!nob_cmd_run(&emsdk_cmd)){
				nob_log(NOB_ERROR, "Failed to initiate emscriptem environment");
				return 1;
			}
		}
		nob_cmd_append(cmd, raylib_platform);
		if (!nob_cmd_run(cmd)){
			nob_log(NOB_ERROR, "Failed to compile raylib");
			return 1;
		}

		nob_log(NOB_INFO, "Changing directory: %s ", "../../../");
		if (!nob_set_current_dir("../../../")){
			nob_log(NOB_ERROR, "Failed to move bact to root directory");
			return 1;
		}
	}

	if (!nob_file_exists(RAYLIB_SRC_DIR "libraylib.a")){
		nob_log(NOB_ERROR, "libraylib.a disappeared!!!");
		return 1;
	}

	return 0;
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

void get_include_directories(Nob_Cmd *cmd){
	nob_cmd_append(cmd, "-I", INCLUDE_FOLDER);
}

void get_defines(Nob_Cmd *cmd){
	if (is_debug){
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

int setup_web(Nob_Cmd *cmd){
	if (!nob_mkdir_if_not_exists(WEB_FOLDER)) return 1;
	if (nob_file_exists(WEB_FOLDER RESOURCES_FOLDER)){
		if (!nob_delete_file(WEB_FOLDER RESOURCES_FOLDER)){
			return 1;
		}
	}
	if (!nob_mkdir_if_not_exists(WEB_FOLDER RESOURCES_FOLDER)) return 1;
	if (!nob_copy_directory_recursively(RESOURCES_FOLDER, WEB_FOLDER RESOURCES_FOLDER)){
		return 1;
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
		else if (strcmp(command_name, "-platform") == 0){
			if (!(argc > 0)){
				nob_log(NOB_ERROR, "[ERROR] No project name provided after `project`");
				return 1;
			}
			const char *platform = nob_shift(argv, argc);
			if (strcmp(platform, "web") == 0){
				current_config.platform = PLATFORM_WEB;
			}
		}
	}

	if (!nob_mkdir_if_not_exists(DOWNLOAD_FOLDER)) return 1;
	if (!nob_mkdir_if_not_exists(DEPENDENCY_FOLDER)) return 1;
	if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;
	if (!nob_mkdir_if_not_exists(BIN_FOLDER)) return 1;

	struct SavedConfig saved_config = {0};
	if (load_binary(&saved_config, sizeof(saved_config), BUILD_FOLDER CONFIG_FILE_NAME, config_version) == 0){
		previous_config = &saved_config;
	}

	Nob_Cmd cmd = {0};
	if (setup_raylib(&cmd) != 0){
		return 1;
	}

	if (current_config.platform == PLATFORM_WEB && setup_web(&cmd) != 0){
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

	if (save_binary(&current_config, sizeof(current_config), BUILD_FOLDER CONFIG_FILE_NAME, config_version) != 0){
		return 1;
	}

	return 0;
}
