#pragma once

#include <Eigen/Core>
#include <QString>
#include <unsupported/Eigen/CXX11/Tensor>
#include <vector>

/**
 * @struct NiftiVolume
 * @brief Container for a 4D NIFTI medical volume using Eigen tensors.
 *
 * This structure encapsulates voxel data and spatial metadata of a NIFTI
 * volume. The voxel intensities are stored in a 4D Eigen tensor with the
 * layout (X, Y, Z, C), where C corresponds to channels.
 *
 * The spacing vector defines the physical voxel size in millimeters
 * along each spatial axis (x, y, z).
 */
struct NiftiVolume {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    using Tensor4f = Eigen::Tensor<float, 4, Eigen::ColMajor>;

    /**
     * @brief Voxel data stored as a 4D Eigen tensor.
     *
     * Tensor layout is (X, Y, Z, C) in column-major order.
     * This layout matches the native NIfTI disk storage, where X is the
     * fastest-varying dimension in memory.
     */
    Tensor4f data;           // (C, X, Y, Z)

    /**
     * @brief Physical voxel spacing in millimeters.
     *
     * spacing = (sx, sy, sz)
     */
    Eigen::Vector3f spacing; // (sx, sy, sz)

    QString file_path; // Original file path (optional)

    static NiftiVolume loadNifti(const QString &path);
    static bool saveNifti(const QString &path, const NiftiVolume &vol);

    static bool saveNiftiWithReference(const QString &path, const NiftiVolume &vol,
                                const QString &refPath);

    std::vector<float> toVector() const;

    std::vector<int64_t> getShape() const;
};