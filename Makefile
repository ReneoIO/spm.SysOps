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
# --------------------------------------------------------------------------------------
# Author: Minesh B. Amin
#

# --------------------------------------------------------------------------------------
# Variables:

SHELL         := /bin/bash
ROOT          := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
PLATFORM      := platform/linux/x86/64bit
PATH_BIN      := ${ROOT}/${PLATFORM}/bin
PATH_LIB      := ${ROOT}/${PLATFORM}/lib
PORT_PXE_HTTP := 69
PORT_PXE_TFTP := 69

# --------------------------------------------------------------------------------------
# Public rules:

default:
	@echo " # [I] Enter:";
	@echo " # [I]     make all; # Build all binaries + libraries";

all    : _exeIPXE _exeSyslinux _exeATFTP _exeDMIDump _exeMemTest86

# --------------------------------------------------------------------------------------
# Private rules:

_exeMemTest86:
	@[ -f ${PATH_BIN}/memtest.bin ]              || \
	(                                               \
      . ${ROOT}/__/scripts/make.sh              && \
      downloadMemTest86 ${ROOT}/src             && \
      compileMemTest86  ${PATH_BIN}                \
                        ${ROOT}/src/memtest86      \
                        ${PATH_BIN}/memtest.bin    \
      ;                                            \
	);

_exeATFTP: _exeSyslinux
	@[ -f ${PATH_BIN}/atftpd ]              || \
	(                                          \
      . ${ROOT}/__/scripts/make.sh         && \
      downloadATFTP ${ROOT}/src            && \
      compileATFTP  ${PORT_PXE_HTTP}          \
                    ${PORT_PXE_TFTP}          \
                    ${PATH_BIN}               \
                    ${PATH_LIB}/__            \
                    ${ROOT}/src/atftp         \
                    ${PATH_BIN}/atftpd        \
      ;                                       \
	);

_exeSyslinux: _exe3rdPartyPackages _exeIPXE
	@[ -f ${PATH_LIB}/__/libSyslinuxATFTP.a ]             || \
	(                                                        \
      . ${ROOT}/__/scripts/make.sh                       && \
      downloadSyslinux ${ROOT}/src                       && \
      compileSyslinux  ${PORT_PXE_HTTP}                     \
                       ${PORT_PXE_TFTP}                     \
                       ${PATH_LIB}/__/libSyslinuxATFTP.a    \
                       ${PATH_BIN}                          \
                       ${ROOT}/src/syslinux                 \
      ;                                                     \
	);


_exeDMIDump: _exe3rdPartyPackages
	@[ -f ${PATH_BIN}/dmidump ]            || \
	(                                                        \
      . ${ROOT}/__/scripts/make.sh             && \
      compileDMIDump ${PATH_BIN}                  \
                     ${ROOT}/src/dmidump          \
                     ${PATH_BIN}/dmidump          \
      ;                                           \
	);

_exeIPXE: _exe3rdPartyPackages
	@(                                        \
      . ${ROOT}/__/scripts/make.sh             && \
      downloadIPXE ${ROOT}/src                 && \
      compileIPXE  ${PATH_BIN}                    \
                   ${ROOT}/src/ipxe               \
      ;                                           \
	);

_exe3rdPartyPackages: _dirBIN
	@(                                       \
      . ${ROOT}/__/scripts/make.sh &&       \
      download3rdPartyPackages ${PLATFORM}  \
                               ${PATH_BIN}  \
      ;                                     \
	);

_dirBIN:
	@[ -d ${PATH_BIN} ] || mkdir -p ${PATH_BIN};

# --------------------------------------------------------------------------------------
