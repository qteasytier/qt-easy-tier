/**
 * @file QmlSingletonRegistrar.cpp
 * @brief QML 单例注册实现
 *
 * 使用模板函数 registerPrecreatedSingleton 批量注册所有 C++ 对象为 QML 单例。
 * 每个注册对象均设置为 CppOwnership，确保 QML 引擎不接管其所有权。
 */
#include "QmlSingletonRegistrar.h"

#include "AppServices.h"
#include "core/util/FontHelper.h"
#include "core/viewmodel/AppState.h"
#include "core/viewmodel/ConfigEditorViewModel.h"
#include "core/viewmodel/ConfigListModel.h"
#include "core/viewmodel/FavoriteNodeViewModel.h"
#include "core/viewmodel/LogViewModel.h"
#include "core/viewmodel/SettingsViewModel.h"
#include "core/viewmodel/nodes/ImportNodesViewModel.h"
#include "core/viewmodel/runtime/BackendStatusViewModel.h"
#include "core/viewmodel/runtime/NetworkPageViewModel.h"
#include "core/vpn_manager/VpnManager.h"

#include <QJSEngine>
#include <QQmlApplicationEngine>

namespace {

/**
 * @brief 将已有 C++ 对象注册为 QML 单例
 * @tparam T  对象的静态类型
 * @param name   在 QML 中使用的单例名称（如 "AppState"）
 * @param object 已创建的 C++ 对象指针
 *
 * 先设置为 CppOwnership 防止 QML 双删，再通过 lambda 工厂注册为 QML singleton。
 */
template <typename T>
void registerPrecreatedSingleton(const char *name, T *object)
{
    // 设置 C++ 所有权，防止 QML 引擎在清理时 delete 该对象
    if (object)
        QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);

    // 注册为 QML 模块 "QtEasyTier" 下的单例类型，QML 中通过名称直接访问
    qmlRegisterSingletonType<T>("QtEasyTier", 1, 0, name,
        [object](QQmlEngine *, QJSEngine *) -> QObject * {
            return object;
        });
}

} // namespace

void registerQmlSingletons(QQmlApplicationEngine &, AppServices &services)
{
    // 应用全局状态（消息通知、页面切换）
    registerPrecreatedSingleton("AppState", services.appState());
    // 配置列表数据源
    registerPrecreatedSingleton("ConfigListModel", services.configListModel());
    // 配置编辑器
    registerPrecreatedSingleton("ConfigEditorViewModel", services.configEditorViewModel());
    // 网络页面综合视图（启停、节点、日志）
    registerPrecreatedSingleton("NetworkPageViewModel", services.networkPageViewModel());
    // 全局设置（日志级别、自启动等）
    registerPrecreatedSingleton("SettingsViewModel", services.settingsViewModel());
    // 收藏节点管理
    registerPrecreatedSingleton("FavoriteNodeViewModel", services.favoriteNodeViewModel());
    // 从公共服务器导入节点
    registerPrecreatedSingleton("ImportNodesViewModel", services.importNodesViewModel());
    // 日志查看
    registerPrecreatedSingleton("LogViewModel", services.logViewModel());
    // 后端 daemon 连接状态
    registerPrecreatedSingleton("BackendStatusViewModel", services.backendStatusViewModel());
    // VPN 启停管理
    registerPrecreatedSingleton("VpnManager", services.vpnManager());
    // 中文字体辅助
    registerPrecreatedSingleton("FontHelper", services.fontHelper());
}
