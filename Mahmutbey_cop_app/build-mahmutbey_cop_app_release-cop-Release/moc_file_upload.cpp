/****************************************************************************
** Meta object code from reading C++ file 'file_upload.h'
**
** Created: Thu Sep 20 17:21:49 2018
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../mahmutbey_cop_app/file_upload.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'file_upload.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_File_Upload[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      13,   12,   12,   12, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_File_Upload[] = {
    "File_Upload\0\0finish_fileUpload()\0"
};

void File_Upload::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        File_Upload *_t = static_cast<File_Upload *>(_o);
        switch (_id) {
        case 0: _t->finish_fileUpload(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData File_Upload::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject File_Upload::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_File_Upload,
      qt_meta_data_File_Upload, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &File_Upload::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *File_Upload::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *File_Upload::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_File_Upload))
        return static_cast<void*>(const_cast< File_Upload*>(this));
    return QThread::qt_metacast(_clname);
}

int File_Upload::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void File_Upload::finish_fileUpload()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
