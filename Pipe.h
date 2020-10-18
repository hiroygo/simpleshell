#pragma once
#include "Job.h"

// fd の内容をファイルとして path に出力する
// エラー時には std::runtime_error を発生させる
void Redirect(int fd, const std::filesystem::path &path);

// fd の内容を標準出力に出力する
// エラー時には std::runtime_error を発生させる
void WriteStdout(int fd);

// job に設定されたコマンドを実行し、最後のコマンドの結果が格納されたファイルディスクリプタを返す
// job に複数のコマンドが設定されている場合はパイプでコマンドを接続して実行する
// 返されたファイルディスクリプタは使用後に閉じること
// エラー時には std::runtime_error を発生させる
int PipeCommand(const Job &job);