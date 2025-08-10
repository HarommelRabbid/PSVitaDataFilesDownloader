// PS Vita Data Files Downloader by Harommel OddSock
// Licensed under GPLv3

#pragma once

#include <vitasdk.h>
#include <stdbool.h>
#include <stdio.h>

namespace Utils{
	bool FileOrPathExists(const char *path) {
		SceIoStat stat;
		int res = sceIoGetstat(path, &stat);
		return res >= 0;
	}
}