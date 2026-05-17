#include "embedder.h"

#include <algorithm>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>
#include <QtMath>

// ============================================================
// Embedder — all-MiniLM-L6-v2 via ONNX Runtime
// ============================================================

Embedder::Embedder() = default;
Embedder::~Embedder() {
#ifdef ONNXRUNTIME_AVAILABLE
    for (const char* name : m_onnxInputNames)
        free(const_cast<char*>(name));
    for (const char* name : m_onnxOutputNames)
        free(const_cast<char*>(name));
#endif
}

// ── Model loading ────────────────────────────────────────────

static QString findTokenizerPath(const QString& modelPath) {
    QFileInfo fi(modelPath);
    QDir dir = fi.dir();
    const QStringList candidates = {
            dir.filePath(QStringLiteral("tokenizer.json")),
            dir.filePath(QStringLiteral("../tokenizer.json")),
    };
    for (const auto& path : candidates) {
        if (QFileInfo::exists(QDir::cleanPath(path))) return QDir::cleanPath(path);
    }
    return {};
}

bool Embedder::loadModel(const QString& modelPath) {
    m_modelPath = modelPath;
    m_loaded = false;

#ifdef ONNXRUNTIME_AVAILABLE
    for (const char* name : m_onnxInputNames)
        free(const_cast<char*>(name));
    m_onnxInputNames.clear();
    for (const char* name : m_onnxOutputNames)
        free(const_cast<char*>(name));
    m_onnxOutputNames.clear();
#endif

    if (!QFileInfo::exists(modelPath)) {
        qWarning("Embedder: model file not found: %s", qUtf8Printable(modelPath));
        return false;
    }

#ifdef ONNXRUNTIME_AVAILABLE
    // ── Load tokenizer ───────────────────────────────────────
    QString tokenizerPath = findTokenizerPath(modelPath);
    if (tokenizerPath.isEmpty()) {
        // Try the standard resource paths
        QStringList searchPaths = {
                QCoreApplication::applicationDirPath() + QStringLiteral("/../Resources/models/"
                                                                        "tokenizer.json"),
                QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/models/"
                                                                        "tokenizer.json"),
                QDir::homePath() + QStringLiteral("/.locai/models/tokenizer.json"),
        };
        for (const auto& p : searchPaths) {
            QString clean = QDir::cleanPath(p);
            if (QFileInfo::exists(clean)) {
                tokenizerPath = clean;
                break;
            }
        }
    }

    if (tokenizerPath.isEmpty()) {
        qWarning("Embedder: tokenizer.json not found");
        return false;
    }

    if (!loadTokenizer(tokenizerPath)) {
        qWarning("Embedder: failed to load tokenizer");
        return false;
    }

    // ── Load ONNX model ──────────────────────────────────────
    try {
        m_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "Embedder");
        m_sessionOpts = std::make_unique<Ort::SessionOptions>();
        m_sessionOpts->SetIntraOpNumThreads(4);
        m_sessionOpts->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        m_session = std::make_unique<Ort::Session>(*m_env, modelPath.toUtf8().constData(),
                                                   *m_sessionOpts);

        m_memoryInfo = std::make_unique<Ort::MemoryInfo>(
                Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault));

        // Query input names
        Ort::AllocatorWithDefaultOptions allocator;
        size_t numInputs = m_session->GetInputCount();
        for (size_t i = 0; i < numInputs; ++i) {
            auto name = m_session->GetInputNameAllocated(i, allocator);
            QByteArray ba(name.get());
            m_onnxInputNames.append(qstrdup(ba.constData()));
            if (ba == "token_type_ids") m_hasTokenTypeIds = true;
        }

        // Query output names and shape
        size_t numOutputs = m_session->GetOutputCount();
        for (size_t i = 0; i < numOutputs; ++i) {
            auto name = m_session->GetOutputNameAllocated(i, allocator);
            QByteArray ba(name.get());
            m_onnxOutputNames.append(qstrdup(ba.constData()));

            auto typeInfo = m_session->GetOutputTypeInfo(i);
            auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
            auto shape = tensorInfo.GetShape();
            // 2D output (e.g. [batch, 384]) → already pooled
            m_outputIsPooled = (shape.size() == 2);
            if (shape.size() >= 2) m_dimension = static_cast<int>(shape.back());
        }

        qInfo("Embedder: ONNX model loaded, %zu inputs, %zu outputs, dim=%d%s", numInputs,
              numOutputs, m_dimension, m_outputIsPooled ? " (pooled)" : " (hidden states)");
    } catch (const Ort::Exception& e) {
        qWarning("Embedder: ONNX error: %s", e.what());
        return false;
    }

    m_loaded = true;
    m_tokenizerPath = tokenizerPath;
    return true;
#else
    // Fallback: no ONNX Runtime → use placeholder
    m_loaded = true;
    return true;
#endif
}

bool Embedder::isLoaded() const {
    return m_loaded;
}
int Embedder::dimension() const {
    return m_dimension;
}

// ── Tokenizer loading ────────────────────────────────────────

bool Embedder::loadTokenizer(const QString& tokenizerPath) {
    QFile file(tokenizerPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Embedder: cannot open tokenizer: %s", qUtf8Printable(tokenizerPath));
        return false;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning("Embedder: tokenizer JSON parse error: %s", qUtf8Printable(err.errorString()));
        return false;
    }

    QJsonObject root = doc.object();

    // Parse config (may also come from tokenizer_config.json)
    QJsonObject model = root.value("model").toObject();
    QJsonObject vocab = model.value("vocab").toObject();

    m_vocab.clear();
    for (auto it = vocab.begin(); it != vocab.end(); ++it) m_vocab[it.key()] = it.value().toInt();

    // Special token IDs (standard BERT)
    m_padTokenId = m_vocab.value(QStringLiteral("[PAD]"), 0);
    m_unkTokenId = m_vocab.value(QStringLiteral("[UNK]"), 100);
    m_clsTokenId = m_vocab.value(QStringLiteral("[CLS]"), 101);
    m_sepTokenId = m_vocab.value(QStringLiteral("[SEP]"), 102);

    // Check tokenizer_config.json for overrides
    QFileInfo fi(tokenizerPath);
    QDir dir = fi.dir();
    QString configPath = dir.filePath(QStringLiteral("tokenizer_config.json"));
    if (QFileInfo::exists(configPath)) {
        QFile cfgFile(configPath);
        if (cfgFile.open(QIODevice::ReadOnly)) {
            QJsonDocument cfgDoc = QJsonDocument::fromJson(cfgFile.readAll());
            QJsonObject cfg = cfgDoc.object();
            m_doLowerCase = cfg.value("do_lower_case").toBool(true);
            m_maxLength = cfg.value("model_max_length").toInt(512);
        }
    }

    qInfo("Embedder: tokenizer loaded, vocab size=%lld, max_length=%d",
          static_cast<long long>(m_vocab.size()), m_maxLength);
    return true;
}

// ── Preprocessing ────────────────────────────────────────────

QString Embedder::preprocessChineseChars(const QString& text) {
    // Add spaces around CJK characters
    QString result;
    result.reserve(text.size() * 2);
    for (const QChar& ch : text) {
        ushort u = ch.unicode();
        if ((u >= 0x2E80 && u <= 0x2FDF) ||  // CJK Radicals
            (u >= 0x3000 && u <= 0x303F) ||  // CJK Symbols
            (u >= 0x3040 && u <= 0x309F) ||  // Hiragana
            (u >= 0x30A0 && u <= 0x30FF) ||  // Katakana
            (u >= 0x3100 && u <= 0x312F) ||  // Bopomofo
            (u >= 0x3130 && u <= 0x318F) ||  // Hangul Compat
            (u >= 0x31F0 && u <= 0x31FF) ||  // Katakana Ext
            (u >= 0x3400 && u <= 0x4DBF) ||  // CJK Ext-A
            (u >= 0x4E00 && u <= 0x9FFF) ||  // CJK Unified
            (u >= 0xA000 && u <= 0xA4CF) ||  // Yi
            (u >= 0xAC00 && u <= 0xD7AF) ||  // Hangul Syllables
            (u >= 0xF900 && u <= 0xFAFF) ||  // CJK Compat
            (u >= 0xFE30 && u <= 0xFE4F) ||  // CJK Compat Forms
            (u >= 0xFF00 && u <= 0xFFEF)) {  // Halfwidth/Fullwidth
            result.append(QChar::Space);
            result.append(ch);
            result.append(QChar::Space);
        } else {
            result.append(ch);
        }
    }
    return result;
}

QString Embedder::preprocessPunctuation(const QString& text) {
    // Split ASCII punctuation from words (BERT basic tokenizer behavior)
    QString result;
    result.reserve(text.size() * 2);
    for (int i = 0; i < text.size(); ++i) {
        QChar ch = text[i];
        if (ch.isSpace()) {
            result.append(ch);
            continue;
        }
        ushort u = ch.unicode();
        // Don't split CJK or Unicode punctuation
        if (u >= 0x2000) result.append(ch);
        // ASCII punctuation to split
        else if (ch == QLatin1Char(',') || ch == QLatin1Char('.') || ch == QLatin1Char('!') ||
                 ch == QLatin1Char('?') || ch == QLatin1Char(';') || ch == QLatin1Char(':') ||
                 ch == QLatin1Char('"') || ch == QLatin1Char('\'') || ch == QLatin1Char('(') ||
                 ch == QLatin1Char(')') || ch == QLatin1Char('[') || ch == QLatin1Char(']') ||
                 ch == QLatin1Char('{') || ch == QLatin1Char('}') || ch == QLatin1Char('/') ||
                 ch == QLatin1Char('\\') || ch == QLatin1Char('@') || ch == QLatin1Char('#') ||
                 ch == QLatin1Char('$') || ch == QLatin1Char('%') || ch == QLatin1Char('^') ||
                 ch == QLatin1Char('&') || ch == QLatin1Char('*') || ch == QLatin1Char('+') ||
                 ch == QLatin1Char('-') || ch == QLatin1Char('=') || ch == QLatin1Char('<') ||
                 ch == QLatin1Char('>') || ch == QLatin1Char('`') || ch == QLatin1Char('~') ||
                 ch == QLatin1Char('|')) {
            result.append(QChar::Space);
            result.append(ch);
            result.append(QChar::Space);
        } else {
            result.append(ch);
        }
    }
    return result;
}

QStringList Embedder::preTokenize(const QString& text) const {
    QString normalized = m_doLowerCase ? text.toLower() : text;
    normalized = normalized.trimmed();

    // Add spaces around Chinese characters (tokenize_chinese_chars: true)
    normalized = preprocessChineseChars(normalized);

    // Split punctuation
    normalized = preprocessPunctuation(normalized);

    // Split on whitespace, filter empties
    QStringList tokens;
    for (const QString& tok :
         normalized.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts))
        tokens.append(tok);

    return tokens;
}

// ── WordPiece ────────────────────────────────────────────────

int Embedder::vocabId(const QString& token) const {
    auto it = m_vocab.constFind(token);
    return (it != m_vocab.constEnd()) ? it.value() : -1;
}

QVector<int> Embedder::wordPieceTokenizeWord(const QString& word) const {
    QVector<int> ids;

    if (word.isEmpty()) return ids;

    // If whole word is in vocab, use it directly
    int id = vocabId(word);
    if (id >= 0) {
        ids.append(id);
        return ids;
    }

    // WordPiece: greedy longest-match-first
    int start = 0;
    bool isFirst = true;

    while (start < word.length()) {
        int end = word.length();
        bool found = false;

        while (start < end) {
            QStringView substr = QStringView(word).mid(start, end - start);
            QString candidate = isFirst ? substr.toString()
                                        : QStringLiteral("##") + substr.toString();
            id = vocabId(candidate);
            if (id >= 0) {
                ids.append(id);
                found = true;
                break;
            }
            --end;
        }

        if (!found) {
            ids.append(m_unkTokenId);
            break;
        }

        start = end;
        isFirst = false;
    }

    return ids;
}

// ── Tokenization ─────────────────────────────────────────────

Embedder::TokenizerResult Embedder::tokenize(const QString& text) const {
    TokenizerResult result;

    // Pre-tokenize
    QStringList tokens = preTokenize(text);

    // WordPiece tokenization
    QVector<int> ids;
    ids.append(m_clsTokenId);  // [CLS]
    for (const QString& token : tokens) ids.append(wordPieceTokenizeWord(token));
    ids.append(m_sepTokenId);  // [SEP]

    // Truncate to max_length
    int len = qMin(ids.size(), m_maxLength);
    if (ids.size() > m_maxLength) {
        // Replace [SEP] at position max_length-1 if truncated
        ids[m_maxLength - 1] = m_sepTokenId;
    }

    // Build result with padding to max_length
    result.inputIds.resize(m_maxLength);
    result.attentionMask.resize(m_maxLength);

    for (int i = 0; i < m_maxLength; ++i) {
        if (i < len) {
            result.inputIds[i] = static_cast<int64_t>(ids[i]);
            result.attentionMask[i] = 1;
        } else {
            result.inputIds[i] = static_cast<int64_t>(m_padTokenId);
            result.attentionMask[i] = 0;
        }
    }

    return result;
}

// ── Embedding ────────────────────────────────────────────────

QVector<float> Embedder::embed(const QString& text) const {
    if (text.isEmpty()) return QVector<float>(m_dimension, 0.0f);

#ifdef ONNXRUNTIME_AVAILABLE
    if (m_session) {
        try {
            auto [inputIds, attentionMask] = tokenize(text);

            // Input shapes: [1, max_length]
            std::vector<int64_t> shape = {1, static_cast<int64_t>(m_maxLength)};

            // Create input tensors
            auto inputTensor = Ort::Value::CreateTensor<int64_t>(
                    *m_memoryInfo, inputIds.data(), inputIds.size(), shape.data(), shape.size());

            auto maskTensor = Ort::Value::CreateTensor<int64_t>(*m_memoryInfo, attentionMask.data(),
                                                                attentionMask.size(), shape.data(),
                                                                shape.size());

            // Run inference
            std::vector<Ort::Value> inputs;
            inputs.reserve(3);
            inputs.push_back(std::move(inputTensor));
            inputs.push_back(std::move(maskTensor));

            // Add token_type_ids (all zeros) if the model expects it
            QVector<int64_t> tokenTypeIds;
            if (m_hasTokenTypeIds) {
                tokenTypeIds.resize(m_maxLength, 0);
                auto typeTensor = Ort::Value::CreateTensor<int64_t>(*m_memoryInfo,
                                                                    tokenTypeIds.data(),
                                                                    tokenTypeIds.size(),
                                                                    shape.data(), shape.size());
                inputs.push_back(std::move(typeTensor));
            }

            auto outputs = m_session->Run(Ort::RunOptions {nullptr}, m_onnxInputNames.constData(),
                                          inputs.data(), inputs.size(),
                                          m_onnxOutputNames.constData(), m_onnxOutputNames.size());

            if (outputs.empty()) return QVector<float>(m_dimension, 0.0f);

            // Extract embedding
            float* rawData = outputs[0].GetTensorMutableData<float>();
            auto outShape = outputs[0].GetTensorTypeAndShapeInfo().GetShape();

            if (m_outputIsPooled) {
                // Output is already pooled: [1, 384]
                int dim = static_cast<int>(outShape.back());
                QVector<float> vec(dim);
                std::memcpy(vec.data(), rawData, dim * sizeof(float));
                // L2 normalize
                float norm = 0.0f;
                for (float v : vec) norm += v * v;
                norm = qSqrt(norm);
                if (norm > 0.0f)
                    for (float& v : vec) v /= norm;
                return vec;
            } else {
                // Output is last_hidden_state: [1, seq_len, 384]
                int seqLen = static_cast<int>(outShape[1]);
                int dim = static_cast<int>(outShape[2]);

                // Mean pooling with attention mask
                QVector<float> pooled(dim, 0.0f);
                float maskSum = 0.0f;
                for (int i = 0; i < seqLen; ++i) {
                    if (i < attentionMask.size() && attentionMask[i] == 0) continue;
                    float weight = 1.0f;
                    maskSum += weight;
                    float* tokenEmb = rawData + i * dim;
                    for (int j = 0; j < dim; ++j) pooled[j] += tokenEmb[j] * weight;
                }

                if (maskSum > 0.0f) {
                    for (int j = 0; j < dim; ++j) pooled[j] /= maskSum;
                }

                // L2 normalize
                float norm = 0.0f;
                for (float v : pooled) norm += v * v;
                norm = qSqrt(norm);
                if (norm > 0.0f)
                    for (float& v : pooled) v /= norm;

                return pooled;
            }
        } catch (const Ort::Exception& e) {
            qWarning("Embedder: inference error: %s", e.what());
        }
    }
#endif

    return placeholderEmbed(text);
}

QVector<QVector<float>> Embedder::embedBatch(const QStringList& texts) const {
    QVector<QVector<float>> results;
    results.reserve(texts.size());
    for (const auto& text : texts) results.append(embed(text));
    return results;
}

float Embedder::cosineSimilarity(const QVector<float>& a, const QVector<float>& b) {
    if (a.size() != b.size() || a.isEmpty()) return 0.0f;

    float dot = 0.0f, normA = 0.0f, normB = 0.0f;
    for (int i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }

    if (normA == 0.0f || normB == 0.0f) return 0.0f;

    return dot / (qSqrt(normA) * qSqrt(normB));
}

// ── Model path discovery ─────────────────────────────────────

QString Embedder::findModelPath() {
    QString modelName;
    QString arch = QSysInfo::currentCpuArchitecture();

    if (arch == QStringLiteral("arm64") || arch == QStringLiteral("aarch64")) {
        modelName = QStringLiteral("all-MiniLM-L6-v2-arm64.onnx");
    } else {
        modelName = QStringLiteral("all-MiniLM-L6-v2-avx2.onnx");
    }

    QStringList searchPaths = {
            QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/models/") +
                    modelName,
            QCoreApplication::applicationDirPath() + QStringLiteral("/../Resources/models/") +
                    modelName,
            QDir::homePath() + QStringLiteral("/.locai/models/") + modelName,
            QCoreApplication::applicationDirPath() + QStringLiteral("/models/") + modelName,
    };

    for (const auto& path : searchPaths) {
        QString cleanPath = QDir::cleanPath(path);
        if (QFileInfo::exists(cleanPath)) return cleanPath;
    }

    return {};
}

// ── Placeholder embedding ────────────────────────────────────

QVector<float> Embedder::placeholderEmbed(const QString& text) const {
    QVector<float> vec(m_dimension, 0.0f);
    if (text.isEmpty()) return vec;

    QString normalized = text.toLower().trimmed();

    // Character-level n-grams (not byte-level) so CJK text gets proper matching.
    // A CJK character is 3 UTF-8 bytes; byte-level trigrams span char boundaries
    // and produce near-zero semantic overlap. Character bigrams capture adjacent
    // character pairs for phrase-level matching (e.g. "三创" in "三创赛").
    const int len = normalized.length();

    // Unigrams (single characters) — term-level matching
    for (int i = 0; i < len; ++i) {
        int h = qHash(normalized[i]) % m_dimension;
        vec[h >= 0 ? h : -h] += 1.0f;
    }

    // Bigrams (adjacent character pairs) — phrase-level matching
    for (int i = 0; i < len - 1; ++i) {
        int h = qHash(QStringView(normalized).mid(i, 2)) % m_dimension;
        vec[h >= 0 ? h : -h] += 0.5f;
    }

    // L2 normalization
    float norm = 0.0f;
    for (float v : vec) norm += v * v;
    norm = qSqrt(norm);

    if (norm > 0.0f) {
        for (float& v : vec) v /= norm;
    }

    return vec;
}
