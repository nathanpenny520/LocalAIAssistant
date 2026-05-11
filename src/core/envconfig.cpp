#include "envconfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

QString EnvConfig::findEnvFile() {
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList searchPaths;
    searchPaths << QDir::cleanPath(appDir + "/.env");
    searchPaths << QDir::cleanPath(appDir + "/../Resources/.env");
    searchPaths << QDir::currentPath() + "/.env";
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!dataDir.isEmpty()) {
        searchPaths << QDir::cleanPath(dataDir + "/.env");
    }

    for (const QString& path : searchPaths) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    return QString();
}

QMap<QString, QString> EnvConfig::parseEnvFile(const QString& path) {
    QMap<QString, QString> result;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return result;
    }

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        int eqPos = line.indexOf('=');
        if (eqPos <= 0) {
            continue;
        }

        QString key = line.left(eqPos).trimmed();
        QString value = line.mid(eqPos + 1).trimmed();

        if ((value.startsWith('"') && value.endsWith('"'))
            || (value.startsWith('\'') && value.endsWith('\''))) {
            value = value.mid(1, value.length() - 2);
        }

        result[key] = value;
    }
    return result;
}
