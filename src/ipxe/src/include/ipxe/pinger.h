#ifndef _IPXE_PINGER_H
#define _IPXE_PINGER_H

/** @file
 *
 * ICMP ping sender
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>
#include <ipxe/interface.h>
#include <ipxe/socket.h>

extern int create_pinger ( struct interface *job, const char *hostname,
                           unsigned long timeout, size_t len, unsigned int count,
                           const char* msgPrefix,
                           void ( * callback ) ( unsigned int sequence,
                                                 unsigned int sequenceMax,
                                                 const char * msgPrefix
                                                 )
                           );

#endif /* _IPXE_PINGER_H */
