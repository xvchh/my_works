#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <filesystem>

int main() {
    std::string folder_path = "input/*.png";
    std::vector<cv::String> file_names;
    cv::glob(folder_path, file_names);
    if (file_names.empty()) {
        std::cout << "no found" << std::endl;
        return -1;
    }

    if (!std::filesystem::exists("out")) {
        std::filesystem::create_directory("out");
    }

    for (int i = 0; i < file_names.size(); i++) {
        cv::Mat img = cv::imread(file_names[i]);
        if (img.empty()) {
            std::cout << "read failed" << std::endl;
            continue;
        }
        std::string filename = file_names[i].substr(6);
        int count = 0;
        std::cout << filename << ": ";
        filename = "out/" + filename;

        cv::Mat gray, blurred;
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
        //cv::imwrite(filename + "gray.jpg", gray);
        cv::medianBlur(gray, blurred, 3);
        //cv::imwrite(filename + "biurred.jpg", blurred);

        cv::Mat binary;
        cv::adaptiveThreshold(blurred, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 21, 2);
        //cv::imwrite(filename + "binary.jpg", binary);

        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;

        cv::findContours(binary, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

        cv::Mat result = img.clone();

        std::vector<cv::Rect> detectedRects;

        for (int j = 0; j < contours.size(); j++)
        {
            int parent = hierarchy[j][3];//只留外层轮廓，保留重叠时在上面的
            if (parent != -1)
            {
                cv::Rect currentRect = cv::boundingRect(contours[j]);//当前轮廓外接矩形
                cv::Rect parentRect = cv::boundingRect(contours[parent]); //父轮廓外接矩形

                double area1 = currentRect.area();
                double area2 = parentRect.area();
                double sizeRatio = std::min(area1, area2) / std::max(area1, area2);

                cv::Rect overlap = currentRect & parentRect;
                double overlapRatio;
                if (std::min(area1, area2) > 0) {
                    overlapRatio = overlap.area() / std::min(area1, area2);
                }
                else {
                    overlapRatio = 0;
                }

                if (sizeRatio > 0.60 && overlapRatio > 0.90)//如果当前轮廓和父轮廓大小相近且重叠度高，则认为是同一个物体的轮廓
                    continue;
            }

            double area = cv::contourArea(contours[j]);
            if (area < 50)
                continue;

            std::vector<cv::Point> approx;//多边形顶点
            double epsilon = 0.02 * cv::arcLength(contours[j], true);
            cv::approxPolyDP(contours[j], approx, epsilon, true);

            if (approx.size() != 4)
                continue;

            if (!cv::isContourConvex(approx))
                continue;

            int completeEdges = 0;//计算四边形的边是否完整
            for (int k = 0; k < 4; k++)
            {
                cv::Point p1 = approx[k];
                cv::Point p2 = approx[(k + 1) % 4];

                double length = cv::norm(p2 - p1);
                int samples = std::max(2, (int)length);
                int valid = 0;//用于计算边上连通点的数量

                for (int t = 0; t <= samples; t++)
                {
                    double ratio = (double)t / samples;
                    int x = cvRound(p1.x + ratio * (p2.x - p1.x));
                    int y = cvRound(p1.y + ratio * (p2.y - p1.y));//当前点坐标

                    bool found = false;
                    for (int dy = -4; dy <= 4 && !found; dy++)//搜索附近9*9是否有白点
                    {
                        for (int dx = -4; dx <= 4; dx++)
                        {
                            int xx = x + dx;
                            int yy = y + dy;

                            if (xx >= 0 && xx < binary.cols &&
                                yy >= 0 && yy < binary.rows &&
                                binary.at<uchar>(yy, xx) == 255)
                            {
                                found = true;
                                break;
                            }
                        }
                    }
                    if (found)
                        valid++;
                }

                double edgeRatio = (double)valid / (samples + 1);

                if (edgeRatio >= 0.95)
                    completeEdges++;
            }
            if (completeEdges != 4)
                continue;

            cv::RotatedRect rr = cv::minAreaRect(approx);//去除倾斜矩形
            double boxArea = rr.size.width * rr.size.height;
            double fillRatio = cv::contourArea(approx) / boxArea;
            if (fillRatio < 0.90)
                continue;

            count++;

            if (count > 1)//打印
                std::cout << " | ";

            cv::polylines(result, approx, true, cv::Scalar(0, 255, 0), 2);

            std::cout << "rect" << count << " ";

            for (int k = 0; k < 4; k++) {
                int outputY = img.rows - 1 - approx[k].y;
                std::cout << "(" << approx[k].x << ", " << outputY << ")";

                if (k < 3)
                    std::cout << "; ";
            }
        }
        if (count == 0)
            std::cout << "null";

        std::cout << std::endl;

        cv::imwrite(filename, result);
    }
    return 0;
}
