#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QSqlDatabase &db, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_searchButton_clicked();

private:
    Ui::MainWindow *ui;
    QSqlDatabase &db;
    void indexFolder(const QString &folderPath);
};

#endif // MAINWINDOW_H
