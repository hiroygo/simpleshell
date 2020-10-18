#include "Pipe.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iterator>
#include <algorithm>

void WriteStdout(const int fd)
{
    std::vector<char> buff(1024, 0);
    while (true)
    {
        const auto rSize = read(fd, buff.data(), buff.size());
        if (rSize == -1)
        {
            const std::string err = "read error, " + std::string(strerror(errno));
            throw std::runtime_error(err);
        }
        if (rSize == 0)
        {
            return;
        }

        const auto wrSize = write(STDOUT_FILENO, buff.data(), rSize);
        if (wrSize == -1)
        {
            const std::string err = "write error, " + std::string(strerror(errno));
            throw std::runtime_error(err);
        }
        if (wrSize != rSize)
        {
            const std::string err = "write size error, " + std::to_string(wrSize);
            throw std::runtime_error(err);
        }
    }
}

void Redirect(const int fd, const std::filesystem::path &path)
{
    FILE *fin = fdopen(fd, "r");
    if (!fin)
    {
        const std::string err = "in fdopen error, " + std::string(std::strerror(errno));
        throw std::runtime_error(err);
    }

    FILE *fout = fopen(path.c_str(), "w");
    if (!fout)
    {
        fclose(fin);
        const std::string err = "out fdopen error, " + std::string(std::strerror(errno));
        throw std::runtime_error(err);
    }

    while (true)
    {
        std::vector<char> buf(1024, '\0');
        if (!fgets(buf.data(), buf.size(), fin))
        {
            break;
        }
        if (fprintf(fout, "%s", buf.data()) < 0)
        {
            fclose(fin);
            fclose(fout);
            throw std::runtime_error("fprintf error");
        }
    }
    fclose(fin);
    fclose(fout);
}

// パイプには lseek できない
int PipeCommand(const Job &job)
{
    // TODO: STDIN_FILENO でいいの?
    int pipelinkfd = STDIN_FILENO;
    for (const auto &command : job.commands)
    {
        int pipes[2];
        if (pipe(pipes) == -1)
        {
            const std::string err = "pipe error, " + std::string(std::strerror(errno));
            close(pipelinkfd);
            throw std::runtime_error(err);
        }
        const int rfd = pipes[0];
        const int wfd = pipes[1];

        const auto forkret = fork();
        if (forkret == -1)
        {
            const std::string err = "fork error, " + std::string(std::strerror(errno));
            close(pipelinkfd);
            close(rfd);
            close(wfd);
            throw std::runtime_error(err);
        }

        // child
        if (forkret == 0)
        {
            if (dup2(pipelinkfd, STDIN_FILENO) == -1)
            {
                const std::string err = "dup2 error, " + std::string(std::strerror(errno));
                close(pipelinkfd);
                close(rfd);
                close(wfd);
                throw std::runtime_error(err);
            }
            if (dup2(wfd, STDOUT_FILENO) == -1)
            {
                const std::string err = "dup2 error, " + std::string(std::strerror(errno));
                close(pipelinkfd);
                close(rfd);
                close(wfd);
                throw std::runtime_error(err);
            }

            std::vector<std::string> args = command.args;
            std::vector<char *> ptrs;
            std::for_each(args.begin(), args.end(), [&ptrs](auto &arg) { ptrs.push_back(arg.data()); });
            ptrs.push_back(nullptr);

            // execv が成功すればそのプロセスに置き換わる
            execv(args.front().c_str(), ptrs.data());

            // error
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
                // 1 つ前のコマンドの標準出力とのパイプを閉じる
                close(pipelinkfd);

                // 今回実行したコマンドの標準出力を取っておき
                // 次のコマンドの標準入力を接続する
                pipelinkfd = rfd;
            }

            // シグナルにより終了した
            if (WIFSIGNALED(status))
            {
                const std::string err = "WIFSIGNALED error, " + std::to_string(WTERMSIG(status));
                close(pipelinkfd);
                close(rfd);
                close(wfd);
                throw std::runtime_error(err);
            }
        }
    }

    return pipelinkfd;
}
