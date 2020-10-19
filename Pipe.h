#pragma once
#include "Job.h"

// f の内容を path に出力する
// f を閉じることは無い
// エラー時には std::runtime_error を発生させる
void Redirect(FILE *f, const std::filesystem::path &path);

// f の内容を標準出力に出力する
// f を閉じることは無い
// エラー時には std::runtime_error を発生させる
void WriteStdout(FILE *f);

// job に設定されたコマンドを実行し、最後のコマンドの結果が格納されたファイルポインタを返す
// job に複数のコマンドが設定されている場合はパイプでコマンドを接続して実行する
// 返されたファイルポインタは使用後に閉じること
// エラー時には std::runtime_error を発生させる
FILE *PipeCommand(const Job &job);