/**
 * @file envconfig.h
 * @brief Shared utility for locating and parsing .env files.
 */
#pragma once

#ifndef ENVCONFIG_H
#define ENVCONFIG_H

#include <QMap>
#include <QString>

class EnvConfig {
public:
    /// Find the first existing .env file across standard locations.
    /// Search order (first match wins):
    ///   1. <appDir>/.env
    ///   2. <appDir>/../Resources/.env      (macOS bundle)
    ///   3. <currentWorkingDir>/.env         (development)
    ///   4. <AppDataLocation>/.env           (user data)
    static QString findEnvFile();

    /// Parse a .env file into a key-value map.
    /// Lines starting with # are comments. Empty lines are ignored.
    /// Quoted values have outer quotes stripped.
    static QMap<QString, QString> parseEnvFile(const QString& path);

private:
    EnvConfig() = delete;
};

#endif // ENVCONFIG_H
