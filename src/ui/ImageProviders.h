#pragma once

#include <QQuickImageProvider>

class AppController;

// image://swatch/<medio>[?outline|?fill][&dark]: muestra de trazo para los menús de pinceles, contorno y relleno.
class SwatchProvider : public QQuickImageProvider {
public:
    SwatchProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}
    QImage requestImage(const QString &id, QSize *size, const QSize &requested) override;
};

// image://layers/<índice>/<revisión>: miniatura de una capa.
class LayerThumbProvider : public QQuickImageProvider {
public:
    explicit LayerThumbProvider(AppController *app)
        : QQuickImageProvider(QQuickImageProvider::Image), m_app(app) {}
    QImage requestImage(const QString &id, QSize *size, const QSize &requested) override;
private:
    AppController *m_app;
};

// image://composite/<revisión>: imagen completa compuesta (panel Miniatura).
class CompositeProvider : public QQuickImageProvider {
public:
    explicit CompositeProvider(AppController *app)
        : QQuickImageProvider(QQuickImageProvider::Image), m_app(app) {}
    QImage requestImage(const QString &id, QSize *size, const QSize &requested) override;
private:
    AppController *m_app;
};
