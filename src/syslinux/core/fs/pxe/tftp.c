/* -----------------------------------------------------------------------
 *
 *   Copyright 1999-2008 H. Peter Anvin - All Rights Reserved
 *   Copyright 2009-2011 Intel Corporation; author: H. Peter Anvin
 *   Copyright 2016-2017 Reneo, Inc; Author: Minesh B. Amin
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 53 Temple Place Ste 330,
 *   Boston MA 02111-1307, USA; either version 2 of the License, or
 *   (at your option) any later version; incorporated herein by reference.
 *
 * ----------------------------------------------------------------------- */

#include <minmax.h>
#include <net.h>
#include "pxe.h"
#include "url.h"
#include "tftp.h"

/**
 * Reneo: Refine tftp API in terms of http API
 */

const struct pxe_conn_ops tftp_conn_ops = {
  .fill_buffer        = core_tcp_fill_buffer,
  .close              = core_tcp_close_file,
};

/**
 * Open a TFTP connection to the server
 *
 * @param:inode, the inode to store our state in
 * @param:ip, the ip to contact to get the file
 * @param:filename, the file we wanna open
 *
 * @out: open_file_t structure, stores in file->open_file
 * @out: the lenght of this file, stores in file->file_len
 *
 */
void tftp_open(struct url_info *url, int flags, struct inode *inode,
	       const char **redir)
{
  // Reneo: Redirect request to http://xxx:PORT_PXE_HTTP

  extern void http_open(struct url_info* url,
                        int              flags,
                        struct inode*    inode,
                        const char**     redir);

  url->port = PORT_PXE_HTTP;
  
  http_open(url, flags, inode, redir);
  return;
}
