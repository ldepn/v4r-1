/****************************************************************************
**
** Copyright (C) 2018 TU Wien, ACIN, Vision 4 Robotics (V4R) group
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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <v4r/core/macros.h>
#include <v4r/io/grabber.h>

namespace v4r {
namespace io {

/// Grab from a OpenNI2-compatible device (Asus Xtion, Kinect).
///
/// # URI format
///
/// General URI format is as follows: \c xxx#yyy/zzz.
///
///  - \c xxx : Device name or address
///
///      Device name is one of "asus", "kinect", "openni2", "oni2". In this case the first available device that
///      is managed by OpenNI2 driver will be used.
///
///      Device address is an address as returned by enumerateConnectedDevices(). This should be used to grab from
///      a specific device.
///
///  - \c yyy : color stream mode
///
///      1-based index into the list returned by getSupportedColorStreamModes().
///      Special values: 0 - disable color stream, 255 - choose default color stream mode.
///
///  - \c zzz : depth stream mode
///
///      1-based index into the list returned by getSupportedDepthStreamModes().
///      Special values: 0 - disable depth stream, 255 - choose default depth stream mode.
///
/// The \c #yyy/zzz part is optional. If it is omitted, then the default streaming modes will be used. Therefore, no
/// mode is equivalent to \c #255/255 mode.
///
/// Empty string is also a valid URI. It means that the first available device should be opened in the default mode.
///
/// ## Examples
///
///  - \c oni2 :           open first available OpenNI2 device in default mode
///
///  - \c 1d27/0601@2/23 : open device with the given address in default mode
///
///  - \c openni2#0/255 :  open first available OpenNI2 device with color stream disabled and default depth mode
///
///  - \c asus#1/1 :       open first available OpenNI2 device with the first available color and depth modes
///
class V4R_EXPORTS OpenNI2Grabber : public Grabber {
 public:
  using Ptr = std::shared_ptr<OpenNI2Grabber>;

  /// Get the list of connected devices that this grabber can manage.
  static std::vector<std::string> enumerateConnectedDevices();

  /// Specification of the mode in which the device should be opened.
  ///
  /// Color and depth fields should contain a 1-based index into the list of supported stream modes (as returned by
  /// getSupportedColorStreamModes() or getSupportedDepthStreamModes() respectively). In order to open the stream in the
  /// default mode pass \c DEFAULT. In order to not open the stream pass \c DISABLE.
  struct Mode {
    enum { DISABLE = 0, DEFAULT = 255 };
    uint8_t color = DEFAULT;
    uint8_t depth = DEFAULT;
    static Mode parse(const std::string& mode_string);
  };

  /// Construct a grabber for a given URI device.
  /// See the class description for supported URI formats.
  explicit OpenNI2Grabber(const std::string& device_uri = "");

  /// Construct a grabber for a given URI in a given mode.
  /// See the class description for supported URI formats. The \a mode argument overrides the mode portion of URI.
  explicit OpenNI2Grabber(Mode mode, const std::string& device_uri = "");

  ~OpenNI2Grabber() override;

  Timestamp grabFrame(cv::OutputArray color, cv::OutputArray depth) override;

  bool hasMoreFrames() const override;

  int getNumberOfFrames() const override;

  int getCurrentFrameIndex() const override;

  Intrinsics getCameraIntrinsics() const override;

  using Grabber::printInfo;

  void printInfo(std::ostream& os) const override;

  StreamModes getSupportedColorStreamModes() const override;

  StreamModes getSupportedDepthStreamModes() const override;

  StreamMode getActiveColorStreamMode() const override;

  StreamMode getActiveDepthStreamMode() const override;

  bool isFeatureSupported(Feature feature) const override;

  void seek(unsigned int index) override;

  void setRepeatEnabled(bool state) override;

  bool getRepeatEnabled() const override;

  void setAutoWhiteBalanceEnabled(bool state) override;

  bool getAutoWhiteBalanceEnabled() const override;

  void setAutoExposureEnabled(bool state) override;

  bool getAutoExposureEnabled() const override;

  void setExposure(unsigned int exposure_ms) override;

  unsigned int getExposure() const override;

  std::pair<unsigned int, unsigned int> getExposureRange() const override;

  void setGain(int gain) override;

  int getGain() const override;

  std::string getSerialNumber() const override;

  std::string getCameraModelName() const override;

 private:
  struct Impl;
  std::unique_ptr<Impl> p;
};

}  // namespace io
}  // namespace v4r
