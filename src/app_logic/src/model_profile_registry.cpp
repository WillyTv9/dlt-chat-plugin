#include "dltchat/model_profile_registry.h"

#include <QVector>

namespace dltchat {

namespace {

struct Entry
{
    QString provider;
    QString modelPrefix; // matched as case-insensitive startsWith
    int maxContextTokens;
    int reservedForResponse;
    int maxConcurrent;
};

const QVector<Entry> &table()
{
    static const QVector<Entry> kTable = {
        // GitHub Copilot — single OAuth bearer, tight upstream rate limits.
        { "copilot", "gpt-4o-mini",       128000, 4000, 2 },
        { "copilot", "gpt-4o",            128000, 8000, 2 },
        { "copilot", "gpt-4-turbo",       128000, 4000, 2 },
        { "copilot", "gpt-4",              32768, 4000, 2 },
        { "copilot", "claude-3-5-sonnet", 200000, 8000, 2 },
        { "copilot", "claude",            200000, 8000, 2 },
        { "copilot", "o1",                128000, 8000, 1 },
        { "copilot", "",                  128000, 4000, 2 }, // generic copilot

        // OpenAI direct.
        { "openai",  "gpt-4o-mini",       128000, 4000, 4 },
        { "openai",  "gpt-4o",            128000, 8000, 4 },
        { "openai",  "gpt-4-turbo",       128000, 4000, 4 },
        { "openai",  "gpt-4",              32768, 4000, 4 },
        { "openai",  "gpt-3.5",            16384, 2048, 6 },
        { "openai",  "o1",                128000, 8000, 2 },
        { "openai",  "",                  128000, 4000, 4 },

        // Local Ollama / LocalAI — small context, high parallelism OK
        // (single user, local hardware bound).
        { "ollama",  "llama3.1",           32768, 4096, 4 },
        { "ollama",  "llama3",              8192, 2048, 6 },
        { "ollama",  "qwen2.5",            32768, 4096, 4 },
        { "ollama",  "mistral",            32768, 4096, 4 },
        { "ollama",  "mixtral",            32768, 4096, 2 },
        { "ollama",  "phi",                 4096, 1024, 6 },
        { "ollama",  "",                    8192, 2048, 4 },

        { "localai", "",                    8192, 2048, 4 },
    };
    return kTable;
}

const Entry *findEntry(const QString &provider, const QString &model)
{
    const QString p = provider.trimmed().toLower();
    const QString m = model.trimmed().toLower();
    const Entry *fallback = nullptr;

    for (const Entry &e : table()) {
        if (e.provider != p)
            continue;
        if (e.modelPrefix.isEmpty()) {
            if (!fallback)
                fallback = &e;
            continue;
        }
        if (m.startsWith(e.modelPrefix))
            return &e;
    }
    return fallback;
}

} // namespace

ModelProfile ModelProfileRegistry::profileFor(const QString &provider, const QString &model)
{
    ModelProfile out;
    out.provider = provider;
    out.model = model;

    if (const Entry *e = findEntry(provider, model)) {
        out.maxContextTokens = e->maxContextTokens;
        out.reservedForResponse = e->reservedForResponse;
        out.maxConcurrent = e->maxConcurrent;
        out.isFallback = e->modelPrefix.isEmpty();
        return out;
    }

    // Truly unknown provider — conservative defaults; pipeline must still work.
    out.maxContextTokens = 8192;
    out.reservedForResponse = 2048;
    out.maxConcurrent = 2;
    out.isFallback = true;
    return out;
}

bool ModelProfileRegistry::isKnown(const QString &provider, const QString &model)
{
    const QString p = provider.trimmed().toLower();
    const QString m = model.trimmed().toLower();
    for (const Entry &e : table()) {
        if (e.provider != p)
            continue;
        if (!e.modelPrefix.isEmpty() && m.startsWith(e.modelPrefix))
            return true;
    }
    return false;
}

} // namespace dltchat
