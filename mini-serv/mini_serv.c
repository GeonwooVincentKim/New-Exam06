#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>

static int	listenfd = -1, maxfd = -1, nextid = 0;
static int	cid[FD_SETSIZE], inlen[FD_SETSIZE], outlen[FD_SETSIZE];
static char	*inbuf[FD_SETSIZE], *outbuf[FD_SETSIZE];
static fd_set	allfds, readfds, writefds;

// in/outのbufをfreeして初期化する
void	initbuf(int fd)
{
	if (inbuf[fd]) free(inbuf[fd]);
	if (outbuf[fd]) free(outbuf[fd]);
	inbuf[fd] = NULL;
	outbuf[fd] = NULL;
	inlen[fd] = 0;
	outlen[fd] = 0;
}

void	wrong()
{
	write(2, "Wrong number of arguments\n", 26);
	exit(1);
}

void	fatal()
{
	write(2, "Fatal error\n", 12);
	for (int fd = 0; fd < FD_SETSIZE; ++fd)
	{
		if (FD_ISSET(fd, &allfds)) close(fd);
		cid[fd] = -1;
		initbuf(fd);
	}
	exit(1);
}

// 文字列をn文字分左に寄せる
void	shift_left(char *str, int *len, int n)
{
	if (*len <= 0 || n <= 0) return;
	if (n >= *len) { *len = 0; str[0] = 0; return; }

	for (int i = 0; i < *len - n; ++i) str[i] = str[n + i];
	*len -= n;
	str[*len] = 0;
}

// strjoinのための関数
void	bufjoin(char **dst, int *dstlen, const char *src, int n)
{
	if (n <= 0) return;
	if (*dstlen < 0) fatal();

	char	*p = realloc(*dst, (size_t)(*dstlen + n) + 1);
	if (!p) fatal();
	*dst = p;
	for (int i = 0; i < n; ++i) (*dst)[*dstlen + i] = src[i];
	*dstlen += n;
	(*dst)[*dstlen] = 0;
}

// 他のクライアントのoutbufにmsgを追加する
void	broadcast(int exceptfd, const char *msg)
{
	int	len = (int)strlen(msg);
	if (len <= 0) return;
	for (int fd = 0; fd <= maxfd; ++fd)
	{
		if (fd == listenfd || fd == exceptfd || cid[fd] == -1) continue;
		bufjoin(&outbuf[fd], &outlen[fd], msg, len);
	}
}

// 全てのoutbufをsendする(各ターミナルに表示させる)
// 送った残りを左に寄せる
void	flushout()
{
	for (int fd = 0; fd <= maxfd; ++fd)
	{
		if (fd == listenfd || cid[fd] == -1 || outlen[fd] == 0) continue;
		if (!FD_ISSET(fd, &writefds)) continue;
		ssize_t	s = send(fd, outbuf[fd], (size_t)outlen[fd], 0);
		if (s < 0) fatal();
		if (s > 0) shift_left(outbuf[fd], &outlen[fd], (int)s);
	}
}

// クライアントが接続を切った時の処理
// メッセージを作りbroadcastする
void	disconnect(int fd)
{
	char	msg[128];
	sprintf(msg, "server: client %d just left\n", cid[fd]);
	broadcast(fd, msg);

	FD_CLR(fd, &allfds);
	close(fd);
	cid[fd] = -1;
	initbuf(fd);

	if (fd == maxfd)
		while (maxfd >= 0 && !FD_ISSET(maxfd, &allfds)) --maxfd;
}

// inbufの中から'\n'までの文字列を切り出して、出力用文字列を作成し、broadcastする
// 残りの文字列を左に寄せる
void	handle_lines(int fd)
{
	if (!inbuf[fd]) return;
	while (1)
	{
		char	*nl = strstr(inbuf[fd], "\n");
		if (!nl) return;
		int	linelen = (int)(nl - inbuf[fd]) + 1;
		
		char	pre[64];
		sprintf(pre, "client %d: ", cid[fd]);
		int	prelen = (int)strlen(pre);

		char	*msg = malloc((size_t)(prelen + linelen) + 1);
		if (!msg) fatal();
		for (int i = 0; i < prelen; ++i) msg[i] = pre[i];
		for (int i = 0; i < linelen; ++i) msg[prelen + i] = inbuf[fd][i];
		msg[prelen + linelen] = 0;
		broadcast(fd, msg);
		free(msg);
		shift_left(inbuf[fd], &inlen[fd], linelen);
	}
}

// クライアントからのメッセージを受け取り、inbufに追加し、送信用の処理にまわす
void	recv_from_client(int fd)
{
	char	buf[4096];
	ssize_t	r = recv(fd, buf, sizeof(buf), 0);
	if (r < 0) fatal();
	if (r == 0) { disconnect(fd); return; }
	bufjoin(&inbuf[fd], &inlen[fd], buf, (int)r);
	handle_lines(fd);
}

// クライアントの接続処理をし、到着メッセージをbroadcastする
void	acceptone()
{
	struct sockaddr_in	cli;
	socklen_t	len = sizeof(cli);

	int	fd = accept(listenfd, (struct sockaddr *)&cli, &len);
	if (fd < 0) fatal();
	if (fd >= FD_SETSIZE) { close(fd); return; }

	FD_SET(fd, &allfds);
	cid[fd] = nextid++;
	initbuf(fd);
	if (fd > maxfd) maxfd = fd;

	char	msg[128];
	sprintf(msg, "server: client %d just arrived\n", cid[fd]);
	broadcast(fd, msg);
}

// サーバを立てる際の初期化処理
void	initserver(char *argv)
{
	FD_ZERO(&allfds);
	for (int fd = 0; fd < FD_SETSIZE; ++fd) { cid[fd] = -1; initbuf(fd); }

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) fatal();

	struct sockaddr_in	serv;
	bzero(&serv, sizeof(serv));

	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = 0x0100007f;
	unsigned short	port = (unsigned short)atoi(argv);
	serv.sin_port = (unsigned short)((port >> 8) | (port << 8));

	if (bind(listenfd, (const struct sockaddr *)&serv, sizeof(serv)) != 0) fatal();
	if (listen(listenfd, 128) != 0) fatal();

	FD_SET(listenfd, &allfds);
	maxfd = listenfd;
}

int	main(int argc, char **argv)
{
	if (argc != 2) wrong();
	initserver(argv[1]);

	while (1)
	{
		readfds = allfds;
		writefds = allfds;

		if (select(maxfd + 1, &readfds, &writefds, NULL, NULL) < 0) fatal();
		if (FD_ISSET(listenfd, &readfds)) acceptone();
		for (int fd = 0; fd <= maxfd; ++fd)
		{
			if (fd == listenfd || cid[fd] == -1) continue;
			if (FD_ISSET(fd, &readfds)) recv_from_client(fd);
		}
		flushout();
	}
}