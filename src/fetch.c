#include <stddef.h>
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include "config.h"

struct
dist {
  char *getPkgCount;
};

struct 
dist info = {
  .getPkgCount = "echo unsupported",
};

char *shellname, *pkgcount,
     *osname,    *wm;

char *krnlver;

char
*pipeRead(const char *exec)
{
  FILE *pipe;
  char *res;

  pipe = popen(exec, "r");
  res  = malloc(256);

  if (fscanf(pipe, "%255[^\n]", res) == EOF) {
    strncpy(res, "0", 256);
  }
  pclose(pipe);
  return res;
}

void
kernel()
{
  static struct utsname kerneldata;
  uname(&kerneldata);
  krnlver = kerneldata.release;
}

void
shell()
{
  char *shell = getenv("SHELL");
  char *slash = strrchr(shell, '/');
  if (slash) {
    shell = slash + 1;
  }
  shellname = shell;
}

const char *
get_wm(void)
{
  int mib[] = { CTL_KERN, KERN_PROC,
                KERN_PROC_PROC, 0 };
  size_t len;
  struct kinfo_proc *kp;
  const char *wms[] = { "i3", "dwm", "sway",
                        "openbox", NULL };
  const char *res = "unknown";

  if (getenv("XDG_CURRENT_DESKTOP"))
    return getenv("XDG_CURRENT_DESKTOP");

  sysctl(mib, 4, NULL, &len, NULL, 0);
  kp = malloc(len);
  if (!kp) {
    perror("maloc shii");
    return "unknown";
  }
  sysctl(mib, 4, kp, &len, NULL, 0);

  for (size_t i = 0; i < len / sizeof(struct kinfo_proc); i++)
    for (int j = 0; wms[j]; j++)
      if (strcmp(kp[i].ki_comm, wms[j]) == 0)
        res = wms[j];

  free(kp);
  return res;
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

  return buf;
}

void
os()
{
  static struct utsname sysinfo;
  uname(&sysinfo);

  if (!strncmp(sysinfo.sysname, "FreeBSD", 7)) {
    info.getPkgCount = "pkg info | wc -l | tr -d ' '";
  } else if (!strncmp(sysinfo.sysname, "OpenBSD", 7)) {
  info.getPkgCount =
    "/bin/ls -1 /var/db/pkg/ | wc -l | tr -d ' '";
	}

  osname = sysinfo.sysname;
  pkgcount = pipeRead(info.getPkgCount);
}

int
main(void)
{
  wm = (char *)get_wm();
  kernel();
  shell();
  os();

  puts("");
  printf("%s%*s%s\n", USERTEXT,    TABSIZE, "", getenv("USER"));
  printf("%s%*s%s\n", OSTEXT,      TABSIZE, "", osname);
  printf("%s%*s%s\n", KERNELTEXT,  TABSIZE, "", krnlver);
  printf("%s%*s%s\n", PACKAGETEXT, TABSIZE, "", pkgcount);
  printf("%s%*s%s\n", SHELLTEXT,   TABSIZE, "", shellname);
  printf("%s%*s%s\n", TERMTEXT,    TABSIZE, "", getenv("TERM"));
  printf("%s%*s%s\n", WMTEXT,      TABSIZE, "", wm);
  printf("%s%*s%s\n", EDTEXT,      TABSIZE, "", getenv("EDITOR"));
  printf("%s%*s%s\n", UPTIMETEXT,  TABSIZE, "", uptime());
  puts("");

  free(pkgcount);
  return 0;
}
