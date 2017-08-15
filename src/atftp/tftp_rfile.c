/*
 * Copyright 2016-present Reneo, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * -------------------------------------------------------------------------------
 * Author: Minesh B. Amin
 *
 */

#ifndef RENEO
#error "Invalid build env!";
#endif /* RENEO */

#include <stdio.h>
#include <string.h>
#include "tftp_def.h"
#include "tftp_rfile.h"

static rfile_t _lpxelinux;

void initEFS()
{
  {
    extern unsigned char _binary_lpxelinux_0_start;
    extern unsigned char _binary_lpxelinux_0_end;
    extern unsigned char _binary_lpxelinux_0_size;
    
    _lpxelinux._start =                     &_binary_lpxelinux_0_start;
    _lpxelinux._end   =                     &_binary_lpxelinux_0_end;
    _lpxelinux._size  = (unsigned long int)(&_binary_lpxelinux_0_size);
    _lpxelinux._head  =                     &_binary_lpxelinux_0_start;
  }
}

int rfileFeof(rfile_t *fp) 
{
  return (fp->_head >= fp->_end);
}

int rfileOpen(rfile_t *fp,
              char    *filename) 
{
  char const * const lpxelinux = "lpxelinux.0";
  char const * const pxelinux  = "pxelinux.0";
  
  if ((strncmp(filename, lpxelinux, strlen(lpxelinux)+1) == 0) ||
      (strncmp(filename, pxelinux,  strlen(pxelinux )+1) == 0)) {
    return (*fp = _lpxelinux, OK);
  } else {
    return ERR;
  }
}

void rfileClose(rfile_t *fp) 
{
  memset(fp, 0x13, sizeof(rfile_t));
}

void rfileRewind(rfile_t *fp)
{
  fp->_head = fp->_start;
}

static int _Getc(rfile_t *fp) 
{
  if (fp->_head >= fp->_end) {
    return EOF;
  } else {
    unsigned char rval = *fp->_head;
    
    fp->_head += 1;
    return rval;
  }
}

static long _Tell(rfile_t *fp) 
{
  return fp->_head - fp->_start;
}

static void _Seek(rfile_t *fp, long offset) 
{
  fp->_head = fp->_start + offset;
  if (fp->_head > fp->_end) {
    fp->_head = fp->_end;
  }
}

static int _Read(rfile_t *fp, char* dest, long nBytes) 
{
  if (fp->_head == fp->_end) {
    return 0;
  } else if ((fp->_head + nBytes) > fp->_end) {
    int nBytes = fp->_end - fp->_head;
    
    memcpy(dest, fp->_head, nBytes);
    fp->_head += nBytes;
  
    return nBytes;
  } else {
    memcpy(dest, fp->_head, nBytes);
    fp->_head += nBytes;
    
    return nBytes;
  }
}

/*
 * Same as tftp_file_read except the first argument (!)
 */
int rfileRead(rfile_t *fp,
              char    *data_buffer,
              int      data_buffer_size,
              int      block_number,
              int      convert,
              int     *prev_block_number,
              int     *prev_file_pos,
              int     *temp)
{
  int i;
  int c;
  char prevchar = *temp & 0xff;
  char newline = (*temp & 0xff00) >> 8;
  
  if (!convert)
    {
      /* In this case, just read the requested data block.
         Anyway, in the multicast case it can be in random
         order. */
      _Seek(fp, block_number * data_buffer_size);
      return _Read(fp, data_buffer, data_buffer_size);
    }
  else
    {
      /* 
       * When converting data, it become impossible to seek in
       * the file based on the block number. So we must always
       * remeber the position in the file from were to read the
       * data requested by the client. Client can only request data
       * for the same block or the next, but we cannot assume this
       * block number will increase at every ACK since it can be
       * lost in transmission.
       *
       * The stategy is to remeber the file position as well as
       * the block number from the current call to this function.
       * If the client request a block number different from that
       * we return ERR.
       * 
       * If the client request many time the same block, the
       * netascii conversion is done each time. Since this is not
       * a normal condition it should not be a problem for system
       * performance.
       *
       */
      if ((block_number != *prev_block_number) && (block_number != *prev_block_number + 1))
        return ERR;
      if (block_number == *prev_block_number)
        _Seek(fp, *prev_file_pos);
      
      *prev_block_number = block_number;
      *prev_file_pos = _Tell(fp);
      
      /*
       * convert to netascii, based on netkit-tftp-0.17 routine in tftpsubs.c
       * i index output buffer
       */
      for (i = 0; i < data_buffer_size; i++)
        {
	       if (newline)
            {
              if (prevchar == '\n')
                c = '\n';       /* lf to cr,lf */
              else
                c = '\0';       /* cr to cr,nul */
              newline = 0;
            }
	       else
            {
              c = _Getc(fp);
              if (c == EOF)
                break;
              if (c == '\n' || c == '\r')
                {
                  prevchar = c;
                  c = '\r';
                  newline = 1;
                }
            }
          data_buffer[i] = c;
        }
      /* save state */
      *temp = (newline << 8) | prevchar;
      
      return i;
    }
  return ERR;
}
