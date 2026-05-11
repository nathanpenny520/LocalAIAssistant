#pragma once

#ifndef EMBEDDER_H
#define EMBEDDER_H

#include <memory>

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

#ifdef ONNXRUNTIME_AVAILABLE
#include <onnxruntime_cxx_api.h>
#endif

class Embedder {
public:
    Embedder();
    ~Embedder();

    bool loadModel(const QString& modelPath);
    bool isLoaded() const;
    int dimension() const;

    QVector<float> embed(const QString& text) const;
    QVector<QVector<float>> embedBatch(const QStringList& texts) const;

    static float cosineSimilarity(const QVector<float>& a, const QVector<float>& b);
    static QString findModelPath();

private:
    QVector<float> placeholderEmbed(const QString& text) const;

    // ── Tokenizer ──────────────────────────────────────────────
    struct TokenizerResult {
        QVector<int64_t> inputIds;
        QVector<int64_t> attentionMask;
    };

    bool loadTokenizer(const QString& tokenizerPath);
    TokenizerResult tokenize(const QString& text) const;

    // Pre-processing
    static QString preprocessChineseChars(const QString& text);
    static QString preprocessPunctuation(const QString& text);
    QStringList preTokenize(const QString& text) const;

    // WordPiece
    QVector<int> wordPieceTokenizeWord(const QString& word) const;
    int vocabId(const QString& token) const;

    // ── State ──────────────────────────────────────────────────
    int m_dimension = 384;
    bool m_loaded = false;
    QString m_modelPath;
    QString m_tokenizerPath;

    // Vocab
    QMap<QString, int> m_vocab;
    int m_clsTokenId = 101;
    int m_sepTokenId = 102;
    int m_padTokenId = 0;
    int m_unkTokenId = 100;
    int m_maxLength = 512;
    bool m_doLowerCase = true;

    // ONNX Runtime
#ifdef ONNXRUNTIME_AVAILABLE
    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::SessionOptions> m_sessionOpts;
    std::unique_ptr<Ort::Session> m_session;
    std::unique_ptr<Ort::MemoryInfo> m_memoryInfo;

    QVector<const char*> m_onnxInputNames;
    QVector<const char*> m_onnxOutputNames;
    bool m_hasTokenTypeIds = false;
    bool m_outputIsPooled = false;  // true if sent_emb, false if last_hidden_state
#endif
};

#endif  // EMBEDDER_H
