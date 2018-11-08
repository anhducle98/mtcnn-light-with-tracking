#include "network.h"
#include "mtcnn.h"
#include <time.h>
#include <mutex>
#include <thread>
#include <chrono>

namespace Runner {
    Mat last_frame;
    Mat temp_frame;

    mutex last_lock;
    atomic_flag last_used;

    VideoCapture cap;
    mtcnn *cnn;

    int init() {
        cap = VideoCapture(0);
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

    void read_camera() {
        while (true) {
            cap >> temp_frame;
            if (!last_lock.try_lock()) continue;
            cv::swap(last_frame, temp_frame);
            last_used.clear();
            last_lock.unlock();
        }
    }
    void run() {
        while (true) {
            last_lock.lock();
            if (!last_used.test_and_set()) {
                cnn->findFace(last_frame);
                imshow("result", last_frame);
                if (waitKey(1) >= 0) break;   
            }
            last_lock.unlock();
        }
    }

    int main() {
        assert(init() == 0);

        thread camera(read_camera);
        thread runner(run);

        runner.join();
        camera.~thread();

        return 0;
    }
};

int old_main() {
    Size sz = Size(640, 480);

    Mat image;
    VideoCapture cap(0);
    cap >> image;
    resize(image, image, sz, 0, 0);

    if(!cap.isOpened())  
        cout<<"fail to open!"<<endl; 
    cap>>image;
    if(!image.data){
        cout<<"fuck"<<endl;  
        return -1;
    }

    cout << image.rows << "x" << image.cols << endl;

    mtcnn find(image.rows, image.cols);

    clock_t start;

    int num_frame = 0;
    double total_time = 0;
    int frame_id = 0;

    SORT sorter;

    while (true) {
        //start = clock();
        cap>>image;
        resize(image, image, sz, 0, 0);
        auto start_time = std::chrono::system_clock::now();
        vector<Rect_<float> > boxes = find.findFace(image);
        auto diff = std::chrono::system_clock::now() - start_time;
        auto t1 = std::chrono::duration_cast<std::chrono::nanoseconds>(diff);

        vector<TrackingBox> detFrameData;
        for (int i = 0; i < boxes.size(); ++i) {
            cerr << boxes[i].x << ' ' << boxes[i].y << ' ' << boxes[i].width << ' ' << boxes[i].height << endl;
            TrackingBox cur_box;
            cur_box.box = boxes[i];
            cur_box.id = i;
            cur_box.frame = frame_id;
            detFrameData.push_back(cur_box);
        }
        ++frame_id;

        vector<TrackingBox> tracking_results = sorter.update(detFrameData);
        //vector<TrackingBox> tracking_results = detFrameData;
        for (TrackingBox it : tracking_results) {
            rectangle(image, Point(it.box.y, it.box.x), Point(it.box.height, it.box.width), sorter.randColor[it.id % 20], 2,8,0);
        }
        
        //cout<<"time is  "<<start/10e3<<endl;
        imshow("result", image);
        if( waitKey(1)>=0 ) break;
        //start = clock() -start;
         
        cerr << num_frame << ' ' << t1.count()/1e6 << endl;
        if (num_frame < 100) {
            num_frame += 1;
            total_time += double(t1.count());
        } else {
            printf("Time=%.2f, Frame=%d, FPS=%.2f\n", total_time / 1e9, num_frame, num_frame * 1e9 / total_time);
            num_frame = 0;
            total_time = 0;
        }
    }

    image.release();

    return 0;
}

int main()
{
    //Mat image = imread("4.jpg");
    //mtcnn find(image.rows, image.cols);
    /*
    clock_t start;
    start = clock();
    find.findFace(image);
    imshow("result", image);
    imwrite("result.jpg",image);
    start = clock() -start;
    cout<<"time is  "<<start/10e3<<endl;*/
    

    //return Runner::main();

    return old_main();
    
    return 0;
}