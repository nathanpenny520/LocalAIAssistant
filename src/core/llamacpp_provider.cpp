#include "llamacpp_provider.h"

LlamaCppProvider::LlamaCppProvider(QObject* parent) : OpenAIProvider(parent) {
    m_isLocalMode = true;
}
