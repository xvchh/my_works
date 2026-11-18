#include "rclcpp/rclcpp.hpp"
#include "base_interfaces_demo/srv/block.hpp"
#include <string>

using base_interfaces_demo::srv::Block;
using namespace std::chrono_literals;

class Client : public rclcpp::Node
{
public:
    Client() : Node("client")
    {
        client = this->create_client<Block>("block");
        RCLCPP_INFO(this->get_logger(), "客户端创建，等待连接服务端！");
    }

    bool connect_server()
    {
        while (!client->wait_for_service(1s))
        {
            if (!rclcpp::ok())
            {
                RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "强制退出！");
                return false;
            }
            RCLCPP_INFO(this->get_logger(), "服务连接中，请稍候...");
        }
        return true;
    }

    std::shared_future<Block::Response::SharedPtr> send_request(const std::string & color)
    {
        auto request = std::make_shared<Block::Request>();
        request->color = color;
        return client->async_send_request(request);
    }
private:
    rclcpp::Client<Block>::SharedPtr client;
};

int main(int argc, char ** argv)
{
    if (argc < 2) 
    {
    RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "用法: cpp_client <颜色>");
    return 1;
    }
    rclcpp::init(argc,argv);

    auto client = std::make_shared<Client>();
    bool flag = client->connect_server();
    if (!flag)
    {
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"),"服务连接失败！");
        return 0;
    }

    auto response = client->send_request(argv[1]);

    // 处理响应
    if (rclcpp::spin_until_future_complete(client,response) == rclcpp::FutureReturnCode::SUCCESS)
    {
        auto res = response.get();
        RCLCPP_INFO(client->get_logger(), "请求正常处理");
        RCLCPP_INFO(client->get_logger(), "响应结果: x=%d y=%d w=%d h=%d",
                    res->x, res->y, res->width, res->height);

    } else {
        RCLCPP_INFO(client->get_logger(),"请求异常");
    }

        rclcpp::shutdown();
        return 0;
}