// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#include "daemon/detect.h"
#include "daemon/logger.h"
#include "daemon/paths.h"
#include <unistd.h>

bool pascal_gov_detect_cpu_psi(void)
{
	if (access(PASCAL_GOV_PATH_PSI_CPU, R_OK | W_OK) == 0)
		return true;

	LOGE("detect: cpu psi node missing or access denied");
	return false;
}

int pascal_gov_detect_privilege(void)
{
	if (getuid() != 0) {
		LOGE("detect: fatal, daemon requires root uid 0");
		return -1;
	}

	return 0;
}
