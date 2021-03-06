 /*****************************************************************************
 * Project Isagoge: Enhancing the OCR Quality of Printed Scientific Documents
 * File name:		Lept_Utils.cpp
 * Written by:	Jake Bruce, Copyright (C) 2013
 * History: 		Created Aug 6, 2013 4:44:12 PM 
 * Description: TODO
 * 
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
 ****************************************************************************/

#include "Lept_Utils.h"

#include <allheaders.h>   // leptonica api

#include <M_Utils.h>

#include <iostream>
using namespace std;

// now fill the connected components within each rectangle and return
// the resulting image (this amounts to simply analyzing the image
// and converting each foreground pixel in it to a given color
Pix* Lept_Utils::fillBoxesForeground(Pix* inputimg, BOXA* boxes, \
    LayoutEval::Color color) {
  l_uint32 numboxes = boxaGetCount(boxes);
  for(l_uint32 i = 0; i < numboxes; i++) {
    Box* bbox = boxaGetBox(boxes, i, L_COPY);
    fillBoxForeground(inputimg, bbox, color);
    boxDestroy(&bbox);
  }
  return inputimg;
}

void Lept_Utils::fillBoxForeground(Pix* inputimg, BOX* bbox, \
    LayoutEval::Color color, PIX* imread, bool write_input_only) {
  l_int32 imwidth = pixGetWidth(inputimg);
  const l_int32 x = bbox->x;
  const l_int32 y = bbox->y;
  const l_int32 w = bbox->w;
  const l_int32 h = bbox->h;
  //set img pointer
  l_uint32* curpixel;
  l_uint32* startpixel;
  if(imread) // if we're writing to a different img than we're reading
    startpixel = pixGetData(imread);
  else
    startpixel = pixGetData(inputimg);
  rgbtype rgb[3];
  // scan from left to right, top to bottom while
  // coloring the foreground
  for(l_uint32 k = y; k < y+h; k++) {
    for(l_uint32 l = x; l < x+w; l++) {
      curpixel = startpixel + k*imwidth + l;
      getPixelRGB(curpixel, rgb);
      if(isDark(rgb)) {
        l_uint32* readpix = curpixel;
        if(write_input_only)
          readpix = pixGetData(inputimg) + k*imwidth + l;
        setPixelRGB(inputimg, readpix, l, k, color);
      }
    }
  }
}

int Lept_Utils::colorPixCount(PIX* im, LayoutEval::Color color) {
  int count = 0;
  for(l_uint32 i = 0; i < im->h; i++) {
    for(l_uint32 j = 0; j < im->w; j++) {
      l_uint32* curpix = getPixelAtXY(im, j, i);
      if(getPixelColor(curpix) == color)
        ++count;
    }
  }
  return count;
}

// draws horizontal line on the given image with provided color and thickness
void Lept_Utils::drawHLine(PIX* im, int x1, int x2, int y, LayoutEval::Color color, int thickness) {
  for(int i = x1; i <= x2; ++i)
    drawAtXY(im, i, y, color, thickness);
}

// draws vertical line on the given image with provided color and thickness
void Lept_Utils::drawVLine(PIX* im, int y1, int y2, int x, LayoutEval::Color color, int thickness) {
  for(int i = y1; i <= y2; ++i)
    drawAtXY(im, x, i, color, thickness);
}

// Draws box from teseract box
void Lept_Utils::drawBox(Pix* im, TBOX tbox) {
  TBOX tbox_ = tbox;
  Box* box = M_Utils::tessTBoxToImBox(&tbox_, im);
  drawBox(im, box);
  boxDestroy(&box);
}

// Draws box with default colro and thickness
void Lept_Utils::drawBox(Pix* im, Box* box) {
  drawBox(im, box, LayoutEval::GREEN, 7);
}

void Lept_Utils::drawBox(Pix* im, TBOX tbox, LayoutEval::Color color, int thickness) {
  TBOX tbox_ = tbox;
  Box* box = M_Utils::tessTBoxToImBox(&tbox_, im);
  drawBox(im, box, color, thickness);
  boxDestroy(&box);
}

// draws the box on the image with the provided color and thickness
void Lept_Utils::drawBox(PIX* im, BOX* box, LayoutEval::Color color, int thickness) {
  const int left = box->x;
  const int right = left + box->w;
  const int top = box->y;
  const int bottom = top + box->h;
  drawHLine(im, left, right, top, color, thickness);
  drawVLine(im, top, bottom, right, color, thickness);
  drawHLine(im, left, right, bottom, color, thickness);
  drawVLine(im, top, bottom, left, color, thickness);
}

