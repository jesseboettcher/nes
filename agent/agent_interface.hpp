#pragma once

#include "build/agent_interface.pb.h"
#include "io/joypads.hpp"
#include "sockpp/unix_stream_socket.h"

#include <atomic>
#include <thread>
#include <vector>

class AgentConnection;


class AgentInterface
{
	// AgentInterface spawns a server on a thread to wait for connections from external agents.
	// Agents recieve screenshots and send button presses.
public:

	// Buttons received will be held for at least this long
	static constexpr uint32_t BUTTON_HOLD_MS = 100;

	AgentInterface();
	~AgentInterface();

	bool any_connections() { return any_connections_.load(std::memory_order_relaxed); }

	// called from the AgentConnection::ButtonPressCallback on the agent connection thread
	void handle_button_press(agent_interface::ButtonPress&);

	// called from the UI thread when a new screenshot is available
	void send_screenshot(std::vector<int8_t> && image_data);

	// called from the UI thread when polling button status
	bool is_button_pressed(Joypads::Button);

private:
	AgentInterface(const AgentInterface&) = delete;
	AgentInterface& operator=(const AgentInterface&) = delete;

	void start();
	void shutdown();

	std::atomic<bool> shutdown_;
	std::shared_ptr<std::thread> server_thread_;

	std::mutex agents_mutex_;
	std::vector<std::unique_ptr<AgentConnection>> agents_;
	std::atomic<bool> any_connections_;

	std::mutex buttons_mutex_;
	agent_interface::ButtonPress last_buttons_;
	uint64_t buttons_expiration_;
};

class AgentConnection
{
public:
	// AgentConnection is spawned on a separate thread for each connection to an agent
	// Each message sent and received is preceeded by a header. The header is this simple
	// struct. The messages are protobuf objects.
	struct MessageHeader
	{
	    int32_t payload_size;
	};

	using ButtonPressCallback = std::function<void(agent_interface::ButtonPress&)>;

	AgentConnection(sockpp::unix_socket &connection_socket,
					ButtonPressCallback button_press_callback);
	~AgentConnection();

	// queues to the screenshot to be sent in the next pass through the run loop
	void queue_screenshot(agent_interface::Screenshot& to_send);

	// when active is false, the run loop has been exited and the agent can be destroyed
	bool active() { return active_; }

private:
	AgentConnection(const AgentConnection&) = delete;
	AgentConnection& operator=(const AgentConnection&) = delete;

	void run();
	void shutdown();

	std::atomic<bool> active_;
	std::atomic<bool> shutdown_;
	std::shared_ptr<std::thread> thread_;

	sockpp::unix_socket sock_;

	ButtonPressCallback button_press_callback_;

	std::mutex screenshot_mutex_;
	agent_interface::Screenshot screenshot_to_send_;

	// init to a non-zero invalid number so that the uninitialized, empty screen_shot_to_send_ is
	// not sent to the agent
	uint64_t last_sent_screenshot_timestamp_{1};
};
