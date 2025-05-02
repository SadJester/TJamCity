#include "MainWindow.h"
#include <QVBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QLabel>

#include "SDLWidget.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Qt SDL Integration");
    resize(400, 800);
    
    // Create central widget and layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    
    // Add a label
    QLabel *label = new QLabel("SDL 3.0 Rendering Inside Qt", this);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    
    // Create and add the SDL widget
    //sdlWidget = new SDLWidget(this);
    //layout->addWidget(sdlWidget, 1);
    
    // Add a button
    QPushButton *button = new QPushButton("Quit", this);
    connect(button, &QPushButton::clicked, this, &QMainWindow::close);
    layout->addWidget(button);
    
    setCentralWidget(centralWidget);
}