#. ******* coding:utf-8 AUTOHEADER START v1.2 *******
#. vim: fileencoding=utf-8 syntax=Makefile sw=4 ts=4 et
#.
#. © 2007-2010 Nima Talebi <nima@autonomy.net.au>
#. © 2009-2010 David Sommerseth <davids@redhat.com>
#. © 2002-2008 Jean Delvare <khali@linux-fr.org>
#. © 2000-2002 Alan Cox <alan@redhat.com>
#. © 2016-2018 Minesh B. Amin <mamin@reneo.com>
#.
#. This file is part of python-dmidecode.
#.
#.     python-dmidecode is free software: you can redistribute it and/or modify
#.     it under the terms of the GNU General Public License as published by
#.     the Free Software Foundation, either version 2 of the License, or
#.     (at your option) any later version.
#.
#.     python-dmidecode is distributed in the hope that it will be useful,
#.     but WITHOUT ANY WARRANTY; without even the implied warranty of
#.     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#.     GNU General Public License for more details.
#.
#.     You should have received a copy of the GNU General Public License
#.     along with python-dmidecode.  If not, see <http://www.gnu.org/licenses/>.
#.
#. THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
#. WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#. MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
#. EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#. INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#. LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#. PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#. LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#. OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
#. ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#.
#. ADAPTED M. STONE & T. PARKER DISCLAIMER: THIS SOFTWARE COULD RESULT IN INJURY
#. AND/OR DEATH, AND AS SUCH, IT SHOULD NOT BE BUILT, INSTALLED OR USED BY ANYONE.
#.
#. $AutoHeaderSerial::20100225                                                 $
#. ******* AUTOHEADER END v1.2 *******

SHELL	:= /bin/bash

###############################################################################
.PHONY: dmidump

all : dmidump

dmidump : util.o efi.o dmilog.o
	$(CC) -o $@ dmidump.c $^ -O2 -Wall

clean:
	-rm -f *.so *.o core dmidump *.o
