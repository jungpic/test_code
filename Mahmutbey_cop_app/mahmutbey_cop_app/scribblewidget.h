#ifndef SCRIBBLEWIDGET_H
#define SCRIBBLEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QImage>
#include <QPainter>

class ScribbleWidget : public QWidget
{
//    Q_OBJECT
public:
//    explicit ScribbleWidget(QWidget *parent = 0);
    ScribbleWidget(QWidget *parent = 0);

    void resizeEvent(QResizeEvent *e);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *);

private:
    void drawLineTo(const QPoint &endpoint);

private:
    bool scribbling;
    QPoint lastPoint;
    QImage image;
};

#endif // SCRIBBLEWIDGET_H
