#include <unistd.h>
#include <sys/wait.h>

#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <stdexcept>

// 最後の要素には nullptr が設定される
std::vector<char *> ExecArgs(const int argc, char *argv[])
{
    std::vector<char *> args;
    for (int i = 1; i < argc; i++)
    {
        args.push_back(argv[i]);
    }
    args.push_back(nullptr);
    return args;
}

// エラー時には std::runtime_error を発生させる
void ChildDo(std::vector<char *> args)
{
    execv(args[0], args.data());
    // exec が呼び出しから戻ったら失敗している
    const std::string err = "execv error, " + std::string(std::strerror(errno));
    throw std::runtime_error(err);
}

// エラー時には std::runtime_error を発生させる
void ParentDo(const int childPid)
{
    int childStatus = 0;
    const auto waitResult = waitpid(childPid, &childStatus, 0);
    if (waitResult == -1)
    {
        const std::string err = "waitpid error, " + std::string(std::strerror(errno));
        throw std::runtime_error(err);
    }

    if (WIFEXITED(childStatus))
    {
        const auto exitCode = WEXITSTATUS(childStatus);
        if (exitCode != EXIT_SUCCESS)
        {
            const std::string err = "子プロセス終了, " + std::to_string(exitCode);
            throw std::runtime_error(err);
        }
    }

    if (WIFSIGNALED(childStatus))
    {
        const std::string err = "子プロセスシグナル終了, " + std::to_string(WTERMSIG(childStatus));
        throw std::runtime_error(err);
    }
}

// エラー時には std::runtime_error を発生させる
void Do(int argc, char *argv[])
{
    const auto pid = fork();
    if (pid == -1)
    {
        const std::string err = "fork error, " + std::string(std::strerror(errno));
        throw std::runtime_error(err);
    }

    if (pid == 0)
    {
        const auto execArgs = ExecArgs(argc, argv);
        try
        {
            ChildDo(execArgs);
        }
        catch (const std::runtime_error &e)
        {
            const std::string err = "ChildDo error, " + std::string(e.what());
            throw std::runtime_error(err);
        }
    }
    else
    {
        try
        {
            ParentDo(pid);
        }
        catch (const std::runtime_error &e)
        {
            const std::string err = "ParentDo error, " + std::string(e.what());
            throw std::runtime_error(err);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fputs("実行するコマンドが指定されていません\n", stderr);
        return EXIT_FAILURE;
    }

    try
    {
        Do(argc, argv);
        return EXIT_SUCCESS;
    }
    catch (const std::runtime_error &e)
    {
        fprintf(stderr, "%s\n", e.what());
        return EXIT_FAILURE;
    }
}