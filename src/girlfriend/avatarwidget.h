#ifndef AVATARWIDGET_H
#define AVATARWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QMap>
#include <QString>
#include <QResizeEvent>

class AvatarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AvatarWidget(QWidget *parent = nullptr);

    void setEmotion(const QString &emotion);
    void setSpeaking(bool speaking);
    QString currentEmotion() const { return m_currentEmotion; }

signals:
    void emotionChanged(const QString &emotion);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void loadAvatarImages();
    void updateDisplay();
    QString getAvatarPath(const QString &emotion) const;

    QLabel *m_avatarLabel;
    QLabel *m_emotionTagLabel;
    QMap<QString, QPixmap> m_avatarImages;

    QString m_currentEmotion;
    bool m_isSpeaking;
};

#endif // AVATARWIDGET_H