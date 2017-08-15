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

#ifndef tftp_rfile_h
#define tftp_rfile_h

#ifndef RENEO
#error "Invalid build env!";
#endif /* RENEO */

typedef struct rfile_t rfile_t;
struct         rfile_t
{
  unsigned char*    _start;
  unsigned char*    _end;
  unsigned long int _size;
  unsigned char*    _head;
};

extern int  rfileFeof  (rfile_t *);
extern int  rfileOpen  (rfile_t *,
                        char    *filename);
extern int  rfileRead  (rfile_t *fp,
                        char    *data_buffer,
                        int      data_buffer_size,
                        int      block_number,
                        int      convert,
                        int     *prev_block_number,
                        int     *prev_file_pos,
                        int     *temp);
extern void rfileClose (rfile_t *);
extern void rfileRewind(rfile_t *);

#endif /* !tftp_rfile_h */
