#include "Job.h"

#include <regex>
#include <filesystem>

#include <unistd.h>

/*
# bash の構文を BNF っぽく定義してみる
* 右辺には正規表現を用いる
* 文字列のクォーテーションには対応しない
* `<JOB>       = <CMD>{'|'<CMD>}*{'>'<STR>}?'\n'`
* `<CMD>       = <STR>{' '<STR>}*`
* `<STR>       = [^ ]+`
*/

namespace
{
    // 解析される文字列を表す
    class StringToBeParsed final
    {
    public:
        StringToBeParsed(const char *s) : m_string(s){};

        // 次の文字に移動し、移動した結果を返す
        // すでに文字列末尾に到達していて、移動できない場合は '\n' を返す
        // m_string が空文字の場合は '\n' を返す
        char NextChar()
        {
            // m_currentPos == m_string.size() のときに文字列末尾が判別できる
            if (m_currentPos < m_string.size())
            {
                m_currentPos++;
            }
            return CurrentChar();
        };

        // すでに文字列末尾に到達していて、移動できない場合は '\n' を返す
        // m_string が空文字の場合は '\n' を返す
        char CurrentChar() const
        {
            if (m_string.empty() || m_currentPos == m_string.size())
            {
                return '\n';
            }
            return m_string.at(m_currentPos);
        };

    public:
        const std::string m_string;

    private:
        size_t m_currentPos = 0;
    };

    enum Token
    {
        Pipe,
        Redirect,
        StrSeparator,
        Str,
        End,
    };

    Token ToToken(const char c)
    {
        switch (c)
        {
        case '|':
        {
            return Token::Pipe;
        }
        case '>':
        {
            return Token::Redirect;
        }
        case ' ':
        {
            return Token::StrSeparator;
        }
        case '\n':
        {
            return Token::End;
        }
        default:
        {
            return Token::Str;
        }
        }
    }

    // p の現在の解析位置から <STR> を取得する
    // <STR> を取得できた場合、p の解析地点も移動する
    std::string ParseStr(StringToBeParsed &p)
    {
        std::string str;
        while (true)
        {
            const auto c = p.CurrentChar();
            if (ToToken(c) != Token::Str)
            {
                return str;
            }

            str += c;
            p.NextChar();
        }
    }

    Command NextCmd(StringToBeParsed &p)
    {
        Command cmd;

        // <STR> をすべて読み込む
        while (true)
        {
            // 連続するスペースを飛ばす
            while (ToToken(p.CurrentChar()) == Token::StrSeparator)
            {
                p.NextChar();
            }

            const auto str = ParseStr(p);
            if (!str.empty())
            {
                cmd.args.push_back(str);
            }

            // <STR> の次のトークンを調べる
            // スペースが存在するなら次の <STR> が存在するかもしれないので続行する
            if (ToToken(p.CurrentChar()) == Token::StrSeparator)
            {
                // 次の <STR> の初めの文字に移動させる
                p.NextChar();
            }
            else
            {
                return cmd;
            }
        }
    }

    Job ParseJob(StringToBeParsed &p)
    {
        Job job;

        // <CMD> をすべて読み込む
        while (true)
        {
            // 連続するスペースを飛ばす
            while (ToToken(p.CurrentChar()) == Token::StrSeparator)
            {
                p.NextChar();
            }

            Command cmd(NextCmd(p));
            if (!cmd.args.empty())
            {
                job.commands.push_back(cmd);
            }

            // NextCmd は スペース+<STR> が連続する箇所を読み取るので、NextCmd 後に出現するトークンはスペース以外になる
            // そのためスペースを飛ばす処理は不要になる

            // <CMD> の次のトークンを調べる
            // '|' が存在するなら次の <CMD> が存在するかもしれないので続行する
            if (ToToken(p.CurrentChar()) == Token::Pipe)
            {
                // 次の <CMD> の初めの文字に移動させる
                p.NextChar();
            }
            else
            {
                break;
            }
        }

        // リダイレクトを読み込む
        if (ToToken(p.CurrentChar()) == Token::Redirect)
        {
            // '>' の次の文字に移動させる
            p.NextChar();

            // 連続するスペースを飛ばす
            while (ToToken(p.CurrentChar()) == Token::StrSeparator)
            {
                p.NextChar();
            }

            job.redirectFilename = ParseStr(p);
        }

        return job;
    }

} // namespace

Job ParseJob(const char *job)
{
    StringToBeParsed p(job);
    return ParseJob(p);
}

std::vector<std::filesystem::path> GetPathes()
{
    const auto ppath = getenv("PATH");
    if (!ppath)
    {
        throw std::runtime_error("getenv error, 環境変数 PATH が存在しません");
    }

    std::vector<std::filesystem::path> pathes;
    const std::string path(ppath);
    const std::regex regex("([^:]+)");
    for (std::sregex_iterator itr(path.cbegin(), path.cend(), regex), end; itr != end; ++itr)
    {
        pathes.push_back(itr->str());
    }

    return pathes;
}

Job ResolveCommandPath(const std::vector<std::filesystem::path> &pathes, const Job &job)
{
    Job ret = job;
    for (auto &command : ret.commands)
    {
        if (command.args.empty())
        {
            continue;
        }

        for (const auto &dir : pathes)
        {
            const auto path = dir / command.args.front();
            try
            {
                if (std::filesystem::exists(path))
                {
                    command.args.front() = path.c_str();
                    break;
                }
            }
            catch (std::filesystem::filesystem_error &e)
            {
                const std::string err = "exists error, " + std::string(e.what());
                throw std::runtime_error(err);
            }
        }
    }

    return ret;
}