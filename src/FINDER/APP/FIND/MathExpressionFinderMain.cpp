/*
 * MathExpressionFinderMain.cpp
 *
 *  Created on: Oct 30, 2016
 *      Author: jake
 */

#include <MathExpressionFinderMain.h>

#include <MathExpressionFinder.h>
#include <MFinderProvider.h>
#include <Utils.h>
#include <MainMenu.h>
#include <Usage.h>
#include <FinderInfo.h>
#include <InfoFileParser.h>

#include <allheaders.h> // leptonica

#include <string>

/**
 * Application point of entry
 */
int main(int argc, char* argv[]) {

//  {
//    std::string datasetDirPath;
//    std::cout << "\nDerp\n";
//    std::cin >> datasetDirPath;
//    std::cout << "got " << datasetDirPath << std::endl;
//    datasetDirPath = "";
//    std::cout << "\nEnter the full path to a directory containing the images you would like to train on "
//        << "as well as a file with the extension of '.rect' which contains the groundtruth bounding boxes "
//        "for where the math regions are in the images. The images should be named as follows assuming "
//        << "they are in .png format (images in most any format are supported): 0.png, 1.png, 2.png, etc...: \n\n" << std::endl;
//    Utils::getline(datasetDirPath);
//    std::cout << "The line I got " << datasetDirPath << std::endl;


//    TessBaseAPI api;
//    Pix* p = Utils::leptReadImg(FinderTrainingPaths::getGroundtruthRoot() + std::string("advcalc1/0.png"));
//    pixDisplay(p, 100, 100);
//    M_Utils::waitForInput();
//    BlobDataGrid* g =
//        BlobDataGridFactory().createBlobDataGrid(p, &api, "0.png");

  }

  if(argc == 2) {
    if(std::string(argv[1]) == std::string("-m")) { // interactive menu
      runInteractiveMenu();
    } else {
      runFinder(argv[1]);
    }
  } else {
    MathExpressionFinderUsage::printUsage();
  }
}

void runInteractiveMenu() {
  MainMenu* const mainMenu = new MainMenu();
  MenuBase* menuSelected = (MenuBase*)mainMenu;
  while(menuSelected->isNotExit()) {
    menuSelected = menuSelected->getNextSelected();
  }
  delete mainMenu;
}

void runFinder(char* path) {

  const std::string trainedFinderPath =
      FinderTrainingPaths::getTrainedFinderRoot();
  Utils::exec(std::string("mkdir -p ") + trainedFinderPath, true);
  std::vector<std::string> trainedFinders =
      Utils::getFileList(trainedFinderPath);

  if(trainedFinders.empty()) {
    std::cout << "There is currently no trained MathFinder available on the system. "
        << "Loading the interactive menu.\n";
    return runInteractiveMenu();
  }

  std::string finderName;
  if(trainedFinders.size() > 1) {
    std::cout << "More than one MathFinder has been trained on this system. Select one of the following."
        << " To see more information run this program in the interactive menu mode (i.e., MathFinder -menu):\n";
    finderName = trainedFinders[Utils::promptSelectStrFromLabeledMatrix(trainedFinders, 2)];
  } else {
    finderName = trainedFinders[0];
  }

  FinderInfo* finderInfo =
      TrainingInfoFileParser().readInfoFromFile(finderName);

  std::string imagePath = std::string(path);
  Pixa* images = pixaCreate(0);
  // if the image path is a directory, then read in all of the files in that
  // directory (assumes they are images)
  if(Utils::existsDirectory(imagePath)) {
    std::vector<std::string> imageNames = Utils::getFileList(imagePath);
    for(int i = 0; i < imageNames.size(); ++i) {
      pixaAddPix(images, Utils::leptReadImg(imagePath), L_INSERT);
    }
  } else if(Utils::existsFile(imagePath)) {
    pixaAddPix(images, Utils::leptReadImg(imagePath), L_INSERT);
  } else {
    std::cout << "Unable to read in the image(s) on the given path." << endl;
    return MathExpressionFinderUsage::printUsage();
  }

  GeometryBasedExtractorCategory spatialCategory;
  RecognitionBasedExtractorCategory recognitionCategory;
  MathExpressionFinder* finder =
      MathExpressionFinderProvider().createMathExpressionFinder(
          &spatialCategory,
          &recognitionCategory,
          finderInfo);

  std::vector<MathExpressionFinderResults*> results =
      finder->findMathExpressions(images, Utils::getFileList(imagePath));

  pixaDestroy(&images); // destroy finished image(s)

  // Destroy the finder
  delete finder;
  delete finderInfo;

  // Display the results and/or write to path
  for(int i = 0; i < results.size(); ++i) {
    results[i]->displayResults();
    results[i]->printResultsToFiles();
  }

  // Destroy results
  for(int i = 0; i < results.size(); ++i) {
    delete results[i];
  }
}

