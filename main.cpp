#include <QApplication>
#include "mainwindow.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>

bool setupDatabase(QSqlDatabase &db) {
    QSqlQuery query(db);
    if (!query.exec("CREATE TABLE IF NOT EXISTS documents ("
                    "id SERIAL PRIMARY KEY, "
                    "filename TEXT, "
                    "filepath TEXT, "
                    "date_added TIMESTAMP DEFAULT NOW())")) {
        qDebug() << "Failed to create documents table:" << query.lastError().text();
        return false;
    }
    if (!query.exec("CREATE TABLE IF NOT EXISTS words ("
                    "id SERIAL PRIMARY KEY, "
                    "word TEXT UNIQUE)")) {
        qDebug() << "Failed to create words table:" << query.lastError().text();
        return false;
    }
    if (!query.exec("CREATE TABLE IF NOT EXISTS document_word ("
                    "document_id INTEGER REFERENCES documents(id) ON DELETE CASCADE, "
                    "word_id INTEGER REFERENCES words(id) ON DELETE CASCADE, "
                    "frequency INTEGER, "
                    "PRIMARY KEY (document_id, word_id))")) {
        qDebug() << "Failed to create document_word table:" << query.lastError().text();
        return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // Настройте соединение с базой PostgreSQL
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");        // замените на свои параметры
    db.setPort(5432);                   // если не по умолчанию
    db.setDatabaseName("yourdbname");   // замените
    db.setUserName("youruser");         // замените
    db.setPassword("yourpassword");     // замените

    if (!db.open()) {
        qDebug() << "Ошибка подключения к базе:" << db.lastError().text();
        return -1;
    }

    if (!setupDatabase(db)) {
        qDebug() << "Ошибка при создании таблиц";
        return -1;
    }

    MainWindow w(db);
    w.show();
    return a.exec();
}
