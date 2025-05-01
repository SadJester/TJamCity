#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>

#include <core/test.h>

int main(int argc, char *argv[])
{
    testFunction();
    // Create the application
    QApplication app(argc, argv);
    
    // Create the main window widget
    QWidget window;
    window.setWindowTitle("Basic Qt6 Application");
    window.resize(400, 300);
    
    // Create a layout for our window
    QVBoxLayout *layout = new QVBoxLayout(&window);
    
    // Add a label
    QLabel *label = new QLabel("Hello Qt6 World!");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    
    // Add a button
    QPushButton *button = new QPushButton("Click Me");
    layout->addWidget(button);
    
    // Connect the button's clicked signal to a lambda function
    QObject::connect(button, &QPushButton::clicked, [label]() {
        label->setText("Button was clicked!");
    });
    
    // Show the window
    window.show();
    
    // Start the event loop
    return app.exec();
}