/**
 * @file ImageFrameProvider.h
 *
 * This file declares a module that provides transformations from the time when the image was recorded.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Framework/Module.h"
#include "Representations/MotionControl/OdometryData.h"
#include "Representations/MotionControl/MotionInfo.h"


MODULE(ImageFrameProvider,
{,
  REQUIRES(MotionOdometryData),
  REQUIRES(MotionMotionInfo),
  PROVIDES(OdometryData),
  PROVIDES(MotionInfo),
});

class ImageFrameProvider : public ImageFrameProviderBase
{
  /**
   * This method updates the odometry data.
   * @param odometryData The updated representation.
   */
  void update(OdometryData& odometryData);
  void update(MotionInfo& motionInfo);
};
