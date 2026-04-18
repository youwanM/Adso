#include "ClickableLabel.h"

ClickableLabel::ClickableLabel(QWidget* parent) : QLabel(parent) {
    // Optional: cursor feedback
    setCursor(Qt::CrossCursor); 
}

void ClickableLabel::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit imageClicked(event->pos().x(), event->pos().y());
    }
}

void ClickableLabel::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        emit imageClicked(event->pos().x(), event->pos().y());
    }
}