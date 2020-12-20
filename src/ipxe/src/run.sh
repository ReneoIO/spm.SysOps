#!/bin/sh
#
# Copyright 2018-present Reneo, Inc. All Rights Reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#
# You can also choose to distribute this program under the terms of
# the Unmodified Binary Distribution Licence (as given in the file
# COPYING.UBDL), provided that you have satisfied its requirements.
#
# -------------------------------------------------------------------------------
# Author: Minesh B. Amin
#

bootstrap="";

while test $1 
do
    case $1 in
        -bootstrap) shift;
                    if test $1; then
                        bootstrap=$1;
                        shift;
                    fi
                    ;;
        *)         help;
                   ;;
    esac
done

if [ "$bootstrap" = "" ]; then
    echo ""
    echo " [E] ./run.sh -bootstrap <path>";
    echo ""
    exit 1;
fi;

# --------------------------------------------------------------

(
    make clean                                             && \
    cd     ./config                                        && \
    rm -f                   ./console.h                    && \
    ln -sf ./console.UEFI.h ./console.h                    && \
    cd     ../                                             && \
    make bin-x86_64-efi/ipxe.efi    EMBED=${bootstrap}     && \
    cp ./bin-x86_64-efi/ipxe.efi ../                       && \
    git checkout ./config/console.h                           \
    ;
) && \
(
    make clean                                             && \
    cd     ./config                                        && \
    rm -f                   ./console.h                    && \
    ln -sf ./console.UEFI.h ./console.h                    && \
    cd     ../                                             && \
    make bin-x86_64-efi/snponly.efi EMBED=${bootstrap}     && \
    cp ./bin-x86_64-efi/snponly.efi ../                    && \
    git checkout ./config/console.h                           \
    ;
) && \
(
    make clean                                             && \
    cd     ./config                                        && \
    rm -f                   ./console.h                    && \
    ln -sf ./console.BIOS.h ./console.h                    && \
    cd     ../                                             && \
    make bin/ipxe.pxe               EMBED=${bootstrap}     && \
    cp ./bin/ipxe.pxe ../                                  && \
    git checkout ./config/console.h                           \
    ;
);

if [ ! $? = 0 ]; then
    echo " [E] compile failed (!)";
    exit 1;
fi;
