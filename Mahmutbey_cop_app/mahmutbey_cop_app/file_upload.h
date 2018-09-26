#ifndef FILE_UPLOAD_H
#define FILE_UPLOAD_H
#include <QThread>

class File_Upload : public QThread
{
public:
    File_Upload();

    Q_OBJECT
public:
    explicit File_Upload(QObject *parent = 0);
private:
    void run_upload();

signals:
    void finish_fileUpload(void);
public slots:
};


#endif // FILE_UPLOAD_H
