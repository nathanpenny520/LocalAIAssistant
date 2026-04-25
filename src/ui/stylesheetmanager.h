#ifndef STYLESHEETMANAGER_H
#define STYLESHEETMANAGER_H

#include <QObject>
#include <QString>

class StyleSheetManager : public QObject
{
    Q_OBJECT

public:
    enum Theme {
        SystemTheme = 0,
        LightTheme,
        DarkTheme
    };
    Q_ENUM(Theme)

    static StyleSheetManager* instance();

    QString currentStyleSheet() const;
    Theme currentTheme() const;
    void setTheme(Theme theme);
    void applyTheme(class QWidget *rootWidget);

    static QString lightStyleSheet();
    static QString darkStyleSheet();

signals:
    void themeChanged(Theme theme);

private:
    explicit StyleSheetManager(QObject *parent = nullptr);
    Theme m_currentTheme;
    QString m_styleSheet;

    void loadThemeFromSettings();
    void saveThemeToSettings();
    QString detectSystemTheme() const;
};

#endif
