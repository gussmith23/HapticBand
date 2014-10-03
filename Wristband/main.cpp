#include <stdio.h>
#include <iostream>
#include "opencv2\core\core.hpp"
#include "opencv2\highgui\highgui.hpp"
#include "opencv2\features2d\features2d.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2\imgproc\imgproc.hpp"
#include "SerialClass.h"
#include "SerialUtil.h"
#include "MedianFlowTracker.h"

using namespace cv;
using namespace tld;

void sendToTeensy(SerialUtil*, int, int);

int main( int argc, char** argv )
{
	Mat objImage = imread( "C:\\Users\\hfs5022\\Downloads\\KelloggsFrostedFlakescereal_450.jpg", CV_LOAD_IMAGE_GRAYSCALE );
	Mat sceneImage;
	// Suggested in a OpenCV tutorial.
	int minHessian = 400;
	bool running = true;
	VideoCapture cap(0);
	namedWindow("Webcam",1);
	SurfFeatureDetector detector(minHessian);
	std::vector<KeyPoint> objectKeyPoints, sceneKeyPoints;
	SurfDescriptorExtractor extractor;
	Mat objectDescriptors,sceneDescriptors;
	Point2f textOffsetFromCenter(10.0,10.0);
	const int maxArea = 100000;
	Scalar green(0,255,0);
	MedianFlowTracker *mft = new MedianFlowTracker();

	// Debug
	SerialUtil* util = new SerialUtil();
	
	
	

	// Process object first
	
	detector.detect(objImage,objectKeyPoints);
	extractor.compute(objImage,objectKeyPoints,objectDescriptors);

	while(running){

		cap >> sceneImage;
		
		// Detect keypoints
		detector.detect(sceneImage,sceneKeyPoints);

		// Calculate descriptors
		extractor.compute(sceneImage,sceneKeyPoints,sceneDescriptors);

		// Matching descriptor vectors
		FlannBasedMatcher matcher;
		std::vector<DMatch> matches;
		matcher.match(objectDescriptors,sceneDescriptors,matches);
		
		// Calculating min/max distance
		double min_dist = 100; 
		double max_dist = 0;

		for(int i=0; i < objectDescriptors.rows; i++){
			double dist = matches[i].distance;
			if(dist < min_dist) min_dist = dist;
			if(dist > max_dist) max_dist = dist;
		}

		// Remove bad matches
		std::vector<DMatch> goodMatches;

		for(int i = 0; i < objectDescriptors.rows; i++){
			if(matches[i].distance < 2*min_dist) goodMatches.push_back(matches[i]);
		}

		
		// Draw matches.
		Mat image = sceneImage;
		//drawMatches( objImage, objectKeyPoints, sceneImage, sceneKeyPoints, goodMatches, image,Scalar::all(-1), Scalar::all(-1),
        //       vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

		// Collect good matches.
		std::vector<Point2f> obj, scene;
		for(int i = 0; i < goodMatches.size(); i++)
		{
			obj.push_back( objectKeyPoints[ goodMatches[i].queryIdx].pt );
			scene.push_back( sceneKeyPoints[ goodMatches[i].trainIdx ].pt );
		}
		
		
		// Find homography.
		Mat homography;
		try{
			homography = findHomography( obj,scene,CV_RANSAC );
		}
		catch(Exception e){
			continue;
		}

		// Create our  bounding box, to be morphed using the homography
		std::vector<Point2f> objCorners(4);
		std::vector<Point2f> sceneCorners(4);
		std::vector<Point2f> objOnScene(4);
		objCorners[0] = Point2f(0,0);
		objCorners[1] = Point2f((float)objImage.cols, 0);
		objCorners[2] = Point2f((float)objImage.cols, (float)objImage.rows);
		objCorners[3] = Point2f(0 , (float)objImage.rows);

		// These two line segments will roughly estimate our orientation.
		Point2f originalSegment[2];
		Point2f orientedSegment[2];
		// First, find the ABSOLUTE locations of the points on the screen.
		originalSegment[0] = (objCorners[0]+objCorners[1])*(.5);
		originalSegment[1] = (objCorners[2]+objCorners[3])*(.5);
		// Then, convert it to a single vector from the origin.
		Point2f originalVector = originalSegment[1] - originalSegment[0];
		

		// Transform
		perspectiveTransform(objCorners,sceneCorners,homography);

		// Now we set the oriented segment endpoints. By comparing these two segments, we can roughly estimate the rotation of the camera.
		orientedSegment[0] = (sceneCorners[0]+sceneCorners[1])*(.5);
		orientedSegment[1] = (sceneCorners[2]+sceneCorners[3])*(.5);
		Point2f orientedVector = orientedSegment[1] - orientedSegment[0];

		// Calculate the rotation of the camera based on the difference between the angles of our two vectors.
		double originalTheta = fastAtan2(originalVector.y,originalVector.x);
		double orientedTheta = fastAtan2(orientedVector.y,orientedVector.x);
		double correctionTheta = orientedTheta - originalTheta;
	
		//printf("Original: %f\nOriented: %f\nCamera is rotated %f degrees.\nPercent: %f\n",originalTheta,orientedTheta, orientedTheta - originalTheta,cameraRotationPercent);
		
		// The locations of the corners of the object on the scene itself.
		//objOnScene = sceneCorners;

		/*
		line( image, objOnScene[0], objOnScene[1] , Scalar(0, 255, 0), 4 );
		line( image, objOnScene[1], objOnScene[2], Scalar( 0, 255, 0), 4 );
		line( image, objOnScene[2], objOnScene[3], Scalar( 0, 255, 0), 4 );
		line( image, objOnScene[3], objOnScene[0], Scalar( 0, 255, 0), 4 );
		*/

		

		// Find the average point.
		Point2f average(0,0);
		for(int i = 0; i < 4; i++)
		{
			average += sceneCorners[i]*.25;
		}

		// Draw average point.
		circle( image, average, 10, Scalar(0,255,0) );
		
		// Draw orientation vectors.
		line(image,average,average+originalVector,green);
		line(image,average,average+orientedVector,green);

		// Find the area.
		double area = contourArea(sceneCorners);
		std::string message = "Area: ";
		message += std::to_string(area);
		putText(image, message, average + textOffsetFromCenter, FONT_HERSHEY_SIMPLEX, .5, Scalar(0,255,0));

		// Compare to center of the screen.
		Point2f centerOfScreen((float)(image.cols/2.0), (float)(image.rows/2.0));			// Find center.
		//circle( image, centerOfScreen, 10, Scalar(255,0,0) );
		//putText(image, "Center", centerOfScreen + textOffsetFromCenter, FONT_HERSHEY_SIMPLEX, .5, Scalar(255,0,0));
		Point2f differenceVector = average - centerOfScreen;			
		//line(image,	centerOfScreen, centerOfScreen + differenceVector, Scalar(0,0,255));
		float originalDirection = cvFastArctan((float)differenceVector.y,(float)differenceVector.x); // This comes out "backwards", in a sense, because everything is measured from the top-left. We correct for it.
		float trueDirection = originalDirection - (float)correctionTheta;
		int trueDirectionPercent = (int) ((trueDirection/360.0) * 100);

		cout << "True Direction: " << trueDirectionPercent << std::endl;

		// Show the angle of rotation of the object
		putText(image, "Theta: " + std::to_string(orientedTheta-originalTheta), (orientedVector*0.5)+average, FONT_HERSHEY_SIMPLEX, .5, green);

		// Calculate intensity based on area.
		int intensityPercent = (int) ((area/maxArea)*100);

		// Send to the wristband.
		sendToTeensy(util, trueDirectionPercent, intensityPercent);

		imshow("Webcam", image);
		if(waitKey(30)>0) running = false;
	}
}

void sendToTeensy(SerialUtil* util, int thetaPercent, int intensityPercent){
	unsigned char* string = new unsigned char[5];
	string[0] = 255;
	string[1] = thetaPercent; // theta
	string[2] = intensityPercent; // intensity
	string[3] = 100; // duration
	string[4] = ((int) string[0] + (int) string[1] + (int) string[2] + (int) string[3]);
	if (string[4] == 255) string[4] = 254;
	util->write((char*)string);
}
