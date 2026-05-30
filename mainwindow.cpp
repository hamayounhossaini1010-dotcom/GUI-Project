#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <sstream>
#include <iomanip>

/* ──────────────────────────────────────────────
   Constructor / Destructor
   ────────────────────────────────────────────── */

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Redirect std::cout → textEditOutput
    m_streamBuffer  = new QtStreamBuffer(ui->textEditOutput);
    m_oldCoutBuffer = std::cout.rdbuf(m_streamBuffer);

    // Load persistent data
    loadFromFile();

    // ── Login page ──
    connect(ui->btnLogin,         &QPushButton::clicked, this, &MainWindow::onLoginClicked);
    connect(ui->lineEditPassword, &QLineEdit::returnPressed, this, &MainWindow::onLoginClicked);

    // ── Main menu ──
    connect(ui->btnMenuAddGrade,  &QPushButton::clicked, this, &MainWindow::onMenuAddGrade);
    connect(ui->btnMenuStats,     &QPushButton::clicked, this, &MainWindow::onMenuStats);
    connect(ui->btnMenuQuery,     &QPushButton::clicked, this, &MainWindow::onMenuQuery);
    connect(ui->btnMenuReport,    &QPushButton::clicked, this, &MainWindow::onMenuReport);
    connect(ui->btnMenuSave,      &QPushButton::clicked, this, &MainWindow::onMenuSave);
    connect(ui->btnMenuExportCSV, &QPushButton::clicked, this, &MainWindow::onMenuExportCSV);
    connect(ui->btnMenuBackup,    &QPushButton::clicked, this, &MainWindow::onMenuBackup);
    connect(ui->btnMenuRestore,   &QPushButton::clicked, this, &MainWindow::onMenuRestore);
    connect(ui->btnMenuLogout,    &QPushButton::clicked, this, &MainWindow::onMenuLogout);

    // ── Add / Update grade page ──
    connect(ui->btnAddGradeSubmit, &QPushButton::clicked, this, &MainWindow::onAddGradeSubmit);
    connect(ui->btnAddGradeBack,   &QPushButton::clicked, this, &MainWindow::onAddGradeBack);

    // ── Statistics page ──
    connect(ui->btnStatsSearch, &QPushButton::clicked, this, &MainWindow::onStatsSearch);
    connect(ui->btnStatsBack,   &QPushButton::clicked, this, &MainWindow::onStatsBack);

    // ── Query page ──
    connect(ui->btnQuerySearch, &QPushButton::clicked, this, &MainWindow::onQuerySearch);
    connect(ui->btnQueryBack,   &QPushButton::clicked, this, &MainWindow::onQueryBack);

    // ── Report page ──
    connect(ui->btnReportGenerate, &QPushButton::clicked, this, &MainWindow::onReportGenerate);
    connect(ui->btnReportBack,     &QPushButton::clicked, this, &MainWindow::onReportBack);

    // Start on login page
    ui->stackedWidget->setCurrentWidget(ui->pageLogin);
}

MainWindow::~MainWindow()
{
    // Restore original cout buffer before destruction
    std::cout.rdbuf(m_oldCoutBuffer);
    delete m_streamBuffer;
    delete ui;
}

/* ──────────────────────────────────────────────
   Helpers
   ────────────────────────────────────────────── */

void MainWindow::setStatus(const QString& msg)
{
    ui->textEditOutput->append(msg);
}

void MainWindow::goToMenu()
{
    ui->labelCurrentUser->setText(
        QString("Logged in as: %1  [%2]")
            .arg(QString::fromStdString(m_currentUser.getUsername()))
            .arg(QString::fromStdString(m_currentUser.getRole())));

    bool isAdminUser = (m_currentUser.getRole() == "admin");
    ui->btnMenuBackup->setEnabled(isAdminUser);
    ui->btnMenuRestore->setEnabled(isAdminUser);

    ui->stackedWidget->setCurrentWidget(ui->pageMenu);
}

/* ──────────────────────────────────────────────
   User Authentication
   ────────────────────────────────────────────── */

std::vector<User> MainWindow::loadUsers()
{
    std::vector<User> users;
    std::ifstream in(USERS_FILE);
    if (!in) {
        std::ofstream out(USERS_FILE);
        out << "admin|admin123|admin\n";
        out << "teacher|teach123|teacher\n";
        out.close();
        users.emplace_back("admin",   "admin123", "admin");
        users.emplace_back("teacher", "teach123", "teacher");
        return users;
    }
    std::string line;
    while (std::getline(in, line)) {
        std::vector<std::string> parts;
        size_t pos;
        while ((pos = line.find('|')) != std::string::npos) {
            parts.push_back(line.substr(0, pos));
            line.erase(0, pos + 1);
        }
        parts.push_back(line);
        if (parts.size() == 3)
            users.emplace_back(parts[0], parts[1], parts[2]);
    }
    return users;
}

void MainWindow::onLoginClicked()
{
    if (m_loginAttempts <= 0) {
        ui->labelLoginStatus->setText("Too many failed attempts. Access denied.");
        return;
    }

    const std::string inputUser = ui->lineEditUsername->text().toStdString();
    const std::string inputPass = ui->lineEditPassword->text().toStdString();
    auto users = loadUsers();

    for (const auto& u : users) {
        if (u.getUsername() == inputUser && u.getPassword() == inputPass) {
            m_currentUser = u;
            m_loginAttempts = 3; // reset for next logout
            setStatus(QString("Login successful. Welcome, %1 [%2]")
                          .arg(QString::fromStdString(u.getUsername()))
                          .arg(QString::fromStdString(u.getRole())));
            goToMenu();
            return;
        }
    }

    --m_loginAttempts;
    ui->labelLoginStatus->setText(
        QString("Invalid credentials. Attempts remaining: %1").arg(m_loginAttempts));

    if (m_loginAttempts <= 0)
        ui->labelLoginStatus->setText("Too many failed attempts. Access denied.");
}

/* ──────────────────────────────────────────────
   Main Menu Slots
   ────────────────────────────────────────────── */

void MainWindow::onMenuAddGrade() {
    // Clear form
    ui->lineEditSemester->clear();
    ui->lineEditClass->clear();
    ui->lineEditStudentID->clear();
    ui->lineEditStudentName->clear();
    ui->spinChinese->setValue(0);
    ui->spinMath->setValue(0);
    ui->spinEnglish->setValue(0);
    ui->spinComputer->setValue(0);
    ui->labelAddStatus->clear();
    ui->stackedWidget->setCurrentWidget(ui->pageAddGrade);
}

void MainWindow::onMenuStats()   { ui->stackedWidget->setCurrentWidget(ui->pageStats); }
void MainWindow::onMenuQuery()   { ui->stackedWidget->setCurrentWidget(ui->pageQuery); }
void MainWindow::onMenuReport()  { ui->stackedWidget->setCurrentWidget(ui->pageReport); }

void MainWindow::onMenuSave() {
    saveToFile();
    setStatus("Data saved to file successfully.");
}

void MainWindow::onMenuExportCSV() {
    exportToCSV();
}

void MainWindow::onMenuBackup() {
    if (m_currentUser.getRole() != "admin") {
        setStatus("Access Denied. This feature is restricted to Admins only.");
        return;
    }
    backupData();
}

void MainWindow::onMenuRestore() {
    if (m_currentUser.getRole() != "admin") {
        setStatus("Access Denied. This feature is restricted to Admins only.");
        return;
    }
    restoreData();
}

void MainWindow::onMenuLogout() {
    saveToFile();
    m_currentUser = User();
    m_loginAttempts = 3;
    ui->lineEditUsername->clear();
    ui->lineEditPassword->clear();
    ui->labelLoginStatus->clear();
    setStatus("Logged out. Data saved.");
    ui->stackedWidget->setCurrentWidget(ui->pageLogin);
}

/* ──────────────────────────────────────────────
   Grade Entry / Modification
   ────────────────────────────────────────────── */

void MainWindow::onAddGradeSubmit()
{
    const std::string semester  = ui->lineEditSemester->text().trimmed().toStdString();
    const std::string className = ui->lineEditClass->text().trimmed().toStdString();
    const std::string id        = ui->lineEditStudentID->text().trimmed().toStdString();
    const std::string name      = ui->lineEditStudentName->text().trimmed().toStdString();

    if (semester.empty() || className.empty() || id.empty()) {
        ui->labelAddStatus->setText("Semester, Class, and Student ID are required.");
        return;
    }

    double chi  = ui->spinChinese->value();
    double math = ui->spinMath->value();
    double eng  = ui->spinEnglish->value();
    double comp = ui->spinComputer->value();

    // Check for existing record
    for (auto& s : m_students) {
        if (s.getSemester() == semester &&
            s.getClassName() == className &&
            s.getStudentID() == id)
        {
            s.updateGrades(chi, math, eng, comp);
            ui->labelAddStatus->setText(
                QString("Record updated for student: %1")
                    .arg(QString::fromStdString(s.getName())));
            setStatus(QString("Grade record modified: %1 (%2)")
                          .arg(QString::fromStdString(s.getName()))
                          .arg(QString::fromStdString(id)));
            return;
        }
    }

    if (name.empty()) {
        ui->labelAddStatus->setText("Student Name is required for a new record.");
        return;
    }

    m_students.emplace_back(semester, id, className, name, chi, math, eng, comp);
    ui->labelAddStatus->setText(
        QString("New student profile registered: %1").arg(QString::fromStdString(name)));
    setStatus(QString("New student added: %1 (%2)")
                  .arg(QString::fromStdString(name))
                  .arg(QString::fromStdString(id)));
}

void MainWindow::onAddGradeBack() { goToMenu(); }

/* ──────────────────────────────────────────────
   Class Statistics
   ────────────────────────────────────────────── */

void MainWindow::onStatsSearch()
{
    const std::string targetClass = ui->lineEditStatsClass->text().trimmed().toStdString();
    if (targetClass.empty()) { ui->textEditStats->setPlainText("Please enter a class name."); return; }

    std::vector<Student> subset;
    for (const auto& s : m_students)
        if (s.getClassName() == targetClass) subset.push_back(s);

    if (subset.empty()) {
        ui->textEditStats->setPlainText(
            QString("No records found for class: %1").arg(QString::fromStdString(targetClass)));
        return;
    }

    if (ui->comboStatsSort->currentIndex() == 0)
        std::sort(subset.begin(), subset.end(),
                  [](const Student& a, const Student& b){ return a.getAverageScore() > b.getAverageScore(); });
    else
        std::sort(subset.begin(), subset.end(),
                  [](const Student& a, const Student& b){ return a.getName() < b.getName(); });

    std::ostringstream oss;
    oss << "=======================================================\n";
    oss << "  GRADE STATISTICS FOR CLASS: " << targetClass << "\n";
    oss << "=======================================================\n";
    oss << std::left << std::setw(6) << "Rank"
        << std::setw(16) << "Name"
        << std::setw(13) << "Total Score"
        << "Average\n";
    oss << "-------------------------------------------------------\n";

    for (size_t i = 0; i < subset.size(); ++i) {
        oss << std::left << std::setw(6) << (i + 1)
        << std::setw(16) << subset[i].getName()
        << std::setw(13) << std::fixed << std::setprecision(1) << subset[i].getTotalScore()
        << std::fixed << std::setprecision(2) << subset[i].getAverageScore() << "\n";
    }

    ui->textEditStats->setPlainText(QString::fromStdString(oss.str()));
}

void MainWindow::onStatsBack() { goToMenu(); }

/* ──────────────────────────────────────────────
   Grade Query
   ────────────────────────────────────────────── */

void MainWindow::printSingleReportCard(QTextEdit* out, const Student& s)
{
    std::ostringstream oss;
    oss << "\n=========================================\n";
    oss << "          OFFICIAL REPORT CARD           \n";
    oss << "=========================================\n";
    oss << " ID        : " << s.getStudentID() << "\n";
    oss << " Name      : " << s.getName()      << "\n";
    oss << " Semester  : " << s.getSemester()  << "\n";
    oss << " Class     : " << s.getClassName() << "\n";
    oss << "-----------------------------------------\n";
    oss << " Chinese   : " << std::setw(6) << s.getScores().getChinese()
        << "  |  Math     : " << s.getScores().getMath()     << "\n";
    oss << " English   : " << std::setw(6) << s.getScores().getEnglish()
        << "  |  Computer : " << s.getScores().getComputer() << "\n";
    oss << "-----------------------------------------\n";
    oss << " Total     : " << std::setw(6) << s.getTotalScore()
        << "  |  Average  : "
        << std::fixed << std::setprecision(2) << s.getAverageScore() << "\n";
    oss << "=========================================\n";
    out->setPlainText(QString::fromStdString(oss.str()));
}

void MainWindow::printFailingRow(QTextEdit* out, const Student& s)
{
    std::string failedList;
    if (s.getScores().getChinese()  < 60) failedList += "Chi ";
    if (s.getScores().getMath()     < 60) failedList += "Math ";
    if (s.getScores().getEnglish()  < 60) failedList += "Eng ";
    if (s.getScores().getComputer() < 60) failedList += "Com ";

    std::ostringstream row;
    row << std::left << std::setw(12) << s.getClassName()
        << std::setw(12) << s.getStudentID()
        << std::setw(16) << s.getName()
        << "Failed: [ " << failedList << "]\n";
    out->append(QString::fromStdString(row.str()));
}

void MainWindow::onQuerySearch()
{
    if (ui->radioQueryByID->isChecked()) {
        const std::string targetID = ui->lineEditQueryID->text().trimmed().toStdString();
        if (targetID.empty()) { ui->textEditQuery->setPlainText("Please enter a Student ID."); return; }

        for (const auto& s : m_students) {
            if (s.getStudentID() == targetID) {
                printSingleReportCard(ui->textEditQuery, s);
                return;
            }
        }
        ui->textEditQuery->setPlainText(
            QString("No profile found for ID: %1").arg(QString::fromStdString(targetID)));
    }
    else {
        ui->textEditQuery->clear();
        std::ostringstream header;
        header << "=======================================================\n";
        header << "      ACADEMIC PROBATION / FAILING DIRECTORY           \n";
        header << "=======================================================\n";
        header << std::left << std::setw(12) << "Class"
               << std::setw(12) << "Student ID"
               << std::setw(16) << "Name"
               << "Status\n";
        header << "-------------------------------------------------------\n";
        ui->textEditQuery->setPlainText(QString::fromStdString(header.str()));

        bool found = false;
        for (const auto& s : m_students) {
            if (s.hasFailingSubject()) {
                printFailingRow(ui->textEditQuery, s);
                found = true;
            }
        }
        if (!found)
            ui->textEditQuery->append("All students meet passing benchmarks.");
    }
}

void MainWindow::onQueryBack() { goToMenu(); }

/* ──────────────────────────────────────────────
   Class Report
   ────────────────────────────────────────────── */

void MainWindow::onReportGenerate()
{
    const std::string targetClass = ui->lineEditReportClass->text().trimmed().toStdString();
    if (targetClass.empty()) { ui->textEditReport->setPlainText("Please enter a class name."); return; }

    std::ostringstream oss;
    oss << "============================================================================\n";
    oss << "           OFFICIAL CLASS REPORT SHEET: " << targetClass << "\n";
    oss << "============================================================================\n";
    oss << std::left
        << std::setw(12) << "ID"
        << std::setw(16) << "Name"
        << std::setw(8)  << "Chi"
        << std::setw(8)  << "Math"
        << std::setw(8)  << "Eng"
        << std::setw(8)  << "Com"
        << "Average\n";
    oss << "----------------------------------------------------------------------------\n";

    bool found = false;
    for (const auto& s : m_students) {
        if (s.getClassName() == targetClass) {
            oss << std::left
                << std::setw(12) << s.getStudentID()
                << std::setw(16) << s.getName()
                << std::setw(8)  << std::fixed << std::setprecision(1) << s.getScores().getChinese()
                << std::setw(8)  << s.getScores().getMath()
                << std::setw(8)  << s.getScores().getEnglish()
                << std::setw(8)  << s.getScores().getComputer()
                << std::fixed << std::setprecision(2) << s.getAverageScore() << "\n";
            found = true;
        }
    }
    if (!found)
        oss << "No records found for class: " << targetClass << "\n";
    oss << "============================================================================\n";

    ui->textEditReport->setPlainText(QString::fromStdString(oss.str()));
}

void MainWindow::onReportBack() { goToMenu(); }

/* ──────────────────────────────────────────────
   Secure Data Persistence (unchanged logic)
   ────────────────────────────────────────────── */

void MainWindow::loadFromFile()
{
    std::ifstream in(FILE_NAME);
    if (!in) return;

    m_students.clear();
    std::string line;
    while (std::getline(in, line)) {
        std::vector<std::string> seg;
        size_t pos;
        while ((pos = line.find('|')) != std::string::npos) {
            seg.push_back(line.substr(0, pos));
            line.erase(0, pos + 1);
        }
        seg.push_back(line);
        if (seg.size() != 8) continue;
        try {
            m_students.emplace_back(seg[0], seg[1], seg[2], seg[3],
                                    std::stod(seg[4]), std::stod(seg[5]),
                                    std::stod(seg[6]), std::stod(seg[7]));
        } catch (...) { continue; }
    }
}

void MainWindow::saveToFile()
{
    std::ofstream out(FILE_NAME);
    if (!out) return;
    for (const auto& s : m_students) {
        out << s.getSemester()          << "|"
            << s.getStudentID()         << "|"
            << s.getClassName()         << "|"
            << s.getName()              << "|"
            << s.getScores().getChinese()  << "|"
            << s.getScores().getMath()     << "|"
            << s.getScores().getEnglish()  << "|"
            << s.getScores().getComputer() << "\n";
    }
}

void MainWindow::exportToCSV()
{
    if (m_students.empty()) { setStatus("No data available to export."); return; }
    std::ofstream csv(CSV_FILE);
    if (!csv) { setStatus("Failed to create CSV export file."); return; }
    csv << "Semester,Student ID,Class,Name,Chinese,Math,English,Computer,Total,Average\n";
    for (const auto& s : m_students) {
        csv << s.getSemester()             << ","
            << s.getStudentID()            << ","
            << s.getClassName()            << ","
            << s.getName()                 << ","
            << s.getScores().getChinese()  << ","
            << s.getScores().getMath()     << ","
            << s.getScores().getEnglish()  << ","
            << s.getScores().getComputer() << ","
            << s.getTotalScore()           << ","
            << std::fixed << std::setprecision(2) << s.getAverageScore() << "\n";
    }
    csv.close();
    setStatus(QString("Data exported to: %1").arg(QString::fromStdString(CSV_FILE)));
}

void MainWindow::backupData()
{
    std::ifstream src(FILE_NAME, std::ios::binary);
    if (!src) { setStatus("No source data file found to backup."); return; }
    std::ofstream dst(BACKUP_FILE, std::ios::binary);
    dst << src.rdbuf();
    setStatus(QString("Backup created: %1").arg(QString::fromStdString(BACKUP_FILE)));
}

void MainWindow::restoreData()
{
    std::ifstream backup(BACKUP_FILE);
    if (!backup) { setStatus("No backup file found."); return; }
    std::ofstream main(FILE_NAME, std::ios::binary);
    main << backup.rdbuf();
    backup.close(); main.close();
    loadFromFile();
    setStatus("Data restored from backup successfully.");
}