#ifndef QTETLABELLIST_H
#define QTETLABELLIST_H

#include <QWidget>
#include <QListWidgetItem>
#include <QPropertyAnimation>
#include <QIcon>
#include <QList>

/// @brief 自定义列表项数据类
/// 存储列表项的文本、图标等数据
class QtETLabelListItem
{
public:
    explicit QtETLabelListItem();
    explicit QtETLabelListItem(const QString &text);
    explicit QtETLabelListItem(const QIcon &icon, const QString &text);
    ~QtETLabelListItem() = default;

    /// @brief 获取文本
    [[nodiscard]] QString text() const { return m_text; }
    /// @brief 设置文本
    void setText(const QString &text) { m_text = text; }

    /// @brief 获取图标
    [[nodiscard]] QIcon icon() const { return m_icon; }
    /// @brief 设置图标
    void setIcon(const QIcon &icon) { m_icon = icon; }

    /// @brief 获取数据
    [[nodiscard]] QVariant data(int role) const;
    /// @brief 设置数据
    void setData(int role, const QVariant &value);

private:
    QString m_text;
    QIcon m_icon;
    QMap<int, QVariant> m_data;
};

/// @brief 标签列表控件
/// 完全自定义绘制的列表控件，不使用任何 QSS
/// 提供圆角高亮效果和渐显渐隐动画
class QtETLabelList : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverOpacity READ hoverOpacity WRITE setHoverOpacity)
    Q_PROPERTY(qreal selectionOpacity READ selectionOpacity WRITE setSelectionOpacity)

public:
    explicit QtETLabelList(QWidget *parent = nullptr);
    ~QtETLabelList() override;

    /// @brief 添加项目
    void addItem(QtETLabelListItem *item);
    /// @brief 添加文本项目
    void addItem(const QString &text);
    /// @brief 获取项目数量
    [[nodiscard]] int count() const;
    /// @brief 获取指定索引的项目
    [[nodiscard]] QtETLabelListItem* item(int index) const;
    /// @brief 获取当前选中项
    [[nodiscard]] QtETLabelListItem* currentItem() const;
    /// @brief 获取当前选中行
    [[nodiscard]] int currentRow() const;
    /// @brief 设置当前选中行
    void setCurrentRow(int row);
    /// @brief 获取项目的行号
    [[nodiscard]] int row(QtETLabelListItem *item) const;
    /// @brief 清除所有项目
    void clear();
    /// @brief 移除并返回指定行的项目
    QtETLabelListItem* takeItem(int row);
    /// @brief 获取指定行的矩形区域
    [[nodiscard]] QRect visualItemRect(QtETLabelListItem *item) const;
    /// @brief 根据位置获取项目（与 QListWidget 兼容）
    [[nodiscard]] QtETLabelListItem* itemAt(const QPoint &pos) const;

    /// @brief 设置指定索引项目的图标并触发重绘
    /// @param index 项目索引
    /// @param icon 要设置的图标
    void setItemIcon(int index, const QIcon &icon);

    /// @brief 获取悬停不透明度（用于动画）
    [[nodiscard]] qreal hoverOpacity() const;
    /// @brief 设置悬停不透明度（用于动画）
    void setHoverOpacity(qreal opacity);

    /// @brief 获取选中不透明度（用于动画）
    [[nodiscard]] qreal selectionOpacity() const;
    /// @brief 设置选中不透明度（用于动画）
    void setSelectionOpacity(qreal opacity);

    /// @brief 设置高亮颜色
    void setHighlightColor(const QColor &color);
    /// @brief 获取高亮颜色
    [[nodiscard]] QColor highlightColor() const;

signals:
    /// @brief 当前项改变信号（与 QListWidget 兼容）
    void currentRowChanged(int currentRow);
    /// @brief 项目点击信号
    void itemClicked(QtETLabelListItem *item);
    /// @brief 项目双击信号（与 QListWidget 兼容）
    void itemDoubleClicked(QtETLabelListItem *item);
    /// @brief 选择改变信号（与 QListWidget 兼容）
    void itemSelectionChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    /// @brief 初始化控件
    void init();
    /// @brief 更新颜色方案（根据深色/浅色模式）
    void updateColorScheme();
    /// @brief 计算指定行的矩形区域
    [[nodiscard]] QRect calculateItemRect(int row) const;
    /// @brief 根据位置获取行号
    [[nodiscard]] int getRowAtPosition(const QPoint &pos) const;
    /// @brief 更新内容高度
    void updateContentHeight();

private:
    QList<QtETLabelListItem*> m_items;      ///< 项目列表
    QPropertyAnimation *m_hoverAnimation;   ///< 悬停动画
    QPropertyAnimation *m_selectionAnimation; ///< 选中动画
    qreal m_hoverOpacity;                   ///< 悬停不透明度
    qreal m_selectionOpacity;               ///< 选中不透明度
    int m_hoveredRow;                       ///< 当前悬停行
    int m_selectedRow;                      ///< 当前选中行
    int m_scrollOffset;                     ///< 滚动偏移量
    QColor m_highlightColor;                ///< 高亮颜色
    QColor m_hoverFillColor;                ///< 悬停填充颜色
    QColor m_textColor;                     ///< 文字颜色
    QColor m_bgColor;                       ///< 背景颜色

    // 尺寸常量
    static constexpr int ANIMATION_DURATION = 200; ///< 动画时长(ms)
    static constexpr int BORDER_RADIUS = 6;        ///< 圆角半径
    static constexpr int ITEM_MARGIN = 4;          ///< 项目边距
    static constexpr int ITEM_HEIGHT = 36;         ///< 项目高度
    static constexpr int ICON_SIZE = 18;           ///< 图标大小
    static constexpr int TEXT_SIZE = 12;           ///< 文字大小
    static constexpr int ICON_TEXT_SPACING = 8;    ///< 图标与文字间距
};

#endif // QTETLABELLIST_H
