// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#include "daemon/scan.h"
#include "pascal_gov/compiler.h"
#include "pascal_gov/str.h"
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

static const char *const THERMAL_PRIORITY[] = {
	"cpu",
	"cpu-1-0-usr",
	"cpu-1-1-usr",
	"cpu-1-2-usr",
	"cpu-1-3-usr",
	"cpu-0-0-usr",
	"cpu-0-1-usr",
	"cpu0_thermal",
	"cpu1_thermal",
	"mtktscpu",
	"mtk_ts_cpu",
	"mtkts_cpu",
	"thermal-cpuss-0",
	"thermal-cpuss-1",
	"exynos_thermal",
	"exynos_dev_thermal",
	"hisi_thermal",
	"mtktsAP",
	"mtk_ts_ap",
	"ap_cdev",
	"ap_thermal",
	"soc_thermal",
	"soc-thermal",
	"cpu_thermal",
	"cpu-thermal",
	"tsens_tz_sensor10",
	"tsens_tz_sensor5",
	"tsens_tz_sensor0",
	"big-core",
	"mid-core",
	"little-core",
};

static const char *const BLACKLIST[] = {
	"battery", "bms",  "bat",  "charger",  "usb",	 "pa_therm", "pa-therm",
	"modem",   "wifi", "wlan", "gpu",      "camera", "flash",    "led",
	"pmic",	   "buck", "ldo",  "xo_therm", "quiet",	 "backlight"};

#define PASCAL_GOV_THERMAL_BASE "/sys/class/thermal"

int pascal_gov_scan_thermal_zone(char *out_path, size_t max_len)
{
	DIR *dir = opendir(PASCAL_GOV_THERMAL_BASE);
	if (!dir) {
		out_path[0] = '\0';
		return -1;
	}

	struct dirent *entry;
	char type_path[256];
	char type_content[64];

	struct {
		char name[64];
		char folder[64];
	} zones[256];

	size_t zone_count = 0;

	while ((entry = readdir(dir)) != NULL && zone_count < 256) {
		if (!pascal_gov_str_has_prefix(entry->d_name, "thermal_zone"))
			continue;

		pascal_gov_str_build_path(type_path, sizeof(type_path),
					  PASCAL_GOV_THERMAL_BASE,
					  entry->d_name, "type");

		int fd = open(type_path, O_RDONLY | O_CLOEXEC);
		if (fd < 0)
			continue;

		ssize_t bytes =
			read(fd, type_content, sizeof(type_content) - 1);

		close(fd);

		if (bytes <= 0)
			continue;

		type_content[bytes] = '\0';

		for (ssize_t i = 0; i < bytes; ++i) {
			if (type_content[i] == '\n' ||
			    type_content[i] == '\r') {
				type_content[i] = '\0';
				break;
			}
		}

		strncpy(zones[zone_count].name, type_content, 63);
		zones[zone_count].name[63] = '\0';

		strncpy(zones[zone_count].folder, entry->d_name, 63);
		zones[zone_count].folder[63] = '\0';

		zone_count++;
	}

	closedir(dir);

	for (size_t i = 0; i < ARRAY_SIZE(THERMAL_PRIORITY); ++i) {
		for (size_t z = 0; z < zone_count; ++z) {
			if (strcmp(zones[z].name, THERMAL_PRIORITY[i]) == 0) {
				pascal_gov_str_build_path(
					out_path, max_len,
					PASCAL_GOV_THERMAL_BASE,
					zones[z].folder, "temp");

				return 0;
			}
		}
	}

	for (size_t z = 0; z < zone_count; ++z) {
		char lower_name[64];
		strcpy(lower_name, zones[z].name);
		pascal_gov_str_to_lower(lower_name);

		bool is_cpu = pascal_gov_str_contains(lower_name, "cpu");

		if (!is_cpu)
			is_cpu = pascal_gov_str_contains(lower_name, "soc");

		if (!is_cpu)
			is_cpu = pascal_gov_str_contains(lower_name, "cluster");

		if (!is_cpu)
			is_cpu = pascal_gov_str_contains(lower_name, "ap");

		if (!is_cpu)
			continue;

		bool is_safe = true;

		for (size_t i = 0; i < ARRAY_SIZE(BLACKLIST); ++i) {
			if (pascal_gov_str_contains(lower_name, BLACKLIST[i])) {
				is_safe = false;
				break;
			}
		}

		if (is_cpu && is_safe) {
			pascal_gov_str_build_path(out_path, max_len,
						  PASCAL_GOV_THERMAL_BASE,
						  zones[z].folder, "temp");

			return 0;
		}
	}

	out_path[0] = '\0';
	return -1;
}
