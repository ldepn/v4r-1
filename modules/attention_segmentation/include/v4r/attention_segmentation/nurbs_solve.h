/****************************************************************************
**
** Copyright (C) 2017 TU Wien, ACIN, Vision 4 Robotics (V4R) group
** Contact: v4r.acin.tuwien.ac.at
**
** This file is part of V4R
**
** V4R is distributed under dual licenses - GPLv3 or closed source.
**
** GNU General Public License Usage
** V4R is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published
** by the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** V4R is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** Please review the following information to ensure the GNU General Public
** License requirements will be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
**
** Commercial License Usage
** If GPL is not suitable for your project, you must purchase a commercial
** license to use V4R. Licensees holding valid commercial V4R licenses may
** use this file in accordance with the commercial license agreement
** provided with the Software or, alternatively, in accordance with the
** terms contained in a written agreement between you and TU Wien, ACIN, V4R.
** For licensing terms and conditions please contact office<at>acin.tuwien.ac.at.
**
**
** The copyright holder additionally grants the author(s) of the file the right
** to use, copy, modify, merge, publish, distribute, sublicense, and/or
** sell copies of their contributions without any restrictions.
**
****************************************************************************/

#ifndef _NURBS_SOLVE_H_
#define _NURBS_SOLVE_H_

#undef Success
#include <Eigen/Dense>

#include "sparse_mat.h"

namespace pcl {
namespace on_nurbs {

/** \brief Solving the linear system of equations using Eigen or UmfPack.
 * (can be defined in on_nurbs.cmake)*/
class NurbsSolve {
 public:
  /** \brief Empty constructor */
  NurbsSolve() : m_quiet(true) {}

  /** \brief Assign size and dimension (2D, 3D) of system of equations. */
  void assign(unsigned rows, unsigned cols, unsigned dims);

  /** \brief Set value for system matrix K (stiffness matrix, basis functions) */
  void K(unsigned i, unsigned j, double v);
  /** \brief Set value for state vector x (control points) */
  void x(unsigned i, unsigned j, double v);
  /** \brief Set value for target vector f (force vector) */
  void f(unsigned i, unsigned j, double v);

  /** \brief Get value for system matrix K (stiffness matrix, basis functions) */
  double K(unsigned i, unsigned j);
  /** \brief Get value for state vector x (control points) */
  double x(unsigned i, unsigned j);
  /** \brief Get value for target vector f (force vector) */
  double f(unsigned i, unsigned j);

  /** \brief Resize target vector f (force vector) */
  void resize(unsigned rows);

  /** \brief Print system matrix K (stiffness matrix, basis functions) */
  void printK();
  /** \brief Print state vector x (control points) */
  void printX();
  /** \brief Print target vector f (force vector) */
  void printF();

  /** \brief Solves the system of equations with respect to x.
   *  - Using UmfPack incredibly speeds up this function.      */
  bool solve();

  /** \brief Compute the difference between solution K*x and target f */
  Eigen::MatrixXd diff();

  /** \brief Enable/Disable debug outputs in console. */
  inline void setQuiet(bool val) {
    m_quiet = val;
  }

  /** \brief get size of system */
  inline void getSize(unsigned &rows, unsigned &cols, unsigned &dims) {
    rows = static_cast<unsigned>(m_feig.rows());
    cols = static_cast<unsigned>(m_xeig.rows());
    dims = static_cast<unsigned>(m_feig.cols());
  }

  inline unsigned rows() {
    return m_feig.rows();
  }
  inline unsigned cols() {
    return m_xeig.rows();
  }
  inline unsigned dims() {
    return m_feig.cols();
  }

 private:
  bool m_quiet;
  SparseMat m_Ksparse;
  Eigen::MatrixXd m_Keig;
  Eigen::MatrixXd m_xeig;
  Eigen::MatrixXd m_feig;

 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};
}  // namespace on_nurbs
}  // namespace pcl

#endif
