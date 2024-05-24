#include "agent/agent_interface.hpp"

#include "config/constants.hpp"
#include "lib/magic_enum.hpp"
#include "sockpp/unix_acceptor.h"

#include <poll.h>

#include <glog/logging.h>
#include <gsl/util>
#include <string_view>


AgentInterface::AgentInterface()
 : shutdown_(false)
 , buttons_expiration_(0)
{
    server_thread_ = std::make_shared<std::thread>(&AgentInterface::start, this);
}

AgentInterface::~AgentInterface()
{
    shutdown();
}

void AgentInterface::start()
{
    shutdown_ = false;
    sockpp::initialize();

    std::filesystem::remove(AGENT_SOCKET_PATH);

    sockpp::unix_acceptor acceptor;
    sockpp::result r = acceptor.open(sockpp::unix_address(std::string(AGENT_SOCKET_PATH)));

    if (!r)
    {
        LOG(ERROR) << "Error creating sockpp::acceptor " << r.error_message();
        return;
    }

    struct pollfd polldata;
    polldata.fd = acceptor.handle();
    polldata.events = POLLERR | POLLIN | POLLPRI | POLLHUP;

    while (!shutdown_)
    {
        polldata.revents = 0;
        int pollres = poll(&polldata, 1, 500); // 500 ms delay
       
        if (polldata.revents || pollres)
        {
            if (polldata.revents & POLLHUP)
            {
                LOG(INFO) << "Socket connection died, closing acceptor";
                acceptor.close();
                break;
            }
            sockpp::result<sockpp::unix_socket> r_accept = acceptor.accept();

            std::scoped_lock lock(agents_mutex_);

            sockpp::unix_socket sock = r_accept.release();
            agents_.push_back(std::make_unique<AgentConnection>(sock,
                [this](agent_interface::ButtonPress& press) { this->handle_button_press(press);}));
        
            any_connections_ = true;
        }
    }
}

void AgentInterface::shutdown()
{
    if (shutdown_)
    {
        return;
    }

    {
        std::scoped_lock lock(agents_mutex_);
        agents_.clear();
    }

    shutdown_.store(true, std::memory_order_relaxed);
    server_thread_->join();    

    std::filesystem::remove(AGENT_SOCKET_PATH);
}

void AgentInterface::handle_button_press(agent_interface::ButtonPress& button_press_data)
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

    // LOG(INFO) << "received buttons from " << button_press_data.timestamp()
    //           << " at " << now_ms.count();

    for (int32_t i = 0; i < button_press_data.pressed_buttons().size();++i)
    {
        std::cout << +button_press_data.pressed_buttons()[i];
    }

    {
        std::scoped_lock lock(buttons_mutex_);
        last_buttons_ = button_press_data;
        buttons_expiration_ = last_buttons_.timestamp() + BUTTON_HOLD_MS;
    }
}

void AgentInterface::send_screenshot(std::vector<int8_t> && image_data)
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

    agent_interface::Screenshot to_send;

    to_send.set_timestamp(now_ms.count());
    to_send.set_sequence_number(to_send.sequence_number() + 1);
    to_send.clear_png_data();
    to_send.add_png_data(reinterpret_cast<const void*>(image_data.data()), image_data.size());

    std::scoped_lock lock(agents_mutex_);

    std::vector<int32_t> to_delete;
    int32_t index = 0;

    for (auto it = agents_.begin(); it != agents_.end();it++)
    {
        (*it)->queue_screenshot(to_send);
        if (not (*it)->active())
        {
            to_delete.push_back(index);
        }
        index++;
    }

    for (auto& i : to_delete)
    {
        agents_.erase(agents_.begin() + i);
    }

    if (agents_.size() == 0)
    {
        any_connections_ = false;
    }    
}

bool AgentInterface::is_button_pressed(Joypads::Button button)
{
    std::scoped_lock lock(buttons_mutex_);

    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

    if (last_buttons_.timestamp() >= buttons_expiration_)
    {
        return false;
    }

    return last_buttons_.pressed_buttons()[magic_enum::enum_integer<Joypads::Button>(button)];
}

AgentConnection::AgentConnection(sockpp::unix_socket &connection_socket,
                        ButtonPressCallback button_press_callback)
 : active_(true)
 , shutdown_(false)
 , sock_(connection_socket.release())
 , button_press_callback_(button_press_callback)
{
    thread_ = std::make_shared<std::thread>(&AgentConnection::run, this);
}

AgentConnection::~AgentConnection()
{
    shutdown();
}

void AgentConnection::shutdown()
{
    if (shutdown_)
    {
        return;
    }

    shutdown_.store(true, std::memory_order_relaxed);
    thread_->join();    
}

void AgentConnection::run()
{
    sockpp::result r = sock_.read_timeout(std::chrono::microseconds(5000));

    auto guard = gsl::finally([this]() { active_ = false; });

    while (sock_ && !shutdown_)
    {
        // if screenshot available, send it
        {
            std::scoped_lock lock(screenshot_mutex_);

            if (last_sent_screenshot_timestamp_ != screenshot_to_send_.timestamp())
            {
                // protocol for reads:
                //      struct  MessageHeader
                //      proto   agent_interface::Screenshot
                last_sent_screenshot_timestamp_ = screenshot_to_send_.timestamp();

                std::string serialized_screenshot;
                screenshot_to_send_.SerializeToString(&serialized_screenshot);

                MessageHeader screenshot_header;
                screenshot_header.payload_size = serialized_screenshot.size();

                sockpp::result r_write = sock_.write(&screenshot_header, sizeof(screenshot_header));
                if (!r_write)
                {
                    LOG(ERROR) << "Error sending message header " << r_write.error_message();

                    if (r_write.last_error().value() == EPIPE)
                    {
                        // clean up connection
                        break;
                    }

                    continue;
                }

                r_write = sock_.write(serialized_screenshot.data(), screenshot_header.payload_size);
                if (!r_write)
                {
                    LOG(ERROR) << "Error sending screenshot " << r_write.error_message();
                    continue;
                }
            }
        }

        // read with a short timeout so that when there is a new screenshot to send,
        // it can be sent immediately

        // protocol for reads:
        //      struct  MessageHeader
        //      proto   agent_interface::ButtonPresses

        MessageHeader header;
        sockpp::result<size_t> r_read = sock_.read(&header, sizeof(MessageHeader));

        if (r_read.value() < 0)
        {
            LOG(ERROR) << "Error reading from socket: " << r_read.error_message();
            break;
        }

        if (!r_read || r_read.value() == 0)
        {
            continue;
        }

        // read_n is blocking (no timeout)

        uint8_t buffer[4096];
        assert(header.payload_size <= sizeof(buffer));

        r_read = sock_.read_n(buffer, header.payload_size);

        if (r_read.value() < 0)
        {
            LOG(ERROR) << "Error reading from socket: " << r_read.error_message();
            break;
        }

        if (!r_read || r_read.value() == 0)
        {
            continue;
        }

        agent_interface::ButtonPress button_press_data;
        button_press_data.ParseFromString(std::string_view(reinterpret_cast<char*>(buffer),
                                                           r_read.value()));
        button_press_callback_(button_press_data);
    }
}

void AgentConnection::queue_screenshot(agent_interface::Screenshot& to_send)
{
    std::scoped_lock lock(screenshot_mutex_);
    screenshot_to_send_ = to_send;
}
