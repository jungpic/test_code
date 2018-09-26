/****************************************************************************
** Meta object code from reading C++ file 'updatemainframe.h'
**
** Created: Fri Sep 21 00:18:10 2018
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../mahmutbey_cop_app/updatemainframe.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'updatemainframe.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_updateMainFrame[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      17,   16,   16,   16, 0x0a,
      38,   16,   16,   16, 0x0a,
      62,   16,   16,   16, 0x0a,
      81,   16,   16,   16, 0x0a,
     100,   16,   16,   16, 0x0a,
     136,  125,   16,   16, 0x0a,
     157,   16,   16,   16, 0x0a,
     175,   16,   16,   16, 0x0a,
     190,   16,   16,   16, 0x0a,
     211,  209,   16,   16, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_updateMainFrame[] = {
    "updateMainFrame\0\0logDownloadClicked()\0"
    "scheduleUploadClicked()\0nextMonthClicked()\0"
    "prevMonthClicked()\0finishedUpdateFileList()\0"
    "year,month\0pageChanged(int,int)\0"
    "SwUpdateClicked()\0StatusUpdate()\0"
    "on_thread_finish()\0,\0"
    "ftpClient_finished(qint32,qint32)\0"
};

void updateMainFrame::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        updateMainFrame *_t = static_cast<updateMainFrame *>(_o);
        switch (_id) {
        case 0: _t->logDownloadClicked(); break;
        case 1: _t->scheduleUploadClicked(); break;
        case 2: _t->nextMonthClicked(); break;
        case 3: _t->prevMonthClicked(); break;
        case 4: _t->finishedUpdateFileList(); break;
        case 5: _t->pageChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 6: _t->SwUpdateClicked(); break;
        case 7: _t->StatusUpdate(); break;
        case 8: _t->on_thread_finish(); break;
        case 9: _t->ftpClient_finished((*reinterpret_cast< qint32(*)>(_a[1])),(*reinterpret_cast< qint32(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData updateMainFrame::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject updateMainFrame::staticMetaObject = {
    { &QFrame::staticMetaObject, qt_meta_stringdata_updateMainFrame,
      qt_meta_data_updateMainFrame, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &updateMainFrame::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *updateMainFrame::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *updateMainFrame::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_updateMainFrame))
        return static_cast<void*>(const_cast< updateMainFrame*>(this));
    return QFrame::qt_metacast(_clname);
}

int updateMainFrame::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
