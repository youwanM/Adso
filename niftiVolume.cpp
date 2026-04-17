#include "niftiVolume.h"
#include <QDir>
#include <QString>
#include <cstring>
#include <stdexcept>
#include <QDebug>

extern "C" {
#include <nifti1_io.h>
}

/**
 * @brief Load a NIFTI file from disk.
 *
 * Reads a NIFTI-1 file and converts its voxel data into a floating-point
 * Eigen tensor. Supported input datatypes include INT16, UINT8, FLOAT32, etc.
 *
 * @param path Path to the input NIFTI file.
 * @return NiftiVolume Loaded volume with voxel data and spacing initialized.
 *
 * @throws std::runtime_error If the file cannot be read or if the datatype
 * is not supported.
 */
NiftiVolume NiftiVolume::loadNifti(const QString &path) {
    nifti_image *nim = nifti_image_read(path.toStdString().c_str(), 1);
    if (!nim)
        throw std::runtime_error("Failed to read NIFTI file: " + path.toStdString());

    NiftiVolume vol;
    vol.file_path = path;

    const int nx = nim->nx;
    const int ny = nim->ny;
    const int nz = nim->nz;
    const int nt = (nim->nt > 1) ? nim->nt : 1;

    vol.spacing = Eigen::Vector3f(nim->dx > 0 ? nim->dx : 1.0f, nim->dy > 0 ? nim->dy : 1.0f,
                                  nim->dz > 0 ? nim->dz : 1.0f);

    const size_t totalVoxels = static_cast<size_t>(nx) * ny * nz * nt;
    std::vector<float> floatBuffer(totalVoxels);
    const void *srcData = nim->data;

    if (nim->datatype == NIFTI_TYPE_FLOAT32) {
        std::memcpy(floatBuffer.data(), srcData, totalVoxels * sizeof(float));
    } else {
        for (size_t i = 0; i < totalVoxels; ++i) {
            if (nim->datatype == NIFTI_TYPE_FLOAT64)
                floatBuffer[i] = static_cast<float>(static_cast<const double *>(srcData)[i]);
            else if (nim->datatype == NIFTI_TYPE_INT16)
                floatBuffer[i] = static_cast<float>(static_cast<const int16_t *>(srcData)[i]);
            else if (nim->datatype == NIFTI_TYPE_UINT16)
                floatBuffer[i] = static_cast<float>(static_cast<const uint16_t *>(srcData)[i]);
            else if (nim->datatype == NIFTI_TYPE_UINT8)
                floatBuffer[i] = static_cast<float>(static_cast<const uint8_t *>(srcData)[i]);
        }
    }

    vol.data = Eigen::Tensor<float, 4, Eigen::ColMajor>(nx, ny, nz, nt);
    std::memcpy(vol.data.data(), floatBuffer.data(), totalVoxels * sizeof(float));

    nifti_image_free(nim);
    return vol;
}


/**
 * @brief Save a NIFTI file to disk.
 *
 * Writes the provided NiftiVolume to a NIFTI-1 file. The voxel spacing
 * metadata is preserved. Data are saved as FLOAT32.
 *
 * @param path Output file path.
 * @param vol Volume to save.
 * * @return bool True if the file was saved successfully.
 *
 * @throws std::runtime_error If the file cannot be written.
 */
bool NiftiVolume::saveNifti(const QString &path, const NiftiVolume &vol) {
    nifti_image *nim = nifti_simple_init_nim();
    if (!nim)
        throw std::runtime_error("Failed to init nifti_image");

    const int nx = vol.data.dimension(0);
    const int ny = vol.data.dimension(1);
    const int nz = vol.data.dimension(2);
    const int nt = vol.data.dimension(3);

    nim->dim[0] = 4;
    nim->dim[1] = nx;
    nim->dim[2] = ny;
    nim->dim[3] = nz;
    nim->dim[4] = nt;
    nim->dim[5] = 1;
    nim->dim[6] = 1;
    nim->dim[7] = 1;
    nim->nx = nx;
    nim->ny = ny;
    nim->nz = nz;
    nim->nt = nt;
    nim->nvox = static_cast<size_t>(nx) * ny * nz * nt;

    nim->pixdim[1] = vol.spacing.x();
    nim->pixdim[2] = vol.spacing.y();
    nim->pixdim[3] = vol.spacing.z();
    nim->dx = nim->pixdim[1];
    nim->dy = nim->pixdim[2];
    nim->dz = nim->pixdim[3];

    nim->datatype = NIFTI_TYPE_FLOAT32;
    nim->nbyper = sizeof(float);
    nim->data = std::malloc(nim->nvox * nim->nbyper);

    std::memcpy(nim->data, vol.data.data(), nim->nvox * sizeof(float));

    nifti_set_filenames(nim, path.toStdString().c_str(), 0, 1);
    nifti_image_write(nim);
    nifti_image_free(nim);

    // --- RELECTURE DEBUG ---
    nifti_image *check = nifti_image_read(path.toStdString().c_str(), 0);
    if (check) {
        qDebug() << "[SAVE CHECK] Saving NIfTI to" << path << "with dimensions:" << nx << ny << nz
                 << nt << "and spacing:" << nim->pixdim[1] << nim->pixdim[2] << nim->pixdim[3];
        nifti_image_free(check);
    }

    return true;
}

bool NiftiVolume::saveNiftiWithReference(const QString &path, const NiftiVolume &vol,
                                         const QString &refPath) {
    nifti_image *nim = nifti_image_read(refPath.toStdString().c_str(), 0);
    if (!nim) {
        qDebug() << "Could not read reference NIfTI header from" << refPath.toStdString();
        return false;
    }

    const int nx = vol.data.dimension(0);
    const int ny = vol.data.dimension(1);
    const int nz = vol.data.dimension(2);
    const int nt = vol.data.dimension(3);

    nim->dim[1] = nx;
    nim->nx = nx;
    nim->dim[2] = ny;
    nim->ny = ny;
    nim->dim[3] = nz;
    nim->nz = nz;
    nim->dim[4] = nt;
    nim->nt = nt;
    nim->nvox = static_cast<size_t>(nx) * ny * nz * nt;

    nim->datatype = NIFTI_TYPE_FLOAT32;
    nim->nbyper = sizeof(float);

    nim->data = std::malloc(nim->nvox * nim->nbyper);
    if (!nim->data) {
        nifti_image_free(nim);
        return false;
    }
    std::memcpy(nim->data, vol.data.data(), nim->nvox * sizeof(float));

    nifti_set_filenames(nim, path.toStdString().c_str(), 0, 1);
    nifti_image_write(nim);

    nifti_image *check = nifti_image_read(path.toStdString().c_str(), 0);
    if (check) {
        qDebug() << "[SAVE CHECK] Saving NIfTI to" << path << "with dimensions:" << nx << ny << nz
                 << nt << "and spacing:" << nim->pixdim[1] << nim->pixdim[2] << nim->pixdim[3];
        nifti_image_free(check);
    }

    nifti_image_free(nim);
    return true;
}

/**
 * @brief Vectorizes the volume tensor into a contiguous 1D array.
 *
 * The returned vector contains all tensor elements flattened in memory order
 * (column-major/Fortran-style by default in this configuration).
 *
 * This operation performs a copy of the underlying data.
 *
 * @return std::vector<float> Flattened tensor data
 *
 * @note The output order is consistent with NIfTI's [X][Y][Z][C] storage.
 * Ensure compatibility with downstream libraries (e.g. ONNX, NumPy).
 */
std::vector<float> NiftiVolume::toVector() const {
    return std::vector<float>(data.data(), data.data() + data.size());
}

/**
 * @brief Returns the shape of the volume tensor.
 *
 * The shape corresponds to the tensor dimensions in the following order:
 * - shape[0] = size along X
 * - shape[1] = size along Y
 * - shape[2] = size along Z
 * - shape[3] = number of channels (C)
 *
 * @return std::vector<int64_t> Tensor dimensions (X, Y, Z, C)
 */
std::vector<int64_t> NiftiVolume::getShape() const {
    return {
        static_cast<int64_t>(data.dimension(0)), // X
        static_cast<int64_t>(data.dimension(1)), // Y
        static_cast<int64_t>(data.dimension(2)), // Z
        static_cast<int64_t>(data.dimension(3))  // C
    };
}