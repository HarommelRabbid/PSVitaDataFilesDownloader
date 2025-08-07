// PS Vita Data Files Downloader by Harommel OddSock
// Licensed under GPLv3

#pragma once

#include <vitasdk.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern double dl_progress;
extern bool is_dl;

void dl_topath(const char *url, const char *path);
void dl_topath_nothread(const char *url, const char *path);
void check_dl_thd_for_free();

#ifdef __cplusplus
}
#endif