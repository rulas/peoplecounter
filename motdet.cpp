/**
 * @file bg_sub.cpp
 * @brief Background subtraction tutorial sample code
 * @author Domenico D. Bloisi
 */

// opencv
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/video.hpp"
#include "opencv2/features2d.hpp"
// C
#include <stdio.h>
#include <stdlib.h>

// C++
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace cv;
using namespace std;

// Global variables
Mat frame;  // current frame
Mat temp;
Mat fgMaskMOG2;  // fg mask fg mask generated by MOG2 method
Mat fgMaskKNN;
Ptr<BackgroundSubtractor> pMOG2;  // MOG2 Background subtractor
Ptr<BackgroundSubtractor> pKNN;
int keyboard;  // input from keyboard
RNG rng(12345);
int thresh = 100;
int closing = 5;
int max_thresh = 255;
int morph_operator = 0;
int morph_elem = 0;
int morph_size = 5;
int const max_operator = 4;
int const max_elem = 2;
int const max_kernel_size = 21;

// windows names
const char *sourceWindow = "Source";
const char *morphOpsWindow = "Morph Operations";
const char *fgMaskMOG2Window = "FG Mask MOG 2";
const char *fgMaskKNNWindow = "FG Mask KNN";
const char *grayWindow = "Gray Frame";
const char *contoursWindow = "Contours";
const char *keypointsWindow = "Keypoints";

/** Function Headers */
void help();
void processVideo(char *videoFilename);
void morphOps(int, void *);
void cannyOps(int, void *);

void help() {
  cout << "--------------------------------------------------------------------"
          "------" << endl
       << "This program shows how to use background subtraction methods "
          "provided by " << endl
       << " OpenCV. You can process both videos (-vid) and images (-img)."
       << endl
       << endl
       << "Usage:" << endl
       << "./bs {-vid <video filename>}" << endl
       << "for example: ./bs -vid video.avi" << endl
       << "or: ./bs -cam" << endl
       << "--------------------------------------------------------------------"
          "------" << endl
       << endl;
}

/**
 * @function main
 */
int main(int argc, char *argv[]) {
  // print help information
  help();

  cout << "argc=" << argc << endl;

  // check for the input parameter correctness
  //    if(argc != 3) {
  //        cerr <<"Incorret input list" << endl;
  //        cerr <<"exiting..." << endl;
  //        return EXIT_FAILURE;
  //    }

  // create GUI windows
  namedWindow(sourceWindow);
  // namedWindow(fgMaskMOG2Window);
  namedWindow(fgMaskKNNWindow);
  // namedWindow(keypointsWindow);
  namedWindow(contoursWindow);
  // namedWindow(morphOpsWindow);

  // createTrackbar(
  //     "Operator:\n 0: Opening - 1: Closing \n 2: Gradient - 3: Top "
  //     "Hat \n 4: Black Hat",
  //     morphOpsWindow, &morph_operator, max_operator, morphOps);
  // /// Create Trackbar to select kernel type
  // createTrackbar("Element:\n 0: Rect - 1: Cross - 2: Ellipse", morphOpsWindow,
  //                &morph_elem, max_elem, morphOps);

  /// Create Trackbar to choose kernel size
  createTrackbar("Canny thresh: ", contoursWindow, &thresh, max_thresh,
                 cannyOps);
  createTrackbar("Kernel size:\n", contoursWindow, &morph_size,
                 max_kernel_size, morphOps);
  
  // createTrackbar("Closing size: ", contoursWindow, &closing, 255, nullptr);

  // create Background Subtractor objects
  pMOG2 = createBackgroundSubtractorMOG2();  // MOG2 approach
  pKNN = createBackgroundSubtractorKNN();

  if (strcmp(argv[1], "-vid") == 0) {
    // input data coming from a video
    processVideo(argv[2]);
  } else if (strcmp(argv[1], "-cam") == 0 || argc == 1) {
    processVideo(nullptr);
  } else {
    // error in reading input parameters
    cerr << "Please, check the input parameters." << endl;
    cerr << "Exiting..." << endl;
    return EXIT_FAILURE;
  }
  // destroy GUI windows
  destroyAllWindows();
  return EXIT_SUCCESS;
}

void printBlobDetectionParams(SimpleBlobDetector::Params &params) {
  // Change thresholds
  cout << "params.minThreshold=" << params.minThreshold << endl;
  cout << "params.maxThreshold=" << params.maxThreshold << endl;
  cout << "params.thresholdSte=" << params.thresholdStep << endl;

  // Filter by Area.
  cout << "params.filterByArea=" << params.filterByArea << endl;
  cout << "params.maxArea=" << params.maxArea << endl;
  cout << "params.minArea=" << params.minArea << endl;

  // Filter by Circularity
  cout << "params.filterByCircularity=" << params.filterByCircularity << endl;
  cout << "params.minCircularity=" << params.minCircularity << endl;
  cout << "params.maxCircularity=" << params.maxCircularity << endl;

  // Filter by Convexity
  cout << "params.filterByConvexity=" << params.filterByConvexity << endl;
  cout << "params.minConvexity=" << params.minConvexity << endl;
  cout << "params.maxConvexity=" << params.maxConvexity << endl;

  // Filter by Inertia
  cout << "params.filterByInertia=" << params.filterByInertia << endl;
  cout << "params.minInertiaRatio=" << params.minInertiaRatio << endl;
  cout << "params.maxInertiaRatio=" << params.maxInertiaRatio << endl;
}

void printImageProperties(Mat &image) {
  cout << setw(15) << "Width:" << image.cols << endl;
  cout << setw(15) << "Height:" << image.rows << endl;

  cout << setw(15) << "Pixel Depth:" << image.depth() << endl;
  cout << setw(15) << "Channels:" << image.channels() << endl;

  // cout << setw(15) << "Width Step:" <<  image.widthStep << endl;
  cout << setw(15) << "Image Size:" << image.size << endl;
  cout << setw(15) << "Image Type:" << image.type() << endl;
}

/**
 * @function processVideo
 */
void processVideo(char *videoFilename) {
  Ptr<SimpleBlobDetector> detector;
  SimpleBlobDetector::Params params;

  //    SimpleBlobDetector detector;
  vector<KeyPoint> keypoints;
  bool highResVideo = false;
  bool printOnceOnly = true;

  // create the capture object
  VideoCapture capture;
  if (!videoFilename) {
    highResVideo = true;
    capture.open(0);
  } else {
    capture.open(videoFilename);
  }

  if (!capture.isOpened()) {
    // error in opening the video input
    cerr << "Unable to open video file: " << videoFilename << endl;
    exit(EXIT_FAILURE);
  }

  params.maxThreshold = 255;
  params.minArea = 10;
  detector = SimpleBlobDetector::create(params);
  printBlobDetectionParams(params);

  // read input data. ESC or 'q' for quitting
  while ((char)keyboard != 'q' && (char)keyboard != 27) {
    // read the current frame
    if (!capture.read(frame)) {
      cerr << "Unable to read next frame." << endl;
      cerr << "Exiting..." << endl;
      exit(EXIT_FAILURE);
    }

    // resize image to half if high resolution video
    if (highResVideo) {
      resize(frame, frame, Size(frame.cols / 2, frame.rows / 2));
    }

    // update the background model
    pMOG2->apply(frame, fgMaskMOG2);
    pKNN->apply(frame, fgMaskKNN);

    // clean background model
    // Mat structure_elem = getStructuringElement(MORPH_RECT, Size(closing,
    // closing));
    // morphologyEx(fgMaskMOG2, fgMaskMOG2, MORPH_CLOSE, structure_elem);
    // morphologyEx(fgMaskKNN, fgMaskKNN, MORPH_CLOSE, structure_elem);

    morphOps(0, 0);

    cannyOps(0, 0);

    // detect blobs
    // detector->detect(fgMaskKNN, keypoints);
    // cout << "Num of keypoints: " << keypoints.size() << std::endl;

    // Draw detected blobs as red circles
    // Mat im_with_keypoints;
    // drawKeypoints(frame, keypoints, im_with_keypoints, Scalar(0, 0, 255),
    // DrawMatchesFlags::DRAW_RICH_KEYPOINTS); // get the frame
    // number and write it
    // on the current
    // frame
    stringstream ss;
    rectangle(frame, cv::Point(10, 2), cv::Point(100, 20),
              cv::Scalar(0xd3, 0xd3, 0xd3), -1);
    ss << capture.get(CAP_PROP_POS_FRAMES);
    string frameNumberString = ss.str();
    putText(frame, frameNumberString.c_str(), cv::Point(15, 15),
            FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));
    // show the current frame and the fg masks
    imshow(sourceWindow, frame);
    // imshow(keypointsWindow, im_with_keypoints);

    if (printOnceOnly) {
      printImageProperties(frame);
      printOnceOnly = false;
    }
    // get the input from the keyboard
    keyboard = waitKey(30);
  }
  // delete capture object
  capture.release();
}

void cannyOps(int, void *) {
  Mat canny_output;
  Mat src_gray;
  vector < vector <Point> > contours;
  vector<Vec4i> hierarchy;

  cvtColor(frame, src_gray, CV_BGR2GRAY);
  blur(src_gray, src_gray, Size(3, 3));

  /// Detect edges using canny
  Canny(fgMaskKNN, canny_output, thresh, thresh * 2, 3);
  findContours(canny_output, contours, hierarchy, CV_RETR_EXTERNAL,
               CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

  Mat drawing = Mat::zeros(canny_output.size(), CV_8UC3);
  for (int i = 0; i < contours.size(); i++) {
    Scalar color =
        Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
    drawContours(drawing, contours, i, color, 2, 8, hierarchy, 0, Point());
  }

  imshow(contoursWindow, drawing);
}

void morphOps(int, void *) {
  // Since MORPH_X : 2,3,4,5 and 6
  int operation = morph_operator + 2;

  Mat element = getStructuringElement(MORPH_RECT, Size(morph_size, morph_size));

  /// Apply the specified morphology operation
  // morphologyEx(frame, , operation, element );
  // morphologyEx(fgMaskMOG2, fgMaskMOG2, MORPH_CLOSE, element);
  morphologyEx(fgMaskKNN, fgMaskKNN, MORPH_CLOSE, element);
  // imshow(fgMaskMOG2Window, fgMaskMOG2);
  imshow(fgMaskKNNWindow, fgMaskKNN);
}