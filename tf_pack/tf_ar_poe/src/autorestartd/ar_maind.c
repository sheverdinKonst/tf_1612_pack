//
// Created by sheverdin on 6/24/24.
//

#include "ar_mainApp.h"
#include <signal.h>
#include <ucontext.h>

void sigill_handler(int sig, siginfo_t *info, void *context) {
    printf("Received SIGILL signal from source: %p\n", info->si_addr);
    printf("Received SIGILL signal si_code: %d\n", info->si_code);
    printf("Received SIGILL signal si_errno: %d\n", info->si_errno);
    printf("Received SIGILL signal si_signoe: %d\n", info->si_signo);
    signal(SIGILL, SIG_DFL);
}

void sigsegv_handler(int sig, siginfo_t *info, void *context) {
    printf("Received SIGSEGV signal from source: %p\n", info->si_addr);
    signal(SIGSEGV, SIG_DFL);
}

int main(int argc, char **argv)
{
    struct sigaction sa_ill, sa_segv;
    memset(&sa_ill, 0, sizeof(struct sigaction));
    sa_ill.sa_sigaction = sigill_handler;
    sa_ill.sa_flags = SA_SIGINFO;

    memset(&sa_segv, 0, sizeof(struct sigaction));
    sa_segv.sa_sigaction = sigsegv_handler;
    sa_segv.sa_flags = SA_SIGINFO;

    if (sigaction(SIGILL, &sa_ill, NULL) == -1) {
        perror("Error setting up SIGILL handler");
        return 1;
    }

    if (sigaction(SIGSEGV, &sa_segv, NULL) == -1) {
        perror("Error setting up SIGSEGV handler");
        return 1;
    }

    // Cause SIGILL signal
    //__asm__ __volatile__("ud2");

    openlog("tf_autoresart", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "MAIN TF AUTORESTART - %s", VERSION);
    closelog();
    ar_mainInit(argc, argv);
    ar_mainApp();
    return 0;
}