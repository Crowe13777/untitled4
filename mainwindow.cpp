#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QMap>
#include <QMessageBox>
#include <QSqlQuery>
#include <QDirIterator>
#include <QSqlError>

MainWindow::MainWindow(QSqlDatabase &db, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), db(db) {
    ui->setupUi(this);
    connect(ui->indexButton, &QPushButton::clicked, [=]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Выберите папку для индексации");
        if (!dir.isEmpty()) {
            indexFolder(dir);
            QMessageBox::information(this, "Готово", "Индексация завершена");
        }
    });
    connect(ui->searchButton, &QPushButton::clicked, this, &MainWindow::on_searchButton_clicked);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::indexFolder(const QString &folderPath) {
    QStringList extensions = {"txt", "md", "ini", "csv"};
    QDirIterator it(folderPath, extensions, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo fi(filePath);
        QString filename = fi.fileName();

        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            content = content.replace(QRegularExpression("[\\p{Punct}\\t\\n\\r]"), " ");
            content = content.toLower();

            QMap<QString, int> wordFreqs;
            QStringList words = content.split(' ', Qt::SkipEmptyParts);
            for (const QString &word : words) {
                if (word.length() >= 3 && word.length() <= 32) {
                    wordFreqs[word] += 1;
                }
            }

            QSqlQuery q;
            q.prepare("INSERT INTO documents (filename, filepath) VALUES (?, ?) RETURNING id");
            q.addBindValue(filename);
            q.addBindValue(filePath);
            if (q.exec() && q.next()) {
                int docId = q.value(0).toInt();

                for (auto it = wordFreqs.begin(); it != wordFreqs.end(); ++it) {
                    QString word = it.key();
                    int freq = it.value();

                    QSqlQuery qw;
                    qw.prepare("SELECT id FROM words WHERE word = ?");
                    qw.addBindValue(word);
                    qw.exec();
                    int wordId;
                    if (qw.next()) {
                        wordId = qw.value(0).toInt();
                    } else {
                        QSqlQuery insertW;
                        insertW.prepare("INSERT INTO words (word) VALUES (?) RETURNING id");
                        insertW.addBindValue(word);
                        insertW.exec();
                        insertW.next();
                        wordId = insertW.value(0).toInt();
                    }

                    QSqlQuery rel;
                    rel.prepare("INSERT INTO document_word (document_id, word_id, frequency) VALUES (?, ?, ?)");
                    rel.addBindValue(docId);
                    rel.addBindValue(wordId);
                    rel.addBindValue(freq);
                    rel.exec();
                }
            }
        }
    }
}

void MainWindow::on_searchButton_clicked() {
    QString queryText = ui->lineEdit->text().toLower().trimmed();
    QStringList words = queryText.split(' ', Qt::SkipEmptyParts);
    ui->listWidget->clear();

    if (words.isEmpty()) return;
    if (words.size() > 4) {
        QMessageBox::warning(this, "Ошибка", "Можно искать не более 4 слов");
        return;
    }

    QString placeholders;
    for (int i = 0; i < words.size(); ++i) {
        placeholders += "?";
        if (i != words.size() - 1) placeholders += ",";
    }

    QString sql = R"(
        SELECT d.id, d.filename, SUM(dw.frequency) AS relevance
        FROM documents d
        JOIN document_word dw ON d.id = dw.document_id
        JOIN words w ON dw.word_id = w.id
        WHERE w.word IN (%1)
        GROUP BY d.id, d.filename
        HAVING COUNT(DISTINCT w.id) = %2
        ORDER BY relevance DESC
        LIMIT 10
    )";

    QString finalSql = sql.arg(placeholders).arg(words.size());

    QSqlQuery q(db);
    q.prepare(finalSql);
    for (const QString &w : words) {
        q.addBindValue(w);
    }

    if (!q.exec()) {
        QMessageBox::critical(this, "Ошибка", q.lastError().text());
        return;
    }

    bool hasResults = false;
    while (q.next()) {
        hasResults = true;
        QString filename = q.value(1).toString();
        ui->listWidget->addItem(filename);
    }
    if (!hasResults) {
        ui->listWidget->addItem("Результаты не найдены");
    }
}
