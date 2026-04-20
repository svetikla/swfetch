#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/sysctl.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "config.h"

static char buf[BUFSIZ];

struct
state {
	char *user;
	char *kernel;
	char *ipaddr;
	char *shell;
	char *pkgs;
	char *arch;
	char *os;
};

char
*shell(struct state *st)
{
	char *sh = getenv("SHELL");
	if (!sh) return (NULL);
	char *slash = strrchr(sh, '/');
	st->shell = strdup(slash ? slash + 1 : sh);
	return (0);
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

char
*packages(struct state *st)
{
	const char* const cmd = "/usr/sbin/pkg info";

	FILE *f = popen(cmd, "r");
	if (f == NULL) return (NULL);

	size_t npkg = 0;

	while (fgets(buf, sizeof buf, f) != NULL) {
		if (strchr(buf, '\n') != NULL)
			npkg++;
	}

	if (pclose(f) != 0) return (NULL);

	snprintf(buf, sizeof(buf), "%zu", npkg);
	st->pkgs = strdup(buf);
	return (0);
}

char
*user(struct state *st)
{
	struct passwd* pw;
	char* p;

	if ((p = getenv("USER")) == NULL || *p == '\0') {
		if ((pw = getpwuid(getuid())) == NULL)
			return (NULL);
		p = pw->pw_name;
	}

	st->user = strdup(p);
	return (0);
}

char
ipaddr(struct state *st)
{
	struct ifaddrs *ifaddr, *ifa;

	getifaddrs(&ifaddr);

	for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr) continue;

		if (ifa->ifa_addr->sa_family == AF_INET) {
			struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
			unsigned int ip = ntohl(sa->sin_addr.s_addr);
			if ((ip & 0xFF000000) == 0x0A000000 ||
			    (ip & 0xFFF00000) == 0xAC100000 ||
			    (ip & 0xFFFF0000) == 0xC0A80000) {
				inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf));
				st->ipaddr = strdup(buf);
        			break;
			}
		}
	}

	freeifaddrs(ifaddr);
	return (0);
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

	char *up = uptime();
	os(&st, &u);
	kernel(&st, &u);
	user(&st);
	arch(&st, &u);
	shell(&st);
	ipaddr(&st);
	packages(&st);

	puts("");
	printf("%s%*s%s\n", USERTEXT,    TABSIZE, "", st.user);
	printf("%s%*s%s\n", OSTEXT,      TABSIZE, "", st.os);
	printf("%s%*s%s\n", ARCHTEXT,    TABSIZE, "", st.arch);
	printf("%s%*s%s\n", KERNELTEXT,	 TABSIZE, "", st.kernel);
	printf("%s%*s%s\n", PACKAGETEXT, TABSIZE, "", st.pkgs);
	printf("%s%*s%s\n", SHELLTEXT,   TABSIZE, "", st.shell);
	printf("%s%*s%s\n", TERMTEXT,    TABSIZE, "", getenv("TERM"));
	printf("%s%*s%s\n", EDTEXT,      TABSIZE, "", getenv("EDITOR"));
	printf("%s%*s%s\n", UPTIMETEXT,  TABSIZE, "", up);
	printf("%s%*s%s\n", IPTEXT,      TABSIZE, "", st.ipaddr);
	puts("");

	free(up);
	free(st.os);
	free(st.arch);
	free(st.pkgs);
	free(st.user);
	free(st.shell);
	free(st.ipaddr);
	free(st.kernel);

	return (0);
}
