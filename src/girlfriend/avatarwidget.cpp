#include "avatarwidget.h"
#include "girlfriend_translations.h"
#include "girlfriendsettings.h"
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QFile>

AvatarWidget::AvatarWidget(QWidget *parent)
    : QWidget(parent)
    , m_avatarLabel(new QLabel(this))
    , m_videoPlayer(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_videoWidget(new QVideoWidget(this))
    , m_emotionTagLabel(new QLabel(this))
    , m_moodBarWidget(new QLabel(this))
    , m_moodPercentLabel(new QLabel(this))
    , m_currentEmotion("default")
    , m_isSpeaking(false)
    , m_currentLevel(GirlfriendSettings::instance()->avatarLevel())
{
    // 设置背景透明
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);

    // Video player for Level 3 - 先创建，确保在底层
    m_videoPlayer->setAudioOutput(m_audioOutput);
    m_videoPlayer->setVideoOutput(m_videoWidget);
    m_videoWidget->setAttribute(Qt::WA_TranslucentBackground);
    m_videoWidget->setAttribute(Qt::WA_ShowWithoutActivating);
    m_videoWidget->setStyleSheet("background: transparent;");
    m_videoWidget->setAspectRatioMode(Qt::IgnoreAspectRatio);
    m_videoWidget->hide();

    loadAvatarImages();

    // Connect to settings changes
    connect(GirlfriendSettings::instance(), &GirlfriendSettings::avatarLevelChanged,
            this, &AvatarWidget::setAvatarLevel);

    // 图片覆盖整个区域，保持比例裁剪
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setScaledContents(true);

    // 情绪标签样式 - 粉红色背景白色文字
    m_emotionTagLabel->setStyleSheet(
        "QLabel { background: rgba(233, 30, 99, 0.85); color: white; "
        "padding: 4px 12px; font-size: 12px; border-radius: 6px; }"
    );
    m_emotionTagLabel->setText(GTr::emotionDefault());
    m_emotionTagLabel->move(12, 12);
    m_emotionTagLabel->raise();

    // Mood bar widget - 粉红色风格
    m_moodBarWidget->setStyleSheet("QLabel { background: transparent; }");
    m_moodBarWidget->setFixedHeight(6);
    m_moodBarWidget->setFixedWidth(50);

    // Mood percentage label - 白色文字
    m_moodPercentLabel->setStyleSheet(
        "QLabel { background: transparent; font-size: 10px; color: white; }"
    );

    // Connect video sound setting
    connect(GirlfriendSettings::instance(), &GirlfriendSettings::videoSoundChanged,
            this, [this](bool enabled) {
        m_audioOutput->setMuted(!enabled);
    });

    // Initial mute setting
    m_audioOutput->setMuted(!GirlfriendSettings::instance()->videoSoundEnabled());

    // 确保层级：视频在底层，标签在上层
    m_videoWidget->stackUnder(m_avatarLabel);
    m_avatarLabel->stackUnder(m_emotionTagLabel);
    m_emotionTagLabel->stackUnder(m_moodBarWidget);
    m_moodBarWidget->stackUnder(m_moodPercentLabel);

    updateMoodDisplay();
    updateDisplay();
}

void AvatarWidget::loadAvatarImages()
{
    // Get avatar path from GirlfriendSettings
    QString avatarDir = GirlfriendSettings::instance()->avatarLevelPath();

    if (avatarDir.isEmpty()) {
        qDebug() << "AvatarWidget: Avatar level path not found";
        return;
    }

    QDir dir(avatarDir);
    if (!dir.exists()) {
        qDebug() << "AvatarWidget: Avatar directory does not exist:" << avatarDir;
        return;
    }

    qDebug() << "AvatarWidget: Loading avatars from:" << avatarDir;

    // Store current level from settings
    m_currentLevel = GirlfriendSettings::instance()->avatarLevel();

    // Clear existing images
    m_avatarImages.clear();

    // Define emotion to filename mapping
    QMap<QString, QString> fileNames = {
        {"default", "default.png"},
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
        {"worried", "worried.png"},
        {"crying", "crying.png"},
        {"travelling", "travelling.png"}
    };

    // Level 1 special case: uses "picture-original.png" for default
    if (m_currentLevel == AvatarLevel::Level1_Belle) {
        fileNames["default"] = "picture-original.png";
    }

    // Load all emotion images
    QStringList emotions = fileNames.keys();
    for (const QString &emotion : emotions) {
        QString fileName = fileNames.value(emotion);
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
    // 状态锁检查：如果锁定，暂存情绪但不立即切换
    if (m_stateLocked) {
        m_pendingEmotion = emotion;
        qDebug() << "AvatarWidget: State locked, pending emotion:" << emotion;
        return;
    }

    if (m_currentEmotion != emotion) {
        m_currentEmotion = emotion;
        updateDisplay();
        emit emotionChanged(emotion);
    }
}

QString AvatarWidget::currentDisplayEmotion() const
{
    // 返回当前实际显示的情绪（考虑 speaking 状态）
    return m_isSpeaking ? "speaking" : m_currentEmotion;
}

void AvatarWidget::setSpeaking(bool speaking)
{
    m_isSpeaking = speaking;
    updateDisplay();

    // 当 speaking 状态改变时，触发情绪变化信号（让 overlay 更新）
    // 使用 displayEmotion（实际显示的情绪），而不是 m_currentEmotion
    QString displayEmotion = m_isSpeaking ? "speaking" : m_currentEmotion;
    emit emotionChanged(displayEmotion);
}

void AvatarWidget::setMood(double mood)
{
    m_currentMood = mood;
    updateMoodDisplay();
}

void AvatarWidget::setAvatarLevel(AvatarLevel level)
{
    if (m_currentLevel != level) {
        // 停止当前视频播放（任何等级切换都要停止）
        stopVideo();

        // 切换等级时重置 speaking 状态（如果没有在播放 TTS）
        // 如果正在播放 TTS，GirlfriendWindow 会处理状态

        m_currentLevel = level;
        loadAvatarImages();

        // 切换到 Level 3 时，确保视频音频输出正确设置
        if (level == AvatarLevel::Level3_Hotter) {
            // 重新设置音频输出（不使用 msleep，避免阻塞）
            m_videoPlayer->setAudioOutput(m_audioOutput);
            // 应用当前的视频声音设置
            m_audioOutput->setMuted(!GirlfriendSettings::instance()->videoSoundEnabled());
            qDebug() << "AvatarWidget: Level 3 audio output set, muted:"
                     << !GirlfriendSettings::instance()->videoSoundEnabled();
        }

        updateDisplay();

        qDebug() << "AvatarWidget: Avatar level changed to" << static_cast<int>(level)
                 << ", pending emotion preserved:" << m_pendingEmotion;
    }
}

void AvatarWidget::hideInternalLabels(bool hide)
{
    m_emotionTagLabel->setVisible(!hide);
    m_moodBarWidget->setVisible(!hide);
    m_moodPercentLabel->setVisible(!hide);
}

void AvatarWidget::updateMoodDisplay()
{
    int percent = static_cast<int>(m_currentMood * 100);

    QString barColor;
    if (m_currentMood > 0.7) {
        barColor = "linear-gradient(90deg, #e91e63, #ff4081)";
    } else if (m_currentMood >= 0.4) {
        barColor = "#e91e63";
    } else {
        barColor = "#9e9e9e";
    }

    // Mood bar using HTML
    QString barHtml = QString(
        "<div style='background: #e0e0e0; border-radius: 3px; width: 50px; height: 6px;'>"
        "<div style='background: %1; border-radius: 3px; width: %2px; height: 6px;'>"
        "</div></div>"
    ).arg(barColor).arg(static_cast<int>(m_currentMood * 50));

    m_moodBarWidget->setText(barHtml);
    m_moodBarWidget->setTextFormat(Qt::RichText);

    m_moodPercentLabel->setText(QString("%1%").arg(percent));
    m_moodPercentLabel->adjustSize();

    // 先确保情绪标签已调整大小
    m_emotionTagLabel->adjustSize();

    // Position below emotion tag - aligned with emotion label left edge
    int emotionLabelHeight = m_emotionTagLabel->sizeHint().height();
    m_moodBarWidget->move(12, 12 + emotionLabelHeight + 4);
    m_moodBarWidget->raise();

    m_moodPercentLabel->move(12 + 54, 12 + emotionLabelHeight + 2);
    m_moodPercentLabel->raise();
}

void AvatarWidget::updateDisplay()
{
    // 如果正在说话，优先显示 speaking 图片/视频
    QString displayEmotion = m_isSpeaking ? "speaking" : m_currentEmotion;

    // Level 3 uses video playback
    if (m_currentLevel == AvatarLevel::Level3_Hotter) {
        // Hide image label, show video widget
        m_avatarLabel->hide();
        m_videoWidget->show();
        m_videoWidget->setGeometry(0, 0, width(), height());
        playVideo(displayEmotion);

        // 视频模式下，确保情绪标签和mood bar在视频之上
        m_emotionTagLabel->raise();
        m_moodBarWidget->raise();
        m_moodPercentLabel->raise();
    } else {
        // Level 1/2 use images
        stopVideo();
        m_avatarLabel->show();

        if (m_avatarImages.contains(displayEmotion)) {
            QPixmap pixmap = m_avatarImages[displayEmotion];

            // Level 2: 使用KeepAspectRatio完整显示图片（窗口已设为9:16比例）
            QSize widgetSize = this->size();

            Qt::AspectRatioMode aspectMode = Qt::KeepAspectRatioByExpanding;  // 默认填充
            if (m_currentLevel == AvatarLevel::Level2_Hot) {
                // Level 2: 保持比例完整显示，窗口已是9:16比例
                aspectMode = Qt::KeepAspectRatio;
            }

            QPixmap scaled = pixmap.scaled(
                widgetSize,
                aspectMode,
                Qt::SmoothTransformation
            );

            // 如果使用KeepAspectRatioByExpanding且缩放后比窗口大，居中裁剪
            if (aspectMode == Qt::KeepAspectRatioByExpanding &&
                (scaled.width() > widgetSize.width() || scaled.height() > widgetSize.height())) {
                int x = (scaled.width() - widgetSize.width()) / 2;
                int y = (scaled.height() - widgetSize.height()) / 2;
                scaled = scaled.copy(x, y, widgetSize.width(), widgetSize.height());
            }

            m_avatarLabel->setPixmap(scaled);
            m_avatarLabel->resize(widgetSize);
            m_avatarLabel->move(0, 0);

            // 居中显示（对于KeepAspectRatio可能不会填满）
            if (scaled.width() < widgetSize.width() || scaled.height() < widgetSize.height()) {
                int x = (widgetSize.width() - scaled.width()) / 2;
                int y = (widgetSize.height() - scaled.height()) / 2;
                m_avatarLabel->move(x, y);
                m_avatarLabel->resize(scaled.size());
            }
        }
    }

    // 更新情绪标签 - 使用 displayEmotion 保持与图片/视频同步
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
        {"worried", GTr::emotionWorried()},
        {"crying", GTr::emotionCrying()},
        {"travelling", GTr::emotionTravelling()}
    };

    QString labelText = emotionLabels.value(displayEmotion, GTr::emotionDefault());
    m_emotionTagLabel->setText(labelText);
    m_emotionTagLabel->adjustSize();
    m_emotionTagLabel->raise();  // 确保标签在图片/视频上方

    // 确保mood bar也在最上层（特别是视频模式下）
    m_moodBarWidget->raise();
    m_moodPercentLabel->raise();
}

QString AvatarWidget::getAvatarPath(const QString &emotion) const
{
    QMap<QString, QString> fileNames = {
        {"default", "default.png"},
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
        {"worried", "worried.png"},
        {"crying", "crying.png"},
        {"travelling", "travelling.png"}
    };

    // Level 1 special case: uses "picture-original.png" for default
    if (m_currentLevel == AvatarLevel::Level1_Belle && emotion == "default") {
        return GirlfriendSettings::instance()->avatarLevelPath() + "/picture-original.png";
    }

    QString fileName = fileNames.value(emotion, emotion + ".png");
    return GirlfriendSettings::instance()->avatarLevelPath() + "/" + fileName;
}

void AvatarWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateDisplay();
    m_emotionTagLabel->move(12, 12);
    m_emotionTagLabel->adjustSize();

    // 重新定位mood bar
    int emotionLabelHeight = m_emotionTagLabel->sizeHint().height();
    m_moodBarWidget->move(12, 12 + emotionLabelHeight + 4);
    m_moodBarWidget->raise();
    m_moodPercentLabel->move(12 + 54, 12 + emotionLabelHeight + 2);
    m_moodPercentLabel->raise();

    // 确保情绪标签在最上层
    m_emotionTagLabel->raise();

    // Update video widget geometry if visible
    if (m_videoWidget->isVisible()) {
        m_videoWidget->setGeometry(0, 0, width(), height());
    }
}

void AvatarWidget::retranslateUi()
{
    // 更新情绪标签文字
    updateDisplay();
}

void AvatarWidget::playVideo(const QString &emotion)
{
    if (m_currentLevel != AvatarLevel::Level3_Hotter) {
        return;
    }

    QString avatarDir = GirlfriendSettings::instance()->avatarLevelPath();
    QString videoPath = avatarDir + "/" + emotion + ".mp4";

    if (!QFile(videoPath).exists()) {
        videoPath = avatarDir + "/default.mp4";
        if (!QFile(videoPath).exists()) {
            qDebug() << "AvatarWidget: No video found for emotion:" << emotion;
            return;
        }
    }

    // 每次播放时确保音频输出正确
    m_videoPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setMuted(!GirlfriendSettings::instance()->videoSoundEnabled());

    // Only change video if emotion changed
    if (m_currentVideoEmotion != emotion) {
        m_currentVideoEmotion = emotion;
        m_videoPlayer->setSource(QUrl::fromLocalFile(videoPath));
        m_videoPlayer->setLoops(QMediaPlayer::Infinite);  // Auto-loop
        m_videoPlayer->play();
        qDebug() << "AvatarWidget: Playing video for emotion:" << emotion
                 << "from" << videoPath
                 << "audio muted:" << m_audioOutput->isMuted();
    }
}

void AvatarWidget::stopVideo()
{
    m_videoPlayer->stop();
    m_videoWidget->hide();
    m_avatarLabel->show();
    m_currentVideoEmotion.clear();
}

void AvatarWidget::lockState()
{
    m_stateLocked = true;
    m_pendingEmotion.clear();
    qDebug() << "AvatarWidget: State locked";
}

void AvatarWidget::unlockState()
{
    m_stateLocked = false;
    qDebug() << "AvatarWidget: State unlocked";

    // 应用暂存的情绪
    if (!m_pendingEmotion.isEmpty()) {
        QString emotion = m_pendingEmotion;
        m_pendingEmotion.clear();
        // 直接设置，不再检查锁
        if (m_currentEmotion != emotion) {
            m_currentEmotion = emotion;
            updateDisplay();
            emit emotionChanged(emotion);
        }
        qDebug() << "AvatarWidget: Applied pending emotion:" << emotion;
    } else {
        // 没有暂存情绪时，触发当前情绪的信号（可能是 speaking 后恢复）
        emit emotionChanged(m_currentEmotion);
    }
}