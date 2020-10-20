#include <vector>

#include "Job.h"
#include "Pipe.h"

int main()
{
    std::vector<std::filesystem::path> pathes;
    try
    {
        pathes = GetPathes();
    }
    catch (const std::runtime_error &e)
    {
        fprintf(stderr, "GetPathes error, %s\n", e.what());
        return EXIT_FAILURE;
    }

    while (true)
    {
        fputs("> ", stdout);
        std::vector<char> buff(1024);
        if (!fgets(buff.data(), buff.size(), stdin))
        {
            break;
        }

        Job job = ParseJob(buff.data());
        try
        {
            job = ResolveCommandPath(pathes, job);
        }
        catch (const std::runtime_error &e)
        {
            fprintf(stderr, "ResolveCommandPath error, %s\n", e.what());
            return EXIT_FAILURE;
        }

        FILE *outfile = nullptr;
        try
        {
            outfile = PipeCommand(stdin, job);
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