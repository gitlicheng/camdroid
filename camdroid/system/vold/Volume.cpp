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

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/param.h>
#define LOG_NDEBUG 0
#include <linux/kdev_t.h>
#include <linux/fs.h>

#include <cutils/properties.h>

#include <diskconfig/diskconfig.h>

#include <private/android_filesystem_config.h>

#define LOG_TAG "Volume"

#include <cutils/log.h>

#include "Volume.h"
#include "VolumeManager.h"
#include "ResponseCode.h"
#include "Fat.h"
#include "Ntfs.h"
#include "Exfat.h"
#include "Ext4.h"
#include "Process.h"
#include "cryptfs.h"

#define  V_MAX_PARTITIONS              16

extern "C" void dos_partition_dec(void const *pp, struct dos_partition *d);
extern "C" void dos_partition_enc(void *pp, struct dos_partition *d);


/*
 * Secure directory - stuff that only root can see
 */
const char *Volume::SECDIR            = "/mnt/secure";

/*
 * Secure staging directory - where media is mounted for preparation
 */
const char *Volume::SEC_STGDIR        = "/mnt/secure/staging";

/*
 * Path to the directory on the media which contains publicly accessable
 * asec imagefiles. This path will be obscured before the mount is
 * exposed to non priviledged users.
 */
const char *Volume::SEC_STG_SECIMGDIR = "/mnt/secure/staging/.android_secure";

/*
 * Path to external storage where *only* root can access ASEC image files
 */
const char *Volume::SEC_ASECDIR_EXT   = "/mnt/secure/asec";

/*
 * Path to internal storage where *only* root can access ASEC image files
 */
const char *Volume::SEC_ASECDIR_INT   = "/data/app-asec";
/*
 * Path to where secure containers are mounted
 */
const char *Volume::ASECDIR           = "/mnt/asec";

/*
 * Path to where OBBs are mounted
 */
const char *Volume::LOOPDIR           = "/mnt/obb";

static const char *stateToStr(int state) {
    if (state == Volume::State_Init)
        return "Initializing";
    else if (state == Volume::State_NoMedia)
        return "No-Media";
    else if (state == Volume::State_Idle)
        return "Idle-Unmounted";
    else if (state == Volume::State_Pending)
        return "Pending";
    else if (state == Volume::State_Mounted)
        return "Mounted";
    else if (state == Volume::State_Unmounting)
        return "Unmounting";
    else if (state == Volume::State_Checking)
        return "Checking";
    else if (state == Volume::State_Formatting)
        return "Formatting";
    else if (state == Volume::State_Shared)
        return "Shared-Unmounted";
    else if (state == Volume::State_SharedMnt)
        return "Shared-Mounted";
    else
        return "Unknown-Error";
}

Volume::Volume(VolumeManager *vm, const char *label, const char *mount_point) {
    mVm = vm;
    mDebug = true;
    mLabel = strdup(label);
    mMountpoint = strdup(mount_point);
    mState = Volume::State_Init;
    mCurrentlyMountedKdev = -1;
    mPartIdx = -1;
    mRetryMount = false;
    mShareFlags  = 0;
    for(int i = 0; i < MAX_PARTITIONS; i++){
   		 mMountPart[i] = NULL;
       mSharelun[i] = 0;
    }

   	for(int i = 0; i < MAX_UNMOUNT_PARTITIONS; i++){
  		 mUnMountPart[i] = NULL;
   	}
}

Volume::~Volume() {
    free(mLabel);
    free(mMountpoint);
}

void Volume::setShareFlags(int flag){    
    mShareFlags = flag;
    ALOGD("setShareFlags mShareFlags = %d", mShareFlags);
}

int Volume::getShareFlags(){
    ALOGD("getShareFlags mShareFlags = %d", mShareFlags);
    return mShareFlags;
}
void Volume::protectFromAutorunStupidity() {
    char filename[255];

    snprintf(filename, sizeof(filename), "%s/autorun.inf", SEC_STGDIR);
    if (!access(filename, F_OK)) {
        ALOGW("Volume contains an autorun.inf! - removing");
        /*
         * Ensure the filename is all lower-case so
         * the process killer can find the inode.
         * Probably being paranoid here but meh.
         */
        rename(filename, filename);
        Process::killProcessesWithOpenFiles(filename, 2);
        if (unlink(filename)) {
            ALOGE("Failed to remove %s (%s)", filename, strerror(errno));
        }
    }
}

void Volume::setDebug(bool enable) {
    mDebug = enable;
}

dev_t Volume::getDiskDevice() {
    return MKDEV(0, 0);
};

dev_t Volume::getShareDevice() {
    return getDiskDevice();
}

void Volume::handleVolumeShared() {
}

void Volume::handleVolumeUnshared() {
}

int Volume::handleBlockEvent(NetlinkEvent *evt) {
    errno = ENOSYS;
    return -1;
}

int Volume::getState()
{
   ALOGD("line=%d,mState=%d",__LINE__,mState);
   return mState; 
}
void Volume::setState(int state) {
    char msg[255];
    int oldState = mState;

    ALOGE("line=%d,setState oldState=%d,state=%d",__LINE__,oldState,state);
    if (oldState == state) {
        ALOGW("line=%d,Duplicate state (%d)\n",__LINE__, state);
        return;
    }

    if ((oldState == Volume::State_Pending) && (state != Volume::State_Idle)) {
        mRetryMount = false;
    }

    mState = state;
   
    snprintf(msg, sizeof(msg),
             "Volume %s %s state changed from %d (%s) to %d (%s)", getLabel(),
             getMountpoint(), oldState, stateToStr(oldState), mState,
             stateToStr(mState));
    SocketListener *sl = mVm->getBroadcaster();
    //ALOGE("111111aaaaaaaVolume::setState msg=%s,mVm=%p,4sl=%p",msg,mVm,sl);    
   // ALOGE("111111aaaaaaaVolume::setState,mVm=%p,4sl=%p",mVm,sl);
    ALOGE("line=%d,sendBroadcast start",__LINE__);
    sl->sendBroadcast(ResponseCode::VolumeStateChange,
                                         msg, false);
    ALOGE("line=%d,sendBroadcast end",__LINE__);
}

int Volume::createDeviceNode(const char *path, int major, int minor) {
    mode_t mode = 0660 | S_IFBLK;
    dev_t dev = (major << 8) | minor;
    ALOGD("---createDeviceNode---path=%s",path);
    if (mknod(path, mode, dev) < 0) {
        if (errno != EEXIST) {
		        return -1;
		}
    }		
    return 0;
}

/* path: partition mount path. eg: '/mnt/usbhost1/8_1' */
int Volume::deleteDeviceNode(const char *path){
#if 0
    int major = 0, minor = 0;
       char devicePath[255];

       char *temp_str1 = NULL;
       char *temp_str2 = NULL;
       char str_major[256];
       char str_path[256];
       int len = 0;

       if(!path){
               ALOGE("Volume::deleteDeviceNode: path(%s) is invalid\n", path);
               return -1;
       }

       ALOGI("Volume::deleteDeviceNode: path=%s\n", path);

       /* get device major and minor from path */
       memset(str_major, 0, 256);
       memset(str_path, 0, 256);
       strcpy(str_path, path);

       temp_str1 = strrchr(str_path, '/');
       temp_str2 = strrchr(str_path, '_');
       if(temp_str1 == NULL || temp_str2 == NULL){
               ALOGE("Volume::deleteDeviceNode: path(%s) is invalid\n", path);
               return -1;
       }

       /* delete '/' & '_' */
       temp_str1++;
       temp_str2++;
       if(temp_str1 == NULL || temp_str2 == NULL){
               ALOGE("Volume::deleteDeviceNode: path(%s) is invalid\n", path);
               return -1;
       }

       len = strcspn(temp_str1, "_");
       strncpy(str_major, temp_str1, len);

       major = strtol(str_major, NULL, 10);
       minor = strtol(temp_str2, NULL, 10);

       ALOGI("Volume::deleteDeviceNode: major=%d, minor=%d\n", major, minor);

       /* delete DeviceNode */
       memset(devicePath, 0, 255);
       sprintf(devicePath, "/dev/block/vold/%d:%d", major, minor);

       if (unlink(devicePath)) {
               ALOGE("Volume::deleteDeviceNode: Failed to remove %s (%s)", path, strerror(errno));
               return -1;
       }else{
               ALOGI("Volume::deleteDeviceNode: delete DeviceNode '%s' successful\n", path);
       }

#endif

       return 0;
}

char* Volume::createMountPoint(const char *path, int major, int minor) {
       char* mountpoint = (char*) malloc(sizeof(char)*256);

       memset(mountpoint, 0, sizeof(char)*256);
       sprintf(mountpoint, "%s/%d_%d", path, major, minor);

       if( access(mountpoint, F_OK) ){
               ALOGI("Volume: file '%s' is not exist, create it", mountpoint);

               if(mkdir(mountpoint, 0777)){
                       ALOGW("Volume: create file '%s' failed, errno is %d", mountpoint, errno);
                       return NULL;
               }
       }else{
               /* ¨¦?¨°?¡ä?¨°?¨¢?¦Ì??¨¤¡¤???1¨°??¦Ì?¡ê??a¡ä?¨º?¡¤??1D¨¨¨°a¡À?¨º1¨®???? */
               ALOGW("Volume: file '%s' is exist, can not create it", mountpoint);
               return mountpoint;
       }

       return mountpoint;
}

int Volume::deleteMountPoint(char* mountpoint) {
		ALOGD("***************Volume::deleteMountPoint: %s****************", mountpoint);
       if(mountpoint){
               if( !access(mountpoint, F_OK) ){
                       ALOGD("***************Volume::deleteMountPoint: %s****************", mountpoint);
                       if(rmdir(mountpoint)){
                               ALOGW("******************Volume: remove file '%s' failed, errno is %d********", mountpoint, errno);
                               return -1;
                       }
					   ALOGD("***************Volume::deleteMountPoint rmdir suceess : %s****************", mountpoint);
               }
               ALOGD("***************Volume::deleteMountPoint suceess : %s****************", mountpoint);
               /* ?T??¨º?¡¤?¨¦?3y3¨¦1|¡ê??a¨¤???¨º¨ª¡¤??¨²¡ä? */
               free(mountpoint);
               mountpoint = NULL;
       }

       return 0;
}

void Volume::saveUnmountPoint(char* mountpoint){
       int i = 0;

       for(i = 0; i < MAX_UNMOUNT_PARTITIONS; i++){
               if(mUnMountPart[i] == NULL){
                       mUnMountPart[i] = mMountPart[i];
               }
       }

       if(i >= MAX_UNMOUNT_PARTITIONS){
               ALOGI("Volume::saveUnmountPoint: unmount point is over %d", MAX_UNMOUNT_PARTITIONS);
       }

       return;
}

/* ¨¦?3y?¨´¨®D1¨°?????? */
void Volume::deleteUnMountPoint(int clear){
       int i = 0;

       for(i = 0; i < MAX_UNMOUNT_PARTITIONS; i++){
               if(mUnMountPart[i]){
                       ALOGW("Volume::deleteUnMountPoint: %s", mUnMountPart[i]);

                       if(deleteMountPoint(mUnMountPart[i]) == 0){
                               deleteDeviceNode(mUnMountPart[i]);
                               mUnMountPart[i] = NULL;
                       }
               }
       }

       return;
}




int Volume::formatVol() {

	  char lable[32];

    if (getState() == Volume::State_NoMedia) {
        errno = ENODEV;
        return -1;
    } else if (getState() != Volume::State_Idle) {
        errno = EBUSY;
        return -1;
    }

    if (isMountpointMounted(getMountpoint())) {
        ALOGW("Volume is idle but appears to be mounted - fixing");
        setState(Volume::State_Mounted);
        // mCurrentlyMountedKdev = XXX
        errno = EBUSY;
        return -1;
    }

    bool formatEntireDevice = (mPartIdx == -1);
    char devicePath[255];
    dev_t diskNode = getDiskDevice();
    dev_t partNode = MKDEV(MAJOR(diskNode), (formatEntireDevice ? 1 : mPartIdx));

    setState(Volume::State_Formatting);

    int ret = -1;
    // Only initialize the MBR if we are formatting the entire device
    getDeviceNodes(&partNode, 1);
    sprintf(devicePath, "/dev/block/vold/%d:%d",MAJOR(partNode), MINOR(partNode));

    if (mDebug) {
        ALOGI("Formatting volume %s (%s)", getLabel(), devicePath);
    }

    property_get("ro.udisk.lable", lable, "udisk");
    if (Fat::format(devicePath, 0,lable)) {
        ALOGE("Failed to format (%s)", strerror(errno));
        goto err;
    }

    ret = 0;

err:
    setState(Volume::State_Idle);
    return ret;
}

bool Volume::isMountpointMounted(const char *path) {
    char device[256];
    char mount_path[256];
    char rest[256];
    FILE *fp;
    char line[1024];

    if (!(fp = fopen("/proc/mounts", "r"))) {
        ALOGE("Error opening /proc/mounts (%s)", strerror(errno));
        return false;
    }

    while(fgets(line, sizeof(line), fp)) {
        line[strlen(line)-1] = '\0';
        sscanf(line, "%255s %255s %255s\n", device, mount_path, rest);
        if (!strcmp(mount_path, path)) {
            fclose(fp);
            return true;
        }

    }

    fclose(fp);
    return false;
}

int Volume::mountVol() {
    dev_t deviceNodes[V_MAX_PARTITIONS];
    int n, i, rc = 0;
    int mounted = 0;
    char errmsg[255];
    const char* externalStorage = getenv("EXTERNAL_STORAGE");
    bool primaryStorage = externalStorage && !strcmp(getMountpoint(), externalStorage);
    char decrypt_state[PROPERTY_VALUE_MAX];
    char crypto_state[PROPERTY_VALUE_MAX];
    char encrypt_progress[PROPERTY_VALUE_MAX];
    int flags;

    property_get("vold.decrypt", decrypt_state, "");
    property_get("vold.encrypt_progress", encrypt_progress, "");

    /* Don't try to mount the volumes if we have not yet entered the disk password
     * or are in the process of encrypting.
     */
    ALOGD("line=%d,mountVol state : %d",__LINE__,getState());
    if ((getState() == Volume::State_NoMedia) ||
        ((!strcmp(decrypt_state, "1") || encrypt_progress[0]) && primaryStorage)) {
        snprintf(errmsg, sizeof(errmsg),
                 "Volume %s %s mount failed - no media",
                 getLabel(), getMountpoint());
        mVm->getBroadcaster()->sendBroadcast(
                                         ResponseCode::VolumeMountFailedNoMedia,
                                         errmsg, false);
        errno = ENODEV;
        return -1;
    } else if (getState() != Volume::State_Idle) {
        errno = EBUSY;
        if (getState() == Volume::State_Pending) {
            mRetryMount = true;
        }
        
        ALOGW("line=%d,Volume::mountVol: Volume mState is not State_Idle",__LINE__);
        
        return -1;
    }

    if (isMountpointMounted(getMountpoint())) {
        ALOGW("Volume is idle but appears to be mounted - fixing");
        setState(Volume::State_Mounted);
        // mCurrentlyMountedKdev = XXX
        return 0;
    }

    n = getDeviceNodes((dev_t *) &deviceNodes, V_MAX_PARTITIONS);
    if (!n) {
        ALOGE("Failed to get device nodes (%s)\n", strerror(errno));
        return -1;
    }

    /* If we're running encrypted, and the volume is marked as encryptable and nonremovable,
     * and vold is asking to mount the primaryStorage device, then we need to decrypt
     * that partition, and update the volume object to point to it's new decrypted
     * block device
     */
    property_get("ro.crypto.state", crypto_state, "");
    flags = getFlags();
    if (primaryStorage &&
        ((flags & (VOL_NONREMOVABLE | VOL_ENCRYPTABLE))==(VOL_NONREMOVABLE | VOL_ENCRYPTABLE)) &&
        !strcmp(crypto_state, "encrypted") && !isDecrypted()) {
       char new_sys_path[MAXPATHLEN];
       char nodepath[256];
       int new_major, new_minor;

       if (n != 1) {
           /* We only expect one device node returned when mounting encryptable volumes */
           ALOGE("Too many device nodes returned when mounting %s\n", getMountpoint());
           return -1;
       }
#ifdef VOLD_SUPPORT_CRYPTFS
       if (cryptfs_setup_volume(getLabel(), MAJOR(deviceNodes[0]), MINOR(deviceNodes[0]),
                                new_sys_path, sizeof(new_sys_path),
                                &new_major, &new_minor)) {
           ALOGE("Cannot setup encryption mapping for %d\n", getMountpoint());
           return -1;
       }
#else
        ALOGE("Cannot setup encryption mapping for %s\n", getMountpoint());
        return -1;
#endif
       /* We now have the new sysfs path for the decrypted block device, and the
        * majore and minor numbers for it.  So, create the device, update the
        * path to the new sysfs path, and continue.
        */
        snprintf(nodepath,
                 sizeof(nodepath), "/dev/block/vold/%d:%d",
                 new_major, new_minor);
        if (createDeviceNode(nodepath, new_major, new_minor)) {
            ALOGE("Error making device node '%s' (%s)", nodepath,
                                                       strerror(errno));
        }

        // Todo: Either create sys filename from nodepath, or pass in bogus path so
        //       vold ignores state changes on this internal device.
        updateDeviceInfo(nodepath, new_major, new_minor);

        /* Get the device nodes again, because they just changed */
        n = getDeviceNodes((dev_t *) &deviceNodes, V_MAX_PARTITIONS);
        if (!n) {
            ALOGE("Failed to get device nodes (%s)\n", strerror(errno));
            return -1;
        }
    }
    
    ALOGD("line=%d,Volume::mountVol: mMountpoint %s\n",__LINE__, mMountpoint);

		if(((mPartIdx == -1) &&(n > 1)) && mMountpoint){
		       chmod(mMountpoint, 0x777);
		
		       deleteUnMountPoint(1);
		}
    ALOGI("line=%d,mountVol: n=%d \n",__LINE__,n);
    for (i = 0; i < n; i++) {
        char devicePath[255];

				memset(devicePath, 0, 255);
        sprintf(devicePath, "/dev/block/vold/%d:%d", MAJOR(deviceNodes[i]),
                MINOR(deviceNodes[i]));

        ALOGI("line=%d,%s being considered for volume %s\n",__LINE__,devicePath, getLabel());

        errno = 0;
        setState(Volume::State_Checking);

#if 0
        if (Fat::check(devicePath)) {
            if (errno == ENODATA) {
                ALOGW("%s does not contain a FAT filesystem\n", devicePath);
                continue;
            }
            errno = EIO;
            /* Badness - abort the mount */
            ALOGE("%s failed FS checks (%s)", devicePath, strerror(errno));
            setState(Volume::State_Idle);
            return -1;
        }
#endif

        /*
         * Mount the device on our internal staging mountpoint so we can
         * muck with it before exposing it to non priviledged users.
         */
        errno = 0;
        int gid;

        if (primaryStorage) {
            // Special case the primary SD card.
            // For this we grant write access to the SDCARD_RW group.
            gid = AID_SDCARD_RW;
        } else {
            // For secondary external storage we keep things locked up.
            gid = AID_MEDIA_RW;
        }
        

        if( !Fat::check(devicePath))
        {
            ALOGD("line=%d,FS checks %s",__LINE__,devicePath);
             if( Fat::doMount(devicePath, getMountpoint(), false, false, true, AID_SYSTEM, gid, 0702, true) )
             {
                    ALOGE("line=%d,%s failed to mount via VFAT (%s)\n",__LINE__, devicePath, strerror(errno));
					setState(Volume::State_Idle);
					return -1;
			 }
        }
        else
        {
            ALOGE("%s failed FS checks (%s)", devicePath, strerror(errno));
            setState(Volume::State_Pending);
            continue;
        }        
        
#if 0
        if (Fat::doMount(devicePath, "/mnt/secure/staging", false, false, false,
                AID_SYSTEM, gid, 0702, true)) {
            ALOGE("%s failed to mount via VFAT (%s)\n", devicePath, strerror(errno));
            continue;
        }
#endif

        ALOGE("line=%d,Device %s, target %s mounted @ /mnt/secure/staging",__LINE__,devicePath, getMountpoint());

        protectFromAutorunStupidity();

        // only create android_secure on primary storage
        if (primaryStorage && createBindMounts()) {
            ALOGE("Failed to create bindmounts (%s)", strerror(errno));
            umount("/mnt/secure/staging");
            setState(Volume::State_Idle);
            return -1;
        }

        /*
         * Now that the bindmount trickery is done, atomically move the
         * whole subtree to expose it to non priviledged users.
         */
#if 0
        if (doMoveMount("/mnt/secure/staging", getMountpoint(), false)) {
            ALOGE("Failed to move mount (%s)", strerror(errno));
            umount("/mnt/secure/staging");
            setState(Volume::State_Idle);
            return -1;
        }
        setState(Volume::State_Mounted);
        mCurrentlyMountedKdev = deviceNodes[i];
        return 0;
#else

#if 0
	       /* auto mount and much partition */
	     if((mPartIdx == -1) && (n > 1)){
            ALOGD("line=%d,mPartIdx=%d,n=%d",__LINE__,mPartIdx,n);
           mMountPart[i] = createMountPoint( mMountpoint, MAJOR(deviceNodes[i]), MINOR(deviceNodes[i]) );
           if(mMountPart[i] == NULL){
                   ALOGE("Part is already mount, can not mount again, (%s)\n", strerror(errno));
                   umount("/mnt/secure/staging");
                   continue;
           }
	
           if (doMoveMount("/mnt/secure/staging", mMountPart[i], false)) {
                   ALOGE("line=%d,Part(%s) failed to move mount (%s)\n",__LINE__,mMountPart[i], strerror(errno));
                   deleteMountPoint(mMountPart[i]);
                   mMountPart[i] = NULL;
                   //umount("/mnt/secure/staging");
                   continue;
           }

           ALOGI("mountVlo: mount %s, successful\n", mMountPart[i]);

           mCurrentlyMountedKdev = deviceNodes[i];
           mounted++;
	     }else{
	        ALOGD("line=%d,mPartIdx=%d,n=%d,mountpoint=%s",__LINE__,mPartIdx,n,getMountpoint()); 
	        if (doMoveMount("/mnt/secure/staging", getMountpoint(), false)) {
				     ALOGE("Failed to move mount (%s)\n", strerror(errno));
				     //umount("/mnt/secure/staging");
				     goto failed;
	     		}
	
		      setState(Volume::State_Mounted);
		      mCurrentlyMountedKdev = deviceNodes[i];
		      mMountedPartNum = 1;
		
		     	return 0;
	     }
#endif
    setState(Volume::State_Mounted);
    mCurrentlyMountedKdev = deviceNodes[i];
    mMountedPartNum = 1;
    return 0;
#endif
	}
	
    mMountedPartNum = n;
    ALOGD("line=%d,mounted=%d",__LINE__,mounted);
    if(mounted){
     setState(Volume::State_Mounted);
    }else{
         ALOGE("line=%d,mount part failed\n",__LINE__);

         mMountedPartNum = 0;
         setState(Volume::State_Idle);

         goto failed;
    }

   ALOGI("Volume::mountVol: getState=%d, State_Mounted=%d\n", getState(), Volume::State_Mounted);

   return 0;

failed:
    ALOGE("Volume %s found no suitable devices for mounting :(\n", getLabel());
    setState(Volume::State_Idle);
    mCurrentlyMountedKdev = -1;

    return -1;
}

int Volume::createBindMounts() {
    unsigned long flags;

    /*
     * Rename old /android_secure -> /.android_secure
     */
    if (!access("/mnt/secure/staging/android_secure", R_OK | X_OK) &&
         access(SEC_STG_SECIMGDIR, R_OK | X_OK)) {
        if (rename("/mnt/secure/staging/android_secure", SEC_STG_SECIMGDIR)) {
            ALOGE("Failed to rename legacy asec dir (%s)", strerror(errno));
        }
    }

    /*
     * Ensure that /android_secure exists and is a directory
     */
    if (access(SEC_STG_SECIMGDIR, R_OK | X_OK)) {
        if (errno == ENOENT) {
            if (mkdir(SEC_STG_SECIMGDIR, 0777)) {
                ALOGE("Failed to create %s (%s)", SEC_STG_SECIMGDIR, strerror(errno));
                return -1;
            }
        } else {
            ALOGE("Failed to access %s (%s)", SEC_STG_SECIMGDIR, strerror(errno));
            return -1;
        }
    } else {
        struct stat sbuf;

        if (stat(SEC_STG_SECIMGDIR, &sbuf)) {
            ALOGE("Failed to stat %s (%s)", SEC_STG_SECIMGDIR, strerror(errno));
            return -1;
        }
        if (!S_ISDIR(sbuf.st_mode)) {
            ALOGE("%s is not a directory", SEC_STG_SECIMGDIR);
            errno = ENOTDIR;
            return -1;
        }
    }

    /*
     * Bind mount /mnt/secure/staging/android_secure -> /mnt/secure/asec so we'll
     * have a root only accessable mountpoint for it.
     */
    if (mount(SEC_STG_SECIMGDIR, SEC_ASECDIR_EXT, "", MS_BIND, NULL)) {
        ALOGE("Failed to bind mount points %s -> %s (%s)",
                SEC_STG_SECIMGDIR, SEC_ASECDIR_EXT, strerror(errno));
        return -1;
    }

    /*
     * Mount a read-only, zero-sized tmpfs  on <mountpoint>/android_secure to
     * obscure the underlying directory from everybody - sneaky eh? ;)
     */
    if (mount("tmpfs", SEC_STG_SECIMGDIR, "tmpfs", MS_RDONLY, "size=0,mode=000,uid=0,gid=0")) {
        ALOGE("Failed to obscure %s (%s)", SEC_STG_SECIMGDIR, strerror(errno));
        umount("/mnt/asec_secure");
        return -1;
    }

    return 0;
}

int Volume::doMoveMount(const char *src, const char *dst, bool force) {
    unsigned int flags = MS_MOVE;
    int retries = 5;
	int ret = 0; 
	
    while(retries--) {
		 ret = mount(src, dst, "", flags, NULL);
        if (!ret) {
            if (mDebug) {
                ALOGD("line=%d,Moved mount %s -> %s sucessfully",__LINE__, src, dst);
            }
            return 0;
        } else if (errno != EBUSY) {
            ALOGE("line=%d,Warning to move mount %s -> %s (%s)",__LINE__,src, dst, strerror(errno));
            return -1;
        }
        int action = 0;

        if (force) {
            if (retries == 1) {
                action = 2; // SIGKILL
            } else if (retries == 2) {
                action = 1; // SIGHUP
            }
        }
        ALOGW("Failed to move %s -> %s (%s, retries %d, action %d)",
                src, dst, strerror(errno), retries, action);
        Process::killProcessesWithOpenFiles(src, action);
        usleep(1000*250);
    }
	
    errno = EBUSY;
    ALOGE("Giving up on move %s -> %s (%s)", src, dst, strerror(errno));
    return -1;
}

int Volume::doUnmount(const char *path, bool force) {
    int retries = 10;
    ALOGD("Unmounting---------- {%s}, force = %d", path, force);
    if (mDebug) {
        ALOGD("Unmounting {%s}, force = %d", path, force);
    }

    while (retries--) {
        if (!umount(path) || errno == EINVAL || errno == ENOENT || errno == ENOTCONN) {
            ALOGI("%s sucessfully unmounted", path);
            return 0;
        }

        int action = 0;

        if (force) {
            if (retries == 1) {
                action = 2; // SIGKILL
            } else if (retries == 2) {
                action = 1; // SIGHUP
            }
        }

        ALOGW("Failed to unmount %s (%s, retries %d, action %d)",
                path, strerror(errno), retries, action);

        Process::killProcessesWithOpenFiles(path, action);
        usleep(1000*1000);
    }
    errno = EBUSY;
    ALOGE("Giving up on unmount %s (%s)", path, strerror(errno));
    return -1;
}


int Volume::unmountVol(bool force, bool revert) {
    int i, rc;
    ALOGD("Volume::unmountVol ");
    if (getState() != Volume::State_Mounted) {
        ALOGE("Volume %s unmount request when not mounted", getLabel());
        errno = EINVAL;
        return UNMOUNT_NOT_MOUNTED_ERR;
    }

    setState(Volume::State_Unmounting);
    usleep(1000 * 1000); // Give the framework some time to react

    if(getMountpoint()!=NULL&&!strstr(getMountpoint(),"usb")&&!strstr(getMountpoint(),"extsd")){
        /*
         * Remove the bindmount we were using to keep a reference to
         * the previously obscured directory.
         */
         ALOGD("Volume::unmountVol 1111,SEC_ASECDIR_EXT=%s",Volume::SEC_ASECDIR_EXT);
        if (doUnmount(Volume::SEC_ASECDIR_EXT, force)) {
            ALOGE("Failed to remove bindmount on %s (%s)", SEC_ASECDIR_EXT, strerror(errno));
            goto fail_remount_tmpfs;
        }

        /*
         * Unmount the tmpfs which was obscuring the asec image directory
         * from non root users
         */
        char secure_dir[PATH_MAX];
        snprintf(secure_dir, PATH_MAX, "%s/.android_secure", getMountpoint());
        if (doUnmount(secure_dir, force)) {
            ALOGE("Failed to unmount tmpfs on %s (%s)", secure_dir, strerror(errno));
            goto fail_republish;
        }
    }
    ALOGD("mPartIdx = %d, mMountedPartNum = %d\n",mPartIdx,mMountedPartNum);
    for(i=0;i<mMountedPartNum;i++)
    {
    	if(mPartIdx==-1&&mMountedPartNum>1){
    		if(mMountPart[i]){
    			if (doUnmount(mMountPart[i], force)) {
    			        ALOGE("Failed to unmount %s (%s)", SEC_STGDIR, strerror(errno));
    			        goto fail_recreate_bindmount;
    			 }
                if(deleteMountPoint(mMountPart[i])){
                        saveUnmountPoint(mMountPart[i]);
                }else{
                        deleteDeviceNode(mMountPart[i]);
                }
                mMountPart[i] = NULL;
    		}
    	}
    }
    /*
     * Finally, unmount the actual block device from the staging dir
     */
      ALOGD("Volume::unmountVol 3333333");
    if (doUnmount(getMountpoint(), force)) {
        ALOGE("Failed to unmount %s (%s)", SEC_STGDIR, strerror(errno));
        goto fail_recreate_bindmount;
    }

    ALOGD("%s unmounted sucessfully", getMountpoint());

    /* If this is an encrypted volume, and we've been asked to undo
     * the crypto mapping, then revert the dm-crypt mapping, and revert
     * the device info to the original values.
     */
    if (revert && isDecrypted()) {
#ifdef VOLD_SUPPORT_CRYPTFS
        cryptfs_revert_volume(getLabel());
#endif
        revertDeviceInfo();
        ALOGI("Encrypted volume %s reverted successfully", getMountpoint());
    }

		if(((mPartIdx == -1) &&(mMountedPartNum > 1)) && mMountpoint){
		       deleteUnMountPoint(0);
		       chmod(mMountpoint, 0x00);
       }

    setState(Volume::State_Idle);
    mCurrentlyMountedKdev = -1;
    return 0;

    /*
     * Failure handling - try to restore everything back the way it was
     */
fail_recreate_bindmount:
    if (mount(SEC_STG_SECIMGDIR, SEC_ASECDIR_EXT, "", MS_BIND, NULL)) {
        ALOGE("Failed to restore bindmount after failure! - Storage will appear offline!");
        goto out_nomedia;
    }
fail_remount_tmpfs:
    if (mount("tmpfs", SEC_STG_SECIMGDIR, "tmpfs", MS_RDONLY, "size=0,mode=0,uid=0,gid=0")) {
        ALOGE("Failed to restore tmpfs after failure! - Storage will appear offline!");
        goto out_nomedia;
    }
fail_republish:
    if (doMoveMount(SEC_STGDIR, getMountpoint(), force)) {
        ALOGE("Failed to republish mount after failure! - Storage will appear offline!");
        goto out_nomedia;
    }

    setState(Volume::State_Mounted);
    return -1;

out_nomedia:
    setState(Volume::State_NoMedia);
    return -1;
}

int Volume::shareVol(int lun) {
       int n = 0, i = 0, share = 0;
       dev_t deviceNodes[V_MAX_PARTITIONS];

       ALOGD("Volume::shareVol, lun=%d", lun);

       n = getDeviceNodes(deviceNodes, V_MAX_PARTITIONS);
       setShareFlags(1);
       for( i=0; i< n; i++ ) {
               dev_t d = deviceNodes[i];
               mSharelun[i] = lun + i;

               if ((MAJOR(d) == 0) && (MINOR(d) == 0)) {
                   // This volume does not support raw disk access
                   errno = EINVAL;
                   return 0;
               }

               int fd = -1;
               char nodepath[255];
               char umslun[255];

               memset(nodepath, 0, 255);
               memset(umslun, 0, 255);

               snprintf(nodepath,
                                sizeof(nodepath), "/dev/block/vold/%d:%d",
                                MAJOR(d), MINOR(d));

               if(mSharelun[i] == 0)
                       snprintf(umslun, sizeof(umslun), "/sys/class/android_usb/android0/f_mass_storage/lun/file");
               else
                       snprintf(umslun, sizeof(umslun), "/sys/class/android_usb/android0/f_mass_storage/lun%d/file", mSharelun[i]);

               ALOGD("shareVol: %s; umslun: %s", nodepath, umslun);

               if ((fd = open(umslun,O_WRONLY)) < 0) {
                   ALOGE("Unable to open ums lunfile (%s)", strerror(errno));
                   return 0;
               }

               if (write(fd, nodepath, strlen(nodepath)) < 0) {
                   ALOGE("Unable to write to ums lunfile (%s)", strerror(errno));
                   close(fd);
                   return 0;
               }

               close(fd);

               handleVolumeShared();

               share++;
       }

       ALOGD("shareVol: share = %d\n", share);

    return share;
}

int Volume::unshareVol() {
       int i,n;
       dev_t deviceNodes[V_MAX_PARTITIONS];
       n = getDeviceNodes(deviceNodes, V_MAX_PARTITIONS);
       ALOGD("Volume::unshareVol n=%d",n);
       setShareFlags(0);
       for( i=0; i< n; i++) {
               int fd;
               char umslun[255];

               memset(umslun, 0, 255);

               if(mSharelun[i] == 0)
                       snprintf(umslun, sizeof(umslun), "/sys/class/android_usb/android0/f_mass_storage/lun/file");
               else
                       snprintf(umslun, sizeof(umslun), "/sys/class/android_usb/android0/f_mass_storage/lun%d/file", mSharelun[i]);

               ALOGI("unshareVol: umslun '%s'", umslun);

               if ((fd = open(umslun, O_WRONLY)) < 0) {
               ALOGE("Unable to open ums lunfile (%s), (%s)", umslun, strerror(errno));
               return 0;
           }

       char ch = 0;
#if 1
               int wait_i = 0;

               while(wait_i < 30){
                       if (write(fd, &ch, 1) >= 0) {
                               break;
                       }

                       ALOGE("---wait :%d\n", wait_i);
                       wait_i++;
                       usleep(100);
               }

               if(wait_i == 30){
                       ALOGE("Unable to write to ums lunfile (%s)", strerror(errno));
                       close(fd);
                       return 0;
               }
#else
               if (write(fd, &ch, 1) < 0) {
                       ALOGE("Unable to write to ums lunfile (%s)", strerror(errno));
                       close(fd);
                       return -1;
               }
#endif

               close(fd);

               handleVolumeUnshared();
       }

    return n;
}


int Volume::initializeMbr(const char *deviceNode) {
    struct disk_info dinfo;

    memset(&dinfo, 0, sizeof(dinfo));

    if (!(dinfo.part_lst = (struct part_info *) malloc(MAX_NUM_PARTS * sizeof(struct part_info)))) {
        ALOGE("Failed to malloc prt_lst");
        return -1;
    }

    memset(dinfo.part_lst, 0, MAX_NUM_PARTS * sizeof(struct part_info));
    dinfo.device = strdup(deviceNode);
    dinfo.scheme = PART_SCHEME_MBR;
    dinfo.sect_size = 512;
    dinfo.skip_lba = 2048;
    dinfo.num_lba = 0;
    dinfo.num_parts = 1;

    struct part_info *pinfo = &dinfo.part_lst[0];

    pinfo->name = strdup("android_sdcard");
    pinfo->flags |= PART_ACTIVE_FLAG;
    pinfo->type = PC_PART_TYPE_FAT32;
    pinfo->len_kb = -1;

    int rc = apply_disk_config(&dinfo, 0);

    if (rc) {
        ALOGE("Failed to apply disk configuration (%d)", rc);
        goto out;
    }

 out:
    free(pinfo->name);
    free(dinfo.device);
    free(dinfo.part_lst);

    return rc;
}
