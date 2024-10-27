/**
 * @file MotionProvider.h
 *
 * This file declares a module that provides representations from the motion
 * thread for the current frame.
 *
 * @author Arne Hasselbring
 */

#include "Framework/Module.h"
#include "Representations/MotionControl/OdometryData.h"
#include "Representations/MotionControl/MotionInfo.h"


MODULE(MotionProvider,
{,
  REQUIRES(MotionOdometryData),
  REQUIRES(MotionMotionInfo),
  PROVIDES(OdometryData),
  PROVIDES(MotionInfo),
});

class MotionProvider : public MotionProviderBase
{
  void update(OdometryData& odometryData);
  void update(MotionInfo& motionInfo);
};
