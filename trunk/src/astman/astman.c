/*
 * libast-api - library for using Asterisk Manager API.
 * Copyright (C) 2010 Baligh GUESMI
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *   along with libast-api.  If not, see <http://www.gnu.org/licenses/>.
 */
/**
 *  @file astman.c
 *  @brief Base Functions implementation for the ASTMAN API.
 *  @author Baligh.GUESMI Emira.MHAROUECH Olivier.BENEZE
 *  @version 0.1
 *  @date 26 Avril 2010
 */

#include <sys/types.h>
#include <sys/socket.h> /* send/recv */
#include <netinet/in.h>  /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntoa function */
#include <netdb.h>  /* gethostbyname */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <stdarg.h>  /* vsnprintf */
#include <string.h>
#include <errno.h>
//#include <openssl/md5.h>

#include "astman.h"
#include "astlog.h"
/*******************************************************************************
 *  \def ASTMAN_DEFAULT_MANAGER_PORT
 *  \brief  Default port used to connect to the AMI Asterisk
 ******************************************************************************/
#define ASTMAN_DEFAULT_MANAGER_PORT 5038
/*******************************************************************************
 *  \def ASTMAN_DEFAULT_EVENT
 *  \brief  Default Event to register over
 ******************************************************************************/
#define ASTMAN_DEFAULT_EVENT "DEFAULT"
/*******************************************************************************
 *  \fn astman_add_param(char *buf, int buflen, char *header, char *value)
 *  \brief  Add a new parameter to the Command
 *  \param  buf
 *  \param  buflen
 *  \param  header
 *  \param  value
 *  \return Number of wrote characters into the buf
 ******************************************************************************/
int astman_add_param(char *buf, int buflen, char *header, const char *value) {

    if (!astman_strlen_zero(value))
        return snprintf(buf+strlen(buf), buflen-strlen(buf)-1, "%s: %s\r\n",
                        header, value);

    return 0;
}
/*******************************************************************************
 *  \fn astman_add_param(char *buf, int buflen, char *header, char *value)
 *  \brief  Add a new parameter to the Command
 *  \param  buf
 *  \param  buflen
 *  \param  header
 *  \param  value
 *  \return Number of wrote characters into the buf
 ******************************************************************************/
void astman_dump_message(struct message *m) {
    int x;
    printf("< Received:\n");
    for (x=0;x<m->hdrcount;x++) {
        printf("< %s\n", m->headers[x]);
    }
    printf("\n");
}
/*******************************************************************************
 *  \fn astman_add_param(char *buf, int buflen, char *header, char *value)
 *  \brief  Add a new parameter to the Command
 *  \param  buf
 *  \param  buflen
 *  \param  header
 *  \param  value
 *  \return Number of wrote characters into the buf
 ******************************************************************************/
static void astman_dump_out_message(char *message) {
    char *s;
    printf("> Transmitted:\n");
    //  printf("> Action: %s\n", action);
    while (*message) {
        s = message;
        while (*s && (*s != '\r')) s++;
        if (!*s)
            break;
        *s = '\0';
        printf("> %s\n", message);
        s+=2;
        message = s;
    }
    printf("\n");
}
/*******************************************************************************
 *  \fn astman_add_param(char *buf, int buflen, char *header, char *value)
 *  \brief  Add a new parameter to the Command
 *  \param  buf
 *  \param  buflen
 *  \param  header
 *  \param  value
 *  \return Number of wrote characters into the buf
 ******************************************************************************/
struct mansession *astman_open(void) {
    int so;
    static struct mansession s;

    so = socket(AF_INET, SOCK_STREAM, 0);
    if (so < 0) {
        perror("socket");
    }

    s.fd = so;
    return &s;
}
/*******************************************************************************
 *  \fn astman_add_param(char *buf, int buflen, char *header, char *value)
 *  \brief  Add a new parameter to the Command
 *  \param  buf
 *  \param  buflen
 *  \param  header
 *  \param  value
 *  \return Number of wrote characters into the buf
 ******************************************************************************/
int astman_connect(struct mansession *s, const char *hostname, const int port) {
    struct hostent *hp;
    struct sockaddr_in addr;
    addr = s->sin;

    s->fd = socket(AF_INET, SOCK_STREAM, 0);
    if ( s->fd < 0 ) {
        perror("socket");
        return -1;
    }

    hp = gethostbyname(hostname);
    if (!hp) {
        fprintf(stderr, "No such address: %s\n", hostname);
        return -1;
    }

    s->sin.sin_family = AF_INET;
    if (port > 0)
        s->sin.sin_port = htons(port);
    else
        s->sin.sin_port = htons(ASTMAN_DEFAULT_MANAGER_PORT);
    memcpy(&(s->sin.sin_addr), hp->h_addr, sizeof(s->sin.sin_addr));


    if ( connect(s->fd, (struct sockaddr *) &(s->sin), sizeof(s->sin)) < 0 ) {
        perror("connect");
        return -1;
    }
    return 0;
}
/*******************************************************************************
 *  \fn astman_add_param(char *buf, int buflen, char *header, char *value)
 *  \brief  Add a new parameter to the Command
 *  \param  buf
 *  \param  buflen
 *  \param  header
 *  \param  value
 *  \return Number of wrote characters into the buf
 ******************************************************************************/
void astman_disconnect(struct mansession *s) {
    if (s->fd) {
        close(s->fd);
        s->fd = 0;
    }
}
/*******************************************************************************
 * @fn astman_add_param(char *buf, int buflen, char *header, char *value)
 * @brief  Add a new parameter to the Command
 * @param  s:
 * @param  output
 * @return
 * @warning output must have at least sizeof(s->inbuf) space
 ******************************************************************************/
static int astman_get_input(struct mansession *s, char *output) {
    /* output must have at least sizeof(s->inbuf) space */
    int res;
    int x;
    struct timeval tv = {
        0, 0
    };
    fd_set fds;
    for (x=1;x<s->inlen;x++) {
        if ((s->inbuf[x] == '\n')) {
            /* Copy output data up to and including \r\n */
            memcpy(output, s->inbuf, x + 1);
            /* Add trailing \0 */
            output[x+1] = '\0';
            /* Move remaining data back to the front */
            memmove(s->inbuf, s->inbuf + x + 1, s->inlen - x);
            s->inlen -= (x + 1);
            return 1;
        }
    }

    if (s->inlen >= sizeof(s->inbuf) - 1) {
        fprintf(stderr, "Dumping long line with no return from %s: %s\n", inet_ntoa(s->sin.sin_addr), s->inbuf);
        s->inlen = 0;
    }
    FD_ZERO(&fds);
    FD_SET(s->fd, &fds);
    res = select(s->fd + 1, &fds, NULL, NULL, &tv);

    if (res < 0) {
        fprintf(stderr, "Select returned error: %s\n", strerror(errno));
    } else if (res > 0) {

        res = recv(s->fd, s->inbuf + s->inlen, sizeof(s->inbuf) - 1 - s->inlen, 0);

        if (res < 1)
            return -1;
        s->inlen += res;
        s->inbuf[s->inlen] = '\0';
    } else {
        return 2;
    }
    return 0;
}
/*******************************************************************************
 *  \fn astman_add_param(char *buf, int buflen, char *header, char *value)
 *  \brief  Add a new parameter to the Command
 *  \param  buf
 *  \param  buflen
 *  \param  header
 *  \param  value
 *  \return Number of wrote characters into the buf
 ******************************************************************************/
char *astman_get_header(struct message *m, const char *var) {
    char cmp[80];
    int x;
    snprintf(cmp, sizeof(cmp), "%s: ", var);
    for (x=0;x<m->hdrcount;x++)
        if (!strncasecmp(cmp, m->headers[x], strlen(cmp)))
            return m->headers[x] + strlen(cmp);
    return "";
}

/*
 * Execute event handler
 */
/*******************************************************************************
 *  \fn astman_add_param(char *buf, int buflen, char *header, char *value)
 *  \brief  Add a new parameter to the Command
 *  \param  buf
 *  \param  buflen
 *  \param  header
 *  \param  value
 *  \return Number of wrote characters into the buf
 ******************************************************************************/
static int astman_process_message(struct mansession *s, struct message *m) {
    int x;
    int res;
    char event[80];

    strncpy(event, astman_get_header(m, "Event"), sizeof(event));
    if (!strlen(event)) {
        astlog(ASTLOG_ERROR, "Missing event in request\n");
        return 0;
    }

    if (s->debug) {
        astlog(ASTLOG_INFO, "Got event packet: %s\n", event);
        for (x=0;x<m->hdrcount;x++) {
            astlog(ASTLOG_INFO, "Header: %s\n", m->headers[x]);
        }
    }

    for (x=0; x < s->eventcount; x++) {
        if (s->events[x].event && !strcasecmp(event, s->events[x].event)) {
            res = s->events[x].func(s, m);
            if (res < 0) {
                return -1;
            } else if (res > 0) {
                return res;
            }
            break;
        }
        /* Execute system event handler */
        if (s->events[x].event && !strcasecmp(ASTMAN_DEFAULT_EVENT, s->events[x].event)) {
            res = s->events[x].func(s, m);
            if (res < 0) {
                return -1;
            } else if (res > 0) {
                return res;
            }
            break;
        }
    }

    if (s->debug && x >= s->eventcount)
        astlog(ASTLOG_ERROR, "Ignoring unknown event '%s'\n", event);

    return 0;
}
/*******************************************************************************
 *  \fn astman_add_param(char *buf, int buflen, char *header, char *value)
 *  \brief  Add a new parameter to the Command
 *  \param  buf
 *  \param  buflen
 *  \param  header
 *  \param  value
 *  \return Number of wrote characters into the buf
 ******************************************************************************/
int astman_wait_for_response(struct mansession *s, struct message *msg, time_t timeout) {
    int res;
    int proc_ev;
    char *sp;
    time_t begin, end;
    struct message m;
    m.hdrcount = 0;
    time(&begin);
    for (;;) {
        res = astman_get_input(s, m.headers[m.hdrcount]);
        if (res == 1) { /* get single line */
            if (!m.gettingdata) {
                /* Strip trailing \r\n */
                if (strlen(m.headers[m.hdrcount]) < 2)
                    continue;
                m.headers[m.hdrcount][strlen(m.headers[m.hdrcount]) - 2] = '\0';
                /* finish line */
                if (strlen(m.headers[m.hdrcount]) == 0) {
                    if (s->debug)
                        astman_dump_message(&m);
                    /* Response packet */
                    if (strlen(astman_get_header(&m, "Response"))) {
                        memcpy(msg, &m, sizeof(m));
                        memset(&m, 0, sizeof(m));
                        return ASTMAN_SUCCESS;
                    }
                    /* Event packet */
                    if ((proc_ev = astman_process_message(s, &m)) < 0) {
                        /* Error */
                        break;
                        /* Complete */
                    } else if ( proc_ev > 0 ) {
                        memcpy(msg, &m, sizeof(m));
                        memset(&m, 0, sizeof(m));
                        return ASTMAN_SUCCESS;
                    }
                    memset(&m, 0, sizeof(m));
                } else if (m.hdrcount < MAX_HEADERS - 1) {
                    /* headers in packet */

                    /* Response: Follows */
                    if (!strncasecmp(m.headers[m.hdrcount], "Response: Follows",
                                     strlen("Response: Follows"))) {
                        m.gettingdata = 1;
                    }
                    m.hdrcount++;
                }
            } else {
                /* Get raw data from command (m.gettingdata = 1) */
                if ((sp = strstr(m.headers[m.hdrcount], "--END COMMAND--"))) {
                    /* End of getting data */
                    *sp = '\0';
                    m.gettingdata = 0;
                }
                if (m.hdrcount < MAX_HEADERS - 1)
                    m.hdrcount++;
            }
        } else if (res < 0) {
            return -1;
        } else if (res == 2) {
            if ( timeout > 0 ) {
                time(&end);
                if ( (end - begin) > timeout ) return 0;
            }
        }
    } /* end loop */
    return -1;
}
/*******************************************************************************
 *  \fn astman_add_param(char *buf, int buflen, char *header, char *value)
 *  \brief  Add a new parameter to the Command
 *  \param  buf
 *  \param  buflen
 *  \param  header
 *  \param  value
 *  \return Number of wrote characters into the buf
 ******************************************************************************/
int astman_manager_action(struct mansession *s, char *action, char *fmt, ...) {
    char tmp[4096];
    va_list ap;
    snprintf(tmp, sizeof(tmp), "Action: %s\r\n", action);
    va_start(ap, fmt);
    vsnprintf(tmp+strlen(tmp), sizeof(tmp)-strlen(tmp), fmt, ap);
    va_end(ap);
    strcpy(tmp+strlen(tmp), "\r\n");
    send(s->fd, tmp, strlen(tmp), 0);
    if (s->debug)
        astman_dump_out_message(tmp);
    return 0;
}
/*******************************************************************************
 *  \fn astman_add_param(char *buf, int buflen, char *header, char *value)
 *  \brief  Add a new parameter to the Command
 *  \param  buf
 *  \param  buflen
 *  \param  header
 *  \param  value
 *  \return Number of wrote characters into the buf
 ******************************************************************************/
int astman_manager_action_params(struct mansession *s, char *action, char *params) {
    return astman_manager_action(s, action, "%s", params);
}

/*
static int has_input(struct mansession *s)
{
  int x;
  for (x=1;x<s->inlen;x++)
    if ((s->inbuf[x] == '\n') && (s->inbuf[x-1] == '\r'))
      return 1;
  return 0;
}
*/
/*******************************************************************************
 *  \fn astman_add_param(char *buf, int buflen, char *header, char *value)
 *  \brief  Add a new parameter to the Command
 *  \param  buf
 *  \param  buflen
 *  \param  header
 *  \param  value
 *  \return Number of wrote characters into the buf
 ******************************************************************************/
int astman_add_event_handler_system(struct mansession *s, ASTMAN_EVENT_CALLBACK callback ) {
    return astman_add_event_handler(s, ASTMAN_DEFAULT_EVENT, callback );
}
/*******************************************************************************
 *  \fn astman_add_param(char *buf, int buflen, char *header, char *value)
 *  \brief  Add a new parameter to the Command
 *  \param  buf
 *  \param  buflen
 *  \param  header
 *  \param  value
 *  \return Number of wrote characters into the buf
 ******************************************************************************/
int astman_add_event_handler(struct mansession *s, char *event, ASTMAN_EVENT_CALLBACK callback ) {
    int x;

    if (s->eventcount >= MAX_EVENTS) {
        return -1;
    }

    for (x=0; x < s->eventcount; x++) {
        if (s->events[x].event && !strcasecmp(event, s->events[x].event)) {
            if (!callback) {
                /* Remove event handler */
                if (s->events[x].event) free(s->events[x].event);
                for (; x+1 < s->eventcount; x++) {
                    s->events[x] = s->events[x+1];
                }
                s->eventcount--;
                return 0;
            } else {
                fprintf(stderr, "%s handler is already defined, not over-writing.", event);
                return -1;
            }
        }
    }

    /* Add event handler */
    s->events[s->eventcount].event = strdup(event);
    s->events[s->eventcount].func = callback;
    s->eventcount++;

    return 1;
}
/*******************************************************************************
 *  \fn astman_add_param(char *buf, int buflen, char *header, char *value)
 *  \brief  Add a new parameter to the Command
 *  \param  buf
 *  \param  buflen
 *  \param  header
 *  \param  value
 *  \return Number of wrote characters into the buf
 ******************************************************************************/
int astman_login(struct mansession *s, char *username, char *secret) {
    struct message m;
    int res;

    if (astman_strlen_zero(username) || astman_strlen_zero(secret))
        return ASTMAN_FAILURE;

    /*astman_manager_action(s, "Challenge", "AuthType: MD5\r\n");
    res = astman_wait_for_response(s, &m, 10000);
    if ( res > 0 && !strcasecmp(astman_get_header(&m, "Response"), "Success")) {
      char *challenge = astman_get_header(&m, "Challenge");
      int x;
      int len = 0;
      char md5key[256] = "";
      MD5_CTX md5;
      unsigned char digest[16];
      MD5_Init(&md5);
      MD5_Update(&md5, (unsigned char*)challenge, strlen(challenge));
      MD5_Update(&md5, (unsigned char*)secret, strlen(secret));
      MD5_Final(digest, &md5);
      for (x=0; x<16; x++)
        len += sprintf(md5key + len, "%2.2x", digest[x]);
      astman_manager_action(s, "Login",
      	             "AuthType: MD5\r\n"
      	             "Username: %s\r\n"
      	             "Key: %s\r\n",
      	             username, md5key);*/
    astman_manager_action(s, "Login",
                          "Username: %s"CRLF
                          "Secret: %s"CRLF
                          "Events: on"CRLF,
                          username,
                          secret);
    res = astman_wait_for_response(s, &m, 10000);

    if (res > 0 && !strcasecmp(astman_get_header(&m, "Response"), "Success"))
        return ASTMAN_SUCCESS;
    astman_manager_action(s, "Login", "Username: %s\r\nSecret: %s\r\n",
                          username, secret);
    res = astman_wait_for_response(s, &m, 0);
    if (res > 0 && !strcasecmp(astman_get_header(&m, "Response"), "Success") )
        return ASTMAN_SUCCESS;
    return ASTMAN_FAILURE;
}
/*******************************************************************************
 *  \fn astman_add_param(char *buf, int buflen, char *header, char *value)
 *  \brief  Add a new parameter to the Command
 *  \param  buf
 *  \param  buflen
 *  \param  header
 *  \param  value
 *  \return Number of wrote characters into the buf
 ******************************************************************************/
int astman_logoff(struct mansession *s) {
    struct message m;

    astman_manager_action(s, "Logoff", "");
    astman_wait_for_response(s, &m, 0);
    astman_disconnect(s);
    return ASTMAN_SUCCESS;

}

