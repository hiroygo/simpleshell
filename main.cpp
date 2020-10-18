#include <unistd.h>

#include "Job.h"
#include "Pipe.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fputs("引数の数が不正です\n", stderr);
        return EXIT_FAILURE;
    }

    int lastfd = -1;
    try
    {
        const auto job = ParseJob(argv[1]);
        lastfd = PipeCommand(job);

        if (!job.redirectFilename.empty())
        {
            Redirect(lastfd, job.redirectFilename);
        }
        else
        {
            WriteStdout(lastfd);
        }

        close(lastfd);
        return EXIT_SUCCESS;
    }
    catch (const std::runtime_error &e)
    {
        close(lastfd);
        fprintf(stderr, "%s\n", e.what());
        return EXIT_FAILURE;
    }
}