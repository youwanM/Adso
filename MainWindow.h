#pragma once

#include <QtWidgets/QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QImage>
#include <QPixmap>
#include <QPainter> // For drawing the crosshairs
#include "niftiVolume.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void openNiftiFile();
    void updateViews(); // Triggered whenever ANY slider moves

private:
    QPushButton* openButton;
    QLabel* statusLabel;

    // View Labels
    QLabel* sagittalLabel;
    QLabel* coronalLabel;
    QLabel* axialLabel;

    // Slice Sliders
    QSlider* sagittalSlider; // X-axis
    QSlider* coronalSlider;  // Y-axis
    QSlider* axialSlider;    // Z-axis

    // Image Adjustments
    QSlider* brightnessSlider;
    QSlider* contrastSlider;

    // State Variables
    NiftiVolume myVolume;    // Saved in the class now!
    bool isLoaded = false;

    int currentX = 0;
    int currentY = 0;
    int currentZ = 0;

    QImage extractSlice(int axis, int sliceIndex);
};