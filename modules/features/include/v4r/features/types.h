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

/**
 * @file types.h
 * @author Thomas Faeulhammer (faeulhammer@acin.tuwien.ac.at)
 * @date 2016
 * @brief
 *
 */
#pragma once
#include <v4r/core/macros.h>
#include <iostream>

namespace v4r {
enum FeatureType {
  SIFT_GPU = 0x01,     // 00000001
  SIFT_OPENCV = 0x02,  // 00000010
  SHOT = 0x04,         // 00000100
  OURCVFH = 0x08,      // 00001000
  FPFH = 0x10,         // 00010000
  ESF = 0x20,          // 00100000
  SHOT_COLOR = 0x40,   // 01000000
  ALEXNET = 0x80,      // 10000000
  ROPS = 0x200,        // 10000000
  SIMPLE_SHAPE = 0x400,
  GLOBAL_COLOR = 0x800,
  AKAZE = 0x1000,
  SURF = 0x2000
};

V4R_EXPORTS std::istream &operator>>(std::istream &in, FeatureType &t);
V4R_EXPORTS std::ostream &operator<<(std::ostream &out, const FeatureType &t);
}  // namespace v4r
