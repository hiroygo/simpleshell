#include "Pipe.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iterator>
#include <algorithm>

void WriteStdout(FILE *f)
{
    while (true)
    {
        std::vector<char> buf(1024, '\0');
        if (!fgets(buf.data(), buf.size(), f))
        {
            break;
        }
        if (fprintf(stdout, "%s", buf.data()) < 0)
        {
            throw std::runtime_error("fprintf error");
        }
    }
}

void Redirect(FILE *f, const std::filesystem::path &path)
{
    FILE *outfile = fopen(path.c_str(), "w");
    if (!outfile)
    {
        const std::string err = "fopen error, " + std::string(std::strerror(errno));
        throw std::runtime_error(err);
    }

    while (true)
    {
        std::vector<char> buf(1024, '\0');
        if (!fgets(buf.data(), buf.size(), f))
        {
            break;
        }
        if (fprintf(outfile, "%s", buf.data()) < 0)
        {
            fclose(outfile);
            throw std::runtime_error("fprintf error");
        }
    }
    fclose(outfile);
}

FILE *PipeCommand(FILE *begin, const Job &job)
{
    const int beginfd = fileno(begin);
    if (beginfd == -1)
    {
        const std::string err = "fileno error, " + std::string(std::strerror(errno));
        throw std::runtime_error(err);
    }

    int pipelinkfd = beginfd;
    for (const auto &command : job.commands)
    {
        // NOTE: パイプには lseek できない
        int pipes[2];
        if (pipe(pipes) == -1)
        {
            const std::string err = "pipe error, " + std::string(std::strerror(errno));
            close(pipelinkfd);
            throw std::runtime_error(err);
        }
        const int rfd = pipes[0];
        const int wfd = pipes[1];

        const auto forked = fork();
        if (forked == -1)
        {
            const std::string err = "fork error, " + std::string(std::strerror(errno));
            close(pipelinkfd);
            close(rfd);
            close(wfd);
            throw std::runtime_error(err);
        }

        // child
        if (forked == 0)
        {
            if (dup2(pipelinkfd, STDIN_FILENO) == -1)
            {
                const std::string err = "infd dup2 error, " + std::string(std::strerror(errno));
                close(pipelinkfd);
                close(rfd);
                close(wfd);
                throw std::runtime_error(err);
            }
            if (dup2(wfd, STDOUT_FILENO) == -1)
            {
                const std::string err = "outfd dup2 error, " + std::string(std::strerror(errno));
                close(pipelinkfd);
                close(rfd);
                close(wfd);
                throw std::runtime_error(err);
            }

            std::vector<std::string> args = command.args;
            std::vector<char *> ptrs;
            std::for_each(args.begin(), args.end(), [&ptrs](auto &arg) { ptrs.push_back(arg.data()); });
            // execv は最後の要素に nullptr が必要
            ptrs.push_back(nullptr);

            // execv が成功すればそのプロセスに置き換わる
            execv(args.front().c_str(), ptrs.data());

            // プロセスが置き換わらなかったのでエラー
            const std::string err = "execv error, " + std::string(std::strerror(errno));
            close(pipelinkfd);
            close(rfd);
            close(wfd);
            throw std::runtime_error(err);
        }

        // parent
        {
            close(wfd);

            int status = 0;
            if (wait(&status) == -1)
            {
                const std::string err = "wait error, " + std::string(std::strerror(errno));
                close(pipelinkfd);
                close(rfd);
                close(wfd);
                throw std::runtime_error(err);
            }

            // プロセスが終了した
            if (WIFEXITED(status))
            {
                if (pipelinkfd != beginfd)
                {
                    close(pipelinkfd);
                }

                // 今回実行したコマンドの標準出力を取っておき
                // 次のコマンドの標準入力を接続する
                pipelinkfd = rfd;
            }

            // シグナルにより終了した
            if (WIFSIGNALED(status))
            {
                const std::string err = "error WIFSIGNALED, " + std::to_string(WTERMSIG(status));
                close(pipelinkfd);
                close(rfd);
                close(wfd);
                throw std::runtime_error(err);
            }
        }
    }

    FILE *f = fdopen(pipelinkfd, "r");
    if (!f)
    {
        const std::string err = "fdopen error, " + std::string(std::strerror(errno));
        throw std::runtime_error(err);
    }

    return f;
}
