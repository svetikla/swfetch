#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <time.h>
#include <pwd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/sysctl.h>
#include <sys/socket.h>

#include "config.h"

struct
state {
	char *user;
	char *kernel;
	char *uptime;
	char *ipaddr;
	char *shell;
	char *pkgs;
	char *arch;
	char *ram;
	char *os;
};

int
set_shell(struct state *st)
{
	char *sh = getenv("SHELL");
	if (!sh) return (1);
	char *slash = strrchr(sh, '/');
	st->shell = strdup(slash ? slash + 1 : sh);
	return (0);
}

int
set_uptime(struct state *st)
{
	char buf[64];
	long  up, days, hrs, mins;
	struct timeval t;
	size_t tsz = sizeof(t);

	if (sysctlbyname("kern.boottime", &t, &tsz, NULL, 0) == -1)
		return (1);

	up = (long)(time(NULL) - t.tv_sec);
	days = up / 86400;
	up %= 86400;
	hrs = up / 3600;
	up %= 3600;
	mins = up / 60;

	if (!days) 
		snprintf(buf, sizeof(buf), "%ld:%02ld", hrs, mins);
	else	
		snprintf(buf, sizeof(buf), "%ld:%02ld:%02ld", days, hrs, mins);

	st->uptime = strdup(buf);
	return (0);
}

int
set_packages(struct state *st)
{
	char buf[64];
	const char* const cmd = "/usr/sbin/pkg info";

	FILE *f = popen(cmd, "r");
	if (f == NULL) return (1);

	size_t npkg = 0;

	while (fgets(buf, sizeof buf, f) != NULL) {
		if (strchr(buf, '\n') != NULL)
			npkg++;
	}

	if (pclose(f) != 0) return (1);

	snprintf(buf, sizeof(buf), "%zu", npkg);
	st->pkgs = strdup(buf);
	return (0);
}

int
set_user(struct state *st)
{
	struct passwd* pw;
	char* p;

	if ((p = getenv("USER")) == NULL || *p == '\0') {
		if ((pw = getpwuid(getuid())) == NULL)
			return (1);
		p = pw->pw_name;
	}

	st->user = strdup(p);
	return (0);
}

int
set_ipaddr(struct state *st)
{
	struct ifaddrs *ifap, *ifa;
	char buf[INET_ADDRSTRLEN];

	if (getifaddrs(&ifap) == -1)
		return (1);

	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		struct sockaddr_in *sin;
		uint32_t ip;

		if (ifa->ifa_addr == NULL)
			continue;

		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;

		if (!(ifa->ifa_flags & IFF_UP) ||
		   (ifa->ifa_flags & IFF_LOOPBACK))
			continue;

		sin = (struct sockaddr_in *)ifa->ifa_addr;
		ip = ntohl(sin->sin_addr.s_addr);

		if (!((ip & 0xff000000) == 0x0a000000 ||
		      (ip & 0xfff00000) == 0xac100000 ||
		      (ip & 0xffff0000) == 0xc0a80000))
			continue;

		if (inet_ntop(AF_INET, &sin->sin_addr,
		    buf, sizeof(buf)) == NULL)
			continue;

		free(st->ipaddr);
		st->ipaddr = strdup(buf);
		break;
	}

	freeifaddrs(ifap);
	return (0);
}

int
set_ram(struct state *st)
{
	char buf[64];
	unsigned long long mem;
	size_t len = sizeof(mem);

	if (sysctlbyname("hw.physmem", &mem, &len, NULL, 0) == -1)
		return (1);

	snprintf(buf, sizeof(buf), "%llu MB", mem / 1024 / 1024);
	st->ram = strdup(buf);
	return (0);
}

void
set_arch(struct state *st, struct utsname *u) {
	st->arch = strdup(u->machine);
}


void
set_os(struct state *st, struct utsname *u) {
	st->os = strdup(u->sysname);
}

void
set_kernel(struct state *st, struct utsname *u) {
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

	set_os(&st, &u);
	set_arch(&st, &u);
	set_kernel(&st, &u);
	set_uptime(&st);

	set_packages(&st);
	set_ipaddr(&st);
	set_ram(&st);

	set_shell(&st);
	set_user(&st);

	puts("");
	printf("%s%*s%s\n", USERTEXT,    TABSIZE, "", st.user);
	printf("%s%*s%s\n", OSTEXT,      TABSIZE, "", st.os);
	printf("%s%*s%s\n", ARCHTEXT,    TABSIZE, "", st.arch);
	printf("%s%*s%s\n", KERNELTEXT,	 TABSIZE, "", st.kernel);
	printf("%s%*s%s\n", PACKAGETEXT, TABSIZE, "", st.pkgs);
	printf("%s%*s%s\n", SHELLTEXT,   TABSIZE, "", st.shell);
	printf("%s%*s%s\n", TERMTEXT,    TABSIZE, "", getenv("TERM"));
	printf("%s%*s%s\n", EDTEXT,      TABSIZE, "", getenv("EDITOR"));
	printf("%s%*s%s\n", UPTIMETEXT,  TABSIZE, "", st.uptime);
	printf("%s%*s%s\n", IPTEXT,      TABSIZE, "", st.ipaddr);
	printf("%s%*s%s\n", RAMTEXT,     TABSIZE, "", st.ram);
	puts("");

	free(st.os);
	free(st.ram);
	free(st.arch);
	free(st.pkgs);
	free(st.user);
	free(st.shell);
	free(st.ipaddr);
	free(st.uptime);
	free(st.kernel);

	return (0);
}
