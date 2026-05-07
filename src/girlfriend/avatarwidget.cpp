#include "avatarwidget.h"
#include "girlfriend_translations.h"
#include "girlfriendsettings.h"
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QRandomGenerator>

AvatarWidget::AvatarWidget(QWidget *parent)
    : QWidget(parent)
    , m_avatarLabel(new QLabel(this))
    , m_videoPlayer(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_graphicsView(new QGraphicsView(this))
    , m_graphicsScene(new QGraphicsScene(this))
    , m_videoItem(new QGraphicsVideoItem())
    , m_emotionTagLabel(new QLabel(this))
    , m_moodBarWidget(new QLabel(this))
    , m_moodPercentLabel(new QLabel(this))
    , m_currentEmotion("default")
    , m_isSpeaking(false)
    , m_currentLevel(GirlfriendSettings::instance()->avatarLevel())
    , m_idleTimer(new QTimer(this))
    , m_idleCycleTimer(new QTimer(this))
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);

    // GraphicsView setup for video (Level 3)
    m_graphicsScene->addItem(m_videoItem);
    m_graphicsView->setScene(m_graphicsScene);
    m_graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_graphicsView->setStyleSheet("background: transparent; border: none;");
    m_graphicsView->setFrameShape(QFrame::NoFrame);
    m_graphicsView->hide();

    // Video player setup
    m_videoPlayer->setAudioOutput(m_audioOutput);
    m_videoPlayer->setVideoOutput(m_videoItem);
    m_audioOutput->setMuted(!GirlfriendSettings::instance()->videoSoundEnabled());

    loadAvatarImages();

    connect(GirlfriendSettings::instance(), &GirlfriendSettings::avatarLevelChanged,
            this, &AvatarWidget::setAvatarLevel);

    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setScaledContents(true);

    // Emotion tag label
    m_emotionTagLabel->setStyleSheet(
        "QLabel { background: rgba(233, 30, 99, 0.85); color: white; "
        "padding: 4px 12px; font-size: 12px; border-radius: 6px; }"
    );
    m_emotionTagLabel->setText(GTr::emotionDefault());
    m_emotionTagLabel->move(12, 12);
    m_emotionTagLabel->raise();

    // Mood bar
    m_moodBarWidget->setStyleSheet("QLabel { background: transparent; }");
    m_moodBarWidget->setFixedHeight(6);
    m_moodBarWidget->setFixedWidth(50);

    // Mood percentage
    m_moodPercentLabel->setStyleSheet(
        "QLabel { background: transparent; font-size: 10px; color: white; }"
    );

    // Video sound setting
    connect(GirlfriendSettings::instance(), &GirlfriendSettings::videoSoundChanged,
            this, [this](bool enabled) {
        m_audioOutput->setMuted(!enabled);
    });

    // Stacking: graphicsView (video) bottom, avatarLabel above, labels on top
    m_graphicsView->stackUnder(m_avatarLabel);
    m_avatarLabel->stackUnder(m_emotionTagLabel);
    m_emotionTagLabel->stackUnder(m_moodBarWidget);
    m_moodBarWidget->stackUnder(m_moodPercentLabel);

    // Idle timer: after 30s of no emotion change, begin cycling
    m_idleTimer->setInterval(30000);
    m_idleTimer->setSingleShot(true);
    connect(m_idleTimer, &QTimer::timeout, this, [this]() {
        m_idleCycling = true;
        m_idleCycleTimer->start(5000);
        onIdleCycle();
    });

    // Idle cycle timer: switch emotions every 5s while idle
    m_idleCycleTimer->setInterval(5000);
    connect(m_idleCycleTimer, &QTimer::timeout, this, &AvatarWidget::onIdleCycle);

    updateMoodDisplay();
    updateDisplay();
}

void AvatarWidget::resetIdleTimer()
{
    m_idleTimer->start(30000);
    if (m_idleCycling) {
        m_idleCycling = false;
        m_idleCycleTimer->stop();
    }
}

void AvatarWidget::onIdleCycle()
{
    if (!m_idleCycling || m_isSpeaking || m_stateLocked) return;

    static const QStringList idlePool = {"default", "studying", "awaiting"};
    static thread_local QString s_lastIdle;
    QString pick;
    do {
        pick = idlePool[QRandomGenerator::global()->bounded(idlePool.size())];
    } while (pick == s_lastIdle && idlePool.size() > 1);
    s_lastIdle = pick;

    setEmotion(pick, true);
}

void AvatarWidget::loadAvatarImages()
{
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

    m_currentLevel = GirlfriendSettings::instance()->avatarLevel();

    m_avatarImages.clear();

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

    if (m_currentLevel == AvatarLevel::Level1_Belle) {
        fileNames["default"] = "picture-original.png";
    }

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

void AvatarWidget::setEmotion(const QString &emotion, bool forceUpdate)
{
    if (m_stateLocked) {
        m_pendingEmotion = emotion;
        qDebug() << "AvatarWidget: State locked, pending emotion:" << emotion;
        return;
    }

    if (forceUpdate || m_currentEmotion != emotion) {
        m_currentEmotion = emotion;
        updateDisplay();
        emit emotionChanged(emotion);
    }

    // Reset idle timer on any setEmotion call (except idle cycling itself)
    if (!m_idleCycling) {
        m_idleTimer->start(30000);
    }
}

QString AvatarWidget::currentDisplayEmotion() const
{
    return m_isSpeaking ? "speaking" : m_currentEmotion;
}

void AvatarWidget::setSpeaking(bool speaking)
{
    m_isSpeaking = speaking;
    updateDisplay();

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
        stopVideo();

        m_currentLevel = level;
        loadAvatarImages();

        if (level == AvatarLevel::Level3_Hotter) {
            m_videoPlayer->setAudioOutput(m_audioOutput);
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

    // 5-segment gradient from dim to bright (higher mood = brighter)
    QString barColor;
    if (m_currentMood > 0.8) {
        barColor = "linear-gradient(90deg, #ff4081, #ff80ab)";  // bright pink-gold
    } else if (m_currentMood > 0.6) {
        barColor = "linear-gradient(90deg, #e91e63, #f06292)";  // warm pink
    } else if (m_currentMood > 0.4) {
        barColor = "#95a5a6";  // neutral gray
    } else if (m_currentMood > 0.2) {
        barColor = "#7f8c8d";  // dim gray
    } else {
        barColor = "#5a5a5a";  // very dim
    }

    QString barHtml = QString(
        "<div style='background: #444; border-radius: 3px; width: 50px; height: 6px;'>"
        "<div style='background: %1; border-radius: 3px; width: %2px; height: 6px;'>"
        "</div></div>"
    ).arg(barColor).arg(static_cast<int>(m_currentMood * 50));

    m_moodBarWidget->setText(barHtml);
    m_moodBarWidget->setTextFormat(Qt::RichText);

    m_moodPercentLabel->setText(QString("%1%").arg(percent));
    m_moodPercentLabel->adjustSize();

    m_emotionTagLabel->adjustSize();

    int emotionLabelHeight = m_emotionTagLabel->sizeHint().height();
    m_moodBarWidget->move(12, 12 + emotionLabelHeight + 4);
    m_moodBarWidget->raise();

    m_moodPercentLabel->move(12 + 54, 12 + emotionLabelHeight + 2);
    m_moodPercentLabel->raise();
}

void AvatarWidget::updateDisplay()
{
    QString displayEmotion = m_isSpeaking ? "speaking" : m_currentEmotion;

    if (m_currentLevel == AvatarLevel::Level3_Hotter) {
        m_avatarLabel->hide();
        m_graphicsView->show();
        m_graphicsView->setGeometry(0, 0, width(), height());
        playVideo(displayEmotion);

        m_emotionTagLabel->raise();
        m_moodBarWidget->raise();
        m_moodPercentLabel->raise();
    } else {
        stopVideo();
        m_avatarLabel->show();

        if (m_avatarImages.contains(displayEmotion)) {
            QPixmap pixmap = m_avatarImages[displayEmotion];
            QSize widgetSize = this->size();

            Qt::AspectRatioMode aspectMode = Qt::KeepAspectRatioByExpanding;

            QPixmap scaled = pixmap.scaled(
                widgetSize,
                aspectMode,
                Qt::SmoothTransformation
            );

            if (aspectMode == Qt::KeepAspectRatioByExpanding &&
                (scaled.width() > widgetSize.width() || scaled.height() > widgetSize.height())) {
                int x = (scaled.width() - widgetSize.width()) / 2;
                int y = (scaled.height() - widgetSize.height()) / 2;
                scaled = scaled.copy(x, y, widgetSize.width(), widgetSize.height());
            }

            m_avatarLabel->setPixmap(scaled);
            m_avatarLabel->resize(widgetSize);
            m_avatarLabel->move(0, 0);

            if (scaled.width() < widgetSize.width() || scaled.height() < widgetSize.height()) {
                int x = (widgetSize.width() - scaled.width()) / 2;
                int y = (widgetSize.height() - scaled.height()) / 2;
                m_avatarLabel->move(x, y);
                m_avatarLabel->resize(scaled.size());
            }
        }
    }

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
    m_emotionTagLabel->raise();

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

    int emotionLabelHeight = m_emotionTagLabel->sizeHint().height();
    m_moodBarWidget->move(12, 12 + emotionLabelHeight + 4);
    m_moodBarWidget->raise();
    m_moodPercentLabel->move(12 + 54, 12 + emotionLabelHeight + 2);
    m_moodPercentLabel->raise();

    m_emotionTagLabel->raise();

    if (m_graphicsView->isVisible()) {
        m_graphicsView->setGeometry(0, 0, width(), height());
        m_videoItem->setSize(QSizeF(width(), height()));
    }
}

void AvatarWidget::retranslateUi()
{
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

    m_videoPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setMuted(!GirlfriendSettings::instance()->videoSoundEnabled());

    if (m_currentVideoEmotion != emotion) {
        m_currentVideoEmotion = emotion;
        m_videoPlayer->setSource(QUrl::fromLocalFile(videoPath));
        m_videoPlayer->setLoops(QMediaPlayer::Infinite);
        m_videoPlayer->play();
        qDebug() << "AvatarWidget: Playing video for emotion:" << emotion
                 << "from" << videoPath
                 << "audio muted:" << m_audioOutput->isMuted();
    }
    // Always update video size to match current widget dimensions
    m_videoItem->setSize(QSizeF(width(), height()));
}

void AvatarWidget::stopVideo()
{
    m_videoPlayer->stop();
    m_graphicsView->hide();
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
    if (!m_pendingEmotion.isEmpty()) {
        QString pending = m_pendingEmotion;
        m_pendingEmotion.clear();
        setEmotion(pending);
        qDebug() << "AvatarWidget: State unlocked, applied pending emotion:" << pending;
    } else {
        qDebug() << "AvatarWidget: State unlocked, no pending emotion";
    }
}
