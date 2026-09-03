/** @file LanguageController.cpp @brief LanguageController 实现 */
#include "LanguageController.h"

#include "core/viewmodels/SettingsViewModel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QTranslator>

LanguageController::LanguageController(QQmlApplicationEngine *engine,
                                       SettingsViewModel *settingsViewModel,
                                       QObject *parent)
    : QObject(parent)
    , m_engine(engine)
    , m_settingsViewModel(settingsViewModel)
{
    // 构造时按持久化的语言安装翻译器（发生在 engine.load 之前，首帧即正确语言）
    applyLanguage(m_settingsViewModel ? m_settingsViewModel->language()
                                      : QStringLiteral("system"));

    // 语言写入唯一路径在 SettingsViewModel，这里监听变化并生效
    if (m_settingsViewModel) {
        connect(m_settingsViewModel, &SettingsViewModel::languageChanged, this, [this]() {
            applyLanguage(m_settingsViewModel->language());
        });
    }
}

QString LanguageController::language() const
{
    return m_language;
}

QVariantList LanguageController::availableLanguages() const
{
    // label 使用 tr()：随当前安装的翻译呈现对应语言名称
    return QVariantList{
        QVariantMap{{"value", QStringLiteral("system")}, {"label", tr("跟随系统")}},
        QVariantMap{{"value", QStringLiteral("zh_CN")}, {"label", tr("简体中文")}},
        QVariantMap{{"value", QStringLiteral("zh_TW")}, {"label", tr("繁體中文")}},
        QVariantMap{{"value", QStringLiteral("en")}, {"label", tr("English")}},
    };
}

void LanguageController::setLanguage(const QString &value)
{
    if (m_settingsViewModel)
        m_settingsViewModel->setLanguage(value);
}

QString LanguageController::resolvedLocale(const QString &language) const
{
    if (language == QLatin1String("zh_TW") || language == QLatin1String("en"))
        return language;

    // system 与 zh_CN 统一按系统 locale 推导：zh_CN 显式选择时视为简体源
    if (language != QLatin1String("system") && language != QLatin1String("zh_CN"))
        return QStringLiteral("zh_CN");

    const QLocale locale = QLocale::system();
    if (locale.language() == QLocale::Chinese) {
        const auto territory = locale.territory();
        if (territory == QLocale::Taiwan || territory == QLocale::HongKong
                || territory == QLocale::Macau) {
            return QStringLiteral("zh_TW");
        }
        return QStringLiteral("zh_CN");
    }
    if (locale.language() == QLocale::English)
        return QStringLiteral("en");
    return QStringLiteral("zh_CN");
}

void LanguageController::applyLanguage(const QString &language)
{
    const QString target = resolvedLocale(language);

    // 卸载旧翻译器；简体源语言不安装任何翻译器
    if (m_translator) {
        QCoreApplication::removeTranslator(m_translator);
        m_translator->deleteLater();
        m_translator = nullptr;
    }
    if (target != QLatin1String("zh_CN")) {
        auto *translator = new QTranslator(this);
        // 资源路径与 qt_add_translations（Qt 6.8）生成的 qrc 对齐：
        // qrc alias 为裸文件名，实际资源为 :/i18n/QtEasyTier_<lang>.qm
        if (translator->load(QStringLiteral(":/i18n/QtEasyTier_%1.qm").arg(target))) {
            QCoreApplication::installTranslator(translator);
            m_translator = translator;
        } else {
            // qm 缺失时回退源语言，不阻塞启动；但失败必须可见，避免静默回归
            qWarning() << "LanguageController: 翻译资源加载失败，回退源语言"
                       << QStringLiteral(":/i18n/QtEasyTier_%1.qm").arg(target);
            translator->deleteLater();
        }
    }

    m_language = language;
    emit languageChanged();

    // QML 绑定即时重求值；C++ 缓存元数据（formSections 等）监听 retranslated 重建
    if (m_engine)
        m_engine->retranslate();
    emit retranslated();
}
