#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/compressed_image.hpp"
#include <opencv2/opencv.hpp>
#include <filesystem>

using namespace std::chrono_literals;

class ImagePublisher : public rclcpp::Node
{
public:
    ImagePublisher() : Node("image_publisher")
    {
        path_ = "photo/result.png";
        RCLCPP_INFO(this->get_logger(), "监视文件: %s", path_.c_str());

        publisher_ = this->create_publisher<sensor_msgs::msg::CompressedImage>("/camera/compressed", 10);
        timer_ = this->create_wall_timer(500ms, std::bind(&ImagePublisher::timer_callback, this));
    }

private:
    void timer_callback()
    {
        if (!std::filesystem::exists(path_)) {
            return;
        }
        
        auto mtime = std::filesystem::last_write_time(path_);
        if (mtime != last_mtime_) {
            cv::Mat img = cv::imread(path_, cv::IMREAD_COLOR);
            if (!img.empty()) {
                image_    = img;
                last_mtime_ = mtime;
                RCLCPP_INFO(this->get_logger(), "检测到新图: %d x %d",
                            image_.cols, image_.rows);
            }
        }

        if (image_.empty()) return;
        
        std::vector<uchar> buf;
        cv::imencode(".jpg", image_, buf);

        sensor_msgs::msg::CompressedImage msg;//造一个空消息
        msg.header.stamp = this->now();//打上时间戳
        msg.header.frame_id = "camera";//标上坐标系
        msg.format = "jpeg";//声明是jpg
        msg.data.assign(buf.begin(), buf.end());//塞进图像字节

        publisher_->publish(msg);

    }

    std::string path_;
    cv::Mat image_;
    std::filesystem::file_time_type last_mtime_{};
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr publisher_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ImagePublisher>());
    rclcpp::shutdown();
    return 0;
}