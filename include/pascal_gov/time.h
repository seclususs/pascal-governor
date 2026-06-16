// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#ifndef PASCAL_GOV_TIME_H
#define PASCAL_GOV_TIME_H

#include "pascal_gov/compiler.h"
#include <time.h>

static inline __attribute__((always_inline)) float
pascal_gov_dt_sec(const struct timespec *PASCAL_GOV_RESTRICT prev,
		  const struct timespec *PASCAL_GOV_RESTRICT now)
{
	float sec = (float)(now->tv_sec - prev->tv_sec);
	float nsec = (float)(now->tv_nsec - prev->tv_nsec) * 1e-9F;
	return sec + nsec;
}

#endif // PASCAL_GOV_TIME_H
