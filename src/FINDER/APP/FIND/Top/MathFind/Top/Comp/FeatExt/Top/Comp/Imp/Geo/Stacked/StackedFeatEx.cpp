/*
 * NumVerticallyStackedBlobs.cpp
 *
 *  Created on: Nov 13, 2016
 *      Author: jake
 */

#include <StackedFeatExt.h>

#include <StackedDesc.h>
#include <BlobDataGrid.h>
#include <BlobData.h>
#include <StackedData.h>
#include <Direction.h>
#include <DoubleFeature.h>
#include <M_Utils.h>
#include <Utils.h>

#include <allheaders.h>

#include <stddef.h>
#include <assert.h>

//#define DBG_SHOW_STACKED_FEATURE
//#define DBG_SHOW_INDIVIDUAL_STACKED_FEATURE
//#define DBG_STACKED_FEATURE_ALOT
//#define DBG_DISPLAY

NumVerticallyStackedBlobsFeatureExtractor
::NumVerticallyStackedBlobsFeatureExtractor(
    NumVerticallyStackedBlobsFeatureExtractorDescription* const description,
    FinderInfo* const finderInfo)
: wordConfidenceThresh(Utils::getCertaintyThresh()) {
  this->description = description;
  this->stackedDirPath =
      Utils::checkTrailingSlash(
          finderInfo->getFinderTrainingPaths()->getFeatureExtDirPath())
            + "VertStacked/";
}

void NumVerticallyStackedBlobsFeatureExtractor::doPreprocessing(BlobDataGrid* const blobDataGrid) {
  blobDataKey = findOpenBlobDataIndex(blobDataGrid);

  // Go ahead and extract the feature for each blob in the grid
  BlobDataGridSearch gridSearch(blobDataGrid);
  gridSearch.StartFullSearch();
  BlobData* blob = NULL;
  while((blob = gridSearch.NextFullSearch()) != NULL) {

    // Create data entry for this feature extractor to add to the blob's variable data array
    NumVerticallyStackedBlobsData* const data = new NumVerticallyStackedBlobsData();

    // Add the data to the blob's variable data array
    assert(blobDataKey == blob->appendNewVariableData(data));


    if(data->hasBeenProcessed()) // this is probably no longer relevant but keeping anyway for now
      continue;

    const int stacked_count =
        countStacked(blob, blobDataGrid, BlobSpatial::UP)
        + countStacked(blob, blobDataGrid, BlobSpatial::DOWN);

    data->setHasBeenProcessed(true); // probably not relevant but keeping for now

    data->setStackedBlobsCount(stacked_count);

    data->appendExtractedFeature(
        new DoubleFeature(
            description,
            M_Utils::expNormalize(
                (double)stacked_count)));
  }
#ifdef DBG_SHOW_STACKED_FEATURE
  gridSearch.StartFullSearch();
  Pix* dbgim2 = pixCopy(NULL, blobDataGrid->getBinaryImage());
  dbgim2 = pixConvertTo32(dbgim2);
  while((blob = gridSearch.NextFullSearch()) != NULL) {
    NumVerticallyStackedBlobsData* const curBlobData = (NumVerticallyStackedBlobsData*)(blob->getVariableDataAt(blobDataKey));
    if(curBlobData->getStackedBlobsCount() == 0)
      continue;
    if(curBlobData->getStackedBlobsCount() < 0) {
      std::cout << "ERROR: stacked character count < 0 >:-[\n";
      assert(false);
    }
    LayoutEval::Color color;
    if(curBlobData->getStackedBlobsCount() == 1)
      color = LayoutEval::RED;
    if(curBlobData->getStackedBlobsCount() == 2)
      color = LayoutEval::GREEN;
    if(curBlobData->getStackedBlobsCount() > 2)
      color = LayoutEval::BLUE;
    M_Utils::drawHlBlobDataRegion(blob, dbgim2, color);
  }
  if(!Utils::existsDirectory(stackedDirPath)) {
    Utils::exec(std::string("mkdir -p " + stackedDirPath));
  }
  pixWrite((stackedDirPath + blobDataGrid->getImageName()).c_str(), dbgim2, IFF_PNG);
#ifdef DBG_DISPLAY
  pixDisplay(dbgim2, 100, 100);
  M_Utils::waitForInput();
#endif
  pixDestroy(&dbgim2);

#endif

}

std::vector<DoubleFeature*> NumVerticallyStackedBlobsFeatureExtractor::extractFeatures(BlobData* const blobData) {
  // Already did the extraction during preprocessing, so just return result
  return blobData->getVariableDataAt(blobDataKey)->getExtractedFeatures();
}

int NumVerticallyStackedBlobsFeatureExtractor::countStacked(BlobData* const blob,
    BlobDataGrid* const blobDataGrid, const BlobSpatial::Direction dir) {
  int count = 0;
  // if the blob belongs to a word Tesseract found to be 'valid' and with high enough
  // confidence then the feature is zero
#ifdef DBG_STACKED_FEATURE_ALOT
  //int left=1773, top=1243, right=1810, bottom=1210;
  //TBOX box(left, bottom, right, top);
#endif

  if((blob->belongsToRecognizedWord()
      || blob->belongsToRecognizedNormalRow())
      && blob->getWordRecognitionConfidence() > wordConfidenceThresh) {
#ifdef DBG_STACKED_FEATURE_ALOT
      //if(blob->bounding_box() == box) {
        std::cout << "showing a blob which is part of a valid word or 'normal' row, "
            "and meets the confidence threshold necessary to be considered "
            << "ineligible for having stacked elements. Its stacked feature is being fixed to zero.\n";
        M_Utils::dbgDisplayBlob(blob);
      //}
#endif
    return 0;
  }

  // go to first element above or below depending on the direction
  BlobDataGridSearch vsearch(blobDataGrid);
  vsearch.StartVerticalSearch(blob->getBoundingBox().left(), blob->getBoundingBox().right(),
      (dir == BlobSpatial::UP) ? blob->getBoundingBox().top() : blob->getBoundingBox().bottom());
  BlobData* const central_blob = blob;

  // Look up this feature's data entry for the current blob
  NumVerticallyStackedBlobsData* const data = (NumVerticallyStackedBlobsData*)blob->getVariableDataAt(blobDataKey);

  GenericVector<BlobData*>& stacked_blobs = data->getStackedBlobs();
  BlobData* stacked_blob = vsearch.NextVerticalSearch((dir == BlobSpatial::UP) ? false : true);
  BlobData* prev_stacked_blob = central_blob;
  while(true) {
    while(stacked_blob->getBoundingBox() == prev_stacked_blob->getBoundingBox()
        || ((dir == BlobSpatial::UP) ? (stacked_blob->getBoundingBox().bottom() < prev_stacked_blob->getBoundingBox().top())
            : (stacked_blob->getBoundingBox().top() > prev_stacked_blob->getBoundingBox().bottom()))
        || stacked_blob->getBoundingBox().left() >= prev_stacked_blob->getBoundingBox().right()
        || stacked_blob->getBoundingBox().right() <= prev_stacked_blob->getBoundingBox().left()
        || stacked_blobs.binary_search(stacked_blob)) {
      stacked_blob = vsearch.NextVerticalSearch((dir == BlobSpatial::UP) ? false : true);
      if(stacked_blob == NULL)
        break;
    }
    if(stacked_blob == NULL) {
#ifdef DBG_STACKED_FEATURE_ALOT
      //if(blob->bounding_box() == box) {
        std::cout << "showing a blob which has no more adjacent blobs and " << count << " stacked elements\n";
        M_Utils::dbgDisplayBlob(blob);
      //}
#endif
      break; // break the chain
    }
    if((stacked_blob->belongsToRecognizedWord()
        || stacked_blob->belongsToRecognizedNormalRow())
        && stacked_blob->getWordRecognitionConfidence() > wordConfidenceThresh) {
      // if it's part of a valid word then it's discarded here
#ifdef DBG_STACKED_FEATURE_ALOT
      //if(blob->bounding_box() == box) {
        std::cout << "showing a blob whose stacked element belongs to a valid word or normal row and thus can't be stacked\n";
        M_Utils::dbgDisplayBlob(blob);
        std::cout << "showing the candidate which can't be stacked\n";
        M_Utils::dbgDisplayBlob(stacked_blob);
      //}
#endif
      break; // break the chain
    }
    // found the element above/below the prev_stacked_blob.
    // is it adjacent based on the prev_stacked blob's location and central blob's height?
    TBOX central_bb = central_blob->getBoundingBox();
    if(isAdjacent(stacked_blob, prev_stacked_blob, dir, false, &central_bb)) {
#ifdef DBG_SHOW_INDIVIDUAL_STACKED_FEATURE
      //int dbgall = true;
      //if(blob->bounding_box() == box || dbgall) {
        std::cout << "showing the blob being measured to have " << count+1 << " stacked items\n";
        M_Utils::dbgDisplayBlob(blob);
        std::cout << "showing the stacked blob\n";
        std::cout << "here is the stacked blob's bottom and top y coords: "
             << stacked_blob->bottom() << ", " << stacked_blob->top() << std::endl;
        std::cout << "here is the bottom and top y coords of the blob being measured: "
             << central_blob->bottom() << ", " << central_blob->top() << std::endl;
        std::cout << "here is the bottom and top y coords of the current blob above/below the stacked blob: "
             << prev_stacked_blob->bottom() << ", " << prev_stacked_blob->top() << std::endl;
        M_Utils::dbgDisplayBlob(stacked_blob);
      //}
#endif
      ++count;
      prev_stacked_blob = stacked_blob;
      assert(prev_stacked_blob->getBoundingBox() == stacked_blob->getBoundingBox());
      stacked_blobs.push_back(stacked_blob);
      stacked_blobs.sort();
    }
    else {
#ifdef DBG_STACKED_FEATURE_ALOT
      //if(blob->bounding_box() == box) {
        std::cout << "showing a blob which has no (or no more) stacked features\n";
        M_Utils::dbgDisplayBlob(blob);
        std::cout << "showing the candidate which was not adjacent\n";
        M_Utils::dbgDisplayBlob(stacked_blob);
      //}
#endif
      break;
    }
  }
  return count;
}

bool NumVerticallyStackedBlobsFeatureExtractor::isAdjacent(
    BlobData* const neighbor, BlobData* const curblob,
    const BlobSpatial::Direction dir, const bool seg_mode, TBOX* dimBox) {
//  if(seg_mode) {
//    if((neighbor->getParentChar() == NULL &&
//        curblob->getParentChar() != NULL) ||
//        (neighbor->getParentChar() != NULL &&
//        curblob->getParentChar() == NULL)) {
//      return false;
//    }
//  }
  TBOX cb_boundbox = curblob->getBoundingBox();
  if(dimBox == NULL)
    dimBox = &cb_boundbox;
  double cutoff = (double)dimBox->area() / (double)16;
  if(seg_mode) {
    cutoff *= 2; // less restrictive in segmentation mode
  }
  if((double)(neighbor->getBoundingBox().area()) < cutoff) {
    //std::cout << "isAdjacent below cutoff!!!!!!\n";
    return false;
  }
  bool vert = (dir == BlobSpatial::UP || dir == BlobSpatial::DOWN) ? true : false;
  bool ascending = (vert ? ((dir == BlobSpatial::UP) ? true : false)
      : ((dir == BlobSpatial::RIGHT) ? true : false));
  double dist_thresh = (double)(vert ? dimBox->height() : dimBox->width());
  double thresh_param = 2;
  if(seg_mode) {
    thresh_param = 1; // less restrictive in segmentation mode
  }
  dist_thresh /= thresh_param;
  double distop1, distop2; // dist = distop1 - distop2
  if(vert) {
    distop1 = ascending ? (double)neighbor->bottom() : (double)curblob->bottom();
    distop2 = ascending ? (double)curblob->top() : (double)neighbor->top();
  }
  else {
    distop1 = ascending ? (double)neighbor->left() : (double)curblob->left();
    distop2 = ascending ? (double)curblob->right() : (double)neighbor->right();
  }
  assert(distop1 >= distop2);
  double dist = distop1 - distop2;
  if(dist <= dist_thresh) {
    return true;
  }
  //std::cout << "is adjacent... tooo far away!!!!!!!!!\n";
  return false;
}

void NumVerticallyStackedBlobsFeatureExtractor::doSegmentationInit(BlobDataGrid* blobDataGrid) {
  if(blobDataKey < 0) {
    blobDataKey = findOpenBlobDataIndex(blobDataGrid);
  }
}

NumVerticallyStackedBlobsData* NumVerticallyStackedBlobsFeatureExtractor::getBlobFeatureData(BlobData* const blobData) {
  if(blobDataKey < 0) {
    return NULL;
  }
  return (NumVerticallyStackedBlobsData*)(blobData->getVariableDataAt(blobDataKey));
}

BlobFeatureExtractorDescription* NumVerticallyStackedBlobsFeatureExtractor::getFeatureExtractorDescription() {
  return description;
}

