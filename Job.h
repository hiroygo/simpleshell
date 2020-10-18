#pragma once
#include <vector>
#include <string>
#include <filesystem>

struct Command final
{
    std::vector<std::string> args;
};

struct Job final
{
    std::vector<Command> commands;

    // リダイレクトが指定されている場合に設定される
    // リダイレクトが指定されていない場合は空になる
    std::filesystem::path redirectFilename;
};

Job ParseJob(const char *job);