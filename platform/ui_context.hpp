#pragma once

#include "platform/ui_window.hpp"

#include <QMainWindow>
#include <QMenuBar>

#include <memory>

class UIContext
{
	UIContext() {}

public:
	static UIContext& instance()
	{
		static UIContext* context = new UIContext;

		return *context;
	}

	QQmlApplicationEngine engine;

	UIMainWindow * main_window{nullptr};
	UIWindow * registers_window{nullptr};
	UIWindow * memory_window{nullptr};
	QMenuBar * menu_bar{nullptr};
};
