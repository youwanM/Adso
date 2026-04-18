#include "MainWindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QCoreApplication>
#include <algorithm>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Adso - Anatomical Data Slice Observer");
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // --- Top Panel: Brightness & Contrast ---
    QHBoxLayout* bcLayout = new QHBoxLayout();
    brightnessSlider = new QSlider(Qt::Horizontal, this);
    brightnessSlider->setRange(0, 255);
    brightnessSlider->setValue(0);

    contrastSlider = new QSlider(Qt::Horizontal, this);
    contrastSlider->setRange(1, 200); // Mapped to 0.02x to 4.0x multiplier
    contrastSlider->setValue(50);     // 1.0x (Default)

    bcLayout->addWidget(new QLabel("Brightness:"));
    bcLayout->addWidget(brightnessSlider);
    bcLayout->addWidget(new QLabel("Contrast:"));
    bcLayout->addWidget(contrastSlider);

    // --- Middle Panel: Images & Slice Sliders ---
    QHBoxLayout* imagesLayout = new QHBoxLayout();

    // A handy lambda to build the UI for each plane

    // Add the 'axis' parameter to the lambda
    auto createViewPanel = [this](ClickableLabel*& label, QSlider*& slider, const QString& title, int axis) {
        QVBoxLayout* layout = new QVBoxLayout();
        label = new ClickableLabel(this); // Now using your custom class
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumSize(256, 256);

        slider = new QSlider(Qt::Horizontal, this);
        slider->setEnabled(false);

        connect(label, &ClickableLabel::imageClicked, [this, axis, label](int px, int py) {
                this->handleImageClick(axis, px, py, label);
            });

        layout->addWidget(label);
        layout->addWidget(slider);
        return layout;
    };

    // Look for these lines around line 58-60 in your MainWindow.cpp
    imagesLayout->addLayout(createViewPanel(sagittalLabel, sagittalSlider, "Sagittal (X)", 0));
    imagesLayout->addLayout(createViewPanel(coronalLabel, coronalSlider, "Coronal (Y)", 1));
    imagesLayout->addLayout(createViewPanel(axialLabel, axialSlider, "Axial (Z)", 2));
    
    // --- Bottom Panel: Controls ---
    openButton = new QPushButton("Open NIfTI Image", this);
    statusLabel = new QLabel("No image loaded.", this);

    // --- Copyright ---
    QLabel* copyrightLabel = new QLabel("Copyright (C) 2026 Youwan Mahé", this);
    copyrightLabel->setAlignment(Qt::AlignRight);
    // Setting small font and gray color to keep it subtle
    copyrightLabel->setStyleSheet("font-size: 9px; color: #888888; margin-top: 5px;");

    mainLayout->addLayout(bcLayout);
    mainLayout->addLayout(imagesLayout);
    mainLayout->addWidget(openButton);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(copyrightLabel);

    setCentralWidget(centralWidget);
    resize(900, 500);

    // --- Connect Signals ---
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openNiftiFile);

    // Wire every single slider to our updateViews function
    connect(sagittalSlider, &QSlider::valueChanged, this, &MainWindow::updateViews);
    connect(coronalSlider, &QSlider::valueChanged, this, &MainWindow::updateViews);
    connect(axialSlider, &QSlider::valueChanged, this, &MainWindow::updateViews);
    connect(brightnessSlider, &QSlider::valueChanged, this, &MainWindow::updateViews);
    connect(contrastSlider, &QSlider::valueChanged, this, &MainWindow::updateViews);
}

MainWindow::~MainWindow() {}

// This is your new shared loading function
void MainWindow::loadVolume(const QString& fileName) {
    if (fileName.isEmpty()) return;

    statusLabel->setText("Loading: " + fileName);
    QCoreApplication::processEvents();

    try {
        myVolume = NiftiVolume::loadNifti(fileName);
        isLoaded = true;

        std::vector<int64_t> shape = myVolume.getShape();
        int dimX = shape[0], dimY = shape[1], dimZ = shape[2];

        sagittalSlider->setRange(0, dimX - 1);
        coronalSlider->setRange(0, dimY - 1);
        axialSlider->setRange(0, dimZ - 1);

        sagittalSlider->setEnabled(true);
        coronalSlider->setEnabled(true);
        axialSlider->setEnabled(true);

        sagittalSlider->setValue(dimX / 2);
        coronalSlider->setValue(dimY / 2);
        axialSlider->setValue(dimZ / 2);

        statusLabel->setText(QString("Successfully loaded: %1x%2x%3").arg(dimX).arg(dimY).arg(dimZ));
        
        // Initial draw
        updateViews(); 
    }
    catch (const std::exception& e) {
        isLoaded = false;
        QMessageBox::critical(this, "Error", QString("Failed to load:\n") + e.what());
        statusLabel->setText("Error loading file.");
    }
}

// Now your button click just calls the shared function
void MainWindow::openNiftiFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open NIfTI Image"), "", tr("NIfTI Files (*.nii *.nii.gz)"));
    
    loadVolume(fileName);
}

// This function acts as the "Game Loop" rendering engine for your app
void MainWindow::updateViews() {
    if (!isLoaded) return;

    currentX = sagittalSlider->value();
    currentY = coronalSlider->value();
    currentZ = axialSlider->value();

    // 1. Extract the raw slices
    QImage sagImg = extractSlice(0, currentX);
    QImage corImg = extractSlice(1, currentY);
    QImage axiImg = extractSlice(2, currentZ);

    // 2. Draw the tracking crosshairs on top of the images
    auto drawCrosshair = [&](QImage& img, int axis) {
        QPainter p(&img);
        p.setPen(QPen(Qt::red, 1));

        std::vector<int64_t> shape = myVolume.getShape();
        int dimX = shape[0], dimY = shape[1], dimZ = shape[2];

        if (axis == 0) { // Sagittal (Y, Z)
            p.drawLine(currentY, 0, currentY, img.height());
            p.drawLine(0, (dimZ - 1) - currentZ, img.width(), (dimZ - 1) - currentZ);
        }
        else if (axis == 1) { // Coronal (X, Z)
            p.drawLine(currentX, 0, currentX, img.height());
            p.drawLine(0, (dimZ - 1) - currentZ, img.width(), (dimZ - 1) - currentZ);
        }
        else if (axis == 2) { // Axial (X, Y)
            p.drawLine(currentX, 0, currentX, img.height());
            // FIX: Draw the Y-line inverted so it matches the flipped image
            p.drawLine(0, (dimY - 1) - currentY, img.width(), (dimY - 1) - currentY);
        }
    };

    drawCrosshair(sagImg, 0);
    drawCrosshair(corImg, 1);
    drawCrosshair(axiImg, 2);

    // 3. Display the final images on the UI
    sagittalLabel->setPixmap(QPixmap::fromImage(sagImg).scaled(sagittalLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    coronalLabel->setPixmap(QPixmap::fromImage(corImg).scaled(coronalLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    axialLabel->setPixmap(QPixmap::fromImage(axiImg).scaled(axialLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

// Math logic to extract the slice and apply brightness/contrast
QImage MainWindow::extractSlice(int axis, int sliceIndex) {
    std::vector<int64_t> shape = myVolume.getShape();
    int nx = shape[0];
    int ny = shape[1];
    int nz = shape[2];

    int width = 0, height = 0;

    if (axis == 0) { width = ny; height = nz; }
    else if (axis == 1) { width = nx; height = nz; }
    else if (axis == 2) { width = nx; height = ny; }

    QImage img(width, height, QImage::Format_RGB32);

    // Read the current slider math
    float contrast = contrastSlider->value() / 50.0f;
    float brightness = brightnessSlider->value();

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            float val = 0.0f;

            // Pluck the specific 3D voxel from the Tensor
            if (axis == 0)      val = myVolume.data(sliceIndex, i, j, 0);
            else if (axis == 1) val = myVolume.data(i, sliceIndex, j, 0);
            else if (axis == 2) val = myVolume.data(i, j, sliceIndex, 0);

            // Apply y = (x * contrast) + brightness
            int pixelVal = std::clamp(static_cast<int>((val * contrast) + brightness), 0, 255);

            // Make it grayscale
            img.setPixel(i, j, qRgb(pixelVal, pixelVal, pixelVal));
        }
    }

    return img.flipped(Qt::Vertical);
}

void MainWindow::handleImageClick(int axis, int px, int py, ClickableLabel* label) {
    if (!isLoaded || label->pixmap().isNull()) return;

    QSize pixSize = label->pixmap().size();
    int offsetX = (label->width() - pixSize.width()) / 2;
    int offsetY = (label->height() - pixSize.height()) / 2;

    double normX = std::clamp((double)(px - offsetX) / pixSize.width(), 0.0, 1.0);
    double normY = std::clamp((double)(py - offsetY) / pixSize.height(), 0.0, 1.0);
    
    // Create an inverted version for vertical movements
    double normYInverted = 1.0 - normY;

    std::vector<int64_t> shape = myVolume.getShape();
    int dimX = shape[0], dimY = shape[1], dimZ = shape[2];

    if (axis == 0) { // Sagittal
        coronalSlider->setValue(normX * (dimY - 1));
        axialSlider->setValue(normYInverted * (dimZ - 1)); // Z is vertical here
    } 
    else if (axis == 1) { // Coronal
        sagittalSlider->setValue(normX * (dimX - 1));
        axialSlider->setValue(normYInverted * (dimZ - 1)); // Z is vertical here
    } 
    else if (axis == 2) { // Axial
        sagittalSlider->setValue(normX * (dimX - 1));
        // FIX: Vertical in Axial is Y. Use inverted so Top click = High Y
        coronalSlider->setValue(normYInverted * (dimY - 1));
    }
}