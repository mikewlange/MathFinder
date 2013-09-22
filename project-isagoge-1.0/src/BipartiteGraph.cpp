/**************************************************************************
 * Project Isagoge: Enhancing the OCR Quality of Printed Scientific Documents
 * File name:   DocumentLayoutTest.h
 * Written by:  Jake Bruce, Copyright (C) 2013
 * History:     Created Aug 5, 2013 7:51 PM
 * Description:
 * By definition a bipartite graph consists of two independent sets with edges
 * only being drawn between the the two sets (there are no edges allowed within
 * a set and of course this makes sense for our application since we are
 * comparing the hypothesis to the groundtruth, not to iteself). One set in the
 * bipartite graph will represent the hypothesis, the other represents the
 * groundtruth. Each element of the set is a rectangular portion of the image
 * and has both an area and a number of foreground pixels. A combination of
 * the area and foreground pixel count is used to measure the strength of an
 * edge between two vertices as well as penalty to be incurred by false
 * positives and false negatives.
 * ----------------------------------------------------------------------
 * This file is part of Project Isagoge.
 *
 * Project Isagoge is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Project Isagoge is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Project Isagoge.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************************************************/

#include "BipartiteGraph.h"

#include <assert.h>

#include "Basic_Utils.h"
using namespace Basic_Utils;

#define debug 0

BipartiteGraph::BipartiteGraph(string type_, GraphInput input)
: type(type_) {
  color = getColorFromType(type);
  if(color == LayoutEval::NONE) {
    cout << "ERROR: No color associated with specified type\n";
    exit(EXIT_FAILURE);
  }
  // extract all the info from the GraphInput struct
  string hypboxfilename = input.hypboxfile;
  string gtboxfilename  = input.gtboxfile;
  string imgname    = input.imgname;
  vector<string> tmp = stringSplit(imgname, '.');
  filenum = atoi(tmp[0].c_str());
  filename = imgname;
  hypimg       = input.hypimg;
  gtimg        = input.gtimg;
  inimg        = input.inimg;

  // open both of the files for reading
  gtfile.open(gtboxfilename.c_str(), ifstream::in);
  if((gtfile.rdstate() & ifstream::failbit) != 0) {
    cout << "ERROR: Could not open " << gtboxfilename << endl;
    exit(EXIT_FAILURE);
  }
  hypfile.open(hypboxfilename.c_str(), ifstream::in);
  if((hypfile.rdstate() & std::ifstream::failbit) != 0 ) {
    cout << "ERROR: Could not open " << hypboxfilename << endl;
    exit(EXIT_FAILURE);
  }

  // build and append all the vertices to the groundtruth set and
  // then the hypothesis set
  makeVertices(Bipartite::GroundTruth);
  makeVertices(Bipartite::Hypothesis);

  // build and append all of the edges to the vertices for the
  // groundtruth set and then the hypothesis set
  makeEdges(Bipartite::GroundTruth);
  makeEdges(Bipartite::Hypothesis);

  if(debug) {
    cout << "-----------------\nPrinting the Groundtruth\n---------------\n";
    printSet(Bipartite::GroundTruth);
    cout << "-----------------\nPrinting the Hypothesis\n---------------\n";
    cout << "hypothesis vertices: " << Hypothesis.size() << endl;
    printSet(Bipartite::Hypothesis);
  }
}

void BipartiteGraph::makeVertices(Bipartite::GraphChoice graph) {
  ifstream* file;
  PIX* img;
  vector<Vertex>* set;

  if(graph == Bipartite::GroundTruth) {
    file = &gtfile;
    img = gtimg;
    set = &GroundTruth;
  } else {
    file = &hypfile;
    img = hypimg;
    set = &Hypothesis;
  }
  int idx = 0;
  int max = 55;
  char* curline = new char[max];
  while(!file->eof()) {
    file->getline(curline, max);
    string curlinestr = (string)curline;
  //  if(graph == Bipartite::Hypothesis)
   //   cout << "hypline: " << curlinestr << endl;
    assert(curlinestr.length() < max);
    // parse the line
    if(curlinestr.empty())
      continue;
    vector<string> splitline = stringSplit(curlinestr);
    string filename   = splitline[0];
    string recttype   = splitline[1];
    int rectleft   = atoi(splitline[2].c_str());
    int recttop    = atoi(splitline[3].c_str());
    int rectright  = atoi(splitline[4].c_str());
    int rectbottom = atoi(splitline[5].c_str());
    vector<string> tmp = stringSplit(filename, '.');
    int curfilenum = atoi(tmp[0].c_str());

    // .dat file is very specific format where data from
    // the same file numbers are clustered together
    // (i.e. all the 1's go before the 2's etc)
    if(curfilenum < filenum)
      continue;
    if(curfilenum > filenum)
      break;
    if(recttype != type)
      continue;

    // now we know we have a vertex so build and append
    // it to the appropriate vector
    BOX* box = boxCreate(rectleft, recttop, rectright-rectleft, \
        rectbottom-recttop); // create the box
    Vertex vert; // create the vertex
    vert.rect = box; // put the box in the vertex
    // count the foreground pixels and put them in the vertex
    vert.pix_foreground = \
        lu.countColorPixels(box, img, color);
    vert.setindex = idx;
    vert.area = (rectright-rectleft)*(rectbottom-recttop);
    vert.whichset = (graph == Bipartite::Hypothesis) ?\
        (string)"Hypothesis" : (string)"GroundTruth";
    set->push_back(vert);
    idx++;
  }
}

void BipartiteGraph::makeEdges(Bipartite::GraphChoice graph) {
  vector<Vertex>* make_edges_for; // set to make the edges for
  vector<Vertex>* make_edges_from; // the set the edges will point to
  PIX* edge_for_pix;
  PIX* edge_from_pix;
  if(graph == Bipartite::GroundTruth) {
    make_edges_for = &GroundTruth;
    make_edges_from = &Hypothesis;
    edge_for_pix = gtimg;
    edge_from_pix = hypimg;
  } else {
    make_edges_for = &Hypothesis;
    make_edges_from = &GroundTruth;
    edge_for_pix = hypimg;
    edge_from_pix = gtimg;
  }
  for(vector<Vertex>::iterator vert = make_edges_for->begin(); \
      vert != make_edges_for->end(); vert++) {
    // look for intersections of pixels and add an edge for each
    // one that is found
    BOX* vertbox = vert->rect;
    for(vector<Vertex>::iterator edgevert = make_edges_from->begin(); \
        edgevert != make_edges_from->end(); edgevert++) {
      BOX* edgebox = edgevert->rect; // potential edge
      l_int32 intersects = 0;
      boxIntersects(vertbox, edgebox, &intersects);
      if(!intersects)
        continue;
      // edge found! create the edge and append it to the current
      // vertex's edge list. first need to find the number of pixels
      // that intersect as well as the area that intersects
      BOX* overlap = boxOverlapRegion(vertbox, edgebox);
      int intersect1 = lu.countColorPixels(overlap, edge_for_pix, color);
      int intersect2 = lu.countColorPixels(overlap, edge_from_pix, color);
      assert(intersect1 == intersect2);
      Edge edge;
      edge.vertexptr = &(*edgevert);
      edge.pixfg_intersecting = intersect1;
      edge.overlap_area = (int)(overlap->w) * (int)(overlap->h);
      vert->edges.push_back(edge);
    }
  }
}

void BipartiteGraph::getHypothesisMetrics() {
  getGroundTruthMetrics();
  // total segmented foreground pixels in the groundtruth
  const int gt_positive_fg_pix = gtmetrics.total_seg_fg_pixels;
  // initialize everything
  hypmetrics.total_fg_pix = gtmetrics.total_fg_pixels;
  hypmetrics.correctsegmentations = 0;
  hypmetrics.total_recall = 0;
  hypmetrics.total_fallout = 0;
  hypmetrics.total_precision = 0;
  hypmetrics.total_fdr = 0;
  hypmetrics.oversegmentations = 0;
  hypmetrics.avg_oversegmentations_perbox = 0;
  hypmetrics.undersegmentations = 0;
  hypmetrics.avg_undersegmentations_perbox = 0;
  hypmetrics.oversegmentedcomponents = 0;
  hypmetrics.undersegmentedcomponents = 0;
  hypmetrics.falsenegatives = 0;
  hypmetrics.falsepositives = 0;
  hypmetrics.negative_predictive_val = 0;
  hypmetrics.specificity = 0;
  hypmetrics.total_positive_fg_pix = 0;
  hypmetrics.total_false_negative_pix = 0;
  hypmetrics.total_false_positive_pix = 0;
  hypmetrics.total_true_positive_fg_pix = 0;
  const int gt_negative_fg_pix = gtmetrics.total_nonseg_fg_pixels;

  // need to sum up all the positive pixels detected in the entire
  // hypothesis set so we can make the precision and false discovery
  // rate calculations later
  int hyp_positive_fg_pix_tmp = 0;
  for(vector<Vertex>::iterator hyp_it = Hypothesis.begin(); \
      hyp_it != Hypothesis.end(); hyp_it++)
    hyp_positive_fg_pix_tmp += hyp_it->pix_foreground;
  const int hyp_positive_fg_pix = hyp_positive_fg_pix_tmp;

  // if there aren't any segmented regions in the groundtruth
  // then there can't be any correct segmentations and
  // we already know that all of the segemented pixels in the
  // hypothesis are false positives! If that's the case we'll
  // go ahead and set a flag to avoid any confusion later
  bool all_false_positives = false;
  if(gt_positive_fg_pix <= 0) {
    all_false_positives = true;
    // if there are boxes and none of them have foreground pixels
    // then we already know off the bat that something is seriously
    // wrong here.
    if(GroundTruth.size() > 0) {
      cout << "ERROR: None of the foreground pixels in the " \
           << "GroundTruth.dat rectangles were detected!!\n";
      exit(EXIT_FAILURE);
    }
  }

  PIX* dbg = pixCreate(gtimg->w, gtimg->h, 32);

  // go ahead and count (and color if debugging) all the true negative
  // pixels in the hypothesis
  int counted_truenegatives = 0;
  l_uint32* cur_gt_pixel;
  l_uint32* cur_hyp_pixel;
  l_uint32* cur_dbg_pixel;
  l_uint32* startpixel_gt = pixGetData(gtimg);
  l_uint32* startpixel_hyp = pixGetData(hypimg);
  l_uint32* startpixel_dbg = pixGetData(dbg);
  for(l_int32 i = 0; i < gtimg->h; i++) {
    for(l_int32 j = 0; j < gtimg->w; j++) {
      cur_gt_pixel = startpixel_gt + l_uint32((i*gtimg->w) + j);
      cur_hyp_pixel = startpixel_hyp + l_uint32((i*hypimg->w) + j);
      cur_dbg_pixel = startpixel_dbg + l_uint32((i*dbg->w) + j);
      rgbtype pixcolorgt;
      lu.getPixelRGB(cur_gt_pixel, &pixcolorgt);
      LayoutEval::Color colorgt = lu.getColor(&pixcolorgt);
      if((!lu.isColorSignificant(colorgt) && lu.isDark(&pixcolorgt))\
          // ^^checks for an actual negative in the groundtruth that is
          // simply a foreground pixel. below we check
          // for a pixel that may be significant but isn't what we are
          // looking at now (i.e. an embedded pixel when we are evaluating
          // for displayed expressions)
          || (lu.isColorSignificant(colorgt) && (colorgt != color))) {
        // negative detected from the groundtruth
        // if the hypothesis pixel here is also negative then we have
        // a true negative!
        LayoutEval::Color colorhyp = lu.getPixelColor(cur_hyp_pixel);
        if(colorgt != color && colorhyp != color) {
          // found a true negative!!!
          counted_truenegatives++;
          // set the true negative to orange in the debug image
          if(dbg) {
            composeRGBPixel(255, 127, 0, cur_dbg_pixel);
            pixSetPixel(dbg, j, i, *cur_dbg_pixel);
          }
        }
      }
    }
  }
  //pixDisplay(dbg, 100, 100);

  // iterate through the hypothesis boxes (vertices) to get
  // metrics on each individual one
  /*  cout << "the image has " << Hypothesis.size() << " hypothesis boxes.\n";
  for(vector<Vertex>::iterator hyp_it = Hypothesis.begin(); \
      hyp_it != Hypothesis.end(); hyp_it++) {
    static int ___i = 0;
    cout << "hypbox " << ___i << ": " << hyp_it->rect->x << ", " << hyp_it->rect->y \
         << ", width: " << hyp_it->rect->w << ", height: " << hyp_it->rect->h << endl;
    ___i++;
  }*/
  for(vector<Vertex>::iterator hyp_it = Hypothesis.begin(); \
      hyp_it != Hypothesis.end(); hyp_it++) {

    // initialize some parameters for the hypothesis box
    // to be used in determining the metrics for the box
    l_int32 h_x, h_y, h_w, h_h; // hypothesis box top left,
                                // width, and height
    BOX* hyp_box = hyp_it->rect;
    boxGetGeometry(hyp_box, &h_x, &h_y, &h_w, &h_h);
    int hyp_box_fg_pix = hyp_it->pix_foreground;
    vector<Edge> edges = hyp_it->edges;

    // initialize the current box's description
    // before we calculate it
    RegionDescription hyp_box_desc; // description of the current box
    hyp_box_desc.num_fg_pixels = hyp_box_fg_pix;
    hyp_box_desc.area = hyp_it->area;
    hyp_box_desc.box = hyp_it->rect;
    hyp_box_desc.vertptr = &(*hyp_it);
    hyp_box_desc.recall = 0;
    hyp_box_desc.fallout = 0;
    hyp_box_desc.precision = 0;
    hyp_box_desc.false_discovery = 0;
    hyp_box_desc.false_negative_pix = 0;
    hyp_box_desc.true_positive_pix = 0;
    hyp_box_desc.false_positive_pix = 0;
    const int num_edges = edges.size();
    hyp_box_desc.num_gt_overlap = num_edges;

    if(num_edges == 0) {
      // no intersections with the groundtruth
      // all the pixels in this region are false positives
      vector<BOX*> bv;
      BOX* b = boxCreate(0,0,0,0);
      bv.push_back(b);
      int falsepositive_pix = lu.countFalsePositives(hyp_box, \
          bv, hypimg, color, dbg);
      double fallout = (double)falsepositive_pix /\
          (double)gt_negative_fg_pix;
      hyp_box_desc.fallout = fallout;
      hypmetrics.falsepositives++;
      hyp_box_desc.false_positive_pix = falsepositive_pix;
      //cout << "false pos: " << falsepositive_pix << endl;
    }
    else if(num_edges >= 1) {
      // there is at least one region in the groundtruth
      // that intersects with this one. If there is more
      //cout << hypmetrics.total_fallout * 1173355 << endl;
      //exit(EXIT_FAILURE);    // than one then we know the region is undersegmented.
      // each overlapping groundtruth region is either a
      // correct segmentation or is only partially correct.
      // it may also have false positives and negatives as well.
      if(num_edges > 1) {
        hypmetrics.undersegmentations += num_edges;
        hypmetrics.undersegmentedcomponents++;
      }

      // first we'll count all the false positive pixels
      // in the hypothesis box and also count the number
      // of false negatives which could spring up in the
      // case that the hypothesis box does not completely
      // overlap the groundtruth one, then count the
      // true positives (pixels in both the groundtruth
      // region and the hypothesis region)
      vector<Box*> edgeboxes; // first put all of the boxes
                              // into a vector
      for(vector<Edge>::iterator edgeit = edges.begin(); \
          edgeit != edges.end(); edgeit++)
        edgeboxes.push_back(edgeit->vertexptr->rect);

      const int falsepositives = lu.countFalsePositives(hyp_box, \
          edgeboxes, hypimg, color, dbg);

      const int truepositives = lu.countTruePositives(hyp_box, \
          edgeboxes, gtimg, color, dbg);

      if((truepositives + falsepositives) != hyp_box_fg_pix) {
        cout << "ERROR: The sum of the true and false positives found in "
             << "a rectangle is not equal to the number of foreground "
             << "pixels in that region!\n";
        cout << "This occurred in image: " << filename << endl;
        cout << "Here is the sum of true and false positives: "
             << truepositives+falsepositives << endl;
        cout << "Here is the number of foreground pixels in the region: "
             << hyp_box_fg_pix << endl;
        cout << "Here is the number of true positives in the region : "
             << truepositives << endl;
        cout << "Here is the number of false positives in the region: "
             << falsepositives << endl;
        cout << "Here are the dimensions of the hypothesis region "
             << "(l,t,w,h): " << h_x << ", " << h_y << ", " << h_w
             << ", " << h_h << endl;
        exit(EXIT_FAILURE);
      }

      // now go ahead and calculate all the metrics for the box
      // we add them here for the case when the hypothesis region
      // is undersegmented (if that is the case the entire box should
      // account for the true positives, false negatives, and false
      // positives for all of the undersegmented groundtruth boxes
      hyp_box_desc.recall = (double)truepositives/\
          (double)gt_positive_fg_pix;
      hyp_box_desc.fallout = (double)falsepositives/\
          (double)gt_negative_fg_pix;
      hyp_box_desc.precision = (double)truepositives/\
          (double)hyp_positive_fg_pix;
      hyp_box_desc.false_discovery = (double)falsepositives/\
          (double)hyp_positive_fg_pix;
      hyp_box_desc.true_positive_pix = truepositives;
      hyp_box_desc.false_positive_pix = falsepositives;
    }
    else {
      cout << "ERROR: Box with negative # of edges!\n";
      exit(EXIT_FAILURE);
    }
    hypmetrics.boxes.push_back(hyp_box_desc);
  }
  if(hypmetrics.undersegmentations > 0) {
    hypmetrics.avg_undersegmentations_perbox = \
        (double)hypmetrics.undersegmentations /\
        (double)hypmetrics.undersegmentedcomponents;
  }

  // now iterate the groundtruth to catch any remaining false negative
  // regions in the hypothesis and go ahead and add them onto the
  // vector of RegionDescriptions
  // also find the regions that were oversegmented by the hypothesis
  // and include that information in the hypothesis metrics as well
  vector<OverlappingGTRegion> overlappingregions;
  for(vector<Vertex>::iterator gt_it = GroundTruth.begin(); \
      gt_it != GroundTruth.end(); gt_it++) {
    BOX* gtbox = gt_it->rect;
    vector<Edge> edges = gt_it->edges;
    const int numedges = edges.size();
    vector<Box*> hypboxes;
    for(vector<Edge>::iterator edgeit = edges.begin(); \
        edgeit != edges.end(); edgeit++)
      hypboxes.push_back(edgeit->vertexptr->rect);
    const int gt_fg_pix = gt_it->pix_foreground;
    if(numedges == 0) {
      // there are no edges thus this entire region was
      // missed completely by the hypothesis
      RegionDescription missedregion;
      missedregion.num_fg_pixels = gt_fg_pix;
      missedregion.area = gt_it->area;
      missedregion.box = gt_it->rect;
      missedregion.vertptr = &(*gt_it);
      missedregion.recall = 0;
      missedregion.fallout = 0;
      missedregion.precision = 0;
      missedregion.false_discovery = 0;
      missedregion.true_positive_pix = 0;
      missedregion.false_positive_pix = 0;
      missedregion.num_gt_overlap = 0;
      missedregion.false_negative_pix = gt_fg_pix;
      if(dbg)
        lu.fillBoxForeground(dbg, gt_it->rect, LayoutEval::GREEN, inimg);
      hypmetrics.boxes.push_back(missedregion);
      hypmetrics.falsenegatives++;
    }
    else {
      OverlappingGTRegion overlapgt;
      // check for false negatives
      int falsenegatives = lu.countFalseNegatives(gtbox, hypboxes, \
          gtimg, color, dbg);
      //cout << "partial false negatives (gt vertex "
      //     << gt_it->setindex << "): " << falsenegatives << endl;
      if(falsenegatives == 0)
        hypmetrics.correctsegmentations++;
      overlapgt.falsenegativepix = falsenegatives;
      overlapgt.box = gtbox;
      overlapgt.vertptr = &(*gt_it);
      overlapgt.numedges = numedges;
      if(numedges > 1){
        hypmetrics.oversegmentations += numedges;
        hypmetrics.oversegmentedcomponents++;
      }
      overlappingregions.push_back(overlapgt);
    }
  }
  hypmetrics.overlapgts = overlappingregions;
  if(hypmetrics.oversegmentations > 0) {
    hypmetrics.avg_oversegmentations_perbox = \
        (double)hypmetrics.oversegmentations /\
        (double)hypmetrics.oversegmentedcomponents;
  }
  // now that we have all the metrics for each rectangle it is
  // time to combine all that information in order to get the
  // total metrics for the entire image
  vector<RegionDescription> regions = hypmetrics.boxes;
  for(vector<RegionDescription>::iterator region = regions.begin(); \
      region != regions.end(); region++) {
    hypmetrics.total_recall += region->recall;
    hypmetrics.total_fallout += region->fallout;
    hypmetrics.total_precision += region->precision;
    hypmetrics.total_fdr += region->false_discovery;
    int tp = region->true_positive_pix;
    int fp = region->false_positive_pix;
    int tp_fp = tp + fp;
    hypmetrics.total_false_positive_pix += fp;
    hypmetrics.total_positive_fg_pix += tp_fp;
    int fn = region->false_negative_pix;
    hypmetrics.total_false_negative_pix += fn;
    hypmetrics.total_true_positive_fg_pix += tp;
  }

  // add any other false negatives for overlapping groundtruth
  // boxes
  for(vector<OverlappingGTRegion>::iterator ogt_it = \
      overlappingregions.begin(); ogt_it != overlappingregions.end(); \
      ogt_it++) {
    hypmetrics.total_false_negative_pix += ogt_it->falsenegativepix;
  }

  // now calculate the total negatively detected pixels by the
  // hypothesis (this is the same as TN+FN, both true negatives
  // and false negatives)
  const int total_fg = hypmetrics.total_fg_pix;
  const int total_positive_fg = hypmetrics.total_positive_fg_pix;
  const int total_hyp_negative = total_fg - total_positive_fg;
  hypmetrics.total_negative_fg_pix = total_hyp_negative;

  // the total actual negatives are known from the groundtruth
  // metrics and the true negatives found in the hypothesis can
  // be either a subset of the actual negatives or their entirety
  // if the specificity was perfect
  const int total_false_neg_pix = hypmetrics.total_false_negative_pix;
  const int hyp_true_negatives = total_hyp_negative -\
      total_false_neg_pix;
  if(hyp_true_negatives != counted_truenegatives) {
    cout << "ERROR: the total false negatives and true negatives " \
         << "counted don't add up to the total negatives " \
         << "(i.e. total_foreground - total_positive)!\n";
    cout << "hyp_true_negatives: " << hyp_true_negatives << endl;
    cout << "counted true negatives: " << counted_truenegatives << endl;
    cout << "false negatives: " << total_false_neg_pix << endl;
    cout << "false positives: " << hypmetrics.total_false_positive_pix << endl;\
    cout << "total_fg - false negatives: " << total_hyp_negative << endl;
    cout << "negatives in the groundtruth: " \
         << gtmetrics.total_nonseg_fg_pixels << endl;
    cout << "positives in the groundtruth: " \
         << gtmetrics.total_seg_fg_pixels << endl;
    pixDisplay(dbg, 100, 100);
    exit(EXIT_FAILURE);
  }
  hypmetrics.total_true_negative_fg_pix = hyp_true_negatives;
  const int gt_true_negatives = gtmetrics.total_nonseg_fg_pixels;

  // now to calculate the specificity (TN/N) and make sure it is
  // the same as 1-FPR
  // FPR = FP/N
  // SPC = TN/N
  // N/N - FP/N = TN/N -> N-FP=TN
  double specificity = (double)hyp_true_negatives/\
      (double)gt_true_negatives;

  //assert(specificity == (1.-hypmetrics.total_fallout));
  double oneminusfpr = (double)1- (double)hypmetrics.total_fallout;
  if(specificity != oneminusfpr) {
    if((gt_true_negatives - hypmetrics.total_false_positive_pix) \
        == hyp_true_negatives) {
      cout << "WARNING: The specificity and 1-FPR are not equal, however ";
      cout << "the false positive and true negative pixels add up to the ";
      cout << "total negatives in the groundtruth.\n";
      cout.precision(20);
      cout << "The specificity and 1-FPR are only off by " \
           << specificity-oneminusfpr << endl;
      // something weird happened with division... if the false positives
      // and true negatives add up then there should be nothing wrong!!
      goto noerror;
    }
    cout << "ERROR:: for some reason the specificity does not equal 1-FPR!!!\n";
    cout << "image: " << filename << endl;
    cout.precision(20);
    cout << "specificity: " << specificity << endl;
    cout << "1-FPR      : " << oneminusfpr << endl;
    cout << "specificity is " << (double)hyp_true_negatives \
         << " / " << (double)gt_true_negatives << endl;
    cout << "1-FPR is " << (double)1 << " - " \
         << (double)hypmetrics.total_fallout << endl;
    cout << "FPR         : " << hypmetrics.total_fallout << endl;
    cout << "Expected FPR: " << (double)hypmetrics.total_false_positive_pix /\
        (double)gt_true_negatives << endl;
    cout << "False positives in hypothesis: " << hypmetrics.total_false_positive_pix << endl;
    cout << "True positives in hypothesis: " << hypmetrics.total_true_positive_fg_pix << endl;
    cout << "true negatives in the hypothesis: " << hyp_true_negatives << endl;
    cout << "true negatives in the groundtruth: " << gt_true_negatives << endl;
    cout << "gt_negative_fg_pix: " << gt_negative_fg_pix << endl;
    cout << "false negatives in the hypothesis: " << total_false_neg_pix << endl;
    cout << "positives in the hypothesis: " << total_positive_fg << endl;
    cout << "positives in the groundtruth: " << gtmetrics.total_seg_fg_pixels << endl;
    cout << "negatives in the hypothesis: " << total_hyp_negative << endl;
    // try drawing all the negatives on the image for debugging
    BOX* imbox = boxCreate(0, 0, hypimg->w, hypimg->h);
    vector<Box*> hypboxes;
    for(vector<RegionDescription>::iterator regionit = hypmetrics.boxes.begin(); \
        regionit != hypmetrics.boxes.end(); regionit++) {
      hypboxes.push_back(regionit->box);
    }
    Pix* allnegim = pixCreate(gtimg->w, gtimg->h, 32);
    int negatives = lu.countFalseNegatives(imbox, hypboxes, gtimg, color, allnegim);
    pixWrite(((string)"Eval_DBG_allnegative_" + type + (string)"_" + filename).c_str(),\
         allnegim, IFF_PNG);
    cout << "after counting all the regions not in the hypothesis boxes we have "
         << negatives << " total false negatives in the hypothesis\n";
    cout << "negatives in the groundtruth: " << gtmetrics.total_nonseg_fg_pixels << endl;
    cout << "total foreground pixels in the hypothesis: " << total_fg << endl;
    cout << "total foreground pixels in the groundtruth: " << gtmetrics.total_fg_pixels << endl;
    cout << "here is the sum of the false positives and the true negatives in hypothesis: " \
         << hyp_true_negatives + hypmetrics.total_false_positive_pix << endl;
    int diff = hyp_true_negatives + hypmetrics.total_false_positive_pix - gt_true_negatives;
    cout << "there are " << diff << " more true negatives than there are supposed to be in the hyp!!\n";
    specificity = (1.-hypmetrics.total_fallout);
    exit(EXIT_FAILURE); // (the above solution is a no more than a band-aid... need to figure it out..)
  }
  noerror:
  hypmetrics.specificity = specificity;

  // now calculate the negative predictive value (TN/TN+FN)
  double npv = (double)hyp_true_negatives/(double)total_hyp_negative;
  hypmetrics.negative_predictive_val = npv;

  // now calculate the accuracy
  const int tp = hypmetrics.total_true_positive_fg_pix;
  const int tn = hyp_true_negatives;
  const int p = gtmetrics.total_seg_fg_pixels;
  const int n = gt_true_negatives;
  assert(p+n == hypmetrics.total_fg_pix);
  double accuracy = ((double)tp+(double)tn)/((double)p+(double)n);
  hypmetrics.accuracy = accuracy;
  pixWrite(((string)"Eval_DBG_" + type + (string)"_" + filename).c_str(),\
      dbg, IFF_PNG);
  pixDestroy(&dbg);
}

void BipartiteGraph::getGroundTruthMetrics() {
  // first initialize everything
  gtmetrics.segmentations = 0;
  gtmetrics.total_seg_fg_pixels = 0;
  gtmetrics.fg_pixel_ratio = 0;
  gtmetrics.total_seg_area = 0;
  gtmetrics.total_area = (l_int32)gtimg->w * (l_int32)gtimg->h;
  gtmetrics.area_ratio = 0;
  // get all the metrics by iterating through the groundtruth
  // vertices. first need to get the totals
  for(vector<Vertex>::iterator gt_it = GroundTruth.begin(); \
      gt_it != GroundTruth.end(); gt_it++) {
    BOX* gtbox = gt_it->rect;
    gtmetrics.total_seg_fg_pixels += \
        lu.countColorPixels(gtbox, gtimg, color);
    gtmetrics.total_seg_area += (int)gtbox->w * (int)gtbox->h;
    gtmetrics.segmentations++;
  }
  // get the ratio of the total segmented foreground pixels
  // to the total foreground pixels
  BOX* fullimgbox = boxCreate(0, 0, (l_int32)gtimg->w, (l_int32)gtimg->h);
  gtmetrics.total_fg_pixels = lu.countColorPixels(fullimgbox, \
      gtimg, color, true);
  gtmetrics.total_nonseg_fg_pixels = gtmetrics.total_fg_pixels \
      - gtmetrics.total_seg_fg_pixels;
  gtmetrics.fg_pixel_ratio = (double)gtmetrics.total_seg_fg_pixels / \
      (double)gtmetrics.total_fg_pixels;
  // get the ratio of the total segmented area to the
  // image's total area
  gtmetrics.area_ratio = (double)gtmetrics.total_seg_area / \
      (double)gtmetrics.total_area;
  // now get the ratios for the area and foreground pixels of each
  // individual box to that of the summation for all the regions
  // that were segmented
  for(vector<Vertex>::iterator gt_it = GroundTruth.begin(); \
      gt_it != GroundTruth.end(); gt_it++) {
    GTBoxDescription boxdesc;
    int area = gt_it->area;
    int total_seg_area = gtmetrics.total_seg_area;
    boxdesc.area_ratio = (double)area / (double)total_seg_area;
    int fgpixels = gt_it->pix_foreground;
    int total_seg_fg = gtmetrics.total_seg_fg_pixels;
    boxdesc.fg_pix_ratio = (double)fgpixels / (double)total_seg_fg;
  }
}

// this prints the image-wide statistics (non-verbose)
// verbose print method pritns the statistics for each individual
// region of the image in turn
void BipartiteGraph::printMetrics(FILE* stream){
  // *****This is how a normal, non-verbose metric file is formated:
  // -----region-wide statistics:
  // [# correctly segemented regions] / [total # regions]
  fprintf(stream, "%d/%d\n", hypmetrics.correctsegmentations, \
      gtmetrics.segmentations);
  // [# regions completely missed (fn)]
  fprintf(stream, "%d\n", hypmetrics.falsenegatives);
  // [# regions completely wrongly detected (fp)]
  fprintf(stream, "%d\n\n", hypmetrics.falsepositives);
  // -----stats on oversegmentations and undersegmentations:
  // [# oversegmented regions]
  fprintf(stream, "%d\n", hypmetrics.oversegmentedcomponents);
  // [# total oversegmentations for all regions]
  fprintf(stream, "%d\n", hypmetrics.oversegmentations);
  // [# avg oversegmentations per oversegmented groundtruth region]
  fprintf(stream, "%f\n", hypmetrics.avg_oversegmentations_perbox);
  // [# undersegmented regions]
  fprintf(stream, "%d\n", hypmetrics.undersegmentedcomponents);
  // [# total undersegmentations for all regions]
  fprintf(stream, "%d\n", hypmetrics.undersegmentations);
  // [# avg undersegmentations for undersegmented hypothesis region]
  fprintf(stream, "%f\n\n", hypmetrics.avg_undersegmentations_perbox);
  // -----pixel counts:
  // [# total foreground pix (tp+fp+tn+fn)]
  fprintf(stream, "%d\n", hypmetrics.total_fg_pix);
  // [# total positively detected pix (tp+fp)]
  fprintf(stream, "%d\n", hypmetrics.total_positive_fg_pix);
  // [# total negatively detected pix (tn+fn)]
  fprintf(stream, "%d\n", hypmetrics.total_negative_fg_pix);
  // [# total true positive pix (tp)]
  fprintf(stream, "%d\n", hypmetrics.total_true_positive_fg_pix);
  // [# total false negative pix (fn)]
  fprintf(stream, "%d\n", hypmetrics.total_false_negative_pix);
  // [# total true negative pix (tn)]
  fprintf(stream, "%d\n", hypmetrics.total_true_negative_fg_pix);
  // [# total false positive pix (fp)]
  fprintf(stream, "%d\n\n", hypmetrics.total_false_positive_pix);
  // -----metrics based on pixel counts (all between 0 and 1)
  // [TPR/Recall/Sensitivity/Hit_Rate = tp/(tp+fn)]
  fprintf(stream, "%f\n", hypmetrics.total_recall);
  // [Precision/Positive_Predictive_Value = tp/(tp+fp)]
  fprintf(stream, "%f\n", hypmetrics.total_precision);
  // [Accuracy = (tp+tn)/(tp+fn+tn+fp)]
  fprintf(stream, "%f\n", hypmetrics.accuracy);
  // [FPR/Fallout = fp/(fp+tn)]
  fprintf(stream, "%f\n", hypmetrics.total_fallout);
  // [False_Discovery_Rate = fp/(fp+tp)]
  fprintf(stream, "%f\n", hypmetrics.total_fdr);
  // [TNR/Specificity = tn/(fp+tn)]
  fprintf(stream, "%f\n", hypmetrics.specificity);
  // [Negative_Predictive_Value = tn/(tn+fn)]
  fprintf(stream, "%f\n", hypmetrics.negative_predictive_val);

  if(debug) {
    cout << "----\n" << filename << ":\n";
    cout << "False negatives found in overlapping groundtruth boxes:\n";
    vector<OverlappingGTRegion> ogt = hypmetrics.overlapgts;
    for(vector<OverlappingGTRegion>::iterator ogt_it = ogt.begin(); \
        ogt_it != ogt.end(); ogt_it++) {
      Vertex* vertptr = ogt_it->vertptr;
      int idx = vertptr->setindex;
      int falseneg = ogt_it->falsenegativepix;
      cout << "GroundTruth region at index " << idx << " has " \
           << falseneg << "false negative pixels\n";
    }
  }
}

void BipartiteGraph::printMetricsVerbose(FILE* stream) {
  // ******Verbose metric files give pixel accurate metrics on each individual region:
  vector<RegionDescription> boxes = hypmetrics.boxes;
  for(vector<RegionDescription>::iterator region = boxes.begin();\
      region != boxes.end(); region++) {
    Vertex* vertptr = region->vertptr;
    string whichset = vertptr->whichset;
    int setindex = vertptr->setindex;
    int setnum;
    if(whichset == (string)"Hypothesis")
      setnum = 0;
    else if(whichset == (string)"GroundTruth")
      setnum = 1;
    else {
      cout << "ERROR: Invalid set type in bipartite graph!\n";
      exit(EXIT_FAILURE);
    }
    // -----info on which box we are on!
    // [Box #] [Set #] // box is the region #, set is either 0 or 1 (0 for hypothesis, 1 for groundtruth)
    fprintf(stream, "\n%d %d\n", setindex, setnum);
    // -----region dimensions
    // [Box area]
    fprintf(stream, "\t%d\n", region->area);
    l_int32 x,y,w,h;
    boxGetGeometry(region->box, &x, &y, &w, &h);
    // [Box width]
    fprintf(stream, "\t%d\n", w);
    // [Box height]
    fprintf(stream, "\t%d\n\n", h);
    // -----number of overlapping groundtruth boxes corresponding to this hypothesis box
    // [# overlapping groundtruth boxes]
    fprintf(stream, "\t%d\n\n", region->num_gt_overlap);
    // -----pixel counts
    // [# total foreground pix (tp+fp+tn+fn)]
    fprintf(stream, "\t%d\n", region->num_fg_pixels);
    // [# total positively detected pix (tp+fp)]
    int p = region->true_positive_pix + region->false_positive_pix;
    fprintf(stream, "\t%d\n", p);
    // [# total true positive pix (tp)]
    fprintf(stream, "\t%d\n", region->true_positive_pix);
    // [# total false negative pix (fn)]
    fprintf(stream, "\t%d\n", region->false_negative_pix);
    // [# total false positive pix (fp)]
    fprintf(stream, "\t%d\n\n", region->false_positive_pix);

    if(debug) {
      if(region->num_gt_overlap >= 1) {
        cout << filename << ", region # " << setindex \
             << " of " << whichset << " set\n";
        cout << "\tOverlaps Groundtruth Boxes at index(es) ";
        vector<Edge> overlapping = vertptr->edges;
        for(vector<Edge>::iterator edge = overlapping.begin(); \
            edge != overlapping.end(); edge++) {
          Vertex* edgevert = edge->vertexptr;
          int setindex_ = edgevert->setindex;
          if((edge + 2) == overlapping.end())
            cout << setindex_ << " and ";
          else if((edge + 1) == overlapping.end())
            cout << setindex_ << "\n";
          else
            cout << setindex_ << ", ";
        }
      }
    }
    // -----metrics based on the pixel counts
    // [TPR/Recall/Sensitivity/Hit_Rate = tp/(tp+fn)]
    fprintf(stream, "\t%f\n", region->recall);
    // [Precision/Positive_Predictive_Value = tp/(tp+fp)]
    fprintf(stream, "\t%f\n", region->precision);
    // [FPR/Fallout = fp/(fp+tn)]
    fprintf(stream, "\t%f\n", region->fallout);
    // [False_Discovery_Rate = fp/(fp+tp)]
    fprintf(stream, "\t%f\n", region->false_discovery);
  }
}

void BipartiteGraph::printSet(Bipartite::GraphChoice graph) {
  vector<Vertex>* set;
  if(graph == Bipartite::GroundTruth)
    set = &GroundTruth;
  else
    set = &Hypothesis;

  int i = 0, j = 0;
  for(vector<Vertex>::iterator vert = set->begin(); \
      vert != set->end(); vert++) {
    cout << "Vertex " << i << ":\n";
    l_int32 t,l,w,h;
    boxGetGeometry(vert->rect, &t, &l, &w, &h);
    cout << "\tBox (t,l,w,h): " << "(" << t << ", " << l \
         << ", " << w << ", " << h << ")\n";
    cout << "\tBox Area: " << w*h << endl;
    cout << "\tNumber of Foreground Pixels: " \
         << vert->pix_foreground << endl;
    cout << "\tForeground Pixels Ratio: " \
         << (double)(vert->pix_foreground) / \
         (double(w) * double(h)) << endl;
    cout << "\tEdges:\n";
    vector<Edge> edges = vert->edges;

    for(vector<Edge>::iterator edge = edges.begin(); \
        edge != edges.end(); edge++) {
      cout << "\t\tEdge " << j << ":\n";
      Vertex* vptr = edge->vertexptr;
      boxGetGeometry(vptr->rect, &t, &l, &w, &h);
      cout << "\t\t\tEdge points at vertex " << vptr->setindex \
           << " from other set\n";
      cout << "\t\t\tArea of box edge points at: " \
           << vptr->area << endl;
      cout << "\t\t\tEdge points to box at (t,l,w,h): " \
           << "(" << t << ", " << l << ", " << w \
           << ", " << h << ")\n";
      cout << "\t\t\tTotal foreground pixels edge points at: " \
           << vptr->pix_foreground << endl;
      cout << "\t\t\tIntersecting pixels: " \
           << edge->pixfg_intersecting << endl;
      cout << "\t\t\tOverlap area: " \
           << edge->overlap_area << endl;
      j++;
    }
    i++;
    j = 0;
  }
}

void BipartiteGraph::clear() {
  Hypothesis.erase(Hypothesis.begin(), Hypothesis.end());
  GroundTruth.erase(GroundTruth.begin(), GroundTruth.end());
  pixDestroy(&inimg);
  pixDestroy(&hypimg);
  pixDestroy(&gtimg);
}

LayoutEval::Color BipartiteGraph::getColorFromType(const string& type) {
  if(type == "displayed")
    return LayoutEval::RED;
  else if(type == "embedded")
    return LayoutEval::BLUE;
  else if(type == "label")
    return LayoutEval::GREEN;
  else
    return LayoutEval::NONE;
}

