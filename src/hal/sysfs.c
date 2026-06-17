// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#include "pascal_gov/sysfs.h"
#include <errno.h>
#include <unistd.h>

static inline size_t pascal_gov_u64_to_str(uint64_t val, char *out_buf)
{
	if (val == 0) {
		out_buf[0] = '0';
		out_buf[1] = '\n';
		return 2;
	}

	char temp[32];
	char *p = &temp[31];
	*p = '\n';

	while (val > 0) {
		*(--p) = (char)((val % 10) + '0');
		val /= 10;
	}

	size_t len = (size_t)(&temp[31] - p) + 1;

	for (size_t i = 0; i < len; ++i)
		out_buf[i] = p[i];

	return len;
}

int pascal_gov_sysfs_write_to_stream(int fd, uint64_t value)
{
	if (fd < 0)
		return -EBADF;

	char write_buf[32];
	size_t len = pascal_gov_u64_to_str(value, write_buf);

	ssize_t res = pwrite(fd, write_buf, len, 0);
	if (res < 0)
		return -errno;

	return 0;
}

static inline bool check_absolute(uint64_t current, uint64_t target,
				  uint64_t threshold)
{
	if (current == target)
		return false;

	uint64_t diff =
		(current > target) ? (current - target) : (target - current);

	return diff >= threshold;
}

static inline bool check_relative(uint64_t current, uint64_t target,
				  uint64_t tolerance_per_mille)
{
	if (current == target)
		return false;

	if (current == 0)
		return target != 0;

	uint64_t diff =
		(current > target) ? (current - target) : (target - current);

	return (diff * (uint64_t)1000) >= (current * tolerance_per_mille);
}

void pascal_gov_sysfs_update(
	pascal_gov_sysfs_cache *PASCAL_GOV_RESTRICT cache, uint64_t new_value,
	bool force,
	const pascal_gov_sysfs_check_strategy *PASCAL_GOV_RESTRICT strategy)
{
	if (!cache->is_active)
		return;

	bool needs_update = false;

	if (force) {
		needs_update = true;
	} else {
		switch (strategy->type) {
		case PASCAL_GOV_CHECK_ABSOLUTE:
			needs_update =
				check_absolute(cache->last_value, new_value,
					       strategy->threshold);
			break;

		case PASCAL_GOV_CHECK_RELATIVE:
			needs_update =
				check_relative(cache->last_value, new_value,
					       strategy->threshold);
			break;

		case PASCAL_GOV_CHECK_STRICT:
			needs_update = (cache->last_value != new_value);
			break;
		}
	}

	if (needs_update &&
	    pascal_gov_sysfs_write_to_stream(cache->fd, new_value) == 0)
		cache->last_value = new_value;
}
