/****************************************************************************
** Meta object code from reading C++ file 'progressdlg.h'
**
** Created: Thu Sep 20 17:21:51 2018
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../mahmutbey_cop_app/progressdlg.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'progressdlg.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_progressDlg[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      75,   13,   12,   12, 0x0a,
     143,  116,   12,   12, 0x0a,
     167,   12,   12,   12, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_progressDlg[] = {
    "progressDlg\0\0"
    "uiTotalFiles,uiRemaindFile,uiTotalPrograssSize,uiPrograssSize\0"
    "prograssing(qint32,qint32,qint32,qint32)\0"
    "uiTotalFiles,uiRemaindFile\0"
    "finished(qint32,qint32)\0closeDlg()\0"
};

void progressDlg::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        progressDlg *_t = static_cast<progressDlg *>(_o);
        switch (_id) {
        case 0: _t->prograssing((*reinterpret_cast< qint32(*)>(_a[1])),(*reinterpret_cast< qint32(*)>(_a[2])),(*reinterpret_cast< qint32(*)>(_a[3])),(*reinterpret_cast< qint32(*)>(_a[4]))); break;
        case 1: _t->finished((*reinterpret_cast< qint32(*)>(_a[1])),(*reinterpret_cast< qint32(*)>(_a[2]))); break;
        case 2: _t->closeDlg(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData progressDlg::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject progressDlg::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_progressDlg,
      qt_meta_data_progressDlg, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &progressDlg::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *progressDlg::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *progressDlg::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_progressDlg))
        return static_cast<void*>(const_cast< progressDlg*>(this));
    return QDialog::qt_metacast(_clname);
}

int progressDlg::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
