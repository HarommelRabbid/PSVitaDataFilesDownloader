// PS Vita Data Files Downloader by Harommel OddSock
// Licensed under GPLv3

#include <vitasdk.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "dl.h"

typedef struct {
    char *url;
    char *path;
} DLInfo;

double dl_progress = 0;
bool is_dl = false;
static DLInfo *dl_info = NULL;
bool is_dl_thd = false;

int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow){
    if (dltotal > 0) {
        dl_progress = (double)dlnow / (double)dltotal * 100.0;
        sceClibPrintf("Download progress: %.2f%%\n", dl_progress);
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
    CURL *curl = curl_easy_init();
	if(curl) {
		is_dl = true;
		is_dl_thd = true;
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
	//free(info->path);
	//free(info->url);
	//free(info);
	dl_progress = 0;
	is_dl = false;
	is_dl_thd = false;
    return sceKernelExitDeleteThread(0);
}

void check_dl_thd_for_free(){
	if(dl_info != NULL && !is_dl && !is_dl_thd){
		free(dl_info->path);
		free(dl_info->url);
		free(dl_info);
		dl_info = NULL;
	}
}

void dl_topath(const char *url, const char *path){
    dl_info = malloc(sizeof(DLInfo));
    dl_info->url = strdup(url);
    dl_info->path = strdup(path);
	SceUID thid = sceKernelCreateThread("Download Thread", download_thread, 0x10000100, 0x100000, 0, 0, NULL);
	int res = sceKernelStartThread(thid, sizeof(DLInfo), dl_info);
	if(res < 0){
		sceClibPrintf("\"sceKernelStartThread(thid, sizeof(DLInfo), dl_info);\" FAILED: 0x%X\n", res);
		free(dl_info->path);
		free(dl_info->url);
		free(dl_info);
		dl_info = NULL;
	}
}

void dl_topath_nothread(const char *url, const char *path){
	SceUID fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    CURL *curl = curl_easy_init();
	if(curl) {
		is_dl = true;
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
	dl_progress = 0;
	is_dl = false;
}