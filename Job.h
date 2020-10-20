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

// 環境変数 PATH に記述されたパス一覧を取得する
// エラー時には std::runtime_error を発生させる
std::vector<std::filesystem::path> GetPathes();

// pathes 中のディレクトリにコマンドが存在するか探し、存在すればそのパスでコマンドパスを置き換える
// エラー時には std::runtime_error を発生させる
Job ResolveCommandPath(const std::vector<std::filesystem::path> &pathes, const Job &job);