/**
 * @file UKFRobotPoseHypothesis.h
 *
 * Declaration of a robot pose estimate based on an Unscented Kalman Filter
 *
 * @author Tim Laue
 * @author Colin Graf
 */

#pragma once

#include "Representations/Modeling/PerceptRegistration.h"
#include "Representations/MotionControl/MotionInfo.h"
#include "Tools/Modeling/UKFPose2D.h"

/**
 * @class UKFRobotPoseHypothesis
 *
 * Hypothesis of a robot's pose, modeled as an Unscented Kalman Filter.
 * Actual UKF stuff is done by the base class UKFPose2D
 * The pose consists of a position in a 2D plane and an orientation in this plane.
 */
class UKFRobotPoseHypothesis : public UKFPose2D
{
public:
  float weighting;   /**< The weighting required for the resampling process. Computation is based on validity and a base weighting. */
  float validity;    /**< The validity represents the average success rate of the measurement matching process. 1 means that all recent measurements are compatible to the sample, 0 means that no measurements are compatible.*/
  int id;            /**< Each sample has a unique identifier, which is set at initialization. */

  /** Initializes the members of this sample.
   * @param pose The initial pose
   * @param poseDeviation The initial deviations of the estimates of the different dimensions
   * @param id The unique identifier (caller must make sure that it is really unique)
   * @param validity The initial validity [0,..,1]
   */
  void init(const Pose2f& pose, const Pose2f& poseDeviation, int id, float validity);

  /** The RoboCup field is point-symmetric. Calling this function turns the whole pose by 180 degrees around the field's center.*/
  void mirror();

  /** Computes a new validity value based on the current validity and the previous validity.
   * @param frames The old validity is weighted by (frames-1)
   * @param currentValidity The validity of this frame's measurements, weighted by 1
   */
  void updateValidity(int frames, float currentValidity);

  /** Sets the validity to 0, which will automatically lead to 0 weighting, too.
   *  This will cause the sample to be eliminated during the next resampling.
   */
  void invalidate();

  /** Yeah, just like the name says.
   *  Call after measurement / sensor updates.
   *  @param baseValidityWeighting The weighting will have at least this value
   */
  void computeWeightingBasedOnValidity(float baseValidityWeighting);

  /** Returns one variance value by combining x+y+rotational variance in some way*/
  float getCombinedVariance() const;

  /** Update the estimate based on the measurement of a landmark (center circle, penalty mark, ...)
   * @param landmark Yes, the landmark.
   */
  void updateByLandmark(const RegisteredLandmark& landmark);

  /** Update the estimate based on the measurement of a field line
   * @param line Yes, the line.
  */
  void updateByLine(const RegisteredLine& line);

  /** Update the estimate based on the measurement of a field line that is assumed to be a small
   *  part of the center circle (which was not detected as a whole).
   * @param line Yes, the line.
   * @param centerCircleRadius Exactly.
   */
  void updateByLineOnCenterCircle(const RegisteredLine& line, float centerCircleRadius);

  /** Update the estimate based on a virtual direct measurement of the own pose,
   *  which can be computed by complex field elements such as a center circle together with the halfway line.
   * @param pose The computed pose
   */
  void updateByPose(const RegisteredAbsolutePoseMeasurement& pose);
};
