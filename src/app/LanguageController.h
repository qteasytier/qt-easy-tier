/**
 * @file LanguageController.h
 * @brief 界面语言控制器：解析语言设置、装卸 QTranslator、驱动 QML 即时重翻译
 *
 * 语言取值（与 settings3.json 的 language 键一致）：
 * - system：跟随系统（zh_TW/zh_HK/zh_MO→繁體，English 系→English，其余含 zh_CN→简体源）
 * - zh_CN：简体中文（源语言，不安装翻译器）
 * - zh_TW：繁體中文（加载 :/i18n/QtEasyTier_zh_TW.qm）
 * - en：English（加载 :/i18n/QtEasyTier_en.qm）
 *
 * 语言写入统一经 SettingsViewModel（持久化唯一路径），本控制器监听其
 * languageChanged 信号装卸翻译器并调用 QQmlEngine::retranslate() 让全部
 * qsTr 绑定即时重求值；同时发 retranslated() 通知 C++ 侧缓存的元数据
 * （如 ConfigEditorViewModel::formSections）重建。
 */
#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

class QQmlApplicationEngine;
class QTranslator;
class SettingsViewModel;

/** @brief 界面语言控制器，装配层组件（由 AppServices 持有并注册为 QML 单例） */
class LanguageController : public QObject {
    Q_OBJECT

    /// 当前语言设置（system/zh_CN/zh_TW/en），与 SettingsViewModel.language 同步
    Q_PROPERTY(QString language READ language NOTIFY languageChanged FINAL)

    /// 可选语言列表 [{value, label}]，label 随当前翻译变化，语言切换后重发
    Q_PROPERTY(QVariantList availableLanguages READ availableLanguages NOTIFY retranslated FINAL)

public:
    /**
     * @brief 构造语言控制器
     * @param engine           QML 引擎（retranslate 用，可为空——测试场景）
     * @param settingsViewModel 设置 ViewModel（语言持久化源，非所有权）
     * @param parent           父对象
     */
    explicit LanguageController(QQmlApplicationEngine *engine,
                                SettingsViewModel *settingsViewModel,
                                QObject *parent = nullptr);

    QString language() const;
    QVariantList availableLanguages() const;

    /// 切换语言：委托 SettingsViewModel（规范化+持久化），经回执信号生效
    Q_INVOKABLE void setLanguage(const QString &value);

signals:
    void languageChanged();
    /// 翻译已装卸完成：C++ 缓存元数据应重建，QML 侧由 engine->retranslate() 刷新
    void retranslated();

private:
    /// 解析语言设置为目标 locale（zh_CN/zh_TW/en）；system 按 QLocale::system() 推导
    QString resolvedLocale(const QString &language) const;
    /// 装卸翻译器并触发 QML 重翻译
    void applyLanguage(const QString &language);

    QQmlApplicationEngine *m_engine = nullptr;
    SettingsViewModel *m_settingsViewModel = nullptr;
    QTranslator *m_translator = nullptr;
    QString m_language;
};
