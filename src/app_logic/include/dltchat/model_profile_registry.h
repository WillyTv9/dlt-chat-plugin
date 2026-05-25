#ifndef DLTCHAT_MODEL_PROFILE_REGISTRY_H
#define DLTCHAT_MODEL_PROFILE_REGISTRY_H

#include <QString>

namespace dltchat {

/**
 * Static metadata describing a (provider, model) combination, used to
 * size LLM prompts and decide map-reduce concurrency without hard-coding
 * limits inside the request path.
 *
 * maxContextTokens     — model's full context window (input + output).
 * reservedForResponse  — tokens that must stay free for the completion;
 *                        prompt budget is (maxContext - reserved - overhead).
 * maxConcurrent        — safe parallelism for fan-out (map-reduce shards).
 *                        For Copilot this is intentionally low (single
 *                        OAuth bearer + tight upstream rate limits).
 */
struct ModelProfile
{
    QString provider;
    QString model;
    int maxContextTokens = 8192;
    int reservedForResponse = 2048;
    int maxConcurrent = 2;
    bool isFallback = false;
};

class ModelProfileRegistry
{
public:
    /**
     * Lookup a profile for the given provider/model identifiers as returned by
     * DltLlmAnalyzerInterface::detectProviderType() and the configured model
     * name. Matching is case-insensitive; falls back to a conservative profile
     * (isFallback=true) when no exact match is known.
     */
    static ModelProfile profileFor(const QString &provider, const QString &model);

    /**
     * Convenience: returns true if the registry has explicit knowledge of the
     * given pair (useful for diagnostic UI error messages).
     */
    static bool isKnown(const QString &provider, const QString &model);
};

} // namespace dltchat

#endif
