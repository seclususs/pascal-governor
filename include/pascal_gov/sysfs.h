// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#ifndef PASCAL_GOV_SYSFS_H
#define PASCAL_GOV_SYSFS_H

#include "pascal_gov/compiler.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
	PASCAL_GOV_CHECK_ABSOLUTE,
	PASCAL_GOV_CHECK_RELATIVE,
	PASCAL_GOV_CHECK_STRICT
} pascal_gov_sysfs_check_type;

typedef struct {
	pascal_gov_sysfs_check_type type;
	uint64_t threshold;
} pascal_gov_sysfs_check_strategy;

typedef struct {
	int fd;
	uint64_t last_value;
	bool is_active;
} pascal_gov_sysfs_cache;

void pascal_gov_sysfs_update(
	pascal_gov_sysfs_cache *PASCAL_GOV_RESTRICT cache, uint64_t new_value,
	bool force,
	const pascal_gov_sysfs_check_strategy *PASCAL_GOV_RESTRICT strategy);

int pascal_gov_sysfs_write_to_stream(int fd, uint64_t value);

#endif // PASCAL_GOV_SYSFS_H
