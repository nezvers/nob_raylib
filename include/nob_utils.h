#ifndef NOB_UTILS_H
#define NOB_UTILS_H

// don't include nob.h, asume it is already included before this

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

enum RESULT {
	SUCCESS,
	FAILED,
};

enum PLATFORM_TARGET {
	PLATFORM_DESKTOP,
	PLATFORM_DESKTOP_GLFW,
	PLATFORM_DESKTOP_SDL,
	PLATFORM_DESKTOP_RGFW,
	PLATFORM_WEB,
	PLATFORM_DRM,
	PLATFORM_ANDROID,
};

struct SavedConfig {
	bool is_debug;
	enum PLATFORM_TARGET platform;
	bool enable_wayland;
};

void swap_dir_slashes(char *dir_path, int length){
	for (int i = 0; i < length; ++i){
		if (dir_path[i] == '\0') break;
		if (dir_path[i] == '/') dir_path[i] = '\\';
		else if (dir_path[i] == '\\') dir_path[i] = '/';
	}
}

enum RESULT delete_directory(const char *dir_path){
#if defined(WINDOWS)
	char dir_buffer[1024] = {0};
	snprintf(dir_buffer, sizeof(dir_buffer) / sizeof(dir_buffer[0]), "%s", dir_path);
	swap_dir_slashes(dir_buffer, sizeof(dir_buffer) / sizeof(dir_buffer[0]));
	// explicitly call from cmd.exe otherwise gives error
	if (system(nob_temp_sprintf("cmd.exe /c \"rmdir /s /q %s \"", dir_buffer)) == FAILED){
		nob_log(NOB_ERROR, "Failed to delete a directory: %s", dir_path);
		return FAILED;
	}
#elif defined(LINUX)
	Nob_Cmd rm_cmd = {0};
	nob_cmd_append(&rm_cmd, "rm", "-rf", dir_path, "||", "true");
	if (!nob_cmd_run(&rm_cmd)){
		nob_log(NOB_ERROR, "Failed to delete a directory: %s", dir_path);
		return FAILED;
	}
#endif
	return SUCCESS;
}

void nob_make(Nob_Cmd *cmd){
#if defined(__MINGW32__)
	nob_cmd_append(cmd, "mingw32-make");
#else
	nob_cmd_append(cmd, "make");
#endif
}

enum RESULT save_binary(const void *buffer, size_t size, const char *file_path, int bin_version){
	FILE *file = fopen(file_path, "wb");
    if (!file) {
        nob_log(NOB_ERROR, "Open file to save: %s ", file_path);
    	fclose(file);
        return FAILED;
    }

	if (fwrite(&bin_version, sizeof(bin_version), 1, file) == 0){
        nob_log(NOB_ERROR, "Couldn't save file bin version: %s ", file_path);
    	fclose(file);
		return FAILED;
	}

	if (fwrite(buffer, size, 1, file) == 0){
        nob_log(NOB_ERROR, "Couldn't save file: %s ", file_path);
   		fclose(file);
		return FAILED;
	}
    fclose(file);

	nob_log(NOB_INFO, "Successful save file: %s ", file_path);
	return SUCCESS;
}

enum RESULT load_binary(void *buffer, size_t size, const char *file_path, int bin_version){
	if (!nob_file_exists(file_path)){
		nob_log(NOB_INFO, "File to load doesn't exist: %s", file_path);
		return FAILED;
	}
	FILE *file = fopen(file_path, "rb");
    if (!file) {
        nob_log(NOB_ERROR, "Open file to load: %s ", file_path);
        return FAILED;
    }
	
	int saved_version;
	if (fread(&saved_version, sizeof(saved_version), 1, file) == 0){
		nob_log(NOB_ERROR, "Couldn't load file bin version: %s ", file_path);
    	fclose(file);
        return FAILED;
	}

	if (saved_version != bin_version){
		nob_log(NOB_INFO, "File bin version missmatch (skip loading): %s (%d != %d)", file_path, saved_version, bin_version);
    	fclose(file);
        return FAILED;
	}

	if (fread(buffer, size, 1, file) == 0){
		nob_log(NOB_ERROR, "Couldn't load file: %s ", file_path);
    	fclose(file);
        return FAILED;
	}
    fclose(file);

	nob_log(NOB_INFO, "Successful load file: %s ", file_path);
	return SUCCESS;
}

// Downloads file using curl or fallbacks to wget
enum RESULT download_file(const char *url, const char *dest) {
	char download_cmd[2048] = {0};
	snprintf(download_cmd, sizeof(download_cmd), "curl -fsSL \"%s\" -o \"%s\"", url, dest);
	nob_log(NOB_INFO, "Downloading file: %s", dest);
	if (system(download_cmd) == FAILED) {
		snprintf(download_cmd, sizeof(download_cmd), "wget -q \"%s\" -O \"%s\"", url, dest);
		if (system(download_cmd) == FAILED) {
			nob_log(NOB_ERROR, "Failed to download %s\n", url);
			return FAILED;
		}
	}
	return SUCCESS;
}

enum RESULT extract_tar_archive(const char *archive_path, const char *target_dir, unsigned int strip_lvl){
	char tar_cmd[2048] = {0};
	snprintf(tar_cmd, sizeof(tar_cmd), "tar -xzf \"%s\" -C \"%s\" --strip-components=%d", archive_path, target_dir, strip_lvl);

	if (system(tar_cmd)){
		nob_log(NOB_ERROR, "Failed to extract: %s -> %s\nCMD: %s", archive_path, target_dir, tar_cmd);
		return FAILED;
	}
	return SUCCESS;
}

enum RESULT extract_zip_archive(const char *archive_path, const char *target_dir, unsigned int strip_lvl){
	char zip_cmd[2048] = {0};
	// snprintf(zip_cmd, sizeof(zip_cmd), "powershell -command \"Expand-Archive -Force '%s' '%s'", archive_path, target_dir);
	snprintf(zip_cmd, sizeof(zip_cmd), "tar -xf \"%s\" -C \"%s\" --strip-components=%d", archive_path, target_dir, strip_lvl);

	if (system(zip_cmd)){
		nob_log(NOB_ERROR, "Failed to extract: %s -> %s\nCMD: %s", archive_path, target_dir, zip_cmd);
		return FAILED;
	}
	return SUCCESS;
}

enum RESULT git_clone(const char *git_repo, const char *tag, unsigned int depth, bool recursive, bool single_branch){
    Nob_Cmd git_cmd = {0};
    nob_cmd_append(&git_cmd, "git", "clone");
    if (recursive) nob_cmd_append(&git_cmd, "--recursive");
    if (single_branch) nob_cmd_append(&git_cmd, "--single-branch");
    if (tag != NULL) nob_cmd_append(&git_cmd, "--branch", nob_temp_sprintf("%s", tag));
    if (depth != 0) nob_cmd_append(&git_cmd, "--depth", nob_temp_sprintf("%d", depth));
    nob_cmd_append(&git_cmd, git_repo);
    if (!nob_cmd_run(&git_cmd)) return FAILED;

    return SUCCESS;
}

#endif //NOB_UTILS_H