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
#include "ClickableLabel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void loadVolume(const QString& fileName);

private slots:
    void handleImageClick(int axis, int px, int py,  ClickableLabel* label);
    void openNiftiFile();
    void updateViews();

private:
    QPushButton* openButton;
    QLabel* statusLabel;

    // View Labels
    ClickableLabel* sagittalLabel;
    ClickableLabel* coronalLabel;
    ClickableLabel* axialLabel;

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