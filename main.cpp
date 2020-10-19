#include <iostream>
#include <string>

#include "Job.h"
#include "Pipe.h"

int main()
{
    while (true)
    {
        fputs("> ", stdout);
        std::vector<char> buff(1024);
        if (!fgets(buff.data(), buff.size(), stdin))
        {
            break;
        }

        const auto job = ParseJob(buff.data());
        FILE *outfile = nullptr;
        try
        {
            outfile = PipeCommand(job);
        }
        catch (const std::runtime_error &e)
        {
            fprintf(stderr, "PipeCommand error, %s\n", e.what());
            return EXIT_FAILURE;
        }

        if (!job.redirectFilename.empty())
        {
            try
            {
                Redirect(outfile, job.redirectFilename);
            }
            catch (const std::runtime_error &e)
            {
                fclose(outfile);
                fprintf(stderr, "Redirect error, %s\n", e.what());
                return EXIT_FAILURE;
            }
        }
        else
        {
            try
            {
                WriteStdout(outfile);
            }
            catch (const std::runtime_error &e)
            {
                fclose(outfile);
                fprintf(stderr, "WriteStdout error, %s\n", e.what());
                return EXIT_FAILURE;
            }
        }

        fclose(outfile);
    }

    return EXIT_SUCCESS;
}