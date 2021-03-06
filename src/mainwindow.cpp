#include <QDialog>
#include <QFileDialog>
#include <QFont>
#include <QFontMetrics>
#include <QRadioButton>
#include <QXmlStreamReader>
#include <QStandardPaths>

#include <QtDebug>

#ifdef CONF_CODE_EDITOR
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerpython.h>
#endif

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "costume/costume.h"
#include "dragelem/dragelemcategory.h"
#include "param/param.h"
#include "teacher/student.h"
#include "teacher/teacher.h"
#include "sprite.h"
#include "newspritename.h"
#include "teacherlogin.h"

MainWindow* sMainWindow = 0;

const SpriteVector* MainWindow::getSpriteVector()
{
    return ui->spriteSelect->getSpriteVector();
}

void MainWindow::addSprite(Sprite *sprite)
{
    return ui->spriteSelect->addSprite(sprite);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _audioEngine(this),
	_slc(this),
    _appDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)) // TODO: make configurable
{
    sMainWindow = this;

    _appDir.mkpath(_appDir.path());
    _appDir.mkdir("tmp");
    _tmpDir = QDir(_appDir.filePath("tmp"));

    qInfo() << "app dir:" << _appDir.absolutePath();
    qInfo() << "temp dir:" << _tmpDir.absolutePath();

    cleanTempDir();

    _pyController.addSysPath(QDir("python").absolutePath().toLatin1().data());

    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);

    ui->scene->setPyController(&_pyController);

#ifdef CONF_CODE_EDITOR
    QFont font;
    font.setFamily("Consolas");
    font.setFixedPitch(true);
    font.setPointSize(10);

    _codeEditor = new QsciScintilla(ui->tabCode);
    _codeEditor->setObjectName(QStringLiteral("codeEditor"));
    ui->tabCode->layout()->addWidget(_codeEditor);

    QFontMetrics fm = QFontMetrics(font);
    _codeEditor->setFont(font);
    _codeEditor->setMarginsFont(font);
    _codeEditor->setMarginWidth(0, fm.width( "00000" ) + 5);
    _codeEditor->setMarginLineNumbers(0, true);
    _codeEditor->setFolding(QsciScintilla::BoxedTreeFoldStyle);
    _codeEditor->setBraceMatching(QsciScintilla::SloppyBraceMatch);
    _codeEditor->setCaretLineVisible(true);
    _codeEditor->setCaretLineBackgroundColor(QColor("#ffe4e4"));
    _codeEditor->setMarginsBackgroundColor(QColor("#333333"));
    _codeEditor->setMarginsForegroundColor(QColor("#CCCCCC"));
    _codeEditor->setFoldMarginColors(QColor("#99CC66"), QColor("#333300"));

    QsciLexerPython lexer;
    lexer.setDefaultFont(font);
    _codeEditor->setLexer(&lexer);
#endif

    connect(this, SIGNAL(currentSpriteChanged(Sprite*)), &_audioEngine, SLOT(setCurrentSprite(Sprite*)));

    Sprite* sprite = new Sprite("sprite", this);
    Costume* costume = new Costume(sprite);
    costume->open("Assets/Costumes/dog2-a.png");
    costume->hide();
    sprite->setCurrentCostume(costume);

    ui->spriteSelect->addSprite(sprite);
    _currentSprite = sprite;
    emit changeCurrentSprite(sprite);

    _backgroundSprite = new Sprite(this);
    costume = new Costume(_backgroundSprite);
    costume->open("Assets/Backgrounds/desert.gif");
    costume->hide();
    _backgroundSprite->setCurrentCostume(costume);
    ((QHBoxLayout*) ui->selectionBackground->layout())->insertWidget(0, _backgroundSprite);

    InitializeDragElem(":/blocks.xml");
    qInfo() << "loaded drag elements";

    connect(this, SIGNAL(newSound()), ui->soundSelect, SLOT(updateSoundList()));
    connect(this, SIGNAL(newCostume()), ui->costumeSelect, SLOT(updateCostumeList()));

    _loading = false;

    connect(this, SIGNAL(currentTeacherChanged(Teacher*)), ui->studentList, SLOT(currentTeacherChanged(Teacher*)));
}

void MainWindow::InitializeDragElem(const QString& path)
{
    QFile styleSheet(path);
    styleSheet.open(QFile::ReadOnly);
    QString xml = QLatin1String(styleSheet.readAll());

    QXmlStreamReader xmlReader(xml);
    QXmlStreamAttributes attributes;

    DraggableElement* lastElement = 0;

    uint paramIndex = 0;
    while(!xmlReader.atEnd())
    {
        if(xmlReader.readNext() == QXmlStreamReader::StartElement)
        {
            QString type = xmlReader.name().toString();
            if(type == QLatin1String("dragableelement"))
            {
                paramIndex = 0;
                attributes = xmlReader.attributes();
                DragElemCategory* category = getCategoryByName(attributes.value(QLatin1String("category")).toString());
                QString type = attributes.value(QLatin1String("type")).toString();
                QString name = attributes.value(QLatin1String("name")).toString();
                QString spec = attributes.value(QLatin1String("spec")).toString();
                QColor color = category->getColor();

                if(!_Cgen.supported(name))
                    color = QColor();

                lastElement = DraggableElement::createNewElement(type, name, spec, color, 0, this);
                if(!lastElement)
                    continue;

                lastElement->makeStatic();
                category->getElemList()->push_back(lastElement);
                _slc.registerElement(lastElement);
            }
            else if(lastElement && type == QLatin1String("parameter"))
            {
                attributes = xmlReader.attributes();
                QString def = attributes.value(QLatin1String("default")).toString();
                const std::vector<ParamBase*> *vec = lastElement->getParamsVector();
                while(paramIndex < vec->size() && !vec->at(paramIndex)->setValue(def))
                    paramIndex++;
                paramIndex++;
            }
            else if(type == QLatin1String("dragelemcategory"))
            {
                attributes = xmlReader.attributes();
                QString name = attributes.value(QLatin1String("name")).toString();
                QColor color = QColor(attributes.value(QLatin1String("color")).toString());
                DragElemCategory* category = new DragElemCategory(ui->elementList, name, color, this);
                ui->categorySelect->getCategoryList()->push_back(category);
            }
        }
    }
    ui->categorySelect->show();
}

QRect MainWindow::getScriptAreaRect()
{
    return QRect(ui->scriptArea->mapToGlobal(QPoint(0,0)), QSize(ui->scriptArea->width(), ui->scriptArea->height()));
}

void MainWindow::initPython()
{
    _pyController.init();
    _pyController.addMediaPath(_tmpDir.absolutePath().toLatin1().data());
    reloadCodeAll();
    qInfo() << "initialized python app";
}

void MainWindow::reindexMedia()
{
    _pyController.reindexMedia();
}

QString MainWindow::getTempPath()
{
    return _tmpDir.path();
}

void MainWindow::reloadCodeAll()
{
    if(_loading)
        return;
    // TODO
#ifdef CONF_CODE_EDITOR
    if(_currentSprite)
        _codeEditor->setText(_Cgen.generateSprite(_currentSprite));
#endif

    QString name = "sprite_" + getBackgroundSprite()->getName();
    QString code = _Cgen.generateSprite(getBackgroundSprite(), true);
    _pyController.loadEntity(name.toLatin1().data(), getBackgroundSprite()->getName().toLatin1().data(), code.toLatin1().data());

    for (SpriteVector::const_iterator it = getSpriteVector()->begin(); it != getSpriteVector()->end(); it++)
    {
        QString name = "sprite_" + (*it)->getName();
        QString code = _Cgen.generateSprite(*it);
        _pyController.loadEntity(name.toLatin1().data(), (*it)->getName().toLatin1().data(), code.toLatin1().data());
    }
}

void MainWindow::reloadCodeSprite(Sprite *sprite)
{
    if(_loading)
        return;
    // TODO
#ifdef CONF_CODE_EDITOR
    if(_currentSprite == sprite)
        _codeEditor->setText(_Cgen.generateSprite(_currentSprite));
#endif

    QString name = "sprite_" + sprite->getName();
    QString code = _Cgen.generateSprite(sprite);
    _pyController.loadEntity(name.toLatin1().data(), sprite->getName().toLatin1().data(), code.toLatin1().data());
}

DragElemCategory* MainWindow::getCategoryByName(const QString& name)
{
    CategoryList *list = ui->categorySelect->getCategoryList();
    for(CategoryList::const_iterator category = list->begin(); category != list->end(); category++)
    {
        if((*category)->getName() == name)
            return *category;
    }
    return 0;
}

void MainWindow::cleanTempDir()
{
    QStringList tmpFiles = _tmpDir.entryList(QDir::Files);
    for (QStringList::const_iterator it = tmpFiles.constBegin(); it != tmpFiles.constEnd(); it++)
    {
        QString file = (*it);
        QFile(_tmpDir.filePath(file)).remove();
    }
}

MainWindow::~MainWindow()
{
    cleanTempDir();

    // TODO: this is not nice!
    for(SpriteVector::const_iterator it = getSpriteVector()->begin(); it != getSpriteVector()->end(); it++)
        (*it)->OverrideParents();
    delete ui;
}

void MainWindow::on_soundFromFile_clicked()
{
    const QString dir;
    const QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open WAV file"), dir, "*.wav");
    if (fileNames.count())
    {
        _audioEngine.loadFile(fileNames.front());
        emit newSound();
    }
}

void MainWindow::on_costumeFromFile_clicked()
{
    const QString dir;
    const QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open PNG file"), dir, "*.png");
    if (fileNames.count())
    {
        Costume* costume = new Costume(_currentSprite);
        costume->open(fileNames.front(), true);
        _currentSprite->addCostume(costume);
        emit newCostume();
        reloadCodeSprite(_currentSprite);
    }
}

void MainWindow::on_buttonScriptStart_clicked()
{
    // TODO: remove reload
    reloadCodeAll();
    _pyController.sendStart();
}

void MainWindow::on_buttonScriptStop_clicked()
{
    _pyController.sendStop();
}

void MainWindow::on_buttonAddDragElem_clicked()
{
    const QString dir;
    const QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open XML file"), dir, "*.xml");
    if (fileNames.count())
    {
        QFile addElem(fileNames.first());
        addElem.open(QFile::ReadOnly);
        QString xml = QLatin1String(addElem.readAll());

        QXmlStreamReader xmlReader(xml);
        QXmlStreamAttributes attributes;

        while(!xmlReader.atEnd())
        {
            if(xmlReader.readNext() == QXmlStreamReader::StartElement)
            {
                QString type = xmlReader.name().toString();
                if(type == QLatin1String("dragableelement"))
                {
                    attributes = xmlReader.attributes();
                    _currentStudent->addAddDragElem(attributes.value(QLatin1String("name")).toString());
                }
            }
        }
        setCurrentStudent(true);
    }
}

void MainWindow::on_spriteFromFile_clicked()
{
    NewSpriteName dialog;
    dialog.exec();

    QString name = dialog.getName();

    const QString dir;
    const QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open PNG file"), dir, "*.png");
    if (fileNames.count())
    {
        Sprite* sprite = new Sprite(name, this);
        ui->spriteSelect->addSprite(sprite);

        Costume* costume = new Costume(sprite);
        costume->open(fileNames.front(), true);
        sprite->setCurrentCostume(costume);

        changeCurrentSprite(sprite);
        reloadCodeSprite(sprite);
    }
}

void MainWindow::on_logInTeacher_clicked()
{
    TeacherLogIn dialog;
    dialog.exec();

    _currentTeacher = new Teacher("Max Mustermann", "Musterschule");
    Student* student = new Student("Max Mustermann");
    _currentTeacher->addStudent(student);
    student = new Student("Maximillian Mustermann");
    _currentTeacher->addStudent(student);
    student = new Student("Maximus Mustermann");
    _currentTeacher->addStudent(student);

    emit currentTeacherChanged(_currentTeacher);

    int currentOnline = 0;
    for(uint i = 0; i < _currentTeacher->getStudentVector()->size(); i++)
    {
        if(_currentTeacher->getStudentVector()->at(i)->isOnline()) ++currentOnline;
    }

    ui->teacherName->setText(tr("Logged in as: %1").arg(_currentTeacher->getName()));
    ui->groupName->setText(tr("Course: %1").arg(_currentTeacher->getGroupName()));
    ui->currentLoggedIn->setText(tr("%1 of %2 students online").arg(
                                     QString::number(currentOnline), QString::number(_currentTeacher->getStudentVector()->size())));
}

void MainWindow::changeCurrentSprite(Sprite *sprite)
{
    _currentSprite = sprite;
    setCurrentCostume(sprite->getCostumeVector()->at(0));
#ifdef CONF_CODE_EDITOR
    _codeEditor->setText(_Cgen.generateSprite(sprite));
#endif
    emit currentSpriteChanged(sprite);
}

void MainWindow::setCurrentCostume(Costume *costume)
{
    ui->costumeLabel->setPixmap(QPixmap::fromImage(costume->getImage()->scaled(400, 400)));
}

void MainWindow::setCurrentStudent(bool)
{
    int studentNumber;
    for(studentNumber = 0; studentNumber < ui->studentList->_gridLayout.rowCount(); studentNumber++)
    {
        int index = studentNumber * 4;
        if(((QRadioButton*) ui->studentList->_gridLayout.itemAt(index)->widget())->isChecked()) break;
    }
    Student* student = _currentTeacher->getStudentVector()->at(studentNumber-1);
    ui->listAddDragElem->clear();
    for(uint i = 0; i < student->getAddDragElemVector()->size(); i++)
    {
        ui->listAddDragElem->insertItem(ui->listAddDragElem->count(),
            new QListWidgetItem(student->getAddDragElemVector()->at(i), ui->listAddDragElem));
    }
    _currentStudent = student;
    emit currentStudentChanged(student);
}

void MainWindow::on_actionNew_triggered()
{
}

void MainWindow::on_actionOpen_triggered()
{
    QString file = QStringLiteral("project.json");
    _loading = true;
    // TODO
    ui->scriptArea->setCurrentSprite(0);
    _pyController.resetApp();
    if(_slc.loadScratch(_appDir.filePath(file), ui->spriteSelect))
    {
        _currentSprite = getSpriteVector()->at(0);
        emit changeCurrentSprite(_currentSprite);
    }

    _loading = false;
    reloadCodeAll();

    qInfo() << "loaded file:" << file;
}

void MainWindow::on_actionSave_triggered()
{
    QString file = QStringLiteral("project.json");
    _slc.saveScratch(_appDir.filePath(file), ui->spriteSelect);

    qInfo() << "saved file:" << file;
}

void MainWindow::on_actionPython_triggered()
{
    _appDir.mkdir("out");
    QDir out(_appDir.filePath("out"));

    // code
    _Cgen.generateAllFiles(out, getBackgroundSprite(), getSpriteVector());

    // lib
    out.mkdir("it4k");
    QDir libDst(out.filePath("it4k"));
    QDir libSrc("python/it4k");

    QStringList libFiles = libSrc.entryList(QDir::Files);
    for (QStringList::const_iterator it = libFiles.constBegin(); it != libFiles.constEnd(); it++)
    {
        QString file = (*it);
        QFile::copy(libSrc.filePath(file), libDst.filePath(file));
    }

    // media
    out.mkdir("media");
    QDir media(out.filePath("media"));

    const CostumeVector *cVec = getBackgroundSprite()->getCostumeVector();
    for (CostumeVector::const_iterator cIt = cVec->begin(); cIt != cVec->end(); cIt++)
        (*cIt)->exportFile(media);

    for (SpriteVector::const_iterator spIt = getSpriteVector()->begin(); spIt != getSpriteVector()->end(); spIt++)
    {
        const CostumeVector *cVec = (*spIt)->getCostumeVector();
        for (CostumeVector::const_iterator cIt = cVec->begin(); cIt != cVec->end(); cIt++)
            (*cIt)->exportFile(media);
    }

    qInfo() << "exported python app:" << out.absolutePath();
}
