// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#define _GNU_SOURCE
#include "daemon/epoll.h"
#include "daemon/config.h"
#include "daemon/logger.h"
#include "daemon/state.h"
#include "pascal_gov/compiler.h"
#include "pascal_gov/time.h"
#include <errno.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <time.h>
#include <unistd.h>

static inline void
handle_epoll_error(pascal_gov_context *PASCAL_GOV_RESTRICT ctx,
		   const struct timespec *PASCAL_GOV_RESTRICT wait_start)
{
	if (errno == EINTR) {
		struct timespec wait_end;
		clock_gettime(CLOCK_MONOTONIC, &wait_end);

		float dt = pascal_gov_dt_sec(wait_start, &wait_end);
		int elapsed = (int)(dt * 1000.0F);

		ctx->next_wake_ms -= elapsed;
		if (ctx->next_wake_ms < 1)
			ctx->next_wake_ms = 1;
	} else {
		LOGE("epoll: wait err=%d, requesting shutdown", errno);
		ctx->shutdown_requested = true;
	}
}

static inline void
handle_epoll_events(pascal_gov_context *PASCAL_GOV_RESTRICT ctx,
		    struct epoll_event *PASCAL_GOV_RESTRICT events, int nfds)
{
	for (int i = 0; i < nfds; ++i) {
		int triggered_fd = events[i].data.fd;

		if (triggered_fd == ctx->signal_fd) {
			struct signalfd_siginfo siginfo;

			ssize_t s =
				read(ctx->signal_fd, &siginfo, sizeof(siginfo));

			if (s == sizeof(siginfo))
				ctx->shutdown_requested = true;

		} else if (triggered_fd == ctx->trigger_fd) {
			char dummy_buf[8];

			ssize_t PASCAL_GOV_MAYBE_UNUSED unused_ret = read(
				ctx->trigger_fd, dummy_buf, sizeof(dummy_buf));

			if (PASCAL_GOV_LIKELY(ctx->on_trigger != NULL))
				ctx->on_trigger(ctx);
		}
	}
}

int pascal_gov_epoll_add_trigger(pascal_gov_context *ctx)
{
	if (ctx->trigger_fd >= 0 && ctx->epoll_fd >= 0) {
		struct epoll_event ev_trg = {0};
		ev_trg.events = EPOLLPRI | EPOLLERR | EPOLLET;
		ev_trg.data.fd = ctx->trigger_fd;

		return epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, ctx->trigger_fd,
				 &ev_trg);
	}

	return -1;
}

void pascal_gov_epoll_remove_trigger(pascal_gov_context *ctx)
{
	if (ctx->trigger_fd >= 0 && ctx->epoll_fd >= 0) {
		epoll_ctl(ctx->epoll_fd, EPOLL_CTL_DEL, ctx->trigger_fd, NULL);
	}
}

int pascal_gov_epoll_run(pascal_gov_context *ctx)
{
	ctx->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (ctx->epoll_fd < 0)
		return -1;

	struct epoll_event ev_sig = {0};
	ev_sig.events = EPOLLIN | EPOLLERR;
	ev_sig.data.fd = ctx->signal_fd;
	epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, ctx->signal_fd, &ev_sig);

	struct epoll_event ev_trg = {0};
	ev_trg.events = EPOLLPRI | EPOLLERR | EPOLLET;
	ev_trg.data.fd = ctx->trigger_fd;
	epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, ctx->trigger_fd, &ev_trg);

	struct epoll_event events[PASCAL_GOV_MAX_EVENTS];

	while (PASCAL_GOV_LIKELY(!ctx->shutdown_requested)) {
		struct timespec wait_start;
		clock_gettime(CLOCK_MONOTONIC, &wait_start);

		int nfds = epoll_wait(ctx->epoll_fd, events,
				      PASCAL_GOV_MAX_EVENTS, ctx->next_wake_ms);

		if (PASCAL_GOV_UNLIKELY(nfds < 0)) {
			handle_epoll_error(ctx, &wait_start);
			continue;
		}

		if (nfds == 0) {
			if (PASCAL_GOV_LIKELY(ctx->on_timeout != NULL))
				ctx->on_timeout(ctx);
			continue;
		}

		handle_epoll_events(ctx, events, nfds);
	}

	close(ctx->epoll_fd);
	ctx->epoll_fd = -1;
	return 0;
}
