#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#include <unistd.h>
#include <time.h>
#include <cutils/log.h>
#include "sensors.h"

extern "C" int chown(const char *, uid_t, gid_t);

int chown_main(const char *groupname, const char *filename)
{
        char user[32];
        char *group = NULL;
        strncpy(user, groupname, sizeof(user));
        
        if ((group = strchr(user, ':')) != NULL) {
                *group++ = '\0';
        } else if ((group = strchr(user, '.')) != NULL) {
                *group++ = '\0';
        }

        // Lookup uid (and gid if specified)
        struct passwd *pw;
        struct group *grp = NULL;
        uid_t uid;
        gid_t gid = -1; // passing -1 to chown preserves current group

        pw = getpwnam(user);
        if (pw != NULL) {
                uid = pw->pw_uid;
        } else {
                char* endptr;
                uid = (int) strtoul(user, &endptr, 0);
                if (endptr == user) {  // no conversion
                        ALOGD("No such user '%s'\n", user);
                        return 10;
                }
        }

        if (group != NULL) {
                grp = getgrnam(group);
                if (grp != NULL) {
                        gid = grp->gr_gid;
                } else {
                        char* endptr;
                        gid = (int) strtoul(group, &endptr, 0);
                        if (endptr == group) {  // no conversion
                                ALOGD("No such group '%s'\n", group);
                                return 10;
                        }
                }
        }
        ALOGD("%s:uid:%d, gid:%d", filename, uid, gid);
        if (chown(filename, uid, gid) < 0) {
                ALOGD("Unable to chown %s: %s\n", filename, strerror(errno));
                return 10;
        }
        

    return 0;
}
