#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#define errcheck(rc, funcName) \
	if (rc == -1) { perror(funcName); return EXIT_FAILURE; }


int main(int argc, char **argv) {

	openlog("tfortis", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "test message");
	closelog();

	return 0;
}
