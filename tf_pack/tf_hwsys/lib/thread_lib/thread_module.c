//
// Created by sheverdin on 3/26/24.
//

#include "thread_module.h"

pthread_t threads_fd[THREADS_NUM];

uint8_t createThread(uint8_t maxThreads, mainThreads_t *func)
{
    for (int i = 0; i < maxThreads; i++)
    {
        int p_error = pthread_create(&threads_fd[i], NULL, (void*)func[i], NULL);
        if (p_error)
        {
          perror("pthread_create");
          return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

uint8_t joinThread(uint8_t maxThreads)
{
    for (int i = 0; i < maxThreads; i++)
    {
        int status = pthread_join(threads_fd[i], NULL);

        if (status != 0) {
            perror("joinThread");
            printf("ERR_JOIN_THREAD - i %d\n", i);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}


