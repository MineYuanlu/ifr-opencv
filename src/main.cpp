#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/xfeatures2d.hpp>

#include "umt/umt.hpp"
#include<thread>
#include "Camera.h"

using namespace std;
using namespace cv;


int main() {

    cout << "start" << endl;
    thread(ifr::Camera::runCamera).detach();
    umt::Subscriber<Mat> sub(MSG_CAMERA);

    namedWindow("相机", WINDOW_NORMAL);
    resizeWindow("相机", 500, 500);

    cout << "show" << endl;
    while (true) {
        const auto src = sub.pop();
        imshow("相机", src);
        waitKey(0);
    }

    return 0;
}

