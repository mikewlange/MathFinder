/*
 * TrainingMenu.cpp
 *
 *  Created on: Dec 14, 2016
 *      Author: jake
 */

#include <TrainingMenu.h>

#include <MainMenu.h>
#include <AboutTrainingMenu.h>
#include <NameMenu.h>
#include <FeatSelMenuMain.h>
#include <DetMenu.h>
#include <SegMenu.h>
#include <DatasetMenu.h>
#include <DoTrainingMenu.h>

#include <string>
#include <iostream>

TrainingMenu::TrainingMenu(MainMenu* back) {
  NameSelectionMenu* nameSelectionMenu = new NameSelectionMenu(this);
  FeatureSelectionMenuMain* featureSelectionMenu = new FeatureSelectionMenuMain(this);
  DetectorSelectionMenu* detectorSelectionMenu = new DetectorSelectionMenu(this);
  SegmentorSelectionMenu* segmentorSelectionMenu = new SegmentorSelectionMenu(this);
  DatasetSelectionMenu* datasetSelectionMenu = new DatasetSelectionMenu(this);
  this->subMenus.push_back(new AboutTrainingMenu(this));
  this->subMenus.push_back(nameSelectionMenu);
  this->subMenus.push_back(featureSelectionMenu);
  this->subMenus.push_back(detectorSelectionMenu);
  this->subMenus.push_back(segmentorSelectionMenu);
  this->subMenus.push_back(datasetSelectionMenu);
  this->subMenus.push_back(
      new DoTrainingMenu(
          this,
          nameSelectionMenu,
          featureSelectionMenu,
          detectorSelectionMenu,
          segmentorSelectionMenu,
          datasetSelectionMenu));
  this->subMenus.push_back(back);

  this->nameMenu = nameSelectionMenu;
}

TrainingMenu::~TrainingMenu() {
  // delete all except the main menu (that happens last)
  for(int i = 0; i < subMenus.size(); ++i) {
    if(subMenus[i]->getName() != MainMenu::MainMenuName()) {
      delete subMenus[i];
    }
  }
  subMenus.clear();
}

std::string TrainingMenu::getName() const {
  return TrainingMenuName();
}

std::string TrainingMenu::TrainingMenuName() {
  return "Training Menu";
}

void TrainingMenu::doTask() {
  /**
   * Get the finder name
   */
  if(nameMenu->getFinderName() == "") {
    nameMenu->promptSetFinderName();
  }
  std::cout << "The finder you are training is named "
      << nameMenu->getFinderName() << ".\n";

  std::cout << "You have selected to train a math expression finder. "
      << "Select from the following options:\n";
}


