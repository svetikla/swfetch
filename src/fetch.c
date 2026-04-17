#include <stddef.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include "config.h"

static char buf[BUFSIZ];

struct
state {
	char *kernel;
	char *shell;
	char *pkgs;
	char *arch;
	char *os;
	char *wm;
};


char
*pr(const char *cmd)
{
	FILE *p = popen(cmd, "r");
	if (!p) return (NULL);

	if (!fgets(buf, sizeof(buf), p)) {
		pclose(p);
		return (NULL);
	}	

	pclose(p);
	buf[strcspn(buf, "\n")] = 0;

	if (buf[0] == '\0') {
		return (NULL);
	}

	return (strdup(buf));
}

void
shell(struct state *st)
{
	char *sh = getenv("SHELL");
	if (!sh) {
		st->shell = strdup("unknown");
		return;
	}
	char *slash = strrchr(sh, '/');
	st->shell = strdup(slash ? slash + 1 : sh);
}

char
*wm(void)
{
	const char *cmds[] = {
		"i3-msg -v 2>/dev/null | head -n1 | awk '{print \"i3\"}'",
		"xprop -root _NET_WM_NAME 2>/dev/null | cut -d '\"' -f2",
		NULL
	};

	for (int i = 0; cmds[i]; i++) {
		char *r = pr(cmds[i]);
		if (r && *r && strcmp(r, "unknown") != 0)
			return (r);
		free(r);
	}

	return (strdup("unknown"));
}

char
*uptime()
{
	long  up, days, hours, mins;
	struct timeval t; /* boottime */
	size_t tsz = sizeof(t);

	if (sysctlbyname("kern.boottime", &t, &tsz, NULL, 0) == -1)
		return (NULL);

	up = (long)(time(NULL) - t.tv_sec + 30);

	days = up / 86400;
	up %= 86400;
	hours = up / 3600;
	up %= 3600;
	mins = up / 60;

	if (!days) 
		snprintf(buf, sizeof(buf), "%ld:%02ld", hours, mins);
	else	
		snprintf(buf, sizeof(buf), "%ld:%02ld:%02ld", days, hours, mins);

	return (buf);
}

void
get_packages(struct state *st)
{
	const char* const cmd = "/usr/sbin/pkg info";

	FILE *f = popen(cmd, "r");
	if (f == NULL)
		err(1, "popen(%s) failed", cmd);

	/* No. of packages == simple line count */
	size_t npkg = 0;

	while (fgets(buf, sizeof buf, f) != NULL)
		if (strchr(buf, '\n') != NULL)
			npkg++;

	if (pclose(f) != 0)
		err(1, "pclose(%s) failed", cmd);
	snprintf(buf, sizeof(buf), "%zu", npkg);
	st->pkgs = strdup(buf);
}


void
arch(struct state *st, struct utsname *u) {
	st->arch = strdup(u->machine);
}


void
os(struct state *st, struct utsname *u) {
	st->os = strdup(u->sysname);
}

void
kernel(struct state *st, struct utsname *u) {
	st->kernel = strdup(u->release);
}

int
main(void)
{
	struct state st = {0};

	struct utsname u;
	if (uname(&u) == -1) {
		perror("uname");
		return 1;
	}

	os(&st, &u);
	kernel(&st, &u);
	arch(&st, &u);
	st.wm = wm();
	shell(&st);
	get_packages(&st);

	puts("");
	printf("%s%*s%s\n", USERTEXT,    TABSIZE, "", getenv("USER"));
	printf("%s%*s%s\n", OSTEXT,      TABSIZE, "", st.os);
	printf("%s%*s%s\n", ARCHTEXT,    TABSIZE, "", st.arch);
	printf("%s%*s%s\n", KERNELTEXT,	 TABSIZE, "", st.kernel);
	printf("%s%*s%s\n", PACKAGETEXT, TABSIZE, "", st.pkgs);
	printf("%s%*s%s\n", SHELLTEXT,   TABSIZE, "", st.shell);
	printf("%s%*s%s\n", TERMTEXT,    TABSIZE, "", getenv("TERM"));
	printf("%s%*s%s\n", WMTEXT,      TABSIZE, "", st.wm);
	printf("%s%*s%s\n", EDTEXT,      TABSIZE, "", getenv("EDITOR"));
	printf("%s%*s%s\n", UPTIMETEXT,  TABSIZE, "", uptime());
	puts("");

	free(st.wm);
	free(st.os);
	free(st.arch);
	free(st.pkgs);
	free(st.shell);
	free(st.kernel);

	return (0);
}
