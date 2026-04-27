#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <err.h>
#include <pwd.h>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>

#include <sys/socket.h>
#include <sys/statvfs.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/utsname.h>

#include "config.h"

#define FMT COL "%s" RES "%*s%s\n"
#define SYS_WIRED "vm.stats.vm.v_wire_count"
#define SYS_ACTIVE "vm.stats.vm.v_active_count"
#define SYS_PGSIZE "hw.pagesize"
#define SYS_PHYS   "hw.physmem"

struct
state {
	char *os;
	char *kernel;
	char *arch;
	char *user;
	char *shell;
	char *uptime;
	char *disk;
	char *pkgs;
	char *ram;
	char *ipaddr;
	char *locale;
};

void
free_state(struct state *st)
{
	free(st->kernel);
	free(st->ipaddr);
	free(st->uptime);
	free(st->locale);
	free(st->shell);
	free(st->arch);
	free(st->pkgs);
	free(st->user);
	free(st->disk);
	free(st->ram);
	free(st->os);
}

static void
pr(const char *label, const char *value) {
	printf(FMT,label, TAB, "", value ?: "N/A");
}

int
set_shell(struct state *st)
{
	char *sh = getenv("SHELL");
	char *slash = strrchr(sh, '/');
	if (!sh) return (1);
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

	if (sysctlbyname("kern.boottime",
			&t, &tsz, NULL, 0) == -1) {
		return (1);
	}

	up = (long)(time(NULL) - t.tv_sec);
	days = up / 86400;
	up %= 86400;
	hrs = up / 3600;
	up %= 3600;
	mins = up / 60;

	if (!days) {
		snprintf(buf, sizeof(buf),
			"%ld:%02ld", hrs, mins);
	} else	{
		snprintf(buf, sizeof(buf),
			"%ld:%02ld:%02ld", days, hrs, mins);
	}

	st->uptime = strdup(buf);
	return (0);
}

int
set_packages(struct state *st)
{
	char buf[64];
	const char *const cmd = "/usr/sbin/pkg info";

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
	struct passwd *pw;
	char *p;
	p = getenv("USER");
	if (p == NULL || *p == '\0') {
		pw = getpwuid(getuid());
		if (pw == NULL) return (1);
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
	uint64_t t;
	u_int a;
	u_int w;

	sysctlbyname(SYS_ACTIVE, &a,
		&(size_t){sizeof(a)}, NULL, 0);

	sysctlbyname(SYS_WIRED,  &w,
		&(size_t){sizeof(w)}, NULL, 0);

	sysctlbyname(SYS_PHYS,   &t,
		&(size_t){sizeof(t)}, NULL, 0);

	snprintf(buf, sizeof(buf),
		"%luMB / %luMB",
		((u_int64_t)a + w) * sysconf(_SC_PAGESIZE) >> 20,
		t >> 20);

	st->ram = strdup(buf);
	return (0);
}

int
set_disk(struct state *st)
{
	char buf[64];
	struct statvfs vfs;

	uint64_t total;
	uint64_t avail;
	uint64_t used;

	if (statvfs("/", &vfs) == -1)
		return (1);

	total = vfs.f_blocks * vfs.f_frsize;
	avail = vfs.f_bavail * vfs.f_frsize;
	used  = total - avail;

	total /= 1024 * 1024;
	used  /= 1024 * 1024;

	snprintf(buf, sizeof(buf),
		"%luMB / %luMB", used, total);
	st->disk = strdup(buf);
	return (0);
}

int
set_locale(struct state *st)
{
	char *loc;
	loc = getenv("LC_ALL");

	if (!loc || !*loc) {
		loc = getenv("LC_CTYPE");
	}
	if (!loc || !*loc) {
		loc = getenv("LANG");
	}
	st->locale = strdup(loc);
	return (0);
}

void
set_arch(struct state *st, struct utsname *u)
{
	st->arch = strdup(u->machine);
}


void
set_os(struct state *st, struct utsname *u)
{
	st->os = strdup(u->sysname);
}

void
set_kernel(struct state *st, struct utsname *u)
{
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

	set_kernel(&st, &u);
	set_arch(&st, &u);
	set_os(&st, &u);
	set_uptime(&st);

	set_packages(&st);
	set_ipaddr(&st);

	set_disk(&st);
	set_ram(&st);

	set_shell(&st);
	set_locale(&st);
	set_user(&st);

	putchar('\n');
	pr(USERTEXT,    st.user);
	pr(OSTEXT,      st.os);
	pr(ARCHTEXT,    st.arch);
	pr(KERNELTEXT,  st.kernel);
	pr(PACKAGETEXT, st.pkgs);
	pr(SHELLTEXT,   st.shell);
	pr(TERMTEXT,    getenv("TERM"));
	pr(EDTEXT,      getenv("EDITOR"));
	pr(UPTIMETEXT,  st.uptime);
	pr(IPTEXT,      st.ipaddr);
	pr(RAMTEXT,     st.ram);
	pr(DISKTEXT,    st.disk);
	pr(LOCALETEXT,  st.locale);
	putchar('\n');

	free_state(&st);
	return (0);
}
