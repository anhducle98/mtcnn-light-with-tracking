#define NOMINMAX
#include "defines.h"

void sendImageData(std::string data);
std::string getEncodedImage(cv::Mat frame);
std::string makeArrayImage(std::vector<cv::Mat> arrMat);
std::string getToken();
void getTokenByRequest(std::string email, std::string password, std::string camid, std::string endpoint);