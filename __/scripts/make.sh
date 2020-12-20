#
# Copyright 2016-present Reneo, Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# -------------------------------------------------------------------------------
# Author: Minesh B. Amin
#

set -u;

# -------------------------------------------------------------------------------

function download3rdPartyPackages()
{
    local PLATFORM=$1;
    local PATH_BIN=$2;

    if [ -f "${PATH_BIN}/upx" ]; then
        return 0;
    fi;

    local tdir=$(${_MKTEMP} -d --tmpdir="/dev/shm/");
    local tlck=${tdir}.lck;
    local tlog=${tdir}.log;

    (
        # Download git repo ...

        local url="https://github.com/ReneoIO/3rdParty.Packages.git";

        echo "  # [I] 3rdParty.Packages: cloning git repo ..." && \
        (
            (
                (
                    cd ${tdir}                                 && \
                    ${_GIT} clone --progress --depth 1 ${url}     \
                    ;
                ) 2>${tlog}.err 1>${tlog}.out;

                echo ${?} > ${tlck};
            ) &
            displayStatus ${tlog} ${tlck};
        );
    ) && \
    (
        # Extract upx ...

        local pkg="upx";
        local dotbz2="${tdir}/3rdParty.Packages/${PLATFORM}/${pkg}-3.91-amd64_linux.tar.bz2";

        echo "  # [I] 3rdParty.Packages: extracting upx ..." && \
        (
            (
                (
                    ${_MKDIR} -p   ${PATH_BIN}            && \
                    ${_RM}    -f   ${PATH_BIN}/upx        && \
                    cd             ${PATH_BIN}            && \
                    echo "upx" >   ${tdir}/.bz2           && \
                    ${_TAR}   -T   ${tdir}/.bz2 --strip 1    \
                              -xf  ${dotbz2}                 \
                    ;
                ) 2>${tlog}.err 1>${tlog}.out;

                echo ${?} > ${tlck};
            ) &
            displayStatus ${tlog} ${tlck};
        );
    )                      && \
    ${_RM} -rf ${tdir}        \
               ${tlog}.err    \
               ${tlog}.out && \
    return 0;

    # Leave ${tlog}.* alone (!)
    (
        ${_RM} -rf ${tdir};
    ) 2>/dev/null 1>/dev/null;

    return 1;
}

# -------------------------------------------------------------------------------

function downloadSyslinux()
{
    local root=$1;
    local pkg="syslinux";
    local url="https://github.com/ReneoIO/3rdParty.Syslinux/archive/master.zip";
    local pkgzip="${pkg}.zip";

    ${_MKDIR} -p ${root};
    cd           ${root};

    if [ -f ${pkgzip} ]; then
        return 0;
    fi;

    local tdir=$(${_MKTEMP} -d --tmpdir="/dev/shm/");
    local tlck=${tdir}.lck;
    local tlog=${tdir}.log;

    echo "  # [I] 3rdParty.Syslinux: downloading ..." && \
    (
        (
            ${_WGET} ${url} -O ${pkgzip} 2>${tlog}.err 1>${tlog}.out;

            echo ${?} > ${tlck};
        ) &
        displayStatus ${tlog} ${tlck};
    )                      && \
    ${_RM} -rf ${tdir}        \
               ${tlog}.err    \
               ${tlog}.out && \
    return 0;

    # Leave ${tlog}.* alone (!)
    (
        ${_RM} -rf ${tdir};
        cd         ${root};
        ${_UNLINK} ${pkgzip};
    ) 2>/dev/null 1>/dev/null;

    return 1;
}

function compileSyslinux()
{
    local PORT_PXE_HTTP=$1;
    local PORT_PXE_TFTP=$2;
    local libSyslinuxATFTP=$3;
    local PATH_BIN=$4;
    local PATH_LIB=$(cd ${PATH_BIN} && ${_MKDIR} -p ../lib && cd ../lib && pwd);
    local rootSrc=$5;
    local pkg="syslinux";
    local pkgzip=$(cd `${_DIRNAME} ${rootSrc}` && pwd)/${pkg}.zip;
                
    ${_MKDIR} -p ${rootSrc};
    cd           ${rootSrc};

    if ! [ -f ${pkgzip} ]; then
        return 1;
    fi;

    local prefix="3rdParty.Syslinux-master";
    local tdir=$(${_MKTEMP} -d --tmpdir="/dev/shm/");
    local tlck=${tdir}.lck;
    local tlog=${tdir}.log;

    echo "  # [I] 3rdParty.Syslinux: extracting ..."            && \
    (
        (
            (
                ${_UNZIP} -d ${tdir} ${pkgzip}                  && \
                (
                    cd ${tdir} && ${_TAR} -cf ${tdir}.tar .;
                )                                               && \
                ${_TAR} --exclude=${prefix}/Makefile               \
                        --exclude=${prefix}/core/Makefile          \
                        --exclude=${prefix}/core/fs/pxe/pxe.c      \
                        --exclude=${prefix}/core/fs/pxe/tftp.c     \
                        --exclude=${prefix}/core/fs/pxe/tftp.h     \
                        --exclude=${prefix}/com32/lib/sys/module/elf_module.c \
                        --exclude=${prefix}/com32/menu/vesamenu.c  \
                        --exclude=${prefix}/com32/menu/menumain.c  \
                        --exclude=${prefix}/efi/tcp.c              \
                        --exclude=${prefix}/diag/geodsp/Makefile   \
                        --exclude=${prefix}/extlinux/Makefile      \
                        --exclude=${prefix}/linux/Makefile         \
                        --exclude=${prefix}/memdisk/Makefile       \
                        --exclude=${prefix}/mk/build.mk            \
                        --exclude=${prefix}/mk/com32.mk            \
                        --exclude=${prefix}/mk/elf.mk              \
                        --exclude=${prefix}/mk/embedded.mk         \
                        --exclude=${prefix}/mk/lib.mk              \
                        --exclude=${prefix}/mtools/Makefile        \
                        --strip 2                                  \
                        -xf                 ${tdir}.tar         && \
                ${_RM}  -rf                 ${tdir}.tar            \
                                            ${tdir}                \
                ;
            ) 2>${tlog}.err 1>${tlog}.out;

            echo ${?} > ${tlck};
        ) &
        displayStatus ${tlog} ${tlck};
    )                                               && \
    echo "  # [I] ${pkg}: compiling ..."            && \
    (
        (
            (
                (
                    export PATH=${PATH_BIN}:${PATH}; # Use our 'upx' (!)

                    ${_MAKE}  -j $(degreeOfConcurrency) clean         && \
                    ${_MAKE}  -j $(degreeOfConcurrency)                  \
                        PORT_PXE_TFTP=${PORT_PXE_TFTP}                   \
                        PORT_PXE_HTTP=${PORT_PXE_HTTP}                && \
                    ${_MKDIR} -p $(${_DIRNAME} ${libSyslinuxATFTP})   && \
                    ${_RM}    -f               ${libSyslinuxATFTP}    && \
                    for exe in `${_FIND} -type f -name "lpxelinux.0"`    \
                        ; do
                        local vroot=$(${_DIRNAME} ${exe});
                        local exe_=$(${_BASENAME} ${exe});

                        cd ${vroot}                     && \
                        ${_OBJCOPY} -I binary              \
                                    -O elf64-x86-64        \
                                    -B i386:x86-64         \
                                    ${exe_}                \
                                    ${exe_}.o           && \
                        ${_AR} rvs  ${libSyslinuxATFTP}    \
                                    ${exe_}.o           && \
                        ${_UNLINK}  ${exe_}.o              \
                        ;
                    done;
                ) && \
                (
                    ${_MKDIR} -p ${PATH_BIN}                          && \
                    for exe in bios/memdisk/memdisk                      \
                               bios/com32/chain/chain.c32                \
                               bios/com32/elflink/ldlinux/ldlinux.c32    \
                               bios/com32/menu/vesamenu.c32              \
                               bios/com32/modules/pxechn.c32             \
                               bios/core/lpxelinux.0                     \
                        ; do
                        ${_CP} -p ${exe} ${PATH_BIN} || exit -1;
                    done                                              && \
                                                                         \
                    ${_MKDIR} -p ${PATH_LIB}                          && \
                    for lib in bios/com32/libutil/libutil.c32            \
                               bios/com32/lib/libcom32.c32               \
                        ; do
                        ${_CP} -p ${lib} ${PATH_LIB} || exit -1;
                    done;
                );
            ) 2>${tlog}.err 1>${tlog}.out;

            echo ${?} > ${tlck};
        ) &
        displayStatus ${tlog} ${tlck};
    )                      && \
    ${_RM} -rf ${tdir}        \
               ${tdir}.tar    \
               ${tlog}.err    \
               ${tlog}.out && \
    return 0;

    # Leave ${tlog}.* alone (!)
    (
        ${_RM} -rf ${tdir};
        ${_RM} -rf ${tdir}.tar ;
    ) 2>/dev/null 1>/dev/null;

    return 1;
}

# -------------------------------------------------------------------------------

function downloadATFTP()
{
    local root=$1;
    local pkg="atftp";
    local url="https://github.com/ReneoIO/3rdParty.atftp/archive/master.zip";
    local pkgzip="${pkg}.zip";

    ${_MKDIR} -p ${root};
    cd           ${root};

    if [ -f ${pkgzip} ]; then
        return 0;
    fi;

    local tdir=$(${_MKTEMP} -d --tmpdir="/dev/shm/");
    local tlck=${tdir}.lck;
    local tlog=${tdir}.log;

    echo "  # [I] 3rdParty.atftp: downloading ..." && \
    (
        (
            ${_WGET} ${url} -O ${pkgzip} 2>${tlog}.err 1>${tlog}.out;

            echo ${?} > ${tlck};
        ) &
        displayStatus ${tlog} ${tlck};
    )                      && \
    ${_RM} -rf ${tdir}        \
               ${tlog}.err    \
               ${tlog}.out && \
    return 0;

    # Leave ${tlog}.* alone (!)
    (
        ${_RM} -rf ${tdir};
        cd         ${root};
        ${_UNLINK} ${pkgzip};
    ) 2>/dev/null 1>/dev/null;

    return 1;
}

function compileATFTP()
{
    local PORT_PXE_HTTP=$1;
    local PORT_PXE_TFTP=$2;
    local PATH_BIN=$3;
    local rootLib=$4;
    local rootSrc=$5;
    local binATFTPD=$6;
    local pkg="atftp";
    local pkgzip=$(cd `${_DIRNAME} ${rootSrc}` && pwd)/${pkg}.zip;

    if [ -f "${binATFTPD}" ]; then
        return 0;
    fi;

    ${_MKDIR} -p ${rootSrc};
    cd           ${rootSrc};

    local tdir=$(${_MKTEMP} -d --tmpdir="/dev/shm/");
    local tlck=${tdir}.lck;
    local tlog=${tdir}.log;
    local prefix="3rdParty.atftp-master";

    echo "  # [I] 3rdParty.atftp: extracting ..." && \
    (
        (
            (
                ${_UNZIP} -d ${tdir} ${pkgzip}               && \
                (
                    cd ${tdir} && ${_TAR} -cf ${tdir}.tar .;
                )                                            && \
                ${_TAR} --exclude=${prefix}/configure.ac        \
                        --exclude=${prefix}/Makefile.am         \
                        --exclude=${prefix}/tftpd.c             \
                        --exclude=${prefix}/tftpd_file.c        \
                        --strip 2                               \
                        -xf                 ${tdir}.tar      && \
                ${_RM}  -rf                 ${tdir}.tar         \
                                            ${tdir}             \
                ;
            ) 2>${tlog}.err 1>${tlog}.out;

            echo ${?} > ${tlck};
        ) &
        displayStatus ${tlog} ${tlck};
    )                                    && \
    echo "  # [I] ${pkg}: compiling ..." && \
    (
        (
            (
                export LIBRARY_PATH=${rootLib};
                export PORT_PXE_HTTP=${PORT_PXE_HTTP};
                export PORT_PXE_TFTP=${PORT_PXE_TFTP};
                export PATH=${PATH_BIN}:${PATH}; # Use our 'upx' (!)

                rm -f ./atftpd                      && \
                ./autogen.sh                        && \
                ./configure --disable-libreadline      \
                            --disable-libwrap          \
                            --disable-libpcre          \
                            --disable-debug            \
                            --disable-mtftp            \
                            --enable-SyslinuxATFTP  && \
                ${_MAKE} -j $(degreeOfConcurrency)     \
                    PORT_PXE_TFTP=${PORT_PXE_TFTP}     \
                    PORT_PXE_HTTP=${PORT_PXE_HTTP}  && \
                ${_STRIP} ./atftpd                  && \
                ${_WHICH} upx                       && \
                upx       ./atftpd                  && \
                ${_CP} -p ./atftpd ${binATFTPD}        \
                ;
            ) 2>${tlog}.err 1>${tlog}.out;

            echo ${?} > ${tlck};
        ) &
        displayStatus ${tlog} ${tlck};
    )                      && \
    ${_RM} -rf ${tdir}        \
               ${tdir}.tar    \
               ${tlog}.err    \
               ${tlog}.out && \
    return 0;

    # Leave ${tlog}.* alone (!)
    (
        ${_RM} -rf ${tdir}     \
                   ${tdir}.tar ;
        cd         ${root};
        ${_UNLINK} ${pkgzip};
    ) 2>/dev/null 1>/dev/null;
    return 1;
}

# -------------------------------------------------------------------------------

function downloadMemTest86()
{
    local root=$1;
    local pkg="memtest86";
    local url="https://github.com/ReneoIO/3rdParty.memtest86.v4/archive/master.zip";
    local pkgzip="${pkg}.zip";

    ${_MKDIR} -p ${root};
    cd           ${root};

    if [ -f ${pkgzip} ]; then
        return 0;
    fi;

    local tdir=$(${_MKTEMP} -d --tmpdir="/dev/shm/");
    local tlck=${tdir}.lck;
    local tlog=${tdir}.log;

    echo "  # [I] 3rdParty.memtest86: downloading ..." && \
    (
        (
            ${_WGET} ${url} -O ${pkgzip} 2>${tlog}.err 1>${tlog}.out;

            echo ${?} > ${tlck};
        ) &
        displayStatus ${tlog} ${tlck};
    )                      && \
    ${_RM} -rf ${tdir}        \
               ${tlog}.err    \
               ${tlog}.out && \
    return 0;

    # Leave ${tlog}.* alone (!)
    (
        ${_RM} -rf ${tdir};
        cd         ${root};
        ${_UNLINK} ${pkgzip};
    ) 2>/dev/null 1>/dev/null;

    return 1;
}

function compileMemTest86()
{
    local PATH_BIN=$1;
    local rootSrc=$2;
    local binMemTest86=$3;
    local pkg="memtest86";
    local pkgzip=$(cd `${_DIRNAME} ${rootSrc}` && pwd)/${pkg}.zip;

    if [ -f "${binMemTest86}" ]; then
        return 0;
    fi;

    ${_MKDIR} -p ${rootSrc};
    cd           ${rootSrc};

    local tdir=$(${_MKTEMP} -d --tmpdir="/dev/shm/");
    local tlck=${tdir}.lck;
    local tlog=${tdir}.log;
    local prefix="3rdParty.memtest86.v4-master";

    echo "  # [I] 3rdParty.memtest86: extracting ..." && \
    (
        (
            (
                ${_UNZIP} -d ${tdir} ${pkgzip}               && \
                (
                    cd ${tdir} && ${_TAR} -cf ${tdir}.tar .;
                )                                            && \
                ${_TAR} --exclude=${prefix}/config.c            \
                        --exclude=${prefix}/config.h            \
                        --exclude=${prefix}/cpuid.c             \
                        --exclude=${prefix}/error.c             \
                        --exclude=${prefix}/init.c              \
                        --exclude=${prefix}/lib.c               \
                        --exclude=${prefix}/main.c              \
                        --exclude=${prefix}/Makefile            \
                        --exclude=${prefix}/memsize.c           \
                        --exclude=${prefix}/rlib.c              \
                        --exclude=${prefix}/rlib.h              \
                        --exclude=${prefix}/screen_buffer.c     \
                        --exclude=${prefix}/screen_buffer.h     \
                        --exclude=${prefix}/smp.c               \
                        --exclude=${prefix}/test.c              \
                        --exclude=${prefix}/test.h              \
                        --strip 2                               \
                        -xf                 ${tdir}.tar      && \
                ${_RM}  -rf                 ${tdir}.tar         \
                                            ${tdir}             \
                ;
            ) 2>${tlog}.err 1>${tlog}.out;

            echo ${?} > ${tlck};
        ) &
        displayStatus ${tlog} ${tlck};
    )                                    && \
    echo "  # [I] ${pkg}: compiling ..." && \
    (
        (
            (
                ${_MAKE} clean                            && \
                ${_MAKE} -j $(degreeOfConcurrency)        && \
                ${_CP}   -p ./memtest.bin ${binMemTest86}    \
                ;
            ) 2>${tlog}.err 1>${tlog}.out;

            echo ${?} > ${tlck};
        ) &
        displayStatus ${tlog} ${tlck};
    )                      && \
    ${_RM} -rf ${tdir}        \
               ${tdir}.tar    \
               ${tlog}.err    \
               ${tlog}.out && \
    return 0;

    # Leave ${tlog}.* alone (!)
    (
        ${_RM} -rf ${tdir}     \
                   ${tdir}.tar ;
        cd         ${root};
        ${_UNLINK} ${pkgzip};
    ) 2>/dev/null 1>/dev/null;
    return 1;
}

# -------------------------------------------------------------------------------

function degreeOfConcurrency()
{
    local rval=$(
                 ${_CAT} /proc/cpuinfo  | \
                 ${_GREP} processor     | \
                 ${_WC} -l              | \
                 ${_AWK} '{print 2*$1}';
                );
    echo ${rval};

    return 0;
}

function panic()
{
    local msg=$1;

    echo "  # [E] ${msg}";
    exit 1;
}

function displayStatus()
{
    local tlog=$1;
    local tlck=$2;

    while :; do
        nBytesLog=$(${_LS} -la ${tlog}.out                \
                               ${tlog}.err                \
                               2>/dev/null              | \
                    ${_AWK} '{sum += $5}END{print sum}'   );
        for i in '-' '\\' '|' '/'; do
            ${_SLEEP} 0.4;
            echo -ne "  #     ... ${i} ${nBytesLog} bytes (log)" "\r";
        done;
        if [ -e ${tlck} ]; then
            ${_SLEEP} 0.4;

            local stat=$(${_CAT} ${tlck});

            ${_UNLINK} ${tlck};

            if ! [ "${stat}" = 0 ]; then
                echo "  # [E]     encountered errors; logs saved @";
                echo "            ${tlog}.err";
                echo "            ${tlog}.out";
                
                return 1;
            else
                echo   -en "                                            ""\r";
                return 0;
            fi;
        fi;
    done;
}

# -------------------------------------------------------------------------------
# Make sure all required tools are installed.

_WHICH=$(which which)       || panic "Please install 'which'";

_AR=$(which ar)             || panic "Please install 'ar'";
_AUTOCONF=$(which autoconf) || panic "Please install 'autoconf'";
_AUTOMAKE=$(which automake) || panic "Please install 'automake'";
_AWK=$(which awk)           || panic "Please install 'awk'";
_BASENAME=$(which basename) || panic "Please install 'basename'";
_BASH=$(which bash)         || panic "Please install 'bash'";
_CAT=$(which cat)           || panic "Please install 'cat'";
_CP=$(which cp)             || panic "Please install 'cp'";
_DIRNAME=$(which dirname)   || panic "Please install 'dirname'";
_FIND=$(which find)         || panic "Please install 'find'";
_GCC=$(which gcc)           || panic "Please install 'gcc'";
_GIT=$(which git)           || panic "Please install 'git'";
_GREP=$(which grep)         || panic "Please install 'grep'";
_LS=$(which ls)             || panic "Please install 'ls'";
_MAKE=$(which make)         || panic "Please install 'make'";
_MKDIR=$(which mkdir)       || panic "Please install 'mkdir'";
_MKTEMP=$(which mktemp)     || panic "Please install 'mktemp'";
_MV=$(which mv)             || panic "Please install 'mv'";
_OBJCOPY=$(which objcopy)   || panic "Please install 'objcopy'";
_RM=$(which rm)             || panic "Please install 'rm'";
_SLEEP=$(which sleep)       || panic "Please install 'sleep'";
_STRIP=$(which strip)       || panic "Please install 'strip'";
_TAR=$(which tar)           || panic "Please install 'tar'";
_UNLINK=$(which unlink)     || panic "Please install 'unlink'";
_UNLINK=$(which unlink)     || panic "Please install 'unlink'";
_UNZIP=$(which unzip)       || panic "Please install 'unzip'";
_WC=$(which wc)             || panic "Please install 'wc'";
_WGET=$(which wget)         || panic "Please install 'wget'";

# Utilities required when compiling code ...
_NASM=$(which nasm)         || panic "Please install 'nasm'";

# -------------------------------------------------------------------------------
