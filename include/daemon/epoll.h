// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#ifndef PASCAL_GOV_DAEMON_EPOLL_H
#define PASCAL_GOV_DAEMON_EPOLL_H

struct pascal_gov_context;

int pascal_gov_epoll_add_trigger(struct pascal_gov_context *ctx);
void pascal_gov_epoll_remove_trigger(struct pascal_gov_context *ctx);
int pascal_gov_epoll_run(struct pascal_gov_context *ctx);

#endif // PASCAL_GOV_DAEMON_EPOLL_H
