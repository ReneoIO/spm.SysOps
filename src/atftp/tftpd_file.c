/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * tftpd_file.c
 *    state machine for file transfer on the server side
 *
 * $Id: tftpd_file.c,v 1.51 2004/02/18 02:21:47 jp Exp $
 *
 * Copyright (c) 2000 Jean-Pierre Lefebvre <helix@step.polymtl.ca>
 *                and Remi Lefebvre <remi@debian.org>
 * Copyright (c) 2016-2017 Reneo, Inc; Author: Minesh B. Amin
 *
 * atftp is free software; you can redistribute them and/or modify them
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include "tftpd.h"
#include "tftp_io.h"
#include "tftp_def.h"
#include "logger.h"
#include "options.h"
#include "tftp_rfile.h"
#ifdef HAVE_PCRE
#include "tftpd_pcre.h"
#endif
#include <sys/types.h>
#include <sys/wait.h>

#define S_BEGIN         0
#define S_SEND_REQ      1
#define S_SEND_ACK      2
#define S_SEND_OACK     3
#define S_SEND_DATA     4
#define S_WAIT_PACKET   5
#define S_REQ_RECEIVED  6
#define S_ACK_RECEIVED  7
#define S_OACK_RECEIVED 8
#define S_DATA_RECEIVED 9
#define S_ABORT         10
#define S_END           11


/* read only except for the main thread */
extern int tftpd_cancel;

#ifdef HAVE_PCRE
extern tftpd_pcre_self_t *pcre_top;
#endif

extern char* content_generator;

/*
 * Receive a file. It is implemented as a state machine using a while loop
 * and a switch statement. Function flow is as follow:
 *  - sanity check
 *  - check client's request
 *  - enter state machine
 *
 *     1) send a ACK or OACK
 *     2) wait replay
 *          - if DATA packet, read it, send an acknoledge, goto 2
 *          - if ERROR abort
 *          - if TIMEOUT goto previous state
 */
int tftpd_receive_file(struct thread_data *data)
{
     int sockfd = data->sockfd;
     struct sockaddr_storage *sa = &data->client_info->client;

     // write is disabled (!)
     tftp_send_error(sockfd, sa, EACCESS, data->data_buffer, data->data_buffer_size);
     if (data->trace)
          logger(LOG_DEBUG, "sent ERROR <code: %d, msg: %s>", EACCESS,
                 tftp_errmsg[EACCESS]);
     return ERR;
}

/*
 * Send a file. It is implemented as a state machine using a while loop
 * and a switch statement. Function flow is as follow:
 *  - sanity check
 *  - check client's request
 *  - enter state machine
 *
 *     1) send a DATA or OACK
 *     2) wait replay
 *          - if ACK, goto 3
 *          - if ERROR abort
 *          - if TIMEOUT goto previous state
 *     3) send data, goto 2
 */
int tftpd_send_file(struct thread_data *data)
{
     int state = S_BEGIN;
     int timeout_state = state;
     int result;
     int block_number = 0;
     unsigned short recv_number = 0;
     int wraparound_offset = 0;
     int last_block = -1;
     int data_size;
     struct sockaddr_storage *sa = &data->client_info->client;
     struct sockaddr_storage from;
     char addr_str[SOCKADDR_PRINT_ADDR_LEN];
     int sockfd = data->sockfd;
     struct tftphdr *tftphdr = (struct tftphdr *)data->data_buffer;
     rfile_t fp;
     char filename[MAXLEN];
     char string[MAXLEN];
     int timeout = data->timeout;
     int number_of_timeout = 0;
     int mcast_switch = data->mcast_switch_client;
     struct stat file_stat;
     int convert = 0;           /* if true, do netascii conversion */
     struct thread_data *thread = NULL; /* used when looking for a multicast
                                           thread */
     int multicast = 0;         /* set to 1 if multicast */

     struct client_info *client_info = data->client_info;
     struct client_info *client_old = NULL;
     struct tftp_opt options[OPT_NUMBER];

     int prev_block_number = 0; /* needed to support netascii convertion */
     int prev_file_pos = 0;
     int temp = 0;

     /* look for mode option */
     if (strcasecmp(data->tftp_options[OPT_MODE].value, "netascii") == 0)
     {
          convert = 1;
          logger(LOG_DEBUG, "will do netascii convertion");
     }

     /* file name verification */
     Strncpy(filename, data->tftp_options[OPT_FILENAME].value,
             MAXLEN);

     /* verify that the requested file exist */
     switch (rfileOpen(&fp, filename)) {
     case ERR : {
          tftp_send_error(sockfd, sa, EACCESS, data->data_buffer,
                          data->data_buffer_size);
          if (data->trace)
               logger(LOG_DEBUG, "sent ERROR <code: %d, msg: %s>", EACCESS,
                      tftp_errmsg[EACCESS]);
          return ERR;
     }
     default  :;
     }
     
     /* tsize option */
     if ((opt_get_tsize(data->tftp_options) > -1) && !convert)
     {
          opt_set_tsize(fp._size, data->tftp_options);
          logger(LOG_INFO, "tsize option -> %d", fp._size);
     }

     /* timeout option */
     if ((result = opt_get_timeout(data->tftp_options)) > -1)
     {
          if ((result < 1) || (result > 255))
          {
               tftp_send_error(sockfd, sa, EOPTNEG, data->data_buffer, data->data_buffer_size);
               if (data->trace)
                    logger(LOG_DEBUG, "sent ERROR <code: %d, msg: %s>", EOPTNEG,
                           tftp_errmsg[EOPTNEG]);
               rfileClose(&fp);
               return ERR;
          }
          timeout = result;
          opt_set_timeout(timeout, data->tftp_options);
          logger(LOG_INFO, "timeout option -> %d", timeout);
     }

     /* blksize options */
     if ((result = opt_get_blksize(data->tftp_options)) > -1)
     {
          if ((result < 8) || (result > 65464))
          {
               tftp_send_error(sockfd, sa, EOPTNEG, data->data_buffer, data->data_buffer_size);
               if (data->trace)
                    logger(LOG_DEBUG, "sent ERROR <code: %d, msg: %s>", EOPTNEG,
                           tftp_errmsg[EOPTNEG]);
               rfileClose(&fp);
               return ERR;
          }

          data->data_buffer_size = result + 4;
          data->data_buffer = realloc(data->data_buffer, data->data_buffer_size);

          if (data->data_buffer == NULL)
          {
               logger(LOG_ERR, "memory allocation failure");
               rfileClose(&fp);
               return ERR;
          }
          tftphdr = (struct tftphdr *)data->data_buffer;

          if (data->data_buffer == NULL)
          {
               tftp_send_error(sockfd, sa, ENOSPACE, data->data_buffer, data->data_buffer_size);
               if (data->trace)
                    logger(LOG_DEBUG, "sent ERROR <code: %d, msg: %s>", ENOSPACE,
                           tftp_errmsg[ENOSPACE]);
               rfileClose(&fp);
               return ERR;
          }
          opt_set_blksize(result, data->tftp_options);
          logger(LOG_INFO, "blksize option -> %d", result);
     }

     if(!data->tftp_options[OPT_WRAPAROUND].enabled) {
         /* Verify that the file can be sent in 2^16 block of BLKSIZE octets */
         if ((file_stat.st_size / (data->data_buffer_size - 4)) > 65535)
         {
              tftp_send_error(sockfd, sa, EUNDEF, data->data_buffer, data->data_buffer_size);
              logger(LOG_NOTICE, "Requested file to big, increase BLKSIZE");
              if (data->trace)
                   logger(LOG_DEBUG, "sent ERROR <code: %d, msg: %s>", EUNDEF,
                          tftp_errmsg[EUNDEF]);
              rfileClose(&fp);
              return ERR;
         }
     }

     /* multicast option */
     if (data->tftp_options[OPT_MULTICAST].specified &&
         data->tftp_options[OPT_MULTICAST].enabled && !convert)
     {
          /*
           * Find a server with the same options to give up the client.
           */
          logger(LOG_DEBUG, "Searching a server thread to give up this client");
          result = tftpd_list_find_multicast_server_and_add(&thread, data, data->client_info);
          if ( result > 0)
          {
               /* add this client to its list of client */
               if (result == 1)
                    logger(LOG_DEBUG, "Added client %p to thread %p", data->client_info,
                           thread);
               else
                    logger(LOG_DEBUG, "Client (%p) is already in list of thread %p",
                           data->client_info, thread);

               /* NULL our own pointer so we don't free memory */
               if (result == 1)
                    data->client_info = NULL;

               /* Look at needed information to oack that client */
               opt_set_multicast(data->tftp_options, thread->mc_addr,
                                 thread->mc_port, 0);
               logger(LOG_INFO, "multicast option -> %s,%d,%d",
                      thread->mc_addr, thread->mc_port, 0);

               /* Send an OACK to that client. There is a possible race condition
                  here where the new server thread OACK this client before us. This should
                  not be a problem: the client thread will receive a second OACK and fall
                  back to non master mode. Then the server will timeout and either resend
                  OACK or continu with the next client */
               opt_options_to_string(data->tftp_options, string, MAXLEN);
               if (data->trace)
                    logger(LOG_DEBUG, "sent OACK <%s>", string);
               tftp_send_oack(thread->sockfd, sa, data->tftp_options,
                              data->data_buffer, data->data_buffer_size);

               /* We are done */
               logger(LOG_INFO, "Client transfered to %p", thread);
               rfileClose(&fp);
               return OK;
          }
          else
          {
               struct addrinfo hints, *result;

               /* configure socket, get an IP address */
               if (tftpd_mcast_get_tid(&data->mc_addr, &data->mc_port) != OK)
               {
                    logger(LOG_ERR, "No multicast address/port available");
                    rfileClose(&fp);
                    return ERR;
               }
               logger(LOG_DEBUG, "mcast_addr: %s, mcast_port: %d",
                      data->mc_addr, data->mc_port);

               /* convert address */
               memset(&hints, 0, sizeof(hints));
               hints.ai_socktype = SOCK_DGRAM;
               hints.ai_flags = AI_NUMERICHOST;
               if (getaddrinfo(data->mc_addr, NULL, &hints, &result) ||
                   sockaddr_set_addrinfo(&data->sa_mcast, result))
               {
                    logger(LOG_ERR, "bad address %s\n",data->mc_addr);
                    rfileClose(&fp);
                    return ERR;
               }
               freeaddrinfo(result);
               sockaddr_set_port(&data->sa_mcast, data->mc_port);

               /* verify address is multicast */
               if (!sockaddr_is_multicast(&data->sa_mcast))
               {
                    logger(LOG_ERR, "bad multicast address %s\n",
                           sockaddr_print_addr(&data->sa_mcast,
                                               addr_str, sizeof(addr_str)));
                    rfileClose(&fp);
                    return ERR;
               }

               /* initialise multicast address structure */
               sockaddr_get_mreq(&data->sa_mcast, &data->mcastaddr);
               if (data->sa_mcast.ss_family == AF_INET)
                    setsockopt(data->sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
                               &data->mcast_ttl, sizeof(data->mcast_ttl));
               else
                    setsockopt(data->sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
                               &data->mcast_ttl, sizeof(data->mcast_ttl));
               
               /* set options data for OACK */
               opt_set_multicast(data->tftp_options, data->mc_addr,
                                 data->mc_port, 1);
               logger(LOG_INFO, "multicast option -> %s,%d,%d", data->mc_addr,
                      data->mc_port, 1);
            
               /* the socket must be unconnected for multicast */
               sa->ss_family = AF_UNSPEC;
               connect(sockfd, (struct sockaddr *)sa, sizeof(*sa));

               /* set multicast flag */
               multicast = 1;
               /* Now ready to receive new clients */
               tftpd_clientlist_ready(data);
          }
     }

     /* copy options to local structure, used when falling back a client to slave */
     memcpy(options, data->tftp_options, sizeof(options));
     opt_set_multicast(options, data->mc_addr, data->mc_port, 0);

     /* That's it, ready to send the file */
     while (1)
     {
          if (tftpd_cancel)
          {
               /* Send error to all client */
               logger(LOG_DEBUG, "thread cancelled");
               do
               {
                    tftpd_clientlist_done(data, client_info, NULL);
                    tftp_send_error(sockfd, &client_info->client,
                                    EUNDEF, data->data_buffer, data->data_buffer_size);
                    if (data->trace)
                    {
                         logger(LOG_DEBUG, "sent ERROR <code: %d, msg: %s> to %s", EUNDEF,
                                tftp_errmsg[EUNDEF],
                                sockaddr_print_addr(&client_info->client,
                                                    addr_str, sizeof(addr_str)));
                    }
               } while (tftpd_clientlist_next(data, &client_info) == 1);
               state = S_ABORT;
          }

          switch (state)
          {
          case S_BEGIN:
               if (opt_support_options(data->tftp_options))
                    state = S_SEND_OACK;
               else
                    state = S_SEND_DATA;
               break;
          case S_SEND_OACK:
               timeout_state = state;
               opt_options_to_string(data->tftp_options, string, MAXLEN);
               if (data->trace)
                    logger(LOG_DEBUG, "sent OACK <%s>", string);
               tftp_send_oack(sockfd, sa, data->tftp_options,
                              data->data_buffer, data->data_buffer_size);
               state = S_WAIT_PACKET;
               break;
          case S_SEND_DATA:
               timeout_state = state;

               data_size = rfileRead(&fp, tftphdr->th_data, data->data_buffer_size - 4, block_number,
                                     convert, &prev_block_number, &prev_file_pos, &temp);
               data_size += 4;  /* need to consider tftp header */

               /* record the last block number */
               if (rfileFeof(&fp))
                    last_block = block_number;

               if (multicast)
               {
                    tftp_send_data(sockfd, &data->sa_mcast,
                                   block_number + 1, data_size,
                                   data->data_buffer);
               }
               else
               {
                    tftp_send_data(sockfd, sa, block_number + 1,
                                   data_size, data->data_buffer);
               }
               if (data->trace)
                    logger(LOG_DEBUG, "sent DATA <block: %d, size %d>",
                           block_number + 1, data_size - 4);
               state = S_WAIT_PACKET;
               break;
          case S_WAIT_PACKET:
               data_size = data->data_buffer_size;
               result = tftp_get_packet(sockfd, -1, NULL, sa, &from, NULL,
                                        timeout, &data_size, data->data_buffer);
               switch (result)
               {
               case GET_TIMEOUT:
                    number_of_timeout++;
                    
                    if (number_of_timeout > NB_OF_RETRY)
                    {
                         logger(LOG_INFO, "client (%s) not responding",
                                sockaddr_print_addr(&client_info->client,
                                                    addr_str, sizeof(addr_str)));
                         state = S_END;
                    }
                    else
                    {
                         /* The client failed to ACK our last packet. Send an
                            OACK with mc=0 to it, fetch the next client in the
                            list and continu with this new one */
                         if (multicast && mcast_switch)
                         {
                              client_old = client_info;

                              tftpd_clientlist_next(data, &client_info);

                              if (client_info && (client_info != client_old))
                              {
                                   /* Send an OACK to the old client remove is
                                      master client status */
                                   opt_options_to_string(options,
                                                         string, MAXLEN);
                                   if (data->trace)
                                        logger(LOG_DEBUG, "sent OACK <%s>", string);
                                   tftp_send_oack(sockfd, sa, options,
                                                  data->data_buffer, data->data_buffer_size);

                                   /* Proceed normally with the next client,
                                      going to OACK state */
                                   logger(LOG_INFO,
                                          "Serving next client: %s:%d",
                                          sockaddr_print_addr(
                                               &client_info->client,
                                               addr_str, sizeof(addr_str)),
                                          sockaddr_get_port(
                                               &client_info->client));
                                   sa = &client_info->client;
                                   state = S_SEND_OACK;
                                   break;
                              }
                              else if (client_info == NULL)
                              {
                                   /* we got a big problem if this happend */
                                   logger(LOG_ERR,
                                          "%s: %d: abnormal condition",
                                          __FILE__, __LINE__);
                                   state = S_ABORT;
                                   break;
                              }
                         }
                         logger(LOG_WARNING, "timeout: retrying...");
                         state = timeout_state;
                    }
                    break;
               case GET_ACK:
                    /* handle case where packet come from un unexpected client */
                    if (multicast)
                    {
                         if (!sockaddr_equal(sa, &from))
                         {
                              /* We got an ACK from a client that is not the master client.
                               * If this is an ACK for the last block, mark this client as
                               * done
                               */
                              if ((last_block != -1) && (block_number > last_block))
                              {
                                   if (tftpd_clientlist_done(data, NULL, &from) == 1)
                                        logger(LOG_DEBUG, "client done <%s>",
                                               sockaddr_print_addr(
                                                    &from, addr_str,
                                                    sizeof(addr_str)));
                                   else
                                        logger(LOG_WARNING, "packet discarded <%s:%d>",
                                               sockaddr_print_addr(
                                                    &from, addr_str,
                                                    sizeof(addr_str)),
                                               sockaddr_get_port(&from));
                              }
                              else
                                   /* If not, send and OACK with mc=0 to shut it up. */
                              {
                                   opt_options_to_string(options,
                                                         string, MAXLEN);
                                   if (data->trace)
                                        logger(LOG_DEBUG, "sent OACK <%s>", string);
                                   tftp_send_oack(sockfd, &from, options,
                                                  data->data_buffer, data->data_buffer_size);
                              }
                              break;
                              
                         }
                    }
                    else
                    {
                         /* check that the packet is from the current client */
                         if (sockaddr_get_port(sa) != sockaddr_get_port(&from))
                         {
                              if (data->checkport)
                              {
                                   logger(LOG_WARNING, "packet discarded <%s:%d>",
                                          sockaddr_print_addr(&from, addr_str,
                                                              sizeof(addr_str)),
                                          sockaddr_get_port(&from));
                                   break;
                              }
                              else
                                   logger(LOG_WARNING,
                                          "source port mismatch, check bypassed");
                         }
                    }
                    /* The ACK is from the current client */
                    number_of_timeout = 0;
                    recv_number = ntohs(tftphdr->th_block);
                    if(recv_number == 0 && block_number) {
                        wraparound_offset += 0x10000;
                    }
                    block_number = wraparound_offset + recv_number;
                    if (data->trace)
                         logger(LOG_DEBUG, "received ACK <block: %d/%d>",
                                recv_number, block_number);
                    if ((last_block != -1) && (block_number > last_block))
                    {
                         state = S_END;
                         break;
                    }
                    state = S_SEND_DATA;
                    break;
               case GET_ERROR:
                    /* handle case where packet come from un unexpected client */
                    if (multicast)
                    {
                         /* if packet is not from the current master client */
                         if (!sockaddr_equal(sa, &from))
                         {
                              /* mark this client done */
                              if (tftpd_clientlist_done(data, NULL, &from) == 1)
                              {
                                   if (data->trace)
                                        logger(LOG_DEBUG, "client sent ERROR, mark as done <%s>",
                                               sockaddr_print_addr(
                                                    &from, addr_str,
                                                    sizeof(addr_str)));
                              }
                              else
                                   logger(LOG_WARNING, "packet discarded <%s>",
                                          sockaddr_print_addr(&from, addr_str,
                                                              sizeof(addr_str)));
                              /* current state is unchanged */
                              break;
                         }
                    }
                    else
                    {
                         /* check that the packet is from the current client */
                         if (sockaddr_get_port(sa) != sockaddr_get_port(&from))
                         {
                              if (data->checkport)
                              {
                                   logger(LOG_WARNING, "packet discarded <%s>",
                                          sockaddr_print_addr(&from, addr_str,
                                                              sizeof(addr_str)));
                                   break;
                              }
                              else
                                   logger(LOG_WARNING,
                                          "source port mismatch, check bypassed");
                         }
                    }
                    /* Got an ERROR from the current master client */
                    Strncpy(string, tftphdr->th_msg,
                            (((data_size - 4) > MAXLEN) ? MAXLEN :
                             (data_size - 4)));
                    if (data->trace)
                         logger(LOG_DEBUG, "received ERROR <code: %d, msg: %s>",
                                ntohs(tftphdr->th_code), string);
                    if (multicast)
                    {
                         logger(LOG_DEBUG, "Marking client as done");
                         state = S_END;
                    }
                    else
                         state = S_ABORT;
                    break;
               case GET_DISCARD:
                    /* FIXME: should we increment number_of_timeout */
                    logger(LOG_WARNING, "packet discarded <%s>",
                           sockaddr_print_addr(&from, addr_str,
                                               sizeof(addr_str)));
                    break;
               case ERR:
                    logger(LOG_ERR, "%s: %d: recvfrom: %s",
                           __FILE__, __LINE__, strerror(errno));
                    state = S_ABORT;
                    break;
               default:
                    logger(LOG_ERR, "%s: %d: abnormal return value %d",
                           __FILE__, __LINE__, result);
               }
               break;
          case S_END:
               if (multicast)
               {
                    logger(LOG_DEBUG, "End of multicast transfer");
                    /* mark the current client done */
                    tftpd_clientlist_done(data, client_info, NULL);
                    /* Look if there is another client to serve. We lock list of
                       client to make sure no other thread try to add clients in
                       our back */
                    if (tftpd_clientlist_next(data, &client_info) == 1)
                    {
                         logger(LOG_INFO,
                                "Serving next client: %s:%d",
                                sockaddr_print_addr(&client_info->client,
                                                    addr_str, sizeof(addr_str)),
                                sockaddr_get_port(&client_info->client));
                         /* client is a new client structure */
                         sa =  &client_info->client;
                         /* nedd to send an oack to that client */
                         state = S_SEND_OACK;                
                         rfileRewind(&fp);
                    }
                    else
                    {
                         logger(LOG_INFO, "No more client, end of tranfers");
                         rfileClose(&fp);
                         return OK;
                    }
               }
               else
               {
                    logger(LOG_DEBUG, "End of transfer");
                    rfileClose(&fp);
                    return OK;
               }
               break;
          case S_ABORT:
               logger(LOG_DEBUG, "Aborting transfer");
               rfileClose(&fp);
               return ERR;
          default:
               rfileClose(&fp);
               logger(LOG_ERR, "%s: %d: abnormal condition",
                      __FILE__, __LINE__);
               return ERR;
          }
     }
}
