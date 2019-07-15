#include "network.h"
#include "mtcnn.h"
#include <time.h>
#include <mutex>
#include <thread>
#include <chrono>
#include <map>

#include "send_data.h"
#include "detect_blury.h"
#include "pre_processing_image.h"
#include <future>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Runner {
    Mat last_frame;
    Mat temp_frame;

    mutex last_lock;
    atomic_flag last_used;

    VideoCapture cap;
    mtcnn *cnn;
    auto results = vector<future<long>>();
    struct config {
        string email;
        string password;
        string dataURL;
        string endpoint;
        string camera_id;

        config(string inputEmail, string inputPassword, string inputURL, string inputEndpoint, string inputCameraID) {
            email = inputEmail;
            password = inputPassword;
            dataURL = inputURL;
            endpoint = inputEndpoint;
            camera_id = inputCameraID;
        }
    };
    struct dataMat
    {
        std::vector<cv::Mat> arrayMat;
        int id;
        bool isSend;
        dataMat(std::vector<cv::Mat> array, int id_give,bool isSend_give)
        {
            arrayMat = array;
            id = id_give;
            isSend = isSend_give;
        }
    };
    std::vector<dataMat> data;
    std::vector<int> arrID;
    void getDataToBatch();

    void preProcesingImage(cv::Mat &image)
    {
        resize(image, image, Size(112, 112));
    }

    int init() {
        cap = VideoCapture(0);
        //cap = VideoCapture("vn.mp4");
        Mat image;
        if (!cap.isOpened()) {
            cout << "Failed to open!" << endl; 
            return -1;
        }
        cap >> image;
        if (!image.data){
            cout << "No image data!" << endl;  
            return -1;
        }

        cnn = new mtcnn(image.rows, image.cols);
        cap >> last_frame;
        return 0;
    }

    void called_from_async() {
        getDataToBatch();
    }

    void send_data() {
        const auto timeWindow = std::chrono::milliseconds(5000);

        while (true)
        {
            auto start = std::chrono::steady_clock::now();
            getDataToBatch();
            auto end = std::chrono::steady_clock::now();
            auto elapsed = end - start;

            auto timeToWait = timeWindow - elapsed;
            if (timeToWait > std::chrono::milliseconds::zero())
            {
                std::this_thread::sleep_for(timeToWait);
            }
        }
    }

    bool checkDuplicateIdData(int id)
    {
        //cout << "arrID";
        for (auto & value : arrID) {
            //cout << " " + value;
            if (value == id) {
                return true;
            }
        }
        return false;
    }

    void addToDataMat(cv::Mat image, int id)
    {
        //preprocessingImage(image);
        resize(image, image, Size(112, 112));
        //cerr << "image-size=" << image.rows << ' ' << image.cols << endl;
        bool found = false;
        if (data.size() == 0) {
            arrID.push_back(id);
            std::vector<cv::Mat> tempArr;
            tempArr.push_back(image);
            dataMat temp(tempArr, id, false);
            data.push_back(temp);
        }
        else {
            for (auto & value : data) {
                if (value.id == id && !value.isSend) {
                    //cout << "\n check boolean" << value.isSend << "\n";
                    found = true;
                    value.arrayMat.push_back(image);
                }
            }
            if (!found && !checkDuplicateIdData(id)) {
                arrID.push_back(id);
                std::vector<cv::Mat> tempArr;
                tempArr.push_back(image);
                dataMat temp(tempArr, id, false);
                data.push_back(temp);
            }
        }
    }

    void printData(vector<Mat> data, int id) {
            //cout << "size: " << data.size() << " ";
            int count = 0;
            for (auto & value : data) {
                count++;
                cv::imwrite("./test/image_" + std::to_string(id) + "_" + std::to_string(count) + ".jpg", value);
            }
    }

    void getDataToBatch()
    {
        if (data.size() == 0) {
            return;
        }
        //cout << "size " << data.size();
        auto it = std::begin(data);
        int i = 0;
        while (it != std::end(data)) {
            // Do some stuff
            if (data.at(i).arrayMat.size() > 14 && !data.at(i).isSend) {
                //cout << "sendata";
                std::string temp = makeArrayImage(data.at(i).arrayMat);
                
                results.push_back(async(launch::async, [temp]() -> long {
                    sendImageData(temp);
                    return 0;
                }));
                
                //sendImageData(temp);
                cout << "Send data id: " << data.at(i).id << "\n";
                cout << "-----------------\n";
                //cout << makeArrayImage(data.at(i).arrayMat;
                data.at(i).isSend = true;
                //cout << "modify Boolean " << data.at(i).id << " " << data.at(i).isSend;
                data.at(i).arrayMat.clear();
                it = data.erase(it);
            }
            else {
                ++it;
                ++i;
            }
        }
        for (auto &it : results) it.get();
        results.clear();
    }

    config getConfigFile() {
        std::ifstream i("config.json");
        json j;
        i >> j;
        cerr << "json " << j.dump(4) << endl;
        Runner::config config(j["email"].get<std::string>(), j["password"].get<std::string>(), j["dataStream"].get<std::string>(), j["endpoint"].get<std::string>(), j["camera_id"].get<std::string>());
        return config;
    }
};

void draw_5_points(Mat &image, vector< BoundingBox > &boxes) {
    for (auto &box : boxes) {
        bool is_frontal = box.is_frontal();
        printf("is_frontal=%d\n", is_frontal);
        int radius = 1;
        for (auto p : box.points) {
            circle(image,Point((int)p.x, (int)p.y), radius, Scalar(0,255,255), -1);
        }
    }
}

int _main(int argc, char **argv) {
    Mat image;
    VideoCapture cap;
    VideoWriter video_writer;

    Runner::config config = Runner::getConfigFile();
    getTokenByRequest(config.email, config.password, config.camera_id, config.endpoint);
    cout << "config" << config.endpoint << ' ' << config.email << endl;
    
    if (argc > 1) {
        cap = VideoCapture(argv[1]);
    } else {
        cap = VideoCapture(0);
        //cap = VideoCapture(config.dataURL);
    }

    if (!cap.isOpened()) cerr << "Fail to open camera!" << endl;

    cap >> image;
    if (!image.data) {
        cerr << "No image data!" << endl;
        return -1;
    }

    cerr << "Original size = " << image.rows << "x" << image.cols << endl;

    
    //Size sz = Size(image.cols, image.rows);
    // Uncomment to resize frame before processing
    //Size sz = Size(1280, 720);
    Size sz = Size(640, 480);
    
    resize(image, image, sz, 0, 0);

    // Uncomment to crop ROI before processing. Be careful if resize is also enabled!
    //Rect ROI = Rect(image.cols / 6, image.rows / 3, image.cols / 2, image.rows / 3 * 2);
    Rect ROI = Rect(0, 0, image.cols, image.rows); // full image

    
    video_writer.open("output_camera.avi", VideoWriter::fourcc('X', 'V', 'I', 'D'),
        cap.get(CAP_PROP_FPS), image(ROI).size());
    

    mtcnn find(ROI.height, ROI.width);

    clock_t start;

    int num_frame = 0;
    double total_time = 0;
    int frame_id = 0;

    SORT sorter(15);

    // params for fast video navigation, not neccessary for webcam
    int shift_width = 0;
    int last_frame_shifted = 0;

    // params for output image writing
    map<int, int> id_count;
    vector<int> compression_params;
    compression_params.push_back(IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(5);

    while (1) {
        try {
            cerr << "start read" << endl;
            bool readok = cap.read(image);
            cerr << "end read" << endl;
            if (image.empty()) {
		static int empty_count = 0;
		cerr <<"Empty frame #" << ++empty_count << endl;
		continue;
	    }
            resize(image, image, sz);

            image = image(ROI);

            auto start_time = std::chrono::system_clock::now();
            vector< BoundingBox > boxes = find.findFace(image);

            auto diff1 = std::chrono::system_clock::now() - start_time;
            auto t1 = std::chrono::duration_cast<std::chrono::nanoseconds>(diff1);

            vector<TrackingBox> detFrameData;
            for (int i = 0; i < boxes.size(); ++i) {
                //cerr << boxes[i].x << ' ' << boxes[i].y << ' ' << boxes[i].width << ' ' << boxes[i].height << endl;
                TrackingBox cur_box;
                cur_box.box = boxes[i].rect;
                cur_box.id = i;
                cur_box.frame = frame_id;
                detFrameData.push_back(cur_box);
            }
            ++frame_id;

            auto start_track_time = std::chrono::system_clock::now();
            vector<TrackingBox> tracking_results = sorter.update(detFrameData);

            auto diff2 = std::chrono::system_clock::now() - start_track_time;
            auto t2 = std::chrono::duration_cast<std::chrono::nanoseconds>(diff2);
            
            for (TrackingBox it : tracking_results) {
                Mat face_photo = image(Rect(it.box.y, it.box.x, it.box.height - it.box.y, it.box.width - it.box.x));
                //if (detectBlur(face_photo)) continue;
                Runner::addToDataMat(face_photo, it.id);
                
                rectangle(image, Point(it.box.y, it.box.x), Point(it.box.height, it.box.width), sorter.randColor[it.id % 20], 2,8,0);
                //imshow(file_name, face_photo);
                // Uncomment to enable writing output image
                //string file_name = string("./img_out_camera/") + to_string(it.id) + "_" + to_string(++id_count[it.id]) + ".png";
                //imwrite(file_name, face_photo, compression_params);
            }

            Runner::getDataToBatch();

            draw_5_points(image, boxes);

            imshow("result", image);

            // Uncomment to write video output
            // video_writer << image;

            //if (waitKey(1) >= 0) break;
		waitKey(1);
             
            // Statistics
            cerr << num_frame << ' ' << t1.count()/1e6 << ' ' << t2.count()/1e6 << " (ms) " << endl;
            if (num_frame < 100) {
                num_frame += 1;
                total_time += double(t1.count());
                total_time += double(t2.count());
            } else {
                printf("Time=%.2f, Frame=%d, FPS=%.2f\n", total_time / 1e9, num_frame, num_frame * 1e9 / total_time);
                num_frame = 0;
                total_time = 0;
            }

            // Uncomment to enable video navigation
            /*
            if (argc > 1) {
                int key = waitKey(30);
                //cerr << "key= " << key << endl;
                if (key != -1) {
                    cerr << "User Input: " << key << endl;
                    if (key == 81) { // left arrow
                        if (last_frame_shifted + 10 < cap.get(CAP_PROP_POS_FRAMES) || shift_width >= 0) {
                            shift_width = -200;
                        } else {
                            shift_width = shift_width * 1.5;
                        }
                        cap.set(CAP_PROP_POS_FRAMES, cap.get(CAP_PROP_POS_FRAMES) + shift_width);
                        last_frame_shifted = cap.get(CAP_PROP_POS_FRAMES);
                    } else if (key == 83) { // right arrow
                        if (last_frame_shifted + 10 < cap.get(CAP_PROP_POS_FRAMES) || shift_width <= 0) {
                            shift_width = +200;
                        } else {
                            shift_width = shift_width * 1.5;
                        }
                        cap.set(CAP_PROP_POS_FRAMES, cap.get(CAP_PROP_POS_FRAMES) + shift_width);
                        last_frame_shifted = cap.get(CAP_PROP_POS_FRAMES);
                    } else if (key == 13) { // ENTER
                        break;
                    }
                }
            }
            */
            
        } catch (cv::Exception e) {
            cerr << "Warning: an exception occured!" << endl;
            continue;
        }
    }

    image.release();
    video_writer.release();

    return 0;
}

int main(int argc, char **argv)
{
    return _main(argc, argv);
}
