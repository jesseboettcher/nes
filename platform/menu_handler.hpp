#pragma once

#include <QObject>
#include <QWindow>

#include <filesystem>

class MenuHandler : public QObject
{
    Q_OBJECT

public:
    MenuHandler();

public slots:
    void load_rom();

    void run();
    void step();
    void stop();

    void goto_memory();
    void command();
    void run_processor_tests();

    void close();
    void show_registers();
    void show_memory();
    void show_sprites();

private:
	void start_nes(std::filesystem::path path);
};
