#include <yuser.h>

#define MAX_LINE 1024

int main(void) {
	int terminal = 0;
	char buf[MAX_LINE];
	TracePrintf(1, "[tty_test] About to read from terminal: %d\n", terminal);
	int ret = TtyRead(terminal, buf, MAX_LINE);
	if (ret <= 0 || ret >= MAX_LINE) {
		TracePrintf(1, "[tty_test] TtyRead returned bad number of bytes: %d\n", ret);
		return ret;
	}
	buf[ret] = '\0';
	TracePrintf(1, "[tty_test] Read: %d bytes from terminal: %d\n", ret, terminal);
	TracePrintf(1, "[tty_test] buf: %s\n", buf);
	return 0;
}