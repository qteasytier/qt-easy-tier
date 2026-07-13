/**
 * @file FontHelper.h
 * @brief 系统字体辅助类
 *
 * 为 QML 层提供对 Qt 系统字体设置的统一访问入口。
 * 设计目的：避免在 QML 中硬编码字体名称和大小，确保应用字体与系统桌面环境保持一致。
 *
 * 使用方式：
 *   - C++ 侧：由 AppServices 显式创建
 *   - QML 侧：直接使用 FontHelper 单例（通过 qmlRegisterSingletonType 注册）
 *
 * 单例所有权：
 *   必须以 QQmlApplicationEngine 为父对象创建，避免 QGuiApplication 析构时
 *   对已销毁的 engine 子对象调用 delete 导致 double free。
 */
#pragma once

#include <QFont>
#include <QObject>

// 前向声明，避免引入完整的 QML 引擎头文件
class QQmlEngine;
class QJSEngine;

class FontHelper : public QObject {
    Q_OBJECT

    /**
     * @brief 系统设置中的小号字体
     *
     * 对应 QFontDatabase::SmallestReadableFont 的返回值。
     * 在 KDE 等桌面环境中通常对应系统设置的"小号字"。
     * 声明为 CONSTANT FINAL，表示字体在运行时不会变化。
     */
    Q_PROPERTY(QFont smallFont READ smallFont CONSTANT FINAL)

public:
    explicit FontHelper(QObject *parent = nullptr);

    /**
     * @brief 获取系统小号字体
     * @return 从 QFontDatabase::systemFont() 获取的最小可读字体
     *
     * 用于 QML 中需要比默认字体更小的显示场景（如辅助信息、提示文字）。
     */
    QFont smallFont() const;

};
