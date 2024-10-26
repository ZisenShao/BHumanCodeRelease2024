/**
 * @file GoalPostsPerceptor.h
 *
 * This file declares a module that detects goal post candidates which are then validated by a neural net.
 * The lowest center point of a goal posts will then be marked by a neural net.
 *
 * Goal post candidates are generated by utilizing white ColorScanLineRegionsHorizontal.
 * ScanLineRegions that are on top of field lines or too far away from any actual goal post are filtered out.
 *
 * @author Laurens Schiefelbein
 */

#pragma once

#include "Representations/Configuration/FieldDimensions.h"
#include "Representations/Infrastructure/FrameInfo.h"
#include "Representations/Modeling/RobotPose.h"
#include "Representations/Perception/FieldPercepts/FieldLines.h"
#include "Representations/Perception/GoalPercepts/GoalPostsPercept.h"
#include "Representations/Perception/ImagePreprocessing/CameraMatrix.h"
#include "Representations/Perception/ImagePreprocessing/ECImage.h"
#include "Representations/Perception/ImagePreprocessing/ColorScanLineRegions.h"
#include "Representations/Perception/ImagePreprocessing/ImageCoordinateSystem.h"
#include "Representations/Perception/MeasurementCovariance.h"
#include "Framework/Module.h"
#include <CompiledNN/CompiledNN.h>
#include <CompiledNN/Model.h>

MODULE(GoalPostsPerceptor,
{,
  REQUIRES(CameraInfo),
  REQUIRES(CameraMatrix),
  REQUIRES(ColorScanLineRegionsHorizontal),
  REQUIRES(ECImage),
  REQUIRES(FieldDimensions),
  REQUIRES(FieldLines),
  REQUIRES(FrameInfo),
  REQUIRES(ImageCoordinateSystem),
  REQUIRES(MeasurementCovariance),
  REQUIRES(RobotPose),
  PROVIDES(GoalPostsPercept),
  DEFINES_PARAMETERS(
  {,
    /** Color of the goal frame */
    (ScanLineRegion::Color)(ScanLineRegion::Color::white) goalFrameColor,

    /** Minimal horizontal size of a ScanLineRegion that should qualify as a goal post candidate*/
    (unsigned char)(30) minimalRegionSizeX,

    /** Minimum percentage a region should overlap with another to be combines to one */
    (int)(10) minimumLappedRegionPercentage,

    /** Number of pixels to expand a ScanLineRegion in the upward direction. */
    (int)(150) regionExtensionUpwards,

    /** Number of pixels to expand a ScanLineRegion in the downward direction. */
    (int)(40) regionExtensionDownwards,

    /** Number of pixels to expand a ScanLineRegion in the sideway direction*/
    (int)(40) regionExtensionSideways,

    /** Max distance a goal post candidate can have to the robot to be considered for classification */
    (float)(2500) maxDistanceToCandidate,

    /** Max distance a ScanLineRegion can have to an actual goal post to be valid.*/
    (float)(500) maxDistanceFromGoalToRegion,

    /** Size of the square patch of the goal post. */
    (int)(32) patchSize,

    /** Minimum threshold for a candidate to be classified as a goal post */
    (float)(0.82) minClassificationThreshold,

    /** Maximum number of candidates that can be classfied in a single frame. (to limit runtime in case of disaster) */
    (int)(15) candidateLimit,
  }),
});

class GoalPostsPerceptor : public GoalPostsPerceptorBase
{
public:
  GoalPostsPerceptor();

private:

  /** Goal post positions (in field coordinates) */
  const Vector2f goalPostOwnLower = Vector2f(theFieldDimensions.xPosOwnGoalPost, theFieldDimensions.yPosRightGoal);
  const Vector2f goalPostOwnUpper = Vector2f(theFieldDimensions.xPosOwnGoalPost, theFieldDimensions.yPosLeftGoal);
  const Vector2f goalPostOpponentLower = Vector2f(theFieldDimensions.xPosOpponentGoalPost, theFieldDimensions.yPosRightGoal);
  const Vector2f goalPostOpponentUpper = Vector2f(theFieldDimensions.xPosOpponentGoalPost, theFieldDimensions.yPosLeftGoal);

  NeuralNetwork::CompiledNN classifier;
  NeuralNetwork::CompiledNN detector;
  std::unique_ptr<NeuralNetwork::Model> classifier_model;
  std::unique_ptr<NeuralNetwork::Model> detector_model;

  /** Struct that represents a goal post candidate as a rectangle inside the image. */
  struct GoalPostRegion
  {
    Vector2f upperLeft; /** Upper left point of the rectangle */
    Vector2f lowerRight; /** Lower right point of the rectangle */

    GoalPostRegion() : upperLeft(0, 0), lowerRight(0, 0) {};
    GoalPostRegion(const float x1, const float y1, const float x2, const float y2) : upperLeft(x1, y1), lowerRight(x2, y2) {}
  };

  /**
   * Struct that holds a scan line region and it's x-position
   * in the image as that information is not part of ScanLineRegion.
   */
  struct RegionWithPosition
  {
    short yPosition; /** Y-position of the scanline. */
    const ScanLineRegion& region; /** The region itself. */

    RegionWithPosition(const short yPosition, const ScanLineRegion& region) : yPosition(yPosition), region(region) {}
  };

  /**
   * Tries to find the best goal frame candidate in the current image and refreshes the GoalPostsPercept representation.
   * @param goalPostPercept Representation to update.
   */
  void update(GoalPostsPercept& goalPostPercept) override;

  /**
   * Return a vector of RegionWithPosition extracted from scanlines, but only includes white scanlines.
   * @return Vector of white scanline regions.
   */
  std::vector<GoalPostsPerceptor::RegionWithPosition> getRegionsFromScanlines();

  /**
   * Expands every region in the given vector to a GoalPostRegion to create many overlapping regions over the goal posts.
   * @param scanLineRegions Regions to expand.
   * @return Returns a vector of expanded GoalPostRegions.
   */
  std::vector<GoalPostsPerceptor::GoalPostRegion> expandRegions(const std::vector<GoalPostsPerceptor::RegionWithPosition>& scanLineRegions);

  /**
   * Combines overlapping GoalPostRegions
   * @param regionList List of GoalPostRegions to combine.
   * @return Vector of combined GoalPostRegions that don't overlap.
   */
  std::vector<GoalPostsPerceptor::GoalPostRegion> combineOverlappingRegions(const std::vector<GoalPostRegion>& regionList);

  /**
   * Extracts patches from a list of GoalPostRegions and detects the base of the goal post.
   * Alternatively, this function can also be used to extract patches for a training dataset.
   * @param regionList List of GoalPostRegions to generate patches from
   * @return Vector of coordinates for the base of every recognized goal post.   */
  std::vector<Vector2f> generatePatch(const std::vector<GoalPostRegion>& regionList);

  /**
   * Classifies goal post by using a neural net
   * @param patch goal post candidate
   * @return True if candidate is goal post, False else
   */
  bool classifyGoalPost(const Image<PixelTypes::GrayscaledPixel>& patch);

  /**
   * Detects the base of the goal post by using a neural net.
   * @param patch patch of goal post (verified by classifier)
   * @param pInImg center point of patch
   * @param inputSize inputSize of patch
   * @return image coordinates of goal post base.
   */
  Vector2f getGoalPostBase(const Image<PixelTypes::GrayscaledPixel>& patch, const Vector2f& pInImg, const int inputSize);

  /**
   * Checks if a FieldLine is inside the given ScanLineRegion
   * @param region The region ScanLineRegion to be checked.
   * @param y The Y-Position of the ScanLineRegion
   * @return true if line is inside region else false
   */
  bool isLineInRegion(const ScanLineRegion& region, const unsigned short y);

  /**
   * Checks if an overlap between two GoalPostRegion of at least minimumLappedRegionPercentage is present.
   * @param region1,region2 Region to check for an overlap.
   * @return True if a significant overlap is present. False else.
   */
  bool isOverlapPresent(const GoalPostRegion& region1, const GoalPostRegion& region2);

  /**
   * Checks if a given ScanLineRegion is close to a goal post (using localization)
   * The distance is defined by maxDistanceFromGoalToRegion
   * @param region The region ScanLineRegion to be checked.
   * @param y The Y-Position of the ScanLineRegion
   * @return true if region is close to goal post, else false
   */
  bool isCloseToGoalPost(const ScanLineRegion& region, const unsigned short y);

  /**
   * Calculates area of a given rectangle
   * @param x1,y1,x2,y2 The corner points of the rectangle
   * @return The calculated area of the rectangle
   */
  float getAreaOfRectangle(const float x1, const float y1, const float x2, const float y2);

  /**
   * Checks whether the center point of a square patch of given patch size is within the camera bounds
   * @param point The center point of the patch to check (in image coordinates)
   * @param patchSize The size of the patch
   * @return True if the center point is within the camera bounds. Else false
   */
  bool isWithinBounds(const Vector2f& point, const int patchSize);
};