/** @file SettingsStore.h @brief 设置持久化存储，基于 JSON 文件读写全局应用设置 */
#pragma once

#include <QString>

/** @brief 应用设置的 JSON 文件持久化存储，支持带默认值的加载和规范化处理 */
class SettingsStore {
public:
    /** @brief 可持久化的设置项 */
    struct Settings {
        bool autoStart = false;       // 开机自启动
        bool autoCheckUpdates = true; // 启动后自动检查更新
        bool showExitPrompt = true;   // 退出前端程序前显示提示
        bool hideServerNodes = false; // 运行状态页隐藏公共服务器节点
        int logLevel = 1;             // 日志级别 (0-3)
        int maxLogEntries = 100;      // 最大日志条数 (1-1000)
    };

    /** @brief 构造函数 @param filePath 设置文件路径，默认为 AppConfigLocation/settings3.json */
    explicit SettingsStore(QString filePath = defaultFilePath());

    /** @brief 从文件加载设置 @return 设置对象，文件不存在或解析失败时返回默认值 */
    Settings load() const;
    /** @brief 以指定默认值加载设置 @param defaults 默认设置 @return 设置对象 */
    Settings load(const Settings &defaults) const;
    /** @brief 保存设置到文件 @param settings 待保存的设置 @return 是否成功 */
    bool save(const Settings &settings) const;
    /** @brief 获取设置文件路径 @return 文件完整路径 */
    QString filePath() const;

    /** @brief 获取默认设置文件路径 @return AppConfigLocation/settings3.json */
    static QString defaultFilePath();
    /** @brief 规范化设置值，将越界字段重置为默认值 @param settings 原始设置 @return 规范化后的设置 */
    static Settings normalized(Settings settings);

private:
    QString m_filePath;
};
