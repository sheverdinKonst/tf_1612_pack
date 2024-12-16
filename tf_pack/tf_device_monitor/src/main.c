#include "run_CLIcmd.h"
#include "state_handler.h"
#include "settings_module.h"
#include "net_utils.h"
#include "version.h"

int main(int argc, char **argv)
{
    error_code_t errorCode = ERR_OK;
    openlog("tfdm", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "====>>> T_FORTIS DEVICE MANAGER <<<<==== Version %s \n", VERSION);
    closelog();
    printf("argc %d\n", argc);
    if (argc >=2)
    {
        for(int optid=1; optid < argc; optid++)
        {
            if (argv[optid][0] == '-')
            {
                switch (argv[optid][1])
                {
                    case 'h':
                    {
                        printf("help\n");
                        printf("v - Print Version\n");
                        printf("d - Print Debug Info\n");
                        printf("i - Set interface to bind\n");
                    }
                    break;

                    case 'v':
                    {
                        printf("Version %s\n", VERSION);
                    }
                    break;

                    case 'd':
                    {
                        printf("debug on\n");
                        setMainTestFlag(1);
                    }
                    break;

                    case 'l':
                    {
                        int param = 0;
                        printf("location on\n");
                        param = atoi(argv[optid+1]);
                        setLocationTestFlag(param);
                    }

                    case 'i':
                    {
                        printf("if_name = %s\n", argv[2]);
                        set_DevName(argv[2]);
                    }
                    break;

                    default:
                        printf("can not run \"tf device manager\" without -i <devName>");
                        return 101;
                }
            }
        }
    }

    errorCode = client_app();
    printf("main - ERROR %d\n", errorCode);
    return errorCode;
}

