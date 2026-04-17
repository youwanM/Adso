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

    // --- 1. Top Panel: Brightness & Contrast ---
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

    // --- 2. Middle Panel: Images & Slice Sliders ---
    QHBoxLayout* imagesLayout = new QHBoxLayout();

    // A handy lambda to build the UI for each plane
    auto createViewPanel = [this](QLabel*& label, QSlider*& slider, const QString& title) {
        QVBoxLayout* layout = new QVBoxLayout();
        label = new QLabel(title + "\n(Waiting for image)", this);
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumSize(256, 256);

        slider = new QSlider(Qt::Horizontal, this);
        slider->setEnabled(false); // Disabled until an image is loaded

        layout->addWidget(label);
        layout->addWidget(slider);
        return layout;
        };

    imagesLayout->addLayout(createViewPanel(sagittalLabel, sagittalSlider, "Sagittal (X)"));
    imagesLayout->addLayout(createViewPanel(coronalLabel, coronalSlider, "Coronal (Y)"));
    imagesLayout->addLayout(createViewPanel(axialLabel, axialSlider, "Axial (Z)"));

    // --- 3. Bottom Panel: Controls ---
    openButton = new QPushButton("Open NIfTI Image", this);
    statusLabel = new QLabel("No image loaded.", this);

    mainLayout->addLayout(bcLayout);
    mainLayout->addLayout(imagesLayout);
    mainLayout->addWidget(openButton);
    mainLayout->addWidget(statusLabel);

    setCentralWidget(centralWidget);
    resize(900, 500);

    // --- 4. Connect Signals ---
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openNiftiFile);

    // Wire every single slider to our updateViews function
    connect(sagittalSlider, &QSlider::valueChanged, this, &MainWindow::updateViews);
    connect(coronalSlider, &QSlider::valueChanged, this, &MainWindow::updateViews);
    connect(axialSlider, &QSlider::valueChanged, this, &MainWindow::updateViews);
    connect(brightnessSlider, &QSlider::valueChanged, this, &MainWindow::updateViews);
    connect(contrastSlider, &QSlider::valueChanged, this, &MainWindow::updateViews);
}

MainWindow::~MainWindow() {}

void MainWindow::openNiftiFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
    tr("Open NIfTI Image"), "", tr("NIfTI Files (*.nii *.nii.gz)"));

    if (fileName.isEmpty()) return;

    statusLabel->setText("Loading: " + fileName);
    QCoreApplication::processEvents();

    try {
        // Save the volume to our class variable
        myVolume = NiftiVolume::loadNifti(fileName);
        isLoaded = true;

        std::vector<int64_t> shape = myVolume.getShape();
        int dimX = shape[0];
        int dimY = shape[1];
        int dimZ = shape[2];

        // Configure the sliders to match the dimensions of the file
        sagittalSlider->setRange(0, dimX - 1);
        coronalSlider->setRange(0, dimY - 1);
        axialSlider->setRange(0, dimZ - 1);

        sagittalSlider->setEnabled(true);
        coronalSlider->setEnabled(true);
        axialSlider->setEnabled(true);

        // Setting the value automatically triggers updateViews() to draw the first frame!
        sagittalSlider->setValue(dimX / 2);
        coronalSlider->setValue(dimY / 2);
        axialSlider->setValue(dimZ / 2);

        statusLabel->setText(QString("Successfully loaded! Dimensions: %1 x %2 x %3").arg(dimX).arg(dimY).arg(dimZ));

    }
    catch (const std::exception& e) {
        isLoaded = false;
        QMessageBox::critical(this, "Error", QString("Failed to load NIfTI image:\n") + e.what());
        statusLabel->setText("Error loading file.");
    }
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
        p.setPen(QPen(Qt::red, 1)); // 1-pixel wide red line

        if (axis == 0) { // Sagittal (Shows Y and Z)
            p.drawLine(currentY, 0, currentY, img.height());
            p.drawLine(0, currentZ, img.width(), currentZ);
        }
        else if (axis == 1) { // Coronal (Shows X and Z)
            p.drawLine(currentX, 0, currentX, img.height());
            p.drawLine(0, currentZ, img.width(), currentZ);
        }
        else if (axis == 2) { // Axial (Shows X and Y)
            p.drawLine(currentX, 0, currentX, img.height());
            p.drawLine(0, currentY, img.width(), currentY);
        }
        };

    drawCrosshair(sagImg, 0);
    drawCrosshair(corImg, 1);
    drawCrosshair(axiImg, 2);

    // 3. Display the final images on the UI
    sagittalLabel->setPixmap(QPixmap::fromImage(sagImg).scaled(256, 256, Qt::KeepAspectRatio));
    coronalLabel->setPixmap(QPixmap::fromImage(corImg).scaled(256, 256, Qt::KeepAspectRatio));
    axialLabel->setPixmap(QPixmap::fromImage(axiImg).scaled(256, 256, Qt::KeepAspectRatio));
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
