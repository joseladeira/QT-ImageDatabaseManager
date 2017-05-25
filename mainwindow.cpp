#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // ui
    ui->setupUi(this);
    this->setWindowTitle("Image Database manager");

    std::initializer_list <std::pair<QString, int > > headerNameList {
        std::make_pair("File",0),
                std::make_pair("Width",1),
                std::make_pair("Height",2),
                std::make_pair("Size",3),
                std::make_pair("Date",4),
                std::make_pair("Imagedata",5)
    };
    col = new QMap<QString, int >(headerNameList);

    connection();

    model = new QSqlTableModel(this);
    model->setTable(tablename);
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    model->setHeaderData(col->value("File"), Qt::Horizontal, tr("File"));
    model->setHeaderData(col->value("Width"), Qt::Horizontal, tr("Width"));
    model->setHeaderData(col->value("Height"), Qt::Horizontal, tr("Height"));
    model->setHeaderData(col->value("Size"), Qt::Horizontal, tr("Size"));
    model->setHeaderData(col->value("Date"), Qt::Horizontal, tr("Date"));

    ui->tableView->setModel(model);
    ui->tableView->hideColumn(col->value("Imagedata"));
    ui->tableView->setSortingEnabled(true);
    ui->tableView->resizeColumnsToContents();

    model->select();

    connect(ui->tableView, SIGNAL(clicked(QModelIndex)), this, SLOT(setLabelPicture(QModelIndex) ));
    connect(ui->tableView, SIGNAL(activated(QModelIndex)), this, SLOT(setLabelPicture(QModelIndex) ));
    connect(ui->tableView, SIGNAL(clicked(QModelIndex)), this, SLOT(set_current_row(QModelIndex)));
    connect(ui->tableView, SIGNAL(activated(QModelIndex)), this, SLOT(set_current_row(QModelIndex)));
    connect(ui->pushButton_4, SIGNAL(clicked(bool)),this, SLOT(save_as()));



}

// DB connection
void MainWindow::connection() {

    // default db initializing
    dbFilename = "database.db";
    db = QSqlDatabase::addDatabase( "QSQLITE" );
    db.setDatabaseName( dbFilename );
    if (!db.open()) {
        QMessageBox::critical(0, qApp->tr("Cannot open database"),
                              qApp->tr("Unable to establish a database connection.\n"
                                       "This example needs SQLite support. Please read "
                                       "the Qt SQL driver documentation for information how "
                                       "to build it.\n\n"
                                       "Click Cancel to exit."), QMessageBox::Cancel);
        return;
    }
    QSqlQuery query = QSqlQuery( db );
    query.prepare("CREATE TABLE IF NOT EXISTS imgTable "
                  "( filename varchar(100) PRIMARY KEY, width varchar(30), height varchar(30), "
                  "size INT, datetime DATETIME, imagedata BLOB )" );
    //query.bindValue( ":tablename", tablename );
    if( !query.exec() )
        qDebug() << "Error creating table:\n" << query.lastError();
}

MainWindow::~MainWindow()
{
    db.close();
    delete ui;
}

/*
 * add images to DB
 */
void MainWindow::on_pushButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter(tr("Images (*.bmp  *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"));
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    QStringList fileNames;
    if (dialog.exec()) fileNames = dialog.selectedFiles();

    for (QString filename : fileNames) {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) return;
        QByteArray inByteArray = file.readAll();
        QPixmap outPixmap = QPixmap();
        outPixmap.loadFromData( inByteArray );
        QFileInfo fi(filename);
        QString width = QString::number(outPixmap.width());
        QString height = QString::number(outPixmap.height());

        //( "INSERT INTO imgTable (filename, resolution, size, datetime, imagedata) "
        //  "VALUES (:filename, :resolution, :size, :datetime, :imagedata)" );

        int row = model->rowCount(QModelIndex());
        model->insertRow(row, QModelIndex());
        model->setData(model->index(row,col->value("File"),QModelIndex()),fi.fileName(), Qt::EditRole);
        model->setData(model->index(row,col->value("Width"),QModelIndex()),width, Qt::EditRole);
        model->setData(model->index(row,col->value("Height"),QModelIndex()),height, Qt::EditRole);
        model->setData(model->index(row,col->value("Size"),QModelIndex()),fi.size(), Qt::EditRole);
        model->setData(model->index(row,col->value("Date"),QModelIndex()),fi.created(), Qt::EditRole);
        model->setData(model->index(row,col->value("Imagedata"),QModelIndex()),inByteArray, Qt::EditRole);

    }
    ui->tableView->resizeColumnsToContents();
}

/*
 * Remove image from DB
 */
void MainWindow::on_pushButton_2_clicked()
{    
    QModelIndexList selectedLines = ui->tableView->selectionModel()->selectedIndexes();
    for (QModelIndex line : selectedLines) {
        model->removeRow(line.row());
    }
}



// commit changes to DB
void MainWindow::on_pushButton_5_clicked()
{
    model->database().transaction();
    if (model->submitAll()) {
        model->database().commit();
    } else {
        model->database().rollback();
        QMessageBox::warning(this, tr("Cached Table"),
                             tr("The database reported an error: %1")
                             .arg(model->lastError().text()));
    }

    ui->tableView->resizeColumnsToContents();
    model->select();
}

// Undo all changes made to DB
void MainWindow::on_pushButton_6_clicked()
{
    model->revertAll();
    ui->tableView->resizeColumnsToContents();
    model->select();
}

// preview image box
void MainWindow::setLabelPicture (const QModelIndex  qmi) {

    QSqlRecord rec = model->record( (const int) qmi.row());
    QPixmap inPixmap = QPixmap();
    inPixmap.loadFromData( rec.value(col->value("Imagedata")).toByteArray() );
    ui->label->setPixmap(inPixmap);

}

/*
 * Save as - image from DB
 */
void MainWindow::save_as()
{

    QSqlRecord rec = model->record( current_row );
    QString filename = rec.value(col->value("File")).toString(); // hint filename
    QPixmap pixmap = QPixmap();
    pixmap.loadFromData( rec.value(col->value("Imagedata")).toByteArray() );


    QFileDialog dialog(this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    filename = dialog.getSaveFileName(this, tr("Save File"),
                                      filename, tr("Images (*.bmp  *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"));


    QStringList filename_split = filename.split(".");
    pixmap.save(filename,qPrintable(filename_split.last().toUpper()),100);
}
