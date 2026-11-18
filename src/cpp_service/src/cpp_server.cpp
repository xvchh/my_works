#include "rclcpp/rclcpp.hpp"
#include "base_interfaces_demo/srv/block.hpp"
#include <opencv2/opencv.hpp>

using base_interfaces_demo::srv::Block;
using std::placeholders::_1;
using std::placeholders::_2;

class Service : public rclcpp::Node
{
    public:
        Service() : Node("service")
        {
            std::string path = "photo/cube.png";
            image_ = cv::imread(path, cv::IMREAD_COLOR);
            if (image_.empty()) {
                RCLCPP_ERROR(this->get_logger(), "图片打开失败:%s", path.c_str());
            } else {
                RCLCPP_INFO(this->get_logger(), "图片加载成功:%d x %d", image_.cols, image_.rows);
                RCLCPP_INFO(this->get_logger(), "图片路径:%s",path.c_str());
            }
            
            server = this->create_service<Block>("block",std::bind(&Service::handle, this, _1, _2));
            RCLCPP_INFO(this->get_logger(), "服务端启动完毕，等待请求提交...");
        }

    private:
        cv::Mat image_;
        rclcpp::Service<Block>::SharedPtr server;
        void handle(const Block::Request::SharedPtr req,const Block::Response::SharedPtr res)
        {
            if (image_.empty()) return;

            RCLCPP_INFO(this->get_logger(), "收到指令，目标颜色: %s", req->color.c_str());

            cv::Mat hsv;//BRG->HSV
            cv::cvtColor(image_, hsv, cv::COLOR_BGR2HSV);

            cv::Mat mask;
            if (req->color == "red") {
                cv::Mat m1, m2;
                cv::inRange(hsv, cv::Scalar(0, 120, 70),   cv::Scalar(10, 255, 255),  m1);
                cv::inRange(hsv, cv::Scalar(170, 120, 70), cv::Scalar(180, 255, 255), m2);
                mask = m1 | m2;                       // 红色跨 0 度，要两段合并
            }
            else if (req->color == "green") {
                cv::inRange(hsv, cv::Scalar(40, 80, 70), cv::Scalar(80, 255, 255), mask);
            }
            else if (req->color == "blue") {
               cv::inRange(hsv, cv::Scalar(85, 80, 60), cv::Scalar(135, 255, 255), mask);
            }
            else {
                RCLCPP_WARN(this->get_logger(), "未知颜色: %s", req->color.c_str());
                res->x = res->y = res->width = res->height = 0;
                return;
            }

            cv::morphologyEx(mask, mask, cv::MORPH_OPEN,
                     cv::getStructuringElement(cv::MORPH_RECT, cv::Size(9, 9)));

            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            if (contours.empty()) {
                RCLCPP_WARN(this->get_logger(), "没找到 %s 色块", req->color.c_str());
                res->x = res->y = res->width = res->height = 0;
                 return;
            }

            auto max_it = std::max_element(contours.begin(), contours.end(),[](const auto & a, const auto & b)
            {
                return cv::contourArea(a) < cv::contourArea(b);
            });//找出最大轮廓 

            cv::Rect r = cv::boundingRect(*max_it);//外接矩形
            res->x      = r.x;
            res->y      = r.y;
            res->width  = r.width;
            res->height = r.height;

            cv::Mat show = image_.clone();
            cv::rectangle(show, r, cv::Scalar(0, 0, 255), 3);
            cv::imwrite("photo/result.png", show);

            RCLCPP_INFO(this->get_logger(), "识别结果: x=%d y=%d w=%d h=%d",r.x, r.y, r.width, r.height);
        }
};

int main(int argc, char const *argv[])
{
    rclcpp::init(argc, argv);
    auto server = std::make_shared<Service>();
    rclcpp::spin(server);
    rclcpp::shutdown();
    return 0;
}