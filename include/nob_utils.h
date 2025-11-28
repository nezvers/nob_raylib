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
	enum RESULT result = SUCCESS;
	size_t temp_checkpoint = nob_temp_save();
#if defined(WINDOWS)
	char dir_buffer[1024] = {0};
	snprintf(dir_buffer, sizeof(dir_buffer) / sizeof(dir_buffer[0]), "%s", dir_path);
	swap_dir_slashes(dir_buffer, sizeof(dir_buffer) / sizeof(dir_buffer[0]));
	// explicitly call from cmd.exe otherwise gives error
	if (system(nob_temp_sprintf("cmd.exe /c \"rmdir /s /q %s \"", dir_buffer)) == FAILED){
		nob_log(NOB_ERROR, "Failed to delete a directory: %s", dir_path);
		assert(false);
		nob_return_defer(FAILED);
	}
#elif defined(LINUX)
	Nob_Cmd rm_cmd = {0};
	nob_cmd_append(&rm_cmd, "rm", "-rf", dir_path, "||", "true");
	if (!nob_cmd_run(&rm_cmd)){
		nob_log(NOB_ERROR, "Failed to delete a directory: %s", dir_path);
		assert(false);
		nob_return_defer(FAILED);
	}
#endif
defer:
	nob_temp_rewind(temp_checkpoint);
	return result;
}

enum RESULT nob_fetch_files(const char *dir_path, Nob_File_Paths *file_list, const char *extension){
	enum RESULT result = SUCCESS;
	Nob_File_Type type = nob_get_file_type(dir_path);
	size_t temp_checkpoint = nob_temp_save();
	if (type < 0) nob_return_defer(FAILED);
	if (type != NOB_FILE_DIRECTORY) nob_return_defer(FAILED);
	
	Nob_File_Paths children = {0};
	if (!nob_read_entire_dir(dir_path, &children)) nob_return_defer(FAILED);
	
	for (size_t i = 0; i < children.count; ++i){
		if (extension == NULL){
			nob_da_append(file_list, children.items[i]);
		}
		else if (nob_sv_end_with(nob_sv_from_cstr(children.items[i]), extension)){
			nob_da_append(file_list, children.items[i]);
		}
	}

defer:
	nob_temp_rewind(temp_checkpoint);
	nob_da_free(children);
	return result;
}

Nob_String_View get_file_name_no_extension(const char *file_path){
	Nob_String_View sv = nob_sv_from_cstr(file_path);
	unsigned int last_slash = 0;
	unsigned int last_dot = 0;
	for (unsigned i = 0; i < sv.count; ++i){
		if (file_path[i] == '/') last_slash = i;
		if (file_path[i] == '\\') last_slash = i;
		if (file_path[i] == '.') last_dot = i;
	}

	sv.data = file_path + last_slash;
	if (last_dot > last_slash){
		sv.count = last_dot - last_slash;
	}
	return sv;
}

char *nob_temp_cstr_from_string_view(Nob_String_View *sv){
	char *result = (char*)nob_temp_alloc(sv->count + 1);
	if (result == NULL) return result;
	memcpy(result, sv->data, sv->count);
	result[sv->count] = '\0';
	return result;
}

void nob_cmd_make(Nob_Cmd *cmd){
#if defined(__MINGW32__)
	nob_cmd_append(cmd, "mingw32-make");
#else
	nob_cmd_append(cmd, "make");
#endif
}

void nob_cmd_output_shared_object(Nob_Cmd *cmd, const char *src_path, const char *bin_path, bool debug){
#if defined(_MSC_VER)
	// TODO: add missing MSVC flags
#else
	if (debug) nob_cmd_append(cmd, "-g");
	nob_cmd_append(cmd, "-c", src_path);
	nob_cmd_append(cmd, "-fpic");
	nob_cmd_append(cmd, "-o", bin_path);
#endif
}

void nob_cmd_output_shared_library(Nob_Cmd *cmd, const char *name, const char *directory, bool debug){
	size_t temp_checkpoint = nob_temp_save();
#if defined(WINDOWS)
	#if defined(_MSC_VER)
		// TODO: add missing MSVC flags
		if (debug) nob_cmd_append(cmd, "/LDd");
		else nob_cmd_append(cmd, "/LD");

		nob_cmd_append(cmd, nob_temp_sprintf("/F%s", directory));
		nob_cmd_append(cmd, nob_temp_sprintf("/Fe%slib%s.dll", directory, name));
	#else
		if (debug) nob_cmd_append(cmd, "-g");
		nob_cmd_append(cmd, "-shared", "-o");
		nob_cmd_append(cmd, nob_temp_sprintf("%slib%s.dll", directory, name));
		nob_cmd_append(cmd, "-Wl", "--out-implib");
	#endif
#elif defined(LINUX)
	if (debug) nob_cmd_append(cmd, "-g");
	nob_cmd_append(cmd, "-shared", "-o");
	nob_cmd_append(cmd, nob_temp_sprintf("%slib%s.so", directory, name));
#endif
	nob_temp_rewind(temp_checkpoint);
}

void nob_cmd_output_static_library(Nob_Cmd *cmd, const char *name, const char *directory, bool debug){
	size_t temp_checkpoint = nob_temp_save();
#if defined(WINDOWS)
	#if defined(_MSC_VER)
		// TODO: add missing MSVC flags
		
		nob_cmd_append(cmd, nob_temp_sprintf("/F%s", directory));
		nob_cmd_append(cmd, nob_temp_sprintf("/Fe%slib%s.lib", directory, name));
	#else
		nob_cmd_append(cmd, "ar", "rcs");
		nob_cmd_append(cmd, nob_temp_sprintf("%slib%s.a", directory, name));
		if (debug) nob_cmd_append(cmd, "-g");
	#endif
#elif defined(LINUX)
	nob_cmd_append(cmd, "ar", "rcs");
	nob_cmd_append(cmd, nob_temp_sprintf("%slib%s.a", directory, name));
	if (debug) nob_cmd_append(cmd, "-g");
#endif
	nob_temp_rewind(temp_checkpoint);
}

void nob_cmd_append_cmd(Nob_Cmd *target, Nob_Cmd *source){
	for (int i = 0; i < source->count; ++i){
		const char *item = source->items[i];
		nob_cmd_append(target, item);
	}
}

enum RESULT nob_cmd_process_source_dir(Nob_Cmd *cmd, Nob_Cmd *item_cmd, const char *source_dir, const char *output_dir, const char *src_extension, bool force_rebuild){
	enum RESULT result = SUCCESS;
	Nob_Cmd obj_cmd = {0};
	Nob_Procs procs = {0};
	Nob_File_Paths file_list = {0};
	int rebuild_is_needed;
	size_t temp_checkpoint = nob_temp_save();

	if (nob_fetch_files(source_dir, &file_list, src_extension) == FAILED){
		assert(false);
		nob_return_defer(FAILED);
	}

	const char *src_name;
	const char *src_file_path;
	const char *bin_path;
	Nob_String_View src_file;
	size_t temp_obj_checkpoint;
	for (int i = 0; i < file_list.count; ++i){
		// TODO: handle temp memory with many source files
		src_file = get_file_name_no_extension(file_list.items[i]);
		src_name = nob_temp_cstr_from_string_view(&src_file);
		src_file_path = nob_temp_sprintf("%s%s%s", source_dir, src_name, src_extension);
		bin_path = nob_temp_sprintf("%s%s.o", output_dir, src_name); // <- probably need dedicated buffer for each library
		rebuild_is_needed = nob_needs_rebuild1(bin_path, src_file_path);
		if (rebuild_is_needed < 0) nob_return_defer(FAILED);
		if (rebuild_is_needed == 0 && !force_rebuild) continue;
		
		nob_cc(&obj_cmd);
		nob_cmd_append(&obj_cmd, "-c", src_file_path);
		nob_cmd_append(&obj_cmd, "-o", bin_path);
		nob_cmd_append_cmd(&obj_cmd, item_cmd);
		if (!nob_cmd_run(&obj_cmd, .async = &procs)){
			nob_log(NOB_ERROR, "Appending build to queue failed: %s%s%s -> %s%s%s", source_dir, src_name, src_extension, output_dir, src_name, src_extension);
			assert(false);
			nob_return_defer(FAILED);
		}
		nob_cc_inputs(cmd, bin_path); // <- Potentially lost after freeing temp memory
	}

	// Wait on all the async processes to finish and reset procs dynamic array to 0
	if (!nob_procs_flush(&procs)){
		nob_log(NOB_ERROR, "Parallel source build failed dir: %s -> %s", source_dir, output_dir);
		assert(false);
		nob_return_defer(FAILED);
	}

defer:
	nob_temp_rewind(temp_checkpoint);
	nob_cmd_free(obj_cmd);
	nob_da_free(procs);
	nob_da_free(file_list);
	return result;
}

enum RESULT save_binary(const void *buffer, size_t size, const char *file_path, int bin_version){
	enum RESULT result = SUCCESS;
	FILE *file = fopen(file_path, "wb");
    if (!file) {
        nob_log(NOB_ERROR, "Open file to save: %s ", file_path);
		assert(false);
        nob_return_defer(FAILED);
    }

	if (fwrite(&bin_version, sizeof(bin_version), 1, file) == 0){
        nob_log(NOB_ERROR, "Couldn't save file bin version: %s ", file_path);
		assert(false);
    	nob_return_defer(FAILED);
	}

	if (fwrite(buffer, size, 1, file) == 0){
        nob_log(NOB_ERROR, "Couldn't save file: %s ", file_path);
		assert(false);
   		nob_return_defer(FAILED);
	}

	nob_log(NOB_INFO, "Successful save file: %s ", file_path);
defer:
	if (file != NULL) fclose(file);
	return result;
}

enum RESULT load_binary(void *buffer, size_t size, const char *file_path, int bin_version){
	enum RESULT result = SUCCESS;
	if (!nob_file_exists(file_path)){
		nob_log(NOB_INFO, "File to load doesn't exist: %s", file_path);
		nob_return_defer(FAILED);
	}
	FILE *file = fopen(file_path, "rb");
    if (!file) {
        nob_log(NOB_ERROR, "Open file to load: %s ", file_path);
		assert(false);
        nob_return_defer(FAILED);
    }
	
	int saved_version;
	if (fread(&saved_version, sizeof(saved_version), 1, file) == 0){
		nob_log(NOB_ERROR, "Couldn't load file bin version: %s ", file_path);
		assert(false);
    	nob_return_defer(FAILED);
	}

	if (saved_version != bin_version){
		nob_log(NOB_INFO, "File bin version missmatch (skip loading): %s (%d != %d)", file_path, saved_version, bin_version);
		assert(false);
    	nob_return_defer(FAILED);
	}

	if (fread(buffer, size, 1, file) == 0){
		nob_log(NOB_ERROR, "Couldn't load file: %s ", file_path);
		assert(false);
    	nob_return_defer(FAILED);
	}

	nob_log(NOB_INFO, "Successful load file: %s ", file_path);
defer:
	if (file != NULL) fclose(file);
	return result;
}

// Downloads file using curl or fallbacks to wget
enum RESULT download_file(const char *url, const char *dest) {
	enum RESULT result = SUCCESS;
	char download_cmd[2048] = {0};
	snprintf(download_cmd, sizeof(download_cmd), "curl -fsSL \"%s\" -o \"%s\"", url, dest);
	nob_log(NOB_INFO, "Downloading file: %s", dest);
	if (system(download_cmd) == FAILED) {
		snprintf(download_cmd, sizeof(download_cmd), "wget -q \"%s\" -O \"%s\"", url, dest);
		if (system(download_cmd) == FAILED) {
			nob_log(NOB_ERROR, "Failed to download %s\n", url);
			assert(false);
			nob_return_defer(FAILED);
		}
	}
defer:
	return result;
}

enum RESULT extract_tar_archive(const char *archive_path, const char *target_dir, unsigned int strip_lvl){
	enum RESULT result = SUCCESS;
	char tar_cmd[2048] = {0};
	snprintf(tar_cmd, sizeof(tar_cmd), "tar -xzf \"%s\" -C \"%s\" --strip-components=%d", archive_path, target_dir, strip_lvl);

	if (system(tar_cmd) != 0){
		nob_log(NOB_ERROR, "Failed to extract: %s -> %s\nCMD: %s", archive_path, target_dir, tar_cmd);
		assert(false);
		nob_return_defer(FAILED);
	}
defer:
	return result;
}

enum RESULT extract_zip_archive(const char *archive_path, const char *target_dir, unsigned int strip_lvl){
	enum RESULT result = SUCCESS;
	char zip_cmd[2048] = {0};
	// snprintf(zip_cmd, sizeof(zip_cmd), "powershell -command \"Expand-Archive -Force '%s' '%s'", archive_path, target_dir);
	snprintf(zip_cmd, sizeof(zip_cmd), "tar -xf \"%s\" -C \"%s\" --strip-components=%d", archive_path, target_dir, strip_lvl);

	if (system(zip_cmd)){
		nob_log(NOB_ERROR, "Failed to extract: %s -> %s\nCMD: %s", archive_path, target_dir, zip_cmd);
		assert(false);
		nob_return_defer(FAILED);
	}
defer:
	return result;
}

enum RESULT git_clone(const char *git_repo, const char *tag, unsigned int depth, bool recursive, bool single_branch){
	size_t temp_checkpoint = nob_temp_save();
	enum RESULT result = SUCCESS;
    Nob_Cmd git_cmd = {0};
    nob_cmd_append(&git_cmd, "git", "clone");
    if (recursive) nob_cmd_append(&git_cmd, "--recursive");
    if (single_branch) nob_cmd_append(&git_cmd, "--single-branch");
    if (tag != NULL) nob_cmd_append(&git_cmd, "--branch", nob_temp_sprintf("%s", tag));
    if (depth != 0) nob_cmd_append(&git_cmd, "--depth", nob_temp_sprintf("%d", depth));
    nob_cmd_append(&git_cmd, git_repo);
    if (!nob_cmd_run(&git_cmd)){
		assert(false);
		nob_return_defer(FAILED);
	}

defer:
	nob_temp_rewind(temp_checkpoint);
	return result;
}

#endif //NOB_UTILS_H