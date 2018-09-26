/****************************************************************************
** Meta object code from reading C++ file 'ftpclient.h'
**
** Created: Thu Sep 20 17:21:51 2018
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../mahmutbey_cop_app/ftpclient.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ftpclient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ftpClient[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      73,   11,   10,   10, 0x05,
     141,  114,   10,   10, 0x05,
     173,  165,   10,   10, 0x05,
     189,   10,   10,   10, 0x05,

 // slots: signature, parameters, type, tag, flags
     208,  206,   10,   10, 0x0a,
     245,  237,   10,   10, 0x0a,
     265,  206,   10,   10, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ftpClient[] = {
    "ftpClient\0\0"
    "uiTotalFiles,uiRemaindFile,uiTotalPrograssSize,uiPrograssSize\0"
    "prograssing(qint32,qint32,qint32,qint32)\0"
    "uiTotalFiles,uiRemaindFile\0"
    "finished(qint32,qint32)\0ErrCode\0"
    "errCode(qint32)\0updateFileList()\0,\0"
    "ftpCommandFinished(int,bool)\0urlInfo\0"
    "addToList(QUrlInfo)\0"
    "updateDataTransferProgress(qint64,qint64)\0"
};

void ftpClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ftpClient *_t = static_cast<ftpClient *>(_o);
        switch (_id) {
        case 0: _t->prograssing((*reinterpret_cast< qint32(*)>(_a[1])),(*reinterpret_cast< qint32(*)>(_a[2])),(*reinterpret_cast< qint32(*)>(_a[3])),(*reinterpret_cast< qint32(*)>(_a[4]))); break;
        case 1: _t->finished((*reinterpret_cast< qint32(*)>(_a[1])),(*reinterpret_cast< qint32(*)>(_a[2]))); break;
        case 2: _t->errCode((*reinterpret_cast< qint32(*)>(_a[1]))); break;
        case 3: _t->updateFileList(); break;
        case 4: _t->ftpCommandFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 5: _t->addToList((*reinterpret_cast< const QUrlInfo(*)>(_a[1]))); break;
        case 6: _t->updateDataTransferProgress((*reinterpret_cast< qint64(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ftpClient::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ftpClient::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ftpClient,
      qt_meta_data_ftpClient, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ftpClient::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ftpClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ftpClient::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ftpClient))
        return static_cast<void*>(const_cast< ftpClient*>(this));
    return QObject::qt_metacast(_clname);
}

int ftpClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void ftpClient::prograssing(qint32 _t1, qint32 _t2, qint32 _t3, qint32 _t4)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ftpClient::finished(qint32 _t1, qint32 _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ftpClient::errCode(qint32 _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ftpClient::updateFileList()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}
QT_END_MOC_NAMESPACE
