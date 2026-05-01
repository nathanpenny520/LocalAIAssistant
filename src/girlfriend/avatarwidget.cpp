#include "avatarwidget.h"
#include <QDebug>
#include <QDir>
#include <QCoreApplication>

AvatarWidget::AvatarWidget(QWidget *parent)
    : QWidget(parent)
    , m_avatarLabel(new QLabel(this))
    , m_emotionTagLabel(new QLabel(this))
    , m_currentEmotion("default")
    , m_isSpeaking(false)
{
    loadAvatarImages();

    // 设置布局：图片居中，情绪标签在左上角
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setScaledContents(false);

    // 情绪标签样式
    m_emotionTagLabel->setStyleSheet(
        "QLabel { background: rgba(255, 255, 255, 0.6); "
        "padding: 4px 12px; border-radius: 12px; font-size: 12px; }"
    );
    m_emotionTagLabel->setText("💕 默认");
    m_emotionTagLabel->move(12, 12);

    updateDisplay();
}

void AvatarWidget::loadAvatarImages()
{
    // 尝试多个可能的路径
    QStringList possiblePaths;

    QString appDir = QCoreApplication::applicationDirPath();
    possiblePaths << QDir::cleanPath(appDir + "/../Resources/AIGirlfriend");
    possiblePaths << QDir::cleanPath(appDir + "/AIGirlfriend");
    possiblePaths << "AIGirlfriend";
    possiblePaths << "sourcecode-ai-assistant/AIGirlfriend";

    QString avatarDir;
    for (const QString &path : possiblePaths) {
        QDir dir(path);
        if (dir.exists()) {
            avatarDir = path;
            break;
        }
    }

    if (avatarDir.isEmpty()) {
        qDebug() << "AvatarWidget: AIGirlfriend directory not found";
        return;
    }

    qDebug() << "AvatarWidget: Loading avatars from:" << avatarDir;

    // 加载所有表情图片
    QStringList emotions = {
        "default", "happy", "shy", "love", "hate",
        "sad", "angry", "afraid", "awaiting", "speaking", "studying"
    };

    QMap<QString, QString> fileNames = {
        {"default", "picture-original.png"},
        {"happy", "happy.png"},
        {"shy", "shy.png"},
        {"love", "love.png"},
        {"hate", "hate.png"},
        {"sad", "sad.png"},
        {"angry", "angry.png"},
        {"afraid", "afraid.png"},
        {"awaiting", "awaiting.png"},
        {"speaking", "speaking.png"},
        {"studying", "studying.png"}
    };

    for (const QString &emotion : emotions) {
        QString fileName = fileNames.value(emotion, emotion + ".png");
        QString fullPath = avatarDir + "/" + fileName;
        QPixmap pixmap(fullPath);
        if (!pixmap.isNull()) {
            m_avatarImages[emotion] = pixmap;
            qDebug() << "Loaded avatar:" << emotion << "from" << fullPath;
        } else {
            qDebug() << "Failed to load avatar:" << emotion << "from" << fullPath;
        }
    }
}

void AvatarWidget::setEmotion(const QString &emotion)
{
    if (m_currentEmotion != emotion) {
        m_currentEmotion = emotion;
        updateDisplay();
        emit emotionChanged(emotion);
    }
}

void AvatarWidget::setSpeaking(bool speaking)
{
    m_isSpeaking = speaking;
    updateDisplay();
}

void AvatarWidget::updateDisplay()
{
    // 如果正在说话，优先显示 speaking 图片
    QString displayEmotion = m_isSpeaking ? "speaking" : m_currentEmotion;

    if (m_avatarImages.contains(displayEmotion)) {
        QPixmap pixmap = m_avatarImages[displayEmotion];

        // 缩放图片以适应窗口，保持比例
        QSize labelSize = this->size();
        QPixmap scaled = pixmap.scaled(
            labelSize,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );

        m_avatarLabel->setPixmap(scaled);
        m_avatarLabel->resize(scaled.size());
        m_avatarLabel->move(
            (labelSize.width() - scaled.width()) / 2,
            (labelSize.height() - scaled.height()) / 2
        );
    }

    // 更新情绪标签
    QMap<QString, QString> emotionLabels = {
        {"default", "💕 默认"},
        {"happy", "💕 开心"},
        {"shy", " blush 害羞"},
        {"love", "💖 爱意"},
        {"hate", "哼 嫌弃"},
        {"sad", "😢 难过"},
        {"angry", "😤 生气"},
        {"afraid", "关心"},
        {"awaiting", "期待"},
        {"speaking", "说话中"},
        {"studying", "📚 思考"}
    };

    QString labelText = emotionLabels.value(m_currentEmotion, "💕 默认");
    m_emotionTagLabel->setText(labelText);
    m_emotionTagLabel->adjustSize();
}

QString AvatarWidget::getAvatarPath(const QString &emotion) const
{
    QMap<QString, QString> fileNames = {
        {"default", "picture-original.png"},
        {"happy", "happy.png"},
        {"shy", "shy.png"},
        {"love", "love.png"},
        {"hate", "hate.png"},
        {"sad", "sad.png"},
        {"angry", "angry.png"},
        {"afraid", "afraid.png"},
        {"awaiting", "awaiting.png"},
        {"speaking", "speaking.png"},
        {"studying", "studying.png"}
    };

    QString fileName = fileNames.value(emotion, emotion + ".png");
    return "AIGirlfriend/" + fileName;
}

void AvatarWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateDisplay();
    m_emotionTagLabel->move(12, 12);
}