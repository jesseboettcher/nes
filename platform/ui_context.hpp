#pragma once

#include "platform/menu_handler.hpp"
#include "platform/ui_controller.hpp"
#include "platform/ui_window.hpp"
#include "system/nes.hpp"

#include <QMainWindow>
#include <QMenuBar>

#include <memory>

class UIContext
{
	UIContext() {}

public:
	static UIContext& instance()
	{
		static UIContext* context_ = new UIContext;

		return *context_;
	}

	std::shared_ptr<Nes> nes;

	QQmlApplicationEngine engine;

	UIMainWindow * main_window{nullptr};
	UIWindow * registers_window{nullptr};
	UIWindow * memory_window{nullptr};
	UIWindow * sprites_window{nullptr};
	QMenuBar * menu_bar{nullptr};

	MenuHandler menu_handler;
	UIController controller;

	void configure_registers_window();
	void configure_memory_window();
	void configure_sprites_window();
};
