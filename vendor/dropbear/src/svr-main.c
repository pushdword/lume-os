/*
 * Dropbear - a SSH2 server
 * 
 * Copyright (c) 2002-2006 Matt Johnston
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

#include "includes.h"
#include "platform.h"
#include "net_client.h"
#include "profile.h"
#include "dbutil.h"
#include "session.h"
#include "buffer.h"
#include "signkey.h"
#include "runopts.h"
#include "dbrandom.h"
#include "crypto_desc.h"

static size_t listensockets(int *sock, size_t sockcount, int *maxfd);
static void sigchld_handler(int dummy);
static void sigsegv_handler(int);
static void sigintterm_handler(int fish);
static void main_inetd(void);
static void main_noinetd(int argc, char ** argv, const char* multipath);
static void commonsetup(void);

#if defined(DBMULTI_dropbear) || !DROPBEAR_MULTI
#if defined(DBMULTI_dropbear) && DROPBEAR_MULTI
int dropbear_main(int argc, char ** argv, const char* multipath)
#else
int main(int argc, char ** argv)
#endif
{
#if !DROPBEAR_MULTI
	const char* multipath = NULL;
#endif

	_dropbear_exit = svr_dropbear_exit;
	_dropbear_log = svr_dropbear_log;

	disallow_core();

	if (argc < 1) {
		dropbear_exit("Bad argc");
	}

	/* get commandline options */
	svr_getopts(argc, argv);

#if INETD_MODE
	/* service program mode */
	if (svr_opts.inetdmode) {
		main_inetd();
		/* notreached */
	}
#endif

#if DROPBEAR_DO_REEXEC
	if (svr_opts.reexec_childpipe >= 0) {
#ifdef PR_SET_NAME
		/* Fix the "Name:" in /proc/pid/status, otherwise it's
		a FD number from fexecve.
		Failure doesn't really matter, it's mostly aesthetic */
		prctl(PR_SET_NAME, basename(argv[0]), 0, 0);
#endif
		main_inetd();
		/* notreached */
	}
#endif

#if NON_INETD_MODE
	main_noinetd(argc, argv, multipath);
	/* notreached */
#endif

	dropbear_exit("Compiled without normal mode, can't run without -i\n");
	return -1;
}
#endif

#if INETD_MODE || DROPBEAR_DO_REEXEC
static void main_inetd() {
	char *host, *port = NULL;

	/* Set up handlers, syslog */
	commonsetup();

	seedrandom();

	if (svr_opts.reexec_childpipe < 0) {
		/* In case our inetd was lax in logging source addresses */
		get_socket_address(0, NULL, NULL, &host, &port, 0);
			if (strchr(host, ':') != NULL) {
				dropbear_log(LOG_INFO, "Child connection from [%s]:%s", host, port);
			} else {
				dropbear_log(LOG_INFO, "Child connection from %s:%s", host, port);
			}

		m_free(host);
		m_free(port);

		/* Don't check the return value - it may just fail since inetd has
		 * already done setsid() after forking (xinetd on Darwin appears to do
		 * this */
		setsid();
	}

	/* -1 for childpipe in the inetd case is discarded */
	svr_session(0, svr_opts.reexec_childpipe);

	/* notreached */
}
#endif /* INETD_MODE */

#if NON_INETD_MODE
static void main_noinetd(int argc, char ** argv, const char* multipath) {

    (void)argc;(void)argv;(void)multipath;
    commonsetup();seedrandom();
    struct lume_net_info information;
    while(net_info(&information)<0 || !(information.flags&LUME_NET_CONFIGURED))net_idle(100);
    int listener=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in address={.sin_family=AF_INET,.sin_port=htons(22)};
    if(listener<0||bind(listener,(void *)&address,sizeof address)<0||listen(listener,1)<0)dropbear_exit("Lume SSH listen failed: %s",strerror(errno));
    dropbear_log(LOG_INFO,"SSH_READY port=22 single-session root-publickey");
    struct sockaddr_in peer;socklen_t bytes=sizeof peer;
    int client=accept(listener,(void *)&peer,&bytes);
    if(client<0)dropbear_exit("Lume SSH accept failed");
    const unsigned char *ip=(const void *)&peer.sin_addr;
    if(ip[0]!=172||ip[1]!=30||ip[2]!=1||!ip[3]||ip[3]==255)dropbear_exit("Client outside trusted LAN");
    ssh_profile_reset();close(listener);svr_session(client,-1);

}
#endif /* NON_INETD_MODE */


/* catch + reap zombie children */
static void sigchld_handler(int UNUSED(unused)) {
	struct sigaction sa_chld;

	const int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0) {}

	sa_chld.sa_handler = sigchld_handler;
	sa_chld.sa_flags = SA_NOCLDSTOP;
	sigemptyset(&sa_chld.sa_mask);
	if (sigaction(SIGCHLD, &sa_chld, NULL) < 0) {
		dropbear_exit("signal() error");
	}
	errno = saved_errno;
}

/* catch any segvs */
static void sigsegv_handler(int UNUSED(unused)) {
	int i;
	const char *msg = "Aiee, segfault! You should probably report "
			"this as a bug to the developer\n";
	i = write(STDERR_FILENO, msg, strlen(msg));
	/* ignore short writes */
	(void)i;
	_exit(EXIT_FAILURE);
}

/* catch ctrl-c or sigterm */
static void sigintterm_handler(int UNUSED(unused)) {

	ses.exitflag = 1;
}

/* Things used by inetd and non-inetd modes */
static void commonsetup() {

	struct sigaction sa_chld;
#ifndef DISABLE_SYSLOG
	if (opts.usingsyslog) {
		startsyslog(PROGNAME);
	}
#endif

	/* set up cleanup handler */
	if (signal(SIGINT, sigintterm_handler) == SIG_ERR || 
#ifndef DEBUG_VALGRIND
		signal(SIGTERM, sigintterm_handler) == SIG_ERR ||
#endif
		signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		dropbear_exit("signal() error");
	}

	/* catch and reap zombie children */
	sa_chld.sa_handler = sigchld_handler;
	sa_chld.sa_flags = SA_NOCLDSTOP;
	sigemptyset(&sa_chld.sa_mask);
	if (sigaction(SIGCHLD, &sa_chld, NULL) < 0) {
		dropbear_exit("signal() error");
	}
	if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
		dropbear_exit("signal() error");
	}

	crypto_init();

	/* Now we can setup the hostkeys - needs to be after logging is on,
	 * otherwise we might end up blatting error messages to the socket */
	load_all_hostkeys();
}

/* Set up listening sockets for all the requested ports */
static size_t listensockets(int *socks, size_t sockcount, int *maxfd) {

	unsigned int i, n;
	char* errstring = NULL;
	size_t sockpos = 0;
	int nsock;

	TRACE(("listensockets: %d to try", svr_opts.portcount))

	for (i = 0; i < svr_opts.portcount; i++) {

		TRACE(("listening on '%s:%s'", svr_opts.addresses[i], svr_opts.ports[i]))

		nsock = dropbear_listen(svr_opts.addresses[i], svr_opts.ports[i], &socks[sockpos], 
				sockcount - sockpos,
				&errstring, maxfd, svr_opts.interface);

		if (nsock < 0) {
			dropbear_log(LOG_WARNING, "Failed listening on '%s': %s", 
							svr_opts.ports[i], errstring);
			m_free(errstring);
			continue;
		}

		for (n = 0; n < (unsigned int)nsock; n++) {
			int sock = socks[sockpos + n];
			set_sock_priority(sock, DROPBEAR_PRIO_LOWDELAY);
#if DROPBEAR_SERVER_TCP_FAST_OPEN
			set_listen_fast_open(sock);
#endif
		}

		sockpos += nsock;

	}
	return sockpos;
}
