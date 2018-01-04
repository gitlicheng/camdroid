#!/bin/bash

#***************************************************************#
#                      vendorsetup.sh                           #
#            Initialization by richard liu (liubaihao@sina.com) #   
#                     March 26  2015                            #
# function:   camdroid && linux  && uboot  compile              # 
#***************************************************************#

function set_environment()
{
	LICHEE_CHIP="sun8iw8p1"
	export CAMLINUX_BUILD_TOP=`pwd`
	export DEVICE_DIR=$CAMLINUX_BUILD_TOP/device/softwinner
	export TARGET_OUT=$CAMLINUX_BUILD_TOP/output
	export OUT_DIR=$CAMLINUX_BUILD_TOP/out
	export LICHEE_DIR=$CAMLINUX_BUILD_TOP/../lichee
	export LICHEE_OUT_DIR=$LICHEE_DIR/out/sun8iw8p1/linux
	export BR_ROOTFS_DIR=$TARGET_OUT/target
	export LICHEE_BR_DIR=${LICHEE_DIR}/buildroot
	export LICHEE_KERN_DIR=${LICHEE_DIR}/linux-3.4
	export LINUXOUT_MODULE_DIR=$LICHEE_KERN_DIR/output/lib/modules/*/*
	export LICHEE_KERN_OUTDIR=$LICHEE_KERN_DIR/output
	export LICHEE_TOOLS_DIR=${LICHEE_DIR}/tools
	export LICHEE_UBOOT_DIR=${LICHEE_DIR}/brandy/u-boot-2011.09
}

function cdevice()
{	
	cd $DEVICE
}

function ccamdroid()
{	
	cd $CAMLINUX_BUILD_TOP
}

function cout()
{
	cd $OUT	
}

function clinux()
{
	cd $LICHEE_KERN_DIR	
}

function mk_error()
{
    echo -e "\033[47;31mERROR: $*\033[0m"
}

function mk_warn()
{
    echo -e "\033[47;34mWARN: $*\033[0m"
}

function mk_info()
{
    echo -e "\033[47;30mINFO: $*\033[0m"
}

function prepare_toolchain()
{
    echo "prepare toolchain ..."
    
    tooldir=${LICHEE_BR_OUT}/external-toolchain
    if [ ! -d ${tooldir} ] ; then
        mkbr
    fi

    if ! echo $PATH | grep -q "${tooldir}" ; then
        export PATH=${tooldir}/bin:$PATH
    fi
}

# return true if used default config
function check_br_defconf()
{
	local defconf
    local platformdef=$tdevice
	echo " platformdef ==>>"${platformdef}
	if [ ! -n $tdevice ]; then
		echo "Please lunch device"
		return 1
	fi
    
    defconf="${platformdef}_defconfig"
    if [ ! -f ${LICHEE_BR_DIR}/configs/${defconf} ] ; then
        return 1
    fi
    export LICHEE_BR_DEFCONF=${defconf}

    return 0
}

# return true if used default config
function check_kern_defconf()
{
    local defconf
    local platformdef=$tdevice
    
    defconf="${platformdef}_defconfig"
    if [ ! -f ${LICHEE_KERN_DIR}/arch/arm/configs/${defconf} ] ; then
        return 1
    fi
    
    export LICHEE_KERN_DEFCONF=${defconf}

    return 0
}

# return true if used default config
function check_uboot_defconf()
{
    local defconf
    local platformdef=$tdevice

    defconf="${platformdef}_defconfig"
    if [ ! -f ${LICHEE_UBOOT_DIR}/include/configs/${defconf} ] ; then
        return 1
    fi
    export LICHEE_UBOOT_DEFCONF=${defconf}

    return 0
}

# output to out/<chip>/<platform>/common directory only when
# both buildroot and kernel use default config file.
function init_outdir()
{
    if check_br_defconf && check_kern_defconf ; then
        export LICHEE_PLAT_OUT="${LICHEE_OUT_DIR}/common"
        export LICHEE_BR_OUT="${LICHEE_PLAT_OUT}/buildroot"
    else
        export LICHEE_PLAT_OUT="${LICHEE_OUT_DIR}/common"
        export LICHEE_BR_OUT="${LICHEE_PLAT_OUT}/buildroot"
    fi

    mkdir -p ${LICHEE_BR_OUT}
}

function mksetting()
{
    printf "\n"
    printf "mkscript current setting:\n"
    printf "        Chip: ${LICHEE_CHIP}\n"
    printf "    Platform: ${LICHEE_PLATFORM}\n"
    printf "       Board: ${LICHEE_BOARD}\n"
    printf "  Output Dir: ${LICHEE_PLAT_OUT}\n"
    printf "\n"
}

function mkkernel()
{
	local platformdef=$tdevice
	
	if [ ! -n $tdevice ]; then
		echo "Please lunch device"
		return 1
	fi

	echo "Make the kernel"  
	echo "platformdef="${platformdef}
	(cd ${LICHEE_KERN_DIR}/; ./build.sh -p ${platformdef})
	[ $? -ne 0 ] && mk_error "build mkkernel fail" && return 1
    echo "Make the kernel finish"
	
	return 0
}

function mkbr()
{
    mk_info "build buildroot ..."
    local build_script
    build_script="scripts/build.sh"
	LICHEE_PLATFORM="linux"
	
    (cd ${LICHEE_BR_DIR} && [ -x ${build_script} ] && ./${build_script} "buildroot" ${LICHEE_PLATFORM} ${LICHEE_CHIP})
    [ $? -ne 0 ] && mk_error "build buildroot Failed" && return 1
    mk_info "build buildroot OK."
}

function mkuboot()
{
	(cd ${LICHEE_UBOOT_DIR}; ./build.sh -p sun8iw8p1_nor)
	[ $? -ne 0 ] && echo "build u-boot Failed" && return 1
	(cd ${LICHEE_UBOOT_DIR}; ./build.sh -p sun8iw8p1)
	[ $? -ne 0 ] && echo "build u-boot Failed" && return 1
    return 0
}

function mklichee()
{
    mksetting
    mk_info "build lichee ..."
	mkbr && mkkernel
#    mkbr && mkkernel && mkuboot
    [ $? -ne 0 ] && return 1
	return 0
}

function mkall()
{
	mklichee
	make installclean
	extract-bsp
	make -j24
	pack
}

function clbr()
{
	(cd ${LICHEE_BR_DIR}/; rm ${LICHEE_BR_DIR}/output -rf;make clean)
	echo "Clean the root"
	return 0
}

function clkernel()
{
	(cd ${LICHEE_KERN_DIR}/;make clean)
	echo "Clean the kernel"
	return 0
}

function cluboot()
{
	local build_script
	local platformdef=$tdevice
	
	if check_uboot_defconf ; then
        build_script="build.sh"
    else
        build_script="build_${platformdef}.sh"
    fi

    prepare_toolchain

    (cd ${LICHEE_UBOOT_DIR} && [ -x ${build_script} ] && ./${build_script} "clean")

    mk_info "clean u-boot OK."
}

function camdroidclean
{
	rm -rf ${CAMLINUX_BUILD_TOP}/out
}

function licheeclean()
{
    clkernel
    clbr
    mk_info "clean product output dir ..."
    rm -rf ${LICHEE_PLAT_OUT}
	rm -rf $LICHEE_DIR/out
	rm -rf /$LICHEE_DIR/linux-3.4/.config
}

function extract-bsp()
{
	CURDIR=$PWD
	
	cd $DEVICE
	
#	echo "CURDIR=" ${CURDIR}
#	echo "DEVICE=" ${DEVICE}
#	echo "LICHEE_KERN_OUTDIR=" ${LICHEE_KERN_OUTDIR}

	#extract kernel
	if [ -f kernel ]; then
		rm kernel
	fi
	cp  -rf $LICHEE_KERN_OUTDIR/zImage kernel
	echo "$DEVICE/zImage copied!"

	#extract linux modules
	if [ -d modules ]; then
		rm -rf modules
	fi
	mkdir -p modules/modules
	cp -rf $LINUXOUT_MODULE_DIR modules/modules
	echo "$DEVICE/modules copied!"
	chmod 0755 modules/modules/*
#	rm modules/modules/Module.symvers

# create modules.mk
#(cat << EOF) > ./modules/modules.mk 
# modules.mk generate by extract-files.sh , do not edit it !!!!
#PRODUCT_COPY_FILES += \\
#	\$(call find-copy-subdir-files,*,\$(LOCAL_PATH)/modules,system/vendor/modules)

#EOF
#
	cd $CURDIR
}

function make-all()
{
	# check lichee dir
	LICHEE_DIR=$ANDROID_BUILD_TOP/../lichee
	if [ ! -d $LICHEE_DIR ] ; then
		echo "$LICHEE_DIR not exists!"
		return
	fi

	extract-bsp
	m -j8
} 

function pack_normal()
{
	local platformdef=$tdevice
    echo "Pack to image........." ${platformdef}
    
    export CAMLINUX_IMAGE_OUT="$CAMLINUX_BUILD_TOP/out/target/product/${platformdef}"
	if [ "tiger-ipc" == ${platformdef} ]; then
        echo "copy tiger-ipc uboot bin files"
		cp -rf   ${LICHEE_TOOLS_DIR}/pack/chips/sun8iw8p1/configs/tiger-ipc/bin    ${LICHEE_TOOLS_DIR}/pack/chips/sun8iw8p1/
	fi		
	(cd ${LICHEE_TOOLS_DIR}/pack; ./pack -c sun8iw8p1 -p camdroid -b ${platformdef} )
	[ $? -ne 0 ] && echo "pack Failed" && return 0 
    return 0
}

function pack_card()
{
	local platformdef=$tdevice
    echo "Pack to image........." ${platformdef}
	
	export CAMLINUX_IMAGE_OUT="$CAMLINUX_BUILD_TOP/out/target/product/${platformdef}"
	if [ "tiger-ipc" == ${platformdef} ]; then
        echo "copy tiger-ipc uboot bin files"
		cp -rf   ${LICHEE_TOOLS_DIR}/pack/chips/sun8iw8p1/configs/tiger-ipc/bin    ${LICHEE_TOOLS_DIR}/pack/chips/sun8iw8p1/
	fi		
	(cd ${LICHEE_TOOLS_DIR}/pack; ./pack -c sun8iw8p1 -p camdroid -b ${platformdef} -d card0 )
	[ $? -ne 0 ] && echo "pack Failed" && return 0 
	
	return 0
}

function pack()
{
	if [ "-d" == $1 ]; then
		echo "pack card"
		pack_card
	else
		echo "pack_normal"
		pack_normal
	fi
	
	return 0
}

set_environment
init_outdir
