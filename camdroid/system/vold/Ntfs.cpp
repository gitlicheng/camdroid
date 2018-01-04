/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>

#include <linux/kdev_t.h>

#define LOG_TAG "Vold"

#include <cutils/log.h>
#include <cutils/properties.h>

#include "Ntfs.h"

static char FSCK_NTFS3G_PATH[] 	="/system/bin/ntfs-3g.probe";
static char MOUNT_NTFS3G_PATH[] ="/system/bin/ntfs-3g";

extern "C" int logwrap(int argc, const char **argv, int background);
extern "C" int mount(const char *, const char *, const char *, unsigned long, const void *);

int Ntfs::check(const char *fsPath) {
    SLOGV("Ntfs::check");
	const char *args[4];
	int rc;

	if (access(FSCK_NTFS3G_PATH, X_OK)) {
        SLOGW("Skipping fs checks\n");
        return 0;
    }

	/* we first mount it read and write*/
    args[0] = FSCK_NTFS3G_PATH;
    args[1] = "--readwrite";
    args[2] = fsPath;
    args[3] = NULL;

    rc = logwrap(3, args, 1);
	if( (rc != 0) && (rc !=15) )
	{
       SLOGE("Filesystem check failed (unknown exit code %d)", rc);
	   return -1;
    }

    return 0;
}

int Ntfs::doMount(const char *fsPath, const char *mountPoint,
                 bool ro, bool remount, bool executable,
                 int ownerUid, int ownerGid, int permMask, bool createLost) {
#if 0
    int rc;
    unsigned long flags;
    char mountData[255];

 //  flags = MS_NODEV | MS_NOSUID | MS_DIRSYNC;
  flags = MS_NODEV | MS_NOSUID ;//| MS_DIRSYNC;
    flags |= (executable ? 0 : MS_NOEXEC);
    flags |= (ro ? MS_RDONLY : 0);
    flags |= (remount ? MS_REMOUNT : 0);

        permMask = 0;

    sprintf(mountData,
            "nls=utf8,uid=%d,gid=%d,fmask=%o,dmask=%o",
            ownerUid, ownerGid, permMask, permMask);

    SLOGE("[lkj]:mount flags %s, mountData %s\n",flags, mountData, );
    rc = mount(fsPath, mountPoint, "ntfs", flags, mountData);

    if (rc && errno == EROFS) {
        SLOGE("%s appears to be a read only filesystem - retrying mount RO", fsPath);
        flags |= MS_RDONLY;
        rc = mount(fsPath, mountPoint, "ntfs", flags, mountData);
    }
    return rc;
#else
    int rc;
    const char *args[11];
    char mountData[255];

    sprintf(mountData,
            "locale=utf8,uid=%d,gid=%d,fmask=%o,dmask=%o,noatime,nodiratime",
            ownerUid, ownerGid, permMask, permMask);

    args[0] = MOUNT_NTFS3G_PATH;
    args[1] = fsPath;
    args[2] = mountPoint;
    args[3] = "-o";
    args[4] = mountData;
    args[5] = NULL;
    rc = logwrap(5, args, 1);

    return rc;
#endif

}

int Ntfs::format(const char *fsPath, unsigned int numSectors) {
    SLOGW("[lkj]:Skipping ntfs format\n");
    return 0;
}
