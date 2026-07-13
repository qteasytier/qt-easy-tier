/**
 * @file QmlSingletonRegistrar.h
 * @brief 集中注册所有 C++ 对象为 QML 单例类型
 *
 * 将所有通过 AppServices 创建的服务/ViewModel 对象，
 * 统一通过 qmlRegisterSingletonType 注册到 QML 模块 "QtEasyTier" 下。
 * C++ 侧保持对象所有权（CppOwnership），QML 侧通过 import 直接访问。
 *
 * @see AppServices
 */
#pragma once

class AppServices;
class QQmlApplicationEngine;

/**
 * @brief 注册所有 C++ 服务对象为 QML 单例
 * @param engine   QQmlApplicationEngine 引用
 * @param services AppServices 实例（提供所有要注册的服务对象）
 */
void registerQmlSingletons(QQmlApplicationEngine &engine, AppServices &services);
