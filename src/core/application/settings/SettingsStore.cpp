/** @file SettingsStore.cpp @brief SettingsStore 实现 */
#include "SettingsStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <utility>

SettingsStore::SettingsStore(QString filePath)
    : m_filePath(std::move(filePath))
{
}

SettingsStore::Settings SettingsStore::load() const
{
    return load(Settings());
}

SettingsStore::Settings SettingsStore::load(const Settings &defaults) const
{
    // 尝试打开 JSON 文件，失败则返回默认值
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly))
        return defaults;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return defaults;

    // 逐字段读取，缺失项使用默认值填充
    const QJsonObject obj = doc.object();
    Settings settings = defaults;
    settings.autoStart = obj.value(QLatin1String("autoStart")).toBool(settings.autoStart);
    settings.showExitPrompt = obj.value(QLatin1String("showExitPrompt")).toBool(settings.showExitPrompt);
    settings.hideServerNodes = obj.value(QLatin1String("hideServerNodes")).toBool(settings.hideServerNodes);
    settings.logLevel = obj.value(QLatin1String("logLevel")).toInt(settings.logLevel);
    settings.maxLogEntries = obj.value(QLatin1String("maxLogEntries")).toInt(settings.maxLogEntries);
    // 对加载的值做规范化（限制范围）
    return normalized(settings);
}

bool SettingsStore::save(const Settings &settings) const
{
    // 先规范化再序列化，确保写入的数据在合法范围内
    const Settings normalizedSettings = normalized(settings);
    QJsonObject obj;
    obj[QStringLiteral("autoStart")] = normalizedSettings.autoStart;
    obj[QStringLiteral("showExitPrompt")] = normalizedSettings.showExitPrompt;
    obj[QStringLiteral("hideServerNodes")] = normalizedSettings.hideServerNodes;
    obj[QStringLiteral("logLevel")] = normalizedSettings.logLevel;
    obj[QStringLiteral("maxLogEntries")] = normalizedSettings.maxLogEntries;

    // 确保目标目录存在
    QDir dir(QFileInfo(m_filePath).path());
    if (!dir.exists() && !dir.mkpath(dir.path()))
        return false;

    // 以 Compact 格式写入 JSON
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    return file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact)) != -1;
}

QString SettingsStore::filePath() const
{
    return m_filePath;
}

QString SettingsStore::defaultFilePath()
{
    // Linux 下通常在 ~/.config/qteasytier/QtEasyTier/settings3.json
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QDir(dir).filePath(QStringLiteral("settings3.json"));
}

SettingsStore::Settings SettingsStore::normalized(Settings settings)
{
    // logLevel 限制在 0-3，越界重置为 1
    if (settings.logLevel < 0 || settings.logLevel > 3)
        settings.logLevel = 1;
    // maxLogEntries 限制在 1-1000，越界重置为边界值
    if (settings.maxLogEntries < 1)
        settings.maxLogEntries = 1;
    if (settings.maxLogEntries > 1000)
        settings.maxLogEntries = 1000;
    return settings;
}
