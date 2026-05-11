#pragma once

#ifndef LLAMACPPPROVIDER_H
#define LLAMACPPPROVIDER_H

#include "openai_provider.h"

/// Provider for llama.cpp server (OpenAI-compatible protocol).
/// Always local by default, no auth header.
class LlamaCppProvider : public OpenAIProvider {
    Q_OBJECT

public:
    explicit LlamaCppProvider(QObject* parent = nullptr);
};

#endif
