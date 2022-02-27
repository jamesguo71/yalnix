#include <yuser.h>

#define MAX_LINE 1024

int main(void) {
	int terminal = 0;
	char buf[MAX_LINE];
	int ret;
	int bytes;
	while (1) {

		// 1. Read a line from the terminal. TtyRead will block until there is input ready.
		//    The returned line is not null terminated, so be sure to do that yourself.
		TracePrintf(1, "[tty_test] About to read from terminal: %d\n", terminal);
		ret = TtyRead(terminal, buf, MAX_LINE);
		if (ret <= 0 || ret > MAX_LINE) {
			TracePrintf(1, "[tty_test] TtyRead returned bad number of bytes: %d\n", ret);
			return ret;
		}
		buf[ret] = '\0';

		// 2. I noticed that if I try to print ret the value is always 0. I think this is because
		//    TracePrintf returns immediately but does not execute the action right away. Thus,
		//    we end up looping back around and the value of ret gets reset because we use it
		//    for another TtyRead. By saving its previous value in bytes we get the correct print.
		bytes = ret;
		TracePrintf(1, "[tty_test] Read: %d bytes from terminal: %d\n", bytes, terminal);
		TracePrintf(1, "[tty_test] buf: %s\n", buf);
	}
	return 0;
}