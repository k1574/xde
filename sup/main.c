/* pancake <nopcode.org> -- Copyleft 2009-2011 */
/* k1574 k1574@yandex.ru 2020 */

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#define HELP "sup [-hlv] [cmd ..]"

struct rule_t {
	int uid;
	int gid;
	int suid;
	int sgid;
	const char *cmd;
	const char *path;
} ;

#include "config.h"

static int
die(int ret, const char *org, const char *str)
{
	fprintf (stderr, "%s%s%s\n", org?org:"", org?": ":"", str);
	return ret;
}

static char *
getpath(const char *str)
{
	struct stat st;
	static char file[4096];
	char *p, *path = getenv ("PATH");
	if (path)
	for (p = path; *p; p++) {
		if (*p==':' && (p>path&&*(p-1)!='\\')) {
			*p = 0;
			snprintf (file, sizeof(file)-1, "%s/%s", path, str);
			if (!lstat (file, &st))
				return file;
			*p = ':';
			path = p+1;
		}
	}
	return NULL;
}

int
main(int argc, char **argv) {
	const char *cmd;
	int i, uid, gid, suid, sgid, ret;
	if (argc < 2 || !strcmp (argv[1], "-h"))
		return die (1, NULL, HELP);
	if (!strcmp (argv[1], "-l")) {
		for (i = 0; rules[i].cmd != NULL; i++)
			printf ("%d %d %d %d %10s %s\n",
				rules[i].uid, rules[i].gid,
				rules[i].suid, rules[i].sgid,
				rules[i].cmd, rules[i].path);
		return 0;
	}
	uid = getuid() ;
	gid = getgid() ;
	for ( i = 0 ; rules[i].cmd != NULL ; ++i ) {
		if( *rules[i].cmd=='*' || !strcmp(argv[1], rules[i].cmd) ) {
			struct stat st;
			if (*rules[i].path=='*') {
				if (*argv[1]=='.' || *argv[1]=='/')
					cmd = argv[1];
				else if (!(cmd = getpath(argv[1])))
					return die(1, "execv", "cannot find program") ;
			} else
				cmd = rules[i].path ;
			
			if( lstat(cmd, &st) == -1 )
				return die (1, "lstat", "cannot stat program");
			if( st.st_mode & 0222 )
			#if ENFORCE_WRITE_CHECK
				return die (1, "stat", "cannot run writable binaries");
			#else
				fprintf(stderr, "%s: warning: %s: the file is writable\n", argv[0], cmd);	
			#endif
			suid = rules[i].suid ;
			sgid = rules[i].sgid ;
			if (uid != suid && rules[i].uid != -1 && rules[i].uid != uid)
				return die (1, "urule", "UID does not match.");
			if( gid!=sgid && rules[i].gid!=-1 && rules[i].gid!=gid )
				return die (1, "grule", "GID does not match.");
			if( setuid(suid)==-1 || setgid(sgid)==-1
				    || seteuid(suid)==-1 || setegid(sgid)==-1 )
				return die(1, "set[e][ug]id", strerror (errno)) ;
#ifdef CHROOT
			if (*CHROOT)
				if (chdir(CHROOT) == -1 || chroot(".") == -1)
					return die (1, "chroot", strerror (errno));
			if (*CHRDIR)
				if (chdir (CHRDIR) == -1)
					return die (1, "chdir", strerror (errno));
#endif
			ret = execv(cmd, argv+1) ;
			return die(ret, "execv", strerror (errno)) ;
		}
	}
	return die(1, NULL, "Nope") ;
}
