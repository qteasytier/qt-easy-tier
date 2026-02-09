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
#include <QPair>

/**
 * @brief: 生成EasyTier配置参数
 * @param netPage: 网络设置页面指针
 * @return: EasyTier配置参数字符串
 */
QStringList generateConfCommand(NetPage *netPage);

/**
 * @brief: Base32编码函数
 * @param data: 要编码的数据
 * @return: Base32编码后的字符串
 */
QString base32Encode(const QByteArray& data);

/**
 * @brief: Base32解码函数
 * @param encoded: Base32编码的字符串
 * @return: 解码后的数据
 */
QByteArray base32Decode(const QString& encoded);

/**
 * @brief: 生成房间号和密码
 * @return: QPair<房间号, 密码>
 */
QPair<QString, QString> generateRoomCredentials();

/**
 * @brief: 编码房间信息为联机码
 * @param networkId: 网络号
 * @param password: 密码
 * @return: 格式化的联机码 XXXX-XXXX-XXXX-XXXX
 */
QString encodeConnectionCode(const QString& networkId, const QString& password);

/**
 * @brief: 解码联机码获取房间信息
 * @param code: 联机码
 * @return: QPair<网络号, 密码>，如果解码失败返回空pair
 */
QPair<QString, QString> decodeConnectionCode(const QString& code);

#endif //QTEASYTIER_GENERATECONF_H