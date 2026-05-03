#include "avatarwidget.h"
#include "girlfriend_translations.h"
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
    // 设置背景透明
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);

    loadAvatarImages();

    // 图片覆盖整个区域，保持比例裁剪
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setScaledContents(true);  // 允许缩放填充

    // 情绪标签样式 - 完全透明背景
    m_emotionTagLabel->setStyleSheet(
        "QLabel { background: transparent; "
        "padding: 4px 12px; font-size: 12px; }"
    );
    m_emotionTagLabel->setText(GTr::emotionDefault());
    m_emotionTagLabel->move(12, 12);
    m_emotionTagLabel->raise();  // 确保标签在图片上方

    updateDisplay();
}

void AvatarWidget::loadAvatarImages()
{
    // 尝试多个可能的路径
    QStringList possiblePaths;

    QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_MACOS
    // macOS app bundle 结构
    possiblePaths << QDir::cleanPath(appDir + "/../Resources/AIGirlfriend");
#elif defined(Q_OS_WIN)
    // Windows: 资源在可执行文件同级目录
    possiblePaths << QDir::cleanPath(appDir + "/AIGirlfriend");
#else
    // Linux
    possiblePaths << QDir::cleanPath(appDir + "/AIGirlfriend");
#endif

    // 通用备用路径
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
        "sad", "angry", "afraid", "awaiting", "speaking", "studying", "worried"
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
        {"studying", "studying.png"},
        {"worried", "worried.png"}   // 新增 worried 情绪
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

        // 缩放图片覆盖整个区域，保持比例裁剪
        QSize widgetSize = this->size();

        // 计算缩放比例，选择能覆盖整个区域的缩放方式
        QPixmap scaled = pixmap.scaled(
            widgetSize,
            Qt::KeepAspectRatioByExpanding,  // 覆盖整个区域，可能裁剪
            Qt::SmoothTransformation
        );

        // 如果缩放后仍比窗口大，居中裁剪
        if (scaled.width() > widgetSize.width() || scaled.height() > widgetSize.height()) {
            int x = (scaled.width() - widgetSize.width()) / 2;
            int y = (scaled.height() - widgetSize.height()) / 2;
            scaled = scaled.copy(x, y, widgetSize.width(), widgetSize.height());
        }

        m_avatarLabel->setPixmap(scaled);
        m_avatarLabel->resize(widgetSize);
        m_avatarLabel->move(0, 0);
    }

    // 更新情绪标签 - 使用 displayEmotion 保持与图片同步
    QMap<QString, QString> emotionLabels = {
        {"default", GTr::emotionDefault()},
        {"happy", GTr::emotionHappy()},
        {"shy", GTr::emotionShy()},
        {"love", GTr::emotionLove()},
        {"hate", GTr::emotionHate()},
        {"sad", GTr::emotionSad()},
        {"angry", GTr::emotionAngry()},
        {"afraid", GTr::emotionAfraid()},
        {"awaiting", GTr::emotionAwaiting()},
        {"speaking", GTr::emotionSpeaking()},
        {"studying", GTr::emotionStudying()},
        {"worried", GTr::emotionWorried()}
    };

    QString labelText = emotionLabels.value(displayEmotion, GTr::emotionDefault());
    m_emotionTagLabel->setText(labelText);
    m_emotionTagLabel->adjustSize();
    m_emotionTagLabel->raise();  // 确保标签在图片上方
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
        {"studying", "studying.png"},
        {"worried", "worried.png"}
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

void AvatarWidget::retranslateUi()
{
    // 更新情绪标签文字
    updateDisplay();
}