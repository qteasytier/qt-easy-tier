#ifndef QTETLISTWIDGET_H
#define QTETLISTWIDGET_H

#include <QListWidget>
#include <QPropertyAnimation>
#include <QStyledItemDelegate>
#include <QScrollBar>
#include <QTimer>

class QtETListWidget;

/// @brief 自定义滚动条
/// 提供圆角样式、半透明效果和平滑滚动
class QtETScrollBar : public QScrollBar
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit QtETScrollBar(Qt::Orientation orientation, QWidget *parent = nullptr);
    ~QtETScrollBar() override = default;

    /// @brief 设置滚动条宽度
    void setScrollBarWidth(int width);
    /// @brief 获取滚动条宽度
    [[nodiscard]] int scrollBarWidth() const;

    /// @brief 设置显示/隐藏动画时长
    void setFadeDuration(int duration);
    /// @brief 获取显示/隐藏动画时长
    [[nodiscard]] int fadeDuration() const;

    /// @brief 设置透明度
    void setOpacity(qreal opacity);
    /// @brief 获取透明度
    [[nodiscard]] qreal opacity() const;

    /// @brief 开始显示动画
    void startShowAnimation();
    /// @brief 开始隐藏动画
    void startHideAnimation();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void sliderChange(SliderChange change) override;

private:
    /// @brief 更新颜色方案
    void updateColorScheme();

private:
    int m_scrollBarWidth;           ///< 滚动条宽度
    int m_fadeDuration;             ///< 显示/隐藏动画时长（毫秒）
    qreal m_opacity;                ///< 当前透明度
    bool m_isHovered;               ///< 是否悬停

    QPropertyAnimation *m_fadeAnimation;  ///< 透明度动画

    mutable QColor m_handleColor;         ///< 滑块颜色
    mutable QColor m_grooveColor;         ///< 轨道颜色

    static constexpr int DEFAULT_WIDTH = 8;      ///< 默认宽度
    static constexpr int DEFAULT_FADE_DURATION = 200;  ///< 默认动画时长
    static constexpr int HIDE_DELAY = 800;       ///< 隐藏延迟（毫秒）
};

/// @brief 内容列表项，继承 QListWidgetItem
/// 提供与 QListWidgetItem 兼容的接口，支持图标和详细文字
class QtETListWidgetItem : public QListWidgetItem
{
public:
    explicit QtETListWidgetItem(QListWidget *parent = nullptr, int type = Type);
    explicit QtETListWidgetItem(const QString &text, QListWidget *parent = nullptr, int type = Type);
    explicit QtETListWidgetItem(const QIcon &icon, const QString &text,
                                QListWidget *parent = nullptr, int type = Type);
    explicit QtETListWidgetItem(const QtETListWidgetItem &other);
    ~QtETListWidgetItem() override = default;

    QtETListWidgetItem &operator=(const QtETListWidgetItem &other);

    /// @brief 设置详细文字（显示在主文字下方）
    void setDetailText(const QString &text);
    /// @brief 获取详细文字
    [[nodiscard]] QString detailText() const;

private:
    QString m_detailText;  ///< 详细文字
};

/// @brief 内容列表委托
/// 负责绘制卡片式项目，支持边框高亮动画
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

    /// @brief 设置项目高度
    void setItemHeight(int height);
    /// @brief 获取项目高度
    [[nodiscard]] int itemHeight() const;

    /// @brief 设置右边距（滚动条占用）
    void setRightMargin(int margin);
    /// @brief 获取右边距
    [[nodiscard]] int rightMargin() const;

private:
    /// @brief 更新颜色方案
    void updateColorScheme() const;

    mutable QColor m_highlightColor;       ///< 高亮颜色 (#66ccff)
    mutable QColor m_hoverFillColor;       ///< 悬停填充颜色
    mutable QColor m_selectedFillColor;    ///< 选中填充颜色
    mutable QColor m_normalBorderColor;    ///< 普通边框颜色
    mutable QColor m_backgroundColor;      ///< 背景颜色
    mutable QColor m_textColor;            ///< 主文字颜色
    mutable QColor m_detailTextColor;      ///< 详细文字颜色

    int m_itemHeight;                      ///< 项目高度
    int m_rightMargin;                     ///< 右边距（滚动条占用）

    // 尺寸常量
    static constexpr int BORDER_RADIUS = 5;          ///< 项目圆角
    static constexpr int ITEM_SPACING = 0;           ///< 项目间距（无边距）
    static constexpr int ITEM_PADDING_H = 6;         ///< 项目水平内边距
    static constexpr int ITEM_PADDING_V = 2;         ///< 项目垂直内边距（无边距）
    static constexpr int ICON_SIZE = 20;             ///< 图标大小
    static constexpr int TEXT_SIZE = 10;             ///< 主文字大小
    static constexpr int DETAIL_TEXT_SIZE = 9;       ///< 详细文字大小
    static constexpr int ICON_TEXT_SPACING = 10;     ///< 图标与文字间距
    static constexpr int LINE_SPACING = 2;           ///< 主文字与详细文字间距
};

/// @brief 内容列表控件
/// 基于 QListWidget，提供卡片式列表项显示
/// 支持边框高亮动画、空状态提示、深浅模式适配
class QtETListWidget : public QListWidget
{
    Q_OBJECT
    Q_PROPERTY(QString emptyText READ emptyText WRITE setEmptyText)
    Q_PROPERTY(QColor highlightColor READ highlightColor WRITE setHighlightColor)
    Q_PROPERTY(int itemHeight READ itemHeight WRITE setItemHeight)

public:
    explicit QtETListWidget(QWidget *parent = nullptr);
    ~QtETListWidget() override;

    /// @brief 设置空状态提示文字
    void setEmptyText(const QString &text);
    /// @brief 获取空状态提示文字
    [[nodiscard]] QString emptyText() const;

    /// @brief 设置高亮颜色
    void setHighlightColor(const QColor &color);
    /// @brief 获取高亮颜色
    [[nodiscard]] QColor highlightColor() const;

    /// @brief 设置项目高度
    void setItemHeight(int height);
    /// @brief 获取项目高度
    [[nodiscard]] int itemHeight() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    /// @brief 初始化控件
    void init();
    /// @brief 更新颜色方案
    void updateColorScheme();
    /// @brief 初始化自定义滚动条
    void initScrollBar();
    /// @brief 开始平滑滚动
    void startSmoothScroll(int delta);
    /// @brief 更新滚动条位置和可见性
    void updateScrollBar();

private:
    QtETListWidgetDelegate *m_delegate;    ///< 自定义委托
    QtETScrollBar *m_scrollBar;            ///< 自定义滚动条
    QString m_emptyText;                    ///< 空状态提示文字
    QColor m_listBorderColor;               ///< 列表整体边框颜色
    QColor m_listBackgroundColor;           ///< 列表整体背景颜色

    // 平滑滚动
    QPropertyAnimation *m_scrollAnimation;  ///< 滚动动画
    int m_smoothScrollDuration;             ///< 平滑滚动动画时长（毫秒）

    // 尺寸常量
    static constexpr int LIST_BORDER_RADIUS = 5;    ///< 列表整体圆角
    static constexpr int LIST_PADDING = 8;          ///< 列表内边距
    static constexpr int LIST_BORDER_WIDTH = 1;     ///< 列表边框宽度
    static constexpr int SCROLL_BAR_MARGIN = 4;     ///< 滚动条距离边框的间距
};

#endif // QTETLISTWIDGET_H
