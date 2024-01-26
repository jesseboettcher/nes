#pragma once

#include <QMainWindow>
#include <QWindow>

class UIWindow : public QQuickWindow
{
	Q_OBJECT
    QML_ELEMENT

public:
	UIWindow()
	 : QQuickWindow()
	{

	}

	void set_close_callback(std::function<void()> close_callback)
	{
		close_callback_ = close_callback;
	}

protected:
	virtual void closeEvent(QCloseEvent *ev)
	{
		LOG(INFO) << "close event";
		close_callback_();
	}

private:
	std::function<void()> close_callback_;
};

class UIMainWindow : public QMainWindow
{
	Q_OBJECT
    QML_ELEMENT

public:
	UIMainWindow()
	 : QMainWindow()
	{

	}

	void set_close_callback(std::function<void()> close_callback)
	{
		close_callback_ = close_callback;
	}

protected:
	virtual void closeEvent(QCloseEvent *ev)
	{
		close_callback_();
	}

private:
	std::function<void()> close_callback_;
};

