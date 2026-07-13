/** @file AutoStartService.h @brief 开机自启动服务，封装系统级自启动操作的业务接口 */
#pragma once

class AutoStartService {
public:
    /** @brief 设置开机自启动开关 @param enabled 是否启用 @return 操作是否成功 */
    bool setEnabled(bool enabled) const;
    /** @brief 查询当前自启动状态 @return 是否已启用 */
    bool isEnabled() const;
};
