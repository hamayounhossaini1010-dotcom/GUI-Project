#pragma once

#include <QMainWindow>
#include <QTextEdit>
#include <streambuf>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/* ================= BUSINESS LOGIC (unchanged from console app) ================= */

const std::string FILE_NAME   = "student_grades.txt";
const std::string BACKUP_FILE = "student_grades_backup.txt";
const std::string CSV_FILE    = "student_grades_export.csv";
const std::string USERS_FILE  = "users.txt";

class SubjectScores {
private:
    double chinese{0}, math{0}, english{0}, computer{0};
public:
    double getChinese()  const { return chinese; }
    double getMath()     const { return math; }
    double getEnglish()  const { return english; }
    double getComputer() const { return computer; }
    void setScores(double c, double m, double e, double comp) {
        chinese = c; math = m; english = e; computer = comp;
    }
};

class Student {
private:
    std::string semester, studentID, className, name;
    SubjectScores scores;
    double totalScore{0}, averageScore{0};
    void calculateScores() {
        totalScore   = scores.getChinese() + scores.getMath() +
                     scores.getEnglish() + scores.getComputer();
        averageScore = totalScore / 4.0;
    }
public:
    Student() = default;
    Student(std::string sem, std::string id, std::string cls, std::string n,
            double c, double m, double e, double comp)
        : semester(sem), studentID(id), className(cls), name(n) {
        scores.setScores(c, m, e, comp);
        calculateScores();
    }
    std::string getSemester()   const { return semester; }
    std::string getStudentID()  const { return studentID; }
    std::string getClassName()  const { return className; }
    std::string getName()       const { return name; }
    const SubjectScores& getScores() const { return scores; }
    double getTotalScore()   const { return totalScore; }
    double getAverageScore() const { return averageScore; }
    void updateGrades(double c, double m, double e, double comp) {
        scores.setScores(c, m, e, comp);
        calculateScores();
    }
    bool hasFailingSubject() const {
        return scores.getChinese() < 60 || scores.getMath() < 60 ||
               scores.getEnglish() < 60 || scores.getComputer() < 60;
    }
};

class User {
private:
    std::string username, password, role;
public:
    User() = default;
    User(std::string u, std::string p, std::string r)
        : username(u), password(p), role(r) {}
    std::string getUsername() const { return username; }
    std::string getPassword() const { return password; }
    std::string getRole()     const { return role; }
};

/* ================= Qt STREAM REDIRECT ================= */

// Redirects std::cout into a QTextEdit widget.
class QtStreamBuffer : public std::streambuf {
public:
    explicit QtStreamBuffer(QTextEdit* output) : m_output(output) {}

protected:
    std::streamsize xsputn(const char* s, std::streamsize n) override {
        m_output->moveCursor(QTextCursor::End);
        m_output->insertPlainText(QString::fromLocal8Bit(s, static_cast<int>(n)));
        m_output->moveCursor(QTextCursor::End);
        return n;
    }
    int overflow(int c) override {
        if (c != EOF) {
            char ch = static_cast<char>(c);
            m_output->moveCursor(QTextCursor::End);
            m_output->insertPlainText(QString(QChar::fromLatin1(ch)));
            m_output->moveCursor(QTextCursor::End);
        }
        return c;
    }
private:
    QTextEdit* m_output;
};

/* ================= MAIN WINDOW ================= */

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // Login
    void onLoginClicked();

    // Menu navigation
    void onMenuAddGrade();
    void onMenuStats();
    void onMenuQuery();
    void onMenuReport();
    void onMenuSave();
    void onMenuExportCSV();
    void onMenuBackup();
    void onMenuRestore();
    void onMenuLogout();

    // Page actions
    void onAddGradeSubmit();
    void onAddGradeBack();

    void onStatsSearch();
    void onStatsBack();

    void onQuerySearch();
    void onQueryBack();

    void onReportGenerate();
    void onReportBack();

private:
    Ui::MainWindow* ui;

    // Stream redirect
    QtStreamBuffer* m_streamBuffer{nullptr};
    std::streambuf* m_oldCoutBuffer{nullptr};

    // App state
    User              m_currentUser;
    int               m_loginAttempts{3};
    std::vector<Student> m_students;

    // Business logic helpers (mirror original functions)
    std::vector<User> loadUsers();
    void              loadFromFile();
    void              saveToFile();
    void              exportToCSV();
    void              backupData();
    void              restoreData();

    // Display helpers writing to a QTextEdit
    void printSingleReportCard(QTextEdit* out, const Student& s);
    void printFailingRow(QTextEdit* out, const Student& s);

    void goToMenu();
    void setStatus(const QString& msg);  // writes to textEditOutput
};