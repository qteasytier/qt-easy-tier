#ifndef QTETLISTWIDGET_H
#define QTETLISTWIDGET_H

#include <QListWidget>
#include <QPropertyAnimation>
#include <QStyledItemDelegate>

class QtETListWidget;

/// @brief 自定义列表项，重命名自 QListWidgetItem
/// 提供与 QListWidgetItem 兼容的接口
class QtETListWidgetItem : public QListWidgetItem
{
public:
    explicit QtETListWidgetItem(QListWidget *parent = nullptr, int type = Type);
    explicit QtETListWidgetItem(const QString &text, QListWidget *parent = nullptr, int type = Type);
    explicit QtETListWidgetItem(const QIcon &icon, const QString &text, QListWidget *parent = nullptr, int type = Type);
    explicit QtETListWidgetItem(const QtETListWidgetItem &other);
    ~QtETListWidgetItem() override = default;

    QtETListWidgetItem &operator=(const QtETListWidgetItem &other);
};

/// @brief 自定义列表视图委托
/// 负责绘制列表项的高亮效果和动画
class QtETListWidgetDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit QtETListWidgetDelegate(QObject *parent = nullptr);
    ~QtETListWidgetDelegate() override = default;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    /// @brief 设置高亮颜色
    void setHighlightColor(const QColor &color);
    /// @brief 获取高亮颜色
    [[nodiscard]] QColor highlightColor() const;

private:
    QColor m_highlightColor;    ///< 高亮颜色 (#66ccff)
    QColor m_hoverFillColor;    ///< 悬停时填充颜色 (更淡的高亮色)

    static constexpr int BORDER_RADIUS = 6;     ///< 圆角半径
    static constexpr int ITEM_MARGIN = 4;       ///< 项目边距
    static constexpr int ICON_SIZE = 18;        ///< 图标大小
    static constexpr int TEXT_SIZE = 12;        ///< 文字大小
    static constexpr int ICON_TEXT_SPACING = 8; ///< 图标与文字间距
};

/// @brief 自定义列表控件
/// 基于 QListWidget，提供圆角高亮效果和渐显渐隐动画
class QtETListWidget : public QListWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverOpacity READ hoverOpacity WRITE setHoverOpacity)
    Q_PROPERTY(qreal selectionOpacity READ selectionOpacity WRITE setSelectionOpacity)

public:
    explicit QtETListWidget(QWidget *parent = nullptr);
    ~QtETListWidget() override;

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

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &deselected) override;

private:
    /// @brief 初始化控件
    void init();
    /// @brief 更新颜色方案（根据深色/浅色模式）
    void updateColorScheme();
    /// @brief 启动悬停动画
    void startHoverAnimation(bool visible);
    /// @brief 启动选中动画
    void startSelectionAnimation(bool visible);
    /// @brief 更新当前悬停项
    void updateHoverItem(const QPoint &pos);

private:
    QtETListWidgetDelegate *m_delegate;     ///< 自定义委托
    QPropertyAnimation *m_hoverAnimation;   ///< 悬停动画
    QPropertyAnimation *m_selectionAnimation; ///< 选中动画
    qreal m_hoverOpacity;                   ///< 悬停不透明度
    qreal m_selectionOpacity;               ///< 选中不透明度
    int m_hoverRow;                         ///< 当前悬停行
    int m_selectedRow;                      ///< 当前选中行
    QColor m_highlightColor;                ///< 高亮颜色

    static constexpr int ANIMATION_DURATION = 200; ///< 动画时长(ms)
};

#endif // QTETLISTWIDGET_H
