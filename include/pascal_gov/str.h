// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#ifndef PASCAL_GOV_STR_H
#define PASCAL_GOV_STR_H

#include <stdbool.h>
#include <stddef.h>

static inline bool pascal_gov_str_has_prefix(const char *str,
					     const char *prefix)
{
	while (*prefix)

		if (*prefix++ != *str++)
			return false;

	return true;
}

static inline bool pascal_gov_str_contains(const char *str, const char *substr)
{
	if (!*substr)
		return true;

	const char *p1;
	const char *p2;

	for (const char *p1_adv = str; *p1_adv; ++p1_adv) {
		p1 = p1_adv;
		p2 = substr;

		while (*p1 && *p2 && *p1 == *p2) {
			p1++;
			p2++;
		}

		if (!*p2)
			return true;
	}

	return false;
}

static inline void pascal_gov_str_to_lower(char *str)
{
	for (; *str; ++str)

		if (*str >= 'A' && *str <= 'Z')
			*str += 32;
}

static inline void pascal_gov_str_build_path(char *out, size_t max_len,
					     const char *dir, const char *sub,
					     const char *file)
{
	size_t i = 0;

	while (*dir && i < max_len - 1)
		out[i++] = *dir++;

	if (i < max_len - 1)
		out[i++] = '/';

	while (*sub && i < max_len - 1)
		out[i++] = *sub++;

	if (i < max_len - 1)
		out[i++] = '/';

	while (*file && i < max_len - 1)
		out[i++] = *file++;

	out[i] = '\0';
}

#endif // PASCAL_GOV_STR_H
