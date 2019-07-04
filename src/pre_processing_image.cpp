#include "pre_processing_image.h"

cv::Mat preprocessingImage(cv::Mat frame) {
	if (frame.channels() >= 3)
	{
		cv::Mat ycrcb;

		cv::cvtColor(frame, ycrcb, cv::COLOR_BGR2YCrCb);

		std::vector<cv::Mat> channels;
		cv::split(ycrcb, channels);

		cv::equalizeHist(channels[0], channels[0]);

		cv::Mat result;
		cv::merge(channels, ycrcb);

		cv::cvtColor(ycrcb, result, cv::COLOR_YCrCb2BGR);

		return result;
	}
	return frame;
}

cv::Mat preprocessingImageGray(cv::Mat frame) {
	cv::Mat gray;

	cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);


	cv::equalizeHist(gray, gray);

	return gray;
}

cv::Mat denoisingImage(cv::Mat frame) {
	cv::Mat result;
	cv::fastNlMeansDenoisingColored(frame, result, 3, 3, 7, 21);
	return result;
}

cv::Mat denoisingImageG(cv::Mat frame) {
	cv::Mat result;
	for (int i = 1; i < 31; i = i + 2)
	{
		cv::GaussianBlur(result, result, cv::Size(i, i), 0, 0);
	}
	return result;
}