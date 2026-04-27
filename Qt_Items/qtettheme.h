#ifndef QTETTHEME_H
#define QTETTHEME_H

#include <QColor>
#include <QObject>

/// @brief 主题色彩管理工具类
/// 集中管理深浅色模式下的所有配色，避免硬编码散布在各地
class QtETTheme : public QObject
{
    Q_OBJECT

public:
    /// @brief 获取全局单例
    static QtETTheme *instance();

    /// @brief 主题色（高亮蓝）
    static inline const QColor AccentColor = QColor(0x66, 0xcc, 0xff);
    static inline const QColor AccentColorDark = QColor(0x33, 0x99, 0xdd);

    /// @brief 连接类型颜色
    static inline const QColor ConnDirect = QColor(0x4C, 0xAF, 0x50);
    static inline const QColor ConnRelay = QColor(0xFF, 0x98, 0x00);
    static inline const QColor ConnServer = QColor(0x66, 0xcc, 0xff);
    static inline const QColor ConnLocal = QColor(0x9C, 0x27, 0xB0);

    /// @brief 当前是否为深色模式
    [[nodiscard]] bool isDarkMode() const;

    // === 背景色 ===
    [[nodiscard]] QColor backgroundColor() const;
    [[nodiscard]] QColor widgetBackgroundColor() const;
    [[nodiscard]] QColor selectedBackgroundColor() const;

    // === 边框色 ===
    [[nodiscard]] QColor normalBorderColor() const;
    [[nodiscard]] QColor highlightBorderColor() const;
    [[nodiscard]] QColor pressedBorderColor() const;
    [[nodiscard]] QColor focusBorderColor() const;

    // === 文字色 ===
    [[nodiscard]] QColor textColor() const;
    [[nodiscard]] QColor textInactiveColor() const;
    [[nodiscard]] QColor selectedTextColor() const; ///< 选中项文字色（自动适配深浅）

    // === 开关/按钮 ===
    [[nodiscard]] QColor switchActiveColor() const;
    [[nodiscard]] QColor switchInactiveColor() const;
    [[nodiscard]] QColor switchSliderColor() const;

    // === 悬停 ===
    [[nodiscard]] QColor hoverBackgroundColor() const;
    [[nodiscard]] QColor hoverFillColor(const QColor &base = AccentColor) const;

private:
    explicit QtETTheme(QObject *parent = nullptr);
};

#endif // QTETTHEME_H
