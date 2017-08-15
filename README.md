# spm.SysOps
Open source component of SysOps stack based on both patents granted and pending;
the stack provides a single platform from which to automatically perform the
following operations across racks of heterogeneous servers:
  * wipe drives
  * probe hardware resources
  * update BIOS, IPMI and RAID firmware
  * install OS
	
The open source component consists of:
  * atftpd (tftp server)
	  - serves "lpxelinux.0" from builtin embedded file system
	  - ignores requests for all other files
  * syslinux / lpxelinux.0
	  - all requests are made using http protocol
	  - asks for default config file using url ".../-@-/pxeReneo.cfg/<MAC address>"
