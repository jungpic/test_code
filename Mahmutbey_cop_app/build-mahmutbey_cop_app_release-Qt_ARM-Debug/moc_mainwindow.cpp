/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created: Fri Apr 6 18:18:18 2018
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../mahmutbey_cop_app/mainwindow.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_RowHeightDelegate[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

static const char qt_meta_stringdata_RowHeightDelegate[] = {
    "RowHeightDelegate\0"
};

void RowHeightDelegate::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData RowHeightDelegate::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject RowHeightDelegate::staticMetaObject = {
    { &QItemDelegate::staticMetaObject, qt_meta_stringdata_RowHeightDelegate,
      qt_meta_data_RowHeightDelegate, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &RowHeightDelegate::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *RowHeightDelegate::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *RowHeightDelegate::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_RowHeightDelegate))
        return static_cast<void*>(const_cast< RowHeightDelegate*>(this));
    return QItemDelegate::qt_metacast(_clname);
}

int RowHeightDelegate::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QItemDelegate::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_MainWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      33,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      12,   11,   11,   11, 0x08,
      28,   11,   11,   11, 0x08,
      43,   11,   11,   11, 0x08,
      60,   11,   11,   11, 0x08,
      77,   11,   11,   11, 0x08,
     102,   11,   11,   11, 0x08,
     127,   11,   11,   11, 0x08,
     152,   11,   11,   11, 0x08,
     186,  177,   11,   11, 0x08,
     221,  215,   11,   11, 0x08,
     251,   11,   11,   11, 0x08,
     276,   11,   11,   11, 0x08,
     300,   11,   11,   11, 0x08,
     323,   11,   11,   11, 0x08,
     349,   11,   11,   11, 0x08,
     383,   11,   11,   11, 0x08,
     411,   11,   11,   11, 0x08,
     442,   11,   11,   11, 0x08,
     469,   11,   11,   11, 0x08,
     493,   11,   11,   11, 0x08,
     519,   11,   11,   11, 0x08,
     543,   11,   11,   11, 0x08,
     569,   11,   11,   11, 0x08,
     597,   11,   11,   11, 0x08,
     627,   11,   11,   11, 0x08,
     656,   11,   11,   11, 0x08,
     687,   11,   11,   11, 0x08,
     712,   11,   11,   11, 0x08,
     739,   11,   11,   11, 0x08,
     764,   11,   11,   11, 0x08,
     791,   11,   11,   11, 0x08,
     819,  813,   11,   11, 0x08,
     858,   11,   11,   11, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_MainWindow[] = {
    "MainWindow\0\0ConnectUpdate()\0ScreenUpdate()\0"
    "PEI_CallUpdate()\0CAB_CallUpdate()\0"
    "PEI_BTN_1_BLINK_Update()\0"
    "PEI_BTN_2_BLINK_Update()\0"
    "CAB_BTN_1_BLINK_Update()\0"
    "CAB_BTN_2_BLINK_Update()\0filename\0"
    "ReadCalibrationData(QString)\0event\0"
    "mousePressEvent(QMouseEvent*)\0"
    "menu_button_clicked(int)\0"
    "top_button_clicked(int)\0di_button_clicked(int)\0"
    "route_button_clicked(int)\0"
    "special_route_button_clicked(int)\0"
    "station_button_clicked(int)\0"
    "station_PA_button_clicked(int)\0"
    "on_BUTTON_CANCEL_clicked()\0"
    "on_CAB_MIC_UP_clicked()\0"
    "on_CAB_MIC_DOWN_clicked()\0"
    "on_CAB_SPK_UP_clicked()\0"
    "on_CAB_SPK_DOWN_clicked()\0"
    "on_PAMP_IN_VOL_UP_clicked()\0"
    "on_PAMP_IN_VOL_DOWN_clicked()\0"
    "on_PAMP_OUT_VOL_UP_clicked()\0"
    "on_PAMP_OUT_VOL_DOWN_clicked()\0"
    "on_CALL_MIC_UP_clicked()\0"
    "on_CALL_MIC_DOWN_clicked()\0"
    "on_CALL_SPK_UP_clicked()\0"
    "on_CALL_SPK_DOWN_clicked()\0"
    "on_PEI_JOIN_clicked()\0index\0"
    "on_LANG_COMBO_currentIndexChanged(int)\0"
    "on_TOUCH_CAL_clicked()\0"
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MainWindow *_t = static_cast<MainWindow *>(_o);
        switch (_id) {
        case 0: _t->ConnectUpdate(); break;
        case 1: _t->ScreenUpdate(); break;
        case 2: _t->PEI_CallUpdate(); break;
        case 3: _t->CAB_CallUpdate(); break;
        case 4: _t->PEI_BTN_1_BLINK_Update(); break;
        case 5: _t->PEI_BTN_2_BLINK_Update(); break;
        case 6: _t->CAB_BTN_1_BLINK_Update(); break;
        case 7: _t->CAB_BTN_2_BLINK_Update(); break;
        case 8: _t->ReadCalibrationData((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 9: _t->mousePressEvent((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 10: _t->menu_button_clicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 11: _t->top_button_clicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 12: _t->di_button_clicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 13: _t->route_button_clicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 14: _t->special_route_button_clicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 15: _t->station_button_clicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 16: _t->station_PA_button_clicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 17: _t->on_BUTTON_CANCEL_clicked(); break;
        case 18: _t->on_CAB_MIC_UP_clicked(); break;
        case 19: _t->on_CAB_MIC_DOWN_clicked(); break;
        case 20: _t->on_CAB_SPK_UP_clicked(); break;
        case 21: _t->on_CAB_SPK_DOWN_clicked(); break;
        case 22: _t->on_PAMP_IN_VOL_UP_clicked(); break;
        case 23: _t->on_PAMP_IN_VOL_DOWN_clicked(); break;
        case 24: _t->on_PAMP_OUT_VOL_UP_clicked(); break;
        case 25: _t->on_PAMP_OUT_VOL_DOWN_clicked(); break;
        case 26: _t->on_CALL_MIC_UP_clicked(); break;
        case 27: _t->on_CALL_MIC_DOWN_clicked(); break;
        case 28: _t->on_CALL_SPK_UP_clicked(); break;
        case 29: _t->on_CALL_SPK_DOWN_clicked(); break;
        case 30: _t->on_PEI_JOIN_clicked(); break;
        case 31: _t->on_LANG_COMBO_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 32: _t->on_TOUCH_CAL_clicked(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData MainWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MainWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_MainWindow,
      qt_meta_data_MainWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MainWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow))
        return static_cast<void*>(const_cast< MainWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 33)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 33;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
