/*
 * @author: YMHuang
 * @date: 2026-01-27 15:20:00
 *
 * @brief: 生成EasyTier配置文件
 * @note: 目前的实现是生成Easytier的配置参数，在运行程序时添加到命令行参数中
 *        后续可能考虑实现直接生成配置文件的功能并支持导入配置文件
 */

#ifndef QTEASYTIER_GENERATECONF_H
#define QTEASYTIER_GENERATECONF_H

#include "netpage.h"
#include <QString>
/**
 * @brief: 生成EasyTier配置参数
 * @param netPage: 网络设置页面指针
 * @return: EasyTier配置参数字符串
 */
QStringList generateConfCommand(NetPage *netPage);


#endif //QTEASYTIER_GENERATECONF_H