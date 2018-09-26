/* vi: set sw=4 ts=4: */
/*
 * ftpget
 *
 * Mini implementation of FTP to retrieve a remote file.
 *
 * Copyright (C) 2002 Jeff Angielski, The PTR Group <jeff@theptrgroup.com>
 * Copyright (C) 2002 Glenn McGrath
 *
 * Based on wget.c by Chip Rosenthal Covad Communications
 * <chip@laserlink.net>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define isdigit(a) ((unsigned char)((a) - '0') <= 9)
#define LONE_DASH(s)     ((s)[0] == '-' && !(s)[1])

#define COMMON_BUFSIZE	4096
static char G_Buf[64];
//#define BUFSZ	(COMMON_BUFSIZE	- sizeof(struct globals))
#define BUFSZ	4096
static char ftpbuf[BUFSZ];

struct len_and_sockaddr {
    socklen_t len;
    union {
        struct sockaddr sa;
        struct sockaddr_in sin;
    } u;
};


struct globals {
	struct len_and_sockaddr *lsa;
	FILE *control_stream;
	int verbose_flag;
	/*int do_continue;*/
	//char buf[4]; /* actually [BUFSZ] */
};

static struct globals *G;

//#define user           (G->user          )
//#define password       (G->password      )
#define lsa            (G->lsa           )
#define control_stream (G->control_stream)
#define verbose_flag   (G->verbose_flag  )
//#define do_continue    (G->do_continue   )
//#define buf            (G->buf           )

extern int  bb_copyfd_eof(int fd1, int fd2);

static void INIT_G(void)
{
    G = (struct globals *)&G_Buf[0];
}

static unsigned int xatoul_range(const char *str, int min, int max)
{
    char *ptr;

    int n = strtoul(str, &ptr, 10);

    if (n < min)
        n = min;
    else if (n > max)
        n = max;

    return n;
}

static int xconnect_stream(const struct len_and_sockaddr *__lsa)
{
    int r;
    struct sockaddr *s_addr;
    int fd;

    fd = socket(__lsa->u.sa.sa_family, SOCK_STREAM, 0);
    s_addr = (struct sockaddr *)&__lsa->u.sa;
    r = connect(fd, s_addr, __lsa->len);
    if (r < 0) {
        printf("can't connect to remote host %s\n",
                inet_ntoa(((struct sockaddr_in *)s_addr)->sin_addr));
        return -1;
    }
    return fd;
}

static void print_ftp_rx_msg(const char *msg)
{
#if 0
	char *cp = ftpbuf; /* buf holds peer's response */

	/* Guard against garbage from remote server */
	while (*cp >= ' ' && *cp < '\x7f')
		cp++;
	*cp = '\0';
#endif
	printf(" unexpected server response, %s: %s", msg, ftpbuf);
}

static int ftpcmd(const char *s1, const char *s2)
{
	int n;
        char *ptr;
        int error = 0;

	/*printf("cmd %s %s\n", s1, s2);*/

	if (s1) {
		fprintf(control_stream, (s2 ? "%s %s\r\n" : "%s %s\r\n"+3),
						s1, s2);
		fflush(control_stream);
	}

	do {
		strcpy(ftpbuf, "EOF"); /* for print_ftp_rx_msg */
		if (fgets(ftpbuf, BUFSZ - 2, control_stream) == NULL) {
			print_ftp_rx_msg(NULL);
                        error = 1;
                        break;
		}
	} while (!isdigit(ftpbuf[0]) || ftpbuf[3] != ' ');

        if (error)
            return -1;

	ftpbuf[3] = '\0';
	n = strtoul(ftpbuf, &ptr, 10); //xatou(ftpbuf);
	ftpbuf[3] = ' ';
	return n;
}

static int ftp_login(char *user, char *password)
{
        int error = 0;

	/* Connect to the command socket */
	control_stream = fdopen(xconnect_stream(lsa), "r+");
	if (control_stream == NULL) {
		/* fdopen failed - extremely unlikely */
                return -1;
	}

	if (ftpcmd(NULL, NULL) != 220) {
		print_ftp_rx_msg(NULL);
                return -1;
	}

	/*  Login to the server */
	switch (ftpcmd("USER", user)) {
	case 230:
		break;
	case 331:
		if (ftpcmd("PASS", password) != 230) {
			print_ftp_rx_msg("PASS");
                        error = 1;
		}
		break;
	default:
		print_ftp_rx_msg("USER");
                error = 1;
	}

        if (error)
            return -1;

	if (ftpcmd("TYPE I", NULL) != 200) {
            printf("TYPE I, ERROR\n");
            return -1;
        }

        return 0;
}

static int xconnect_ftpdata(void)
{
	char *buf_ptr;
	unsigned int port_num1, port_num2;
        struct sockaddr_in *sin;

/*
TODO: PASV command will not work for IPv6. RFC2428 describes
IPv6-capable "extended PASV" - EPSV.

"EPSV [protocol]" asks server to bind to and listen on a data port
in specified protocol. Protocol is 1 for IPv4, 2 for IPv6.
If not specified, defaults to "same as used for control connection".
If server understood you, it should answer "229 <some text>(|||port|)"
where "|" are literal pipe chars and "port" is ASCII decimal port#.

There is also an IPv6-capable replacement for PORT (EPRT),
but we don't need that.

NB: PASV may still work for some servers even over IPv6.
For example, vsftp happily answers
"227 Entering Passive Mode (0,0,0,0,n,n)" and proceeds as usual.

TODO2: need to stop ignoring IP address in PASV response.
*/

	if (ftpcmd("PASV", NULL) != 227) {
		print_ftp_rx_msg("PASV");
                return -1;
	}

	/* Response is "NNN garbageN1,N2,N3,N4,P1,P2[)garbage]
	 * Server's IP is N1.N2.N3.N4 (we ignore it)
	 * Server's port for data connection is P1*256+P2 */
	buf_ptr = strrchr(ftpbuf, ')');
	if (buf_ptr)
            *buf_ptr = '\0';

	buf_ptr = strrchr(ftpbuf, ',');
	*buf_ptr = '\0';
	port_num1 = xatoul_range(buf_ptr + 1, 0, 255);

	buf_ptr = strrchr(ftpbuf, ',');
	*buf_ptr = '\0';
	port_num2 = xatoul_range(buf_ptr + 1, 0, 255);
	port_num1 += port_num2 * 256;

	//set_nport(&lsa->u.sa, htons(port_num));
	sin = (struct sockaddr_in *)&lsa->u.sa;
        sin->sin_port = htons(port_num1);

	return xconnect_stream(lsa);
}

static int pump_data_and_QUIT(int from, int to)
{
	/* copy the file */
	if (bb_copyfd_eof(from, to) == -1) {
		/* error msg is already printed by bb_copyfd_eof */
                printf("< ERROR, FILE DONWLOAD >\n");
		return -1;
	}

	/* close data connection */
	close(from); /* don't know which one is that, so we close both */
	close(to);

	/* does server confirm that transfer is finished? */
	if (ftpcmd(NULL, NULL) != 226) {
		print_ftp_rx_msg(NULL);
                return -1;
	}
	ftpcmd("QUIT", NULL);

	return 0;
}

int ftp_receive(const char *local_path, char *server_path)
{
	int fd_data;
	int fd_local = -1;
	//off_t beg_range = 0;

	/* connect to the data socket */
	fd_data = xconnect_ftpdata();
        if (fd_data < 0) {
            printf("ERROR, Cannot connect ftp server\n");
            return -1;
        }

	if (ftpcmd("SIZE", server_path) != 213) {
                printf("SIZE CMD, error\n");
                return -1;
	}

#if 0
	if (LONE_DASH(local_path)) {
		fd_local = STDOUT_FILENO;
		do_continue = 0;
	}
#endif

#if 0
	if (do_continue) {
		struct stat sbuf;
		/* lstat would be wrong here! */
		if (stat(local_path, &sbuf) < 0) {
			printf("ERROR, stat\n");
		}
		if (sbuf.st_size > 0) {
			beg_range = sbuf.st_size;
		} else {
			do_continue = 0;
		}
	}

	if (do_continue) {
		sprintf(buf, "REST %lu", beg_range);
		if (ftpcmd(buf, NULL) != 350) {
			do_continue = 0;
		}
	}
#endif

	if (ftpcmd("RETR", server_path) > 150) {
		print_ftp_rx_msg("RETR");
                return -1;
	}

	/* create local file _after_ we know that remote file exists */
	if (fd_local == -1) {
		fd_local = open(local_path,
			/*do_continue ? (O_APPEND | O_WRONLY) :*/ (O_CREAT | O_TRUNC | O_WRONLY),
                        0666
		);
                if (fd_local < 0) {
                    printf("FILE OPEN ERROR, Local(%s)\n", local_path);
                    return -1;
                }
	}

	return pump_data_and_QUIT(fd_data, fd_local);
}

int ftp_send(const char *server_path, char *local_path);

static struct len_and_sockaddr Global_r;
struct len_and_sockaddr *__host2sockaddr(const char *host, int port)
{
    struct len_and_sockaddr *r;
    struct in_addr in4;
    struct sockaddr_in *sin;

    if (inet_aton(host, &in4) != 0) {
        r = (struct len_and_sockaddr *)&Global_r;
        r->len = sizeof(struct sockaddr_in);
        r->u.sa.sa_family = AF_INET;
        r->u.sin.sin_addr = in4;
	sin = (struct sockaddr_in *)&r->u.sa;
        sin->sin_port = htons(port);
    }
    else
        r = NULL;

    return r;
}

void ftpget_init(void)
{
    INIT_G();
}

int ftpget_login(char *host_ip, char *user_name, char *passwd)
{
        int ret;
        struct sockaddr *sa;
        char host[128];
        char serv[16];
        socklen_t salen;

	INIT_G();

	/* We want to do exactly _one_ DNS lookup, since some
	 * sites (i.e. ftp.us.debian.org) use round-robin DNS
	 * and we want to connect to only one IP... */
	lsa = __host2sockaddr(host_ip, 21);

        sa = &lsa->u.sa;
        salen = sizeof(struct sockaddr_in);
        ret = getnameinfo(sa, salen, host, sizeof(host), serv, sizeof(serv),
            NI_NUMERICHOST | NI_NUMERICSERV);
	printf("[ Connecting to (%s:%s) ]\n", host, serv);

	ret = ftp_login(user_name, passwd);
        if (ret < 0) {
            //printf("< Login Fail >\n");
            return ret;
        }
        else
            //printf("[ Login Success ]\n");

        return 0;
}

#if 0
int ftpget_main(char *host_ip, char *user_name, char *passwd,
        char *local_filename, char *remote_filename)
{
        int ret = 0;
        struct sockaddr *sa;
        char host[128];
        char serv[16];
        socklen_t salen;

	INIT_G();

	/* We want to do exactly _one_ DNS lookup, since some
	 * sites (i.e. ftp.us.debian.org) use round-robin DNS
	 * and we want to connect to only one IP... */
	lsa = __host2sockaddr(host_ip, 21);

        sa = &lsa->u.sa;
        salen = sizeof(struct sockaddr_in);
        ret = getnameinfo(sa, salen, host, sizeof(host), serv, sizeof(serv),
            NI_NUMERICHOST | NI_NUMERICSERV);
	printf("Connecting to %s (%s:%s)\n", host_ip,
			host, serv);

	ret = ftp_login(user_name, passwd);
        if (ret < 0) {
            printf("Login Fail\n");
            return ret;
        }
        else
            printf("Login Success\n");

	return ftp_receive(local_filename, remote_filename);
}
#endif

