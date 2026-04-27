#ifndef QTETLABELLIST_H
#define QTETLABELLIST_H

#include <QWidget>
#include <QListWidgetItem>
#include <QPropertyAnimation>
#include <QIcon>
#include <QList>

class QtETLabelListItem
{
public:
    explicit QtETLabelListItem();
    explicit QtETLabelListItem(const QString &text);
    explicit QtETLabelListItem(const QIcon &icon, const QString &text);
    ~QtETLabelListItem() = default;

    [[nodiscard]] QString text() const { return m_text; }
    void setText(const QString &text) { m_text = text; }

    [[nodiscard]] QIcon icon() const { return m_icon; }
    void setIcon(const QIcon &icon) { m_icon = icon; }

    [[nodiscard]] QVariant data(int role) const;
    void setData(int role, const QVariant &value);

private:
    QString m_text;
    QIcon m_icon;
    QMap<int, QVariant> m_data;
};

class QtETLabelList : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverOpacity READ hoverOpacity WRITE setHoverOpacity)
    Q_PROPERTY(qreal selectionOpacity READ selectionOpacity WRITE setSelectionOpacity)

public:
    explicit QtETLabelList(QWidget *parent = nullptr);
    ~QtETLabelList() override;

    void addItem(QtETLabelListItem *item);
    void addItem(const QString &text);
    [[nodiscard]] int count() const;
    [[nodiscard]] QtETLabelListItem* item(int index) const;
    [[nodiscard]] QtETLabelListItem* currentItem() const;
    [[nodiscard]] int currentRow() const;
    void setCurrentRow(int row);
    [[nodiscard]] int row(QtETLabelListItem *item) const;
    void clear();
    QtETLabelListItem* takeItem(int row);
    [[nodiscard]] QRect visualItemRect(QtETLabelListItem *item) const;
    [[nodiscard]] QtETLabelListItem* itemAt(const QPoint &pos) const;
    void setItemIcon(int index, const QIcon &icon);

    [[nodiscard]] qreal hoverOpacity() const;
    void setHoverOpacity(qreal opacity);
    [[nodiscard]] qreal selectionOpacity() const;
    void setSelectionOpacity(qreal opacity);
    void setHighlightColor(const QColor &color);
    [[nodiscard]] QColor highlightColor() const;

signals:
    void currentRowChanged(int currentRow);
    void itemClicked(QtETLabelListItem *item);
    void itemDoubleClicked(QtETLabelListItem *item);
    void itemSelectionChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void init();
    void updateColorScheme();
    [[nodiscard]] QRect calculateItemRect(int row) const;
    [[nodiscard]] int getRowAtPosition(const QPoint &pos) const;
    void updateContentHeight();

private:
    QList<QtETLabelListItem*> m_items;
    QPropertyAnimation *m_hoverAnimation = nullptr;
    QPropertyAnimation *m_selectionAnimation = nullptr;
    qreal m_hoverOpacity = 0.0;
    qreal m_selectionOpacity = 0.0;
    int m_hoveredRow = -1;
    int m_selectedRow = -1;
    int m_scrollOffset = 0;
    QColor m_highlightColor;
    QColor m_hoverFillColor;
    QColor m_textColor;
    QColor m_bgColor;
    QColor m_selectedTextColor;

    static constexpr int ANIMATION_DURATION = 200;
    static constexpr int BORDER_RADIUS = 6;
    static constexpr int ITEM_MARGIN = 4;
    static constexpr int ITEM_HEIGHT = 36;
    static constexpr int ICON_SIZE = 18;
    static constexpr int TEXT_SIZE = 12;
    static constexpr int ICON_TEXT_SPACING = 8;
};

#endif // QTETLABELLIST_H
