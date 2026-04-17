#include <stddef.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include "config.h"

static char buf[BUFSIZ];

struct
state {
	char *user;
	char *kernel;
	char *shell;
	char *pkgs;
	char *arch;
	char *os;
};


char
*pr(const char *cmd)
{
	char buf[BUFSIZ];
	FILE *p = popen(cmd, "r");
	if (!p) return (NULL);

	if (!fgets(buf, sizeof(buf), p)) {
		pclose(p);
		return (NULL);
	}	

	pclose(p);
	buf[strcspn(buf, "\n")] = 0;

	if (buf[0] == '\0')
		return (NULL);

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
*uptime()
{
	long  up, days, hrs, mins;
	struct timeval t;
	size_t tsz = sizeof(t);

	if (sysctlbyname("kern.boottime", &t, &tsz, NULL, 0) == -1)
		return (NULL);

	up = (long)(time(NULL) - t.tv_sec + 30);

	days = up / 86400;
	up %= 86400;
	hrs = up / 3600;
	up %= 3600;
	mins = up / 60;

	if (!days) 
		snprintf(buf, sizeof(buf), "%ld:%02ld", hrs, mins);
	else	
		snprintf(buf, sizeof(buf), "%ld:%02ld:%02ld", days, hrs, mins);

	return (strdup(buf));
}

void
get_packages(struct state *st)
{
	const char* const cmd = "/usr/sbin/pkg info";

	FILE *f = popen(cmd, "r");
	if (f == NULL)
		err(1, "popen(%s) failed", cmd);

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
user(struct state *st)
{
	struct passwd* pw;
	char* p;

	if ((p = getenv("USER")) == NULL || *p == '\0') {
		if ((pw = getpwuid(getuid())) == NULL)
			err(1, "getpwuid() failed");
		p = pw->pw_name;
	}
	st->user = strdup(p);
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
	user(&st);
	arch(&st, &u);
	shell(&st);
	get_packages(&st);

	puts("");
	printf("%s%*s%s\n", USERTEXT,    TABSIZE, "", st.user);
	printf("%s%*s%s\n", OSTEXT,      TABSIZE, "", st.os);
	printf("%s%*s%s\n", ARCHTEXT,    TABSIZE, "", st.arch);
	printf("%s%*s%s\n", KERNELTEXT,	 TABSIZE, "", st.kernel);
	printf("%s%*s%s\n", PACKAGETEXT, TABSIZE, "", st.pkgs);
	printf("%s%*s%s\n", SHELLTEXT,   TABSIZE, "", st.shell);
	printf("%s%*s%s\n", TERMTEXT,    TABSIZE, "", getenv("TERM"));
	printf("%s%*s%s\n", EDTEXT,      TABSIZE, "", getenv("EDITOR"));
	printf("%s%*s%s\n", UPTIMETEXT,  TABSIZE, "", uptime());
	puts("");

	free(st.os);
	free(st.arch);
	free(st.pkgs);
	free(st.user);
	free(st.shell);
	free(uptime());
	free(st.kernel);

	return (0);
}
