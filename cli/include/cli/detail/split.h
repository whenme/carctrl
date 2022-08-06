// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_DETAIL_SPLIT_H_INCLUDED__
#define __CLI_DETAIL_SPLIT_H_INCLUDED__

#include <algorithm>
#include <cassert>
#include <string>
#include <utility>
#include <vector>

namespace cli::detail
{

class Text
{
public:
    explicit Text(std::string input) : m_input(std::move(input))
    {
    }
    void splitInto(std::vector<std::string>& strs)
    {
        reset();
        for (const char chr : m_input)
        {
            eval(chr);
        }
        removeEmptyEntries();
        m_splitResult.swap(strs);  // puts the result back in strs
    }

private:
    void reset()
    {
        m_state        = State::Space;
        m_prevState    = State::Space;
        m_sentenceType = SentenceType::DoubleQuote;
        m_splitResult.clear();
    }

    void eval(char chr)
    {
        switch (m_state)
        {
            case State::Space:
                evalSpace(chr);
                break;
            case State::Word:
                evalWord(chr);
                break;
            case State::Sentence:
                evalSentence(chr);
                break;
            case State::Escape:
                evalEscape(chr);
                break;
        }
    }

    void evalSpace(char chr)
    {
        if (chr == ' ' || chr == '\t' || chr == '\n')
        {
            // do nothing
        }
        else if (chr == '"' || chr == '\'')
        {
            newSentence(chr);
        }
        else if (chr == '\\')
        {
            // This is the case where the first character of a word is escaped.
            // Should come back into the word state after this.
            m_prevState = State::Word;
            m_state     = State::Escape;
            m_splitResult.emplace_back("");
        }
        else
        {
            m_state = State::Word;
            m_splitResult.emplace_back(1, chr);
        }
    }

    void evalWord(char chr)
    {
        if (chr == ' ' || chr == '\t' || chr == '\n')
        {
            m_state = State::Space;
        }
        else if (chr == '"' || chr == '\'')
        {
            newSentence(chr);
        }
        else if (chr == '\\')
        {
            m_prevState = m_state;
            m_state     = State::Escape;
        }
        else
        {
            assert(!m_splitResult.empty());
            m_splitResult.back() += chr;
        }
    }

    void evalSentence(char chr)
    {
        if (chr == '"' || chr == '\'')
        {
            auto newType = chr == '"' ? SentenceType::DoubleQuote : SentenceType::Quote;
            if (newType == m_sentenceType)
            {
                m_state = State::Space;
            }
            else
            {
                assert(!m_splitResult.empty());
                m_splitResult.back() += chr;
            }
        }
        else if (chr == '\\')
        {
            m_prevState = m_state;
            m_state     = State::Escape;
        }
        else
        {
            assert(!m_splitResult.empty());
            m_splitResult.back() += chr;
        }
    }

    void evalEscape(char chr)
    {
        assert(!m_splitResult.empty());
        if (chr != '"' && chr != '\'' && chr != '\\')
        {
            m_splitResult.back() += "\\";
        }
        m_splitResult.back() += chr;
        m_state = m_prevState;
    }

    void newSentence(char chr)
    {
        m_state        = State::Sentence;
        m_sentenceType = (chr == '"' ? SentenceType::DoubleQuote : SentenceType::Quote);
        m_splitResult.emplace_back("");
    }

    void removeEmptyEntries()
    {
        // remove null entries from the vector:
        m_splitResult.erase(std::remove_if(m_splitResult.begin(),
                                           m_splitResult.end(),
                                           [](const std::string& str) {
                                               return str.empty();
                                           }),
                            m_splitResult.end());
    }

    enum class State
    {
        Space,
        Word,
        Sentence,
        Escape
    };
    enum class SentenceType
    {
        Quote,
        DoubleQuote
    };
    State                    m_state        = State::Space;
    State                    m_prevState    = State::Space;
    SentenceType             m_sentenceType = SentenceType::DoubleQuote;
    const std::string        m_input;
    std::vector<std::string> m_splitResult;
};

// Split the string input into a vector of strings.
// The original string is split where there are spaces.
// Quotes and double quotes can be used to indicate a substring that should not be splitted
// (even if it contains spaces)

//          split(strs, "");                => empty vector
//          split(strs, " ");               => empty vector
//          split(strs, "  ");              => empty vector
//          split(strs, "\t");              => empty vector
//          split(strs, "  \t \t     ");    => empty vector

//          split(strs, "1234567890");      => <"1234567890">
//          split(strs, "  foo ");          => <"foo">
//          split(strs, "  foo \t \t bar \t");  => <"foo","bar">

//          split(strs, "\"\"");            => empty vector
//          split(strs, "\"foo bar\"");     => <"foo bar">
//          split(strs, "    \t\t \"foo \tbar\"     \t");   => <"foo \tbar">
//          split(strs, " first   \t\t \"foo \tbar\"     \t last"); => <"first","foo \tbar","last">
//          split(strs, "first\"foo \tbar\"");  => <"first","foo \tbar">
//          split(strs, "first \"'second' 'thirdh'\""); => <"first","'second' 'thirdh'">

//          split(strs, "''");      => empty vector
//          split(strs, "'foo bar'");   => <"foo bar">
//          split(strs, "    \t\t 'foo \tbar'     \t"); => <"foo \tbar">
//          split(strs, " first   \t\t 'foo \tbar'     \t last"); => <"first","foo \tbar","last">
//          split(strs, "first'foo \tbar'"); => <"first","foo \tbar">
//          split(strs, "first '\"second\" \"thirdh\"'"); => <"first","\"second\" \"thirdh\"">

//          split(strs, R"("foo\"bar")"); // "foo\"bar" => <"foo"bar">
//          split(strs, R"('foo\'bar')"); // 'foo\'bar' => <"foo'bar">
//          split(strs, R"("foo\bar")"); // "foo\bar" => <"foo\bar">
//          split(strs, R"("foo\\"bar")"); // "foo\\"bar" => <"foo\"bar">

inline void split(std::vector<std::string>& strs, const std::string& input)
{
    Text sentence(input);
    sentence.splitInto(strs);
}

}  // namespace cli::detail

#endif  // __CLI_DETAIL_SPLIT_H_INCLUDED__
