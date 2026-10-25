#include "git-compat-util.h"
#include <termios.h>

uintmax_t strtoumax(const char *nptr, char **endptr, int base)
{
	return (uintmax_t)strtoull(nptr, endptr, base);
}

intmax_t strtoimax(const char *nptr, char **endptr, int base)
{
	return (intmax_t)strtoll(nptr, endptr, base);
}

int vprintf(const char *fmt, va_list ap)
{
	return vfprintf(stdout, fmt, ap);
}

int getc(FILE *stream)
{
	return fgetc(stream);
}

FILE *freopen(const char *path, const char *mode, FILE *stream)
{
	FILE *opened;
	int fd;

	if (stream == NULL) {
		errno = EINVAL;
		return NULL;
	}
	opened = fopen(path, mode);
	if (opened == NULL)
		return NULL;
	fd = opened->fd;
	if (fd != stream->fd && dup2(fd, stream->fd) < 0) {
		int saved_errno = errno;
		fclose(opened);
		errno = saved_errno;
		return NULL;
	}
	if (fd != stream->fd)
		fclose(opened);
	else
		free(opened);
	stream->eof = 0;
	stream->has_unget = 0;
	return stream;
}

int execlp(const char *file, const char *arg, ...)
{
	va_list ap;
	size_t argc = 1;
	char **argv;
	const char *value;
	int ret;
	int saved_errno;

	va_start(ap, arg);
	while ((value = va_arg(ap, const char *)) != NULL)
		argc++;
	va_end(ap);
	argv = calloc(argc + 1, sizeof(*argv));
	if (argv == NULL)
		return -1;
	argv[0] = (char *)arg;
	va_start(ap, arg);
	for (size_t i = 1; i < argc; i++)
		argv[i] = (char *)va_arg(ap, const char *);
	va_end(ap);
	ret = execvp(file, argv);
	saved_errno = errno;
	free(argv);
	errno = saved_errno;
	return ret;
}

pid_t tcgetpgrp(int fd)
{
	(void)fd;
	errno = ENOSYS;
	return -1;
}

char *getpass(const char *prompt)
{
	static char password[128];
	struct termios old_termios;
	struct termios noecho;
	FILE *input;
	FILE *output;
	size_t len = 0;
	int echo_changed = 0;
	int ch;

	input = fopen("/dev/tty", "r+");
	if (input == NULL)
		input = stdin;
	output = input == stdin ? stderr : input;
	if (prompt != NULL)
		fputs(prompt, output);
	fflush(output);
	if (tcgetattr(fileno(input), &old_termios) == 0 &&
	    (old_termios.c_lflag & ECHO)) {
		noecho = old_termios;
		noecho.c_lflag &= ~ECHO;
		if (tcsetattr(fileno(input), TCSANOW, &noecho) == 0)
			echo_changed = 1;
	}
	while (len + 1 < sizeof(password) &&
	       (ch = fgetc(input)) != EOF && ch != '\n' && ch != '\r')
		password[len++] = (char)ch;
	if (echo_changed)
		tcsetattr(fileno(input), TCSANOW, &old_termios);
	fputc('\n', output);
	if (input != stdin)
		fclose(input);
	if (ch == EOF && len == 0)
		return NULL;
	password[len] = '\0';
	return password;
}

void openlog(const char *ident, int logopt, int facility)
{
	(void)ident;
	(void)logopt;
	(void)facility;
}

void syslog(int priority, const char *fmt, ...)
{
	va_list ap;

	(void)priority;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}
