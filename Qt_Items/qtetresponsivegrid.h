#ifndef QTETRESPONSIVEGRID_H
#define QTETRESPONSIVEGRID_H

#include <QWidget>
#include <QGridLayout>
#include <QList>

/// @brief 响应式网格布局控件
/// 根据可用宽度自动调整列数，基于子控件的实际 sizeHint() 计算所需宽度，
/// 避免硬编码阈值导致的溢出和迟滞问题。
class QtETResponsiveGrid : public QWidget
{
    Q_OBJECT

public:
    explicit QtETResponsiveGrid(QWidget *parent = nullptr);
    ~QtETResponsiveGrid() override;

    /// @brief 添加子控件到网格中
    void addItem(QWidget *widget);

    /// @brief 移除子控件
    void removeItem(QWidget *widget);

    /// @brief 清空所有子控件
    void clearItems();

    /// @brief 设置最小列数（默认 2）
    void setMinColumns(int columns);
    [[nodiscard]] int minColumns() const;

    /// @brief 设置最大列数（默认 5）
    void setMaxColumns(int columns);
    [[nodiscard]] int maxColumns() const;

    /// @brief 当前列数
    [[nodiscard]] int currentColumns() const;

    /// @brief 子控件数量
    [[nodiscard]] int itemCount() const;

    /// @brief 设置网格水平/垂直间距
    void setGridSpacing(int horizontal, int vertical);

    /// @brief 设置网格边距
    void setGridMargins(int left, int top, int right, int bottom);

    /// @brief 设置列数切换的安全系数（默认 1.0，即恰好容纳时才切换）
    /// 大于 1.0 的值会提前切换到更少列数，防止内容被挤压
    void setSafetyRatio(qreal ratio);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    /// @brief 重新计算并应用网格布局
    void recalculateLayout();

    /// @brief 计算给定列数下所需的最小宽度
    [[nodiscard]] int requiredWidthForColumns(int columns) const;

private:
    QGridLayout *m_gridLayout = nullptr;
    QList<QWidget *> m_items;

    int m_minColumns = 2;
    int m_maxColumns = 5;
    int m_currentColumns = 2;

    qreal m_safetyRatio = 1.0;

    static constexpr int DEFAULT_H_SPACING = 6;
    static constexpr int DEFAULT_V_SPACING = 6;
    static constexpr int DEFAULT_MARGIN_L = 15;
    static constexpr int DEFAULT_MARGIN_T = 5;
    static constexpr int DEFAULT_MARGIN_R = 15;
    static constexpr int DEFAULT_MARGIN_B = 5;
};

#endif // QTETRESPONSIVEGRID_H
