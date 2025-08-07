// PS Vita Data Files Downloader by Harommel OddSock
// Licensed under GPLv3

#include <vitasdk.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>

#include "dl.h"

typedef struct {
    const char *url;
    const char *path;
} DLInfo;

int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow){
    if (dltotal > 0) {
        double progress = (double)dlnow / (double)dltotal * 100.0;
        sceClibPrintf("Download progress: %.2f%%\n", progress);
    }
    return 0; // Return non-zero to abort the transfer
}

static size_t dl_write(void *ptr, size_t size, size_t nmemb, void *stream){
    size_t written = sceIoWrite(*(int*)stream, ptr, size*nmemb);
    return written;
}

static int download_thread(SceSize args, void *argp){
	DLInfo *info = (DLInfo *)argp;
    SceUID fd = sceIoOpen(info->path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
	if(curl) {
		curl_easy_reset(curl);
		curl_easy_setopt(curl, CURLOPT_URL, info->url);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dl_write);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fd);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);

		curl_easy_perform(curl);
	}
	
	sceIoClose(fd);
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	free(info);
    return sceKernelExitDeleteThread(0);
}

void dl_topath(const char *url, const char *path){
    DLInfo *info = malloc(sizeof(DLInfo));
    info->url = url;
    info->path = path;
	SceUID thid = sceKernelCreateThread("Download Thread", download_thread, 0x10000100, 0x100000, 0, 0, NULL);
	int res = sceKernelStartThread(thid, sizeof(DLInfo), info);
	if(res < 0){
		sceClibPrintf("\"sceKernelStartThread(thid, sizeof(info), info);\" FAILED: 0x%X", res);
		free(info);
	}
}

void dl_topath_nothread(const char *url, const char *path){
	SceUID fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
	if(curl) {
		curl_easy_reset(curl);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dl_write);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fd);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);

		curl_easy_perform(curl);
	}
	
	sceIoClose(fd);
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}