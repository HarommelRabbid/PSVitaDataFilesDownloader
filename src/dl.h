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

void dl_topath(const char *url, const char *path);
void dl_topath_nothread(const char *url, const char *path);

#ifdef __cplusplus
}
#endif