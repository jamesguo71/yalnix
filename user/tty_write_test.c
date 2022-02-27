#include <yuser.h>



int main(void) {
    int terminal = 0;
    char buf[] = "hello, world!\n";
    TracePrintf(1, "[tty_write_test] About to write to terminal: %d\n", terminal);

    int len = (int) strlen(buf);
    int ret = TtyWrite(terminal, buf, len);
    TracePrintf(1, "[tty_write_test] wrote %d bytes.\n", ret);
    return 0;
}