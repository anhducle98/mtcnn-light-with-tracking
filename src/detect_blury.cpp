#include "detect_blury.h"

bool detectBlur(cv::Mat frame, int thresh_hold) {
	cv::Mat frame_gray, dst;
	int kernel_size = 3;
	int scale = 1;
	int delta = 0;
	int ddepth = CV_16S;
	cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY); // Convert the image to grayscale
	cv::Laplacian(frame_gray, dst, ddepth, kernel_size, scale, delta, cv::BORDER_DEFAULT);
	cv::Scalar mean, dev;
	meanStdDev(dst, mean, dev);
	if (dev[0] < thresh_hold) {
		return true;
	}
	else {
		return false;
	}
}