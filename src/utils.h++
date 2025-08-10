// PS Vita Data Files Downloader by Harommel OddSock
// Licensed under GPLv3

#pragma once

#include <vitasdk.h>
#include <stdbool.h>
#include <stdio.h>
#include <archive.h>
#include <archive_entry.h>
#include "include/makeheadbin.h"

static void loadPromoter() {
  	static uint32_t argp[] = { 0x180000, -1, -1, 1, -1, -1 };

  	int result = -1;

  	uint32_t buf[4];
  	buf[0] = sizeof(buf);
  	buf[1] = (uint32_t)&result;
  	buf[2] = -1;
  	buf[3] = -1;

  	sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(argp), argp, buf);
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
	scePromoterUtilityInit();
}

static void unloadPromoter() {
	scePromoterUtilityExit();
	sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
  	uint32_t buf = 0;
  	sceSysmoduleUnloadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, 0, NULL, &buf);
}

int recursive_folders(const char* path) {
	if(file_exists(path)) return false;
	
	char dirname[512];
	memset(dirname, 0x00, sizeof(dirname));
	
	for(int i = 0; i < strlen(path); i++) {
		if(path[i] == '/' || path[i] == '\\') {
			memset(dirname, 0, sizeof(dirname));
			strncpy(dirname, path, i);

			if(!file_exists(dirname)){
				sceIoMkdir(dirname, 0777);
			}	
		}
	}
	
	return sceIoMkdir(path, 0777);
}

namespace Utils{
	bool FileOrPathExists(const char *path) {
		SceIoStat stat;
		int res = sceIoGetstat(path, &stat);
		return res >= 0;
	}
	bool ArchiveExtract(const char *archive_path, const char *out_path) {
	    struct archive *a = archive_read_new();
	    struct archive_entry *entry;
	    archive_read_support_format_all(a);
	    archive_read_support_filter_all(a);

	    if (archive_read_open_filename(a, archive_path, 10240) != ARCHIVE_OK) {
	        archive_read_free(a);
	        return false; //archive_error_string(a));
	    }

	    int file_index = 1;

	    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {

	        // Build full output path
	        const char *entry_path = archive_entry_pathname(entry);
	        char full_path[512];
	        snprintf(full_path, sizeof(full_path), "%s/%s", out_path, entry_path);

	        // Ensure directory structure
	        char *last_slash = strrchr(full_path, '/');
	        if (last_slash) {
	            *last_slash = '\0';
	            recursive_folders(full_path);
	            *last_slash = '/';
	        }

	        // Skip directories
	        if (archive_entry_filetype(entry) == AE_IFDIR) {
	            archive_read_data_skip(a);
	            continue;
	        }

	        SceUID fd = sceIoOpen(full_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	        if (fd < 0) {
	            archive_read_free(a);
	            return false;
	        }

	        char buffer[4096];
	        ssize_t size;
	        while ((size = archive_read_data(a, buffer, sizeof(buffer))) > 0) {
	            sceIoWrite(fd, buffer, size);
	        }

	        sceIoClose(fd);
	    }

	    archive_read_close(a);
	    archive_read_free(a);
		return true;
	}
	bool AppExists(const char *titleid){
		loadPromoter();
		int res;
		bool exists = !scePromoterUtilityCheckExist(titleid, &res);
		unloadPromoter();
		return exists;
	}
	bool AppInstall(const char *path, bool headbin = true){
		loadPromoter();
    	if(!headbin) makeHeadBin(path);
		scePromoterUtilityPromotePkgWithRif(path, 1);

		int state = 0;
		do {
			int ret = scePromoterUtilityGetState(&state);
			if (ret < 0)
				break;
			sceKernelDelayThread(150 * 1000);
		} while (state);
		unloadPromoter();
	}
}