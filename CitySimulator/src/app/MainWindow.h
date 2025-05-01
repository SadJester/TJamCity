#pragma once

#include <QMainWindow>
#include "SDLWidget.h"

class MainWindow : public QMainWindow {
    //Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private:
    SDLWidget *sdlWidget;
};