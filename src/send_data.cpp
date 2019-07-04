#include "send_data.h"
#include <curl/curl.h>
#include <chrono>
#include <ctime>
#include "base64.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static std::string token;
static int store_id;
static std::string cam_id;
static int branch_id;
static std::string URL;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

void sendImageData(std::string data) {
	//std::cerr << "sendImageData" << std::endl;
	CURL *curl;
	CURLcode res;

	/* In windows, this will init the winsock stuff */
	//curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */
	curl = curl_easy_init();
	if (curl) {
		/* First set the URL that is about to receive our POST. This URL can
		   just as well be a https:// URL if that is what should receive the
		   data. */
		
		curl_easy_setopt(curl, CURLOPT_URL, (URL + "/identify").c_str());
		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, getToken().c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
		/* Now specify the POST data */
		//const char* dataEscape = curl_easy_escape(curl, data.c_str(), 0);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
}

std::string getEncodedImage(cv::Mat frame) {
	//std::cerr << "frame-size=" << frame.rows << ' ' << frame.cols << std::endl;
	std::vector<uchar> buf;
	cv::imencode(".jpg", frame, buf);
	uchar *enc_msg = new uchar[buf.size()];
	for (int i = 0; i < buf.size(); i++) enc_msg[i] = buf[i];
	std::string encoded = base64_encode(enc_msg, buf.size());
	return encoded;
}

std::string makeArrayImage(std::vector<cv::Mat> arrMat) {
	//std::cout << "make Array";
	json j;
	std::chrono::system_clock::time_point p = std::chrono::system_clock::now();

	std::time_t t = std::chrono::system_clock::to_time_t(p);
	char buffer[80];
	struct tm * timeinfo;
	time(&t);
	timeinfo = localtime(&t);
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
	j["store_id"] = store_id;
	j["camera_id"] = cam_id;
	j["branch_id"] = branch_id;
	j["time_in"] = buffer;
	j["images"] = {};
	j["token"] = token;
	for (auto mat : arrMat)
	{
		j["images"].push_back(getEncodedImage(mat));
	}
	j["images"] = j["images"].dump();
	return j.dump();
}

std::string getToken() {
	std::string header = "Authorization: Bearer ";
	std::string result = header + token;
	return result;
}

void getTokenByRequest(std::string email, std::string password, std::string camid, std::string endpoint) {
	curl_global_init(CURL_GLOBAL_ALL);
	URL = endpoint;
	CURL *curl;
	CURLcode res;
	std::string readBuffer;
	curl = curl_easy_init();
	if (curl) {
		
		std::string data = "email=" + email + "&password=" + password + "&camera_id=" + camid;
		std::cerr << "data = " << data << std::endl;
		std::cerr << "url = " << URL + "/login" << std::endl;
		curl_easy_setopt(curl, CURLOPT_URL, (URL + "/login").c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		
		///* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
		///* Check for errors */
		auto result = json::parse(readBuffer);

		token = result["access_token"].get<std::string>();
		store_id = result["store_id"].get<int>();
		branch_id = result["branch_id"].get<int>();
		cam_id = result["camera_id"].get<std::string>();

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
}

