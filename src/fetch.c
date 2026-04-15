#include <stddef.h>
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

struct
state {
	char *kernel; char *shell;
	char *pkgs;   char *os;
	char *wm;
};

char
*pr(const char *cmd)
{
	FILE *p = popen(cmd, "r");
	if (!p) return (NULL);

	char buf[256];

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
kernel(struct state *st)
{
	static struct utsname kerneldata;
	uname(&kerneldata);
	st->kernel = kerneldata.release;
}

void
shell(struct state *st)
{
	char *sh = getenv("SHELL");
	if (!sh) {
		st->shell = "unknown";
		return;
	}
	char *slash = strrchr(sh, '/');
	st->shell = slash ? slash + 1 : sh;
}

char *wm(void)
{
	const char *cmds[] = {
		"i3-msg -v 2>/dev/null | head -n1 | awk '{print \"i3\"}'",
		"xprop -root _NET_WM_NAME 2>/dev/null | cut -d '\"' -f2",
		NULL
	};

	for (int i = 0; cmds[i]; i++) {
		char *r = pr(cmds[i]);
		if (r && *r && strcmp(r, "unknown") != 0)
			return r;
		free(r);
	}

	return (strdup("unknown"));
}

const char
*uptime()
{
	static char buf[64];
	struct timeval boottime;
	size_t len = sizeof(boottime);
	struct timeval now;
	int mib[2] = {CTL_KERN, KERN_BOOTTIME};

	if (sysctl(mib, 2, &boottime, &len, NULL, 0) != -1) {
		gettimeofday(&now, NULL);
		time_t diff = now.tv_sec - boottime.tv_sec;
		int hrs  = (int)((diff % 86400) / 3600);
		int mins = (int)((diff % 3600) / 60);
		snprintf(buf, sizeof(buf), "%02d:%02d", hrs, mins);
	} else {
		snprintf(buf, sizeof(buf), "unknown");
	}

	return (buf);
}

void
os(struct state *st)
{
	static struct utsname sysinfo;
	uname(&sysinfo);

	st->os = sysinfo.sysname;

	if (!strncmp(sysinfo.sysname, "FreeBSD", 7))
		st->pkgs = pr("pkg info | wc -l | tr -d ' '");
	else
		st->pkgs = strdup("unknown");
}

int
main(void)
{
	struct state st = {0};

	kernel(&st);
	shell(&st);
	os(&st);
	st.wm = wm();

	puts("");
	printf("%s%*s%s\n", USERTEXT,    TABSIZE, "", getenv("USER"));
	printf("%s%*s%s\n", OSTEXT,      TABSIZE, "", st.os);
	printf("%s%*s%s\n", KERNELTEXT,  TABSIZE, "", st.kernel);
	printf("%s%*s%s\n", PACKAGETEXT, TABSIZE, "", st.pkgs);
	printf("%s%*s%s\n", SHELLTEXT,   TABSIZE, "", st.shell);
	printf("%s%*s%s\n", TERMTEXT,    TABSIZE, "", getenv("TERM"));
	printf("%s%*s%s\n", WMTEXT,      TABSIZE, "", st.wm);
	printf("%s%*s%s\n", EDTEXT,      TABSIZE, "", getenv("EDITOR"));
	printf("%s%*s%s\n", UPTIMETEXT,  TABSIZE, "", uptime());
	puts("");
  
	free(st.wm);
	free(st.pkgs);
	return (0);
}
