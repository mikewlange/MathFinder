/*
 * FeatureSelectionMenuMain.h
 *
 *  Created on: Dec 14, 2016
 *      Author: jake
 */

#ifndef FEATURESELECTIONMENUMAIN_H_
#define FEATURESELECTIONMENUMAIN_H_

#include <MenuBase.h>

#include <string>
#include <vector>

class MainMenu;
class TrainingMenu;
class SpatialFeatureMenu;
class RecognitionFeatureMenu;
class BlobFeatureExtractorFactory;
class FinderInfo;

class FeatureSelectionMenuMain: public virtual MenuBase {

 public:

  FeatureSelectionMenuMain(
      TrainingMenu* const trainingMain,
      MainMenu* const mainMenu);

  ~FeatureSelectionMenuMain();

  std::string getName() const;

  /**
   * Combine the selected factories from all menus into a single list and return it
   */
  std::vector<BlobFeatureExtractorFactory*> getSelectedFactories();

  /**
   * Selects all features (added for testing/debugging purposes)
   */
  void selectAllFeatures();

 private:

  SpatialFeatureMenu* spatialMenu;
  RecognitionFeatureMenu* recognitionMenu;
};


#endif /* FEATURESELECTIONMENUMAIN_H_ */
