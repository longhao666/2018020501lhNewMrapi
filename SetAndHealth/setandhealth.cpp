#include "setandhealth.h"
#include "ui_setandhealth.h"
#include <QMessageBox>
#include <QDebug>
#include "joint.h"

#define LHDEBUG 0

#define MONITOR_INTEVAL 2000         // 监视器更新周期（ms）

SetAndHealth::SetAndHealth(QWidget *parent) :
    QWidget(parent),
    uiSetAndHealth(new Ui::SetAndHealth)
{
#if LHDEBUG
    qDebug() <<__DATE__<<__TIME__<<__FILE__<<__LINE__<<__func__;
#endif
    uiSetAndHealth->setupUi(this);
    timer = NULL;
    joint = NULL;
}

SetAndHealth::~SetAndHealth()
{
#if LHDEBUG
    qDebug() <<__DATE__<<__TIME__<<__FILE__<<__LINE__<<__func__;
#endif
    delete uiSetAndHealth;
}

void SetAndHealth::SetAndHealthIint(int ID)
{
#if LHDEBUG
    qDebug() <<__DATE__<<__TIME__<<__FILE__<<__LINE__<<__func__;
    qDebug() << "ID = " << ID;
#endif
    this->joint = jointSelect(ID);
    if(!joint) {
        return ;
    }
    this->set();
    this->health();
}

void SetAndHealth::set()
{
    uint16_t data16 = 0;
    // 读取ID 不能放在定时器里面,不然一直更新
    jointGetId(joint, &data16, 100, NULL);
    uiSetAndHealth->IDLineEdit->setText(QString::number(data16));
}

void SetAndHealth::health()
{
    if(!timer) {
        timer = new QTimer;
//        connect(timer,QTimer::timeout(),this,SetAndHealth::myTimerSlot());
        connect(timer,SIGNAL(timeout()),this,SLOT(myTimerSlot()));
//        timer->start(MONITOR_INTEVAL);
    }
    this->myTimerSlot();
}

void SetAndHealth::myTimerSlot()
{
#if LHDEBUG
    qDebug() <<__DATE__<<__TIME__<<__FILE__<<__LINE__<<__func__;
#endif
    if(!joint) {
        return ;
    }
    uint16_t data16 = 0;
    uint32_t data32 = 0;
    uint16_t data_L = 0;
    uint16_t data_H = 0;
    // 读取速度
    jointGet(SYS_SPEED_L, 4, (Joint *)joint, (void *)&data32, 50, NULL);
    jointGet(SYS_REDU_RATIO, 2, (Joint *)joint, (void *)&data16, 50, NULL); // 减速比
    data_L = (uint16_t)(data32 & 0xffff);
    data_H = (uint16_t)(data32>>16);
    float speed = (float) ((data_L + data_H * 65536) * 60.0 / 65536.0 / data16);
    uiSetAndHealth->speedLineEdit->setText(QString::number(speed, 'f', 2) + "rpm");
    // 读取电流
    jointGetCurrent(joint, &data32, 5, NULL);
    data_L = (uint16_t)(data32 & 0xffff);
    data_H = (uint16_t)(data32>>16);
    double temp = data_L + (data_H * 65536);
    uiSetAndHealth->currentLineEdit->setText(QString::number(temp / 1000, 'f', 2) + "A");
    // 下面这个种方法也行,但是上面这个更自然一点吧.个人感觉
    // 无符号变有符号,不然电流相反后数值特别大
//    int temp = (int)data32;
//    if(data_t < 0) {
//        temp = -temp;
//        uiSetAndHealth->currentLineEdit->setText(QString::number((double)temp/1000, 'f', 2) + "A");
//    }else {
//        uiSetAndHealth->currentLineEdit->setText(QString::number((double)temp/1000, 'f', 2) + "A");
//    }
    // 读取位置
    jointGetPosition(joint, &data32, 5, NULL);
    data_L = (uint16_t)(data32 & 0xffff);
    data_H = (uint16_t)(data32>>16);
    uiSetAndHealth->pOS_LLineEdit->setText(QString::number((double)data_L * 360.0 / 65536.0, 'f', 2) + "°");
    uiSetAndHealth->pOS_HLineEdit->setText(QString::number((short)data_H));
    // 读取温度
    jointGetTemp(joint,&data16,5,NULL);
    uiSetAndHealth->tempLineEdit->setText(QString::number(static_cast<double>(data16) / 10, 'f', 1) + "°C");
    // 读取电压
    jointGetVoltage(joint,&data16,5,NULL);
    uiSetAndHealth->voltageLineEdit->setText(QString::number((double)data16 / 100, 'f', 2) + "V");
    // 读取编码器电池电压
    jointGet(BAT_VOLT, 2, (Joint *)joint, (void *)&data16, 50, NULL);
    uiSetAndHealth->bATVoltLineEdit->setText(QString::number((double)data16 / 100, 'f', 2) + "V");
    // 更新三轴加速度,并显示
    jointGet(ACC_X, 2, (Joint *)joint, (void *)&data16, 50, NULL);
    uiSetAndHealth->accXLineEdit->setText(QString::number((short)data16) + "mg");
    jointGet(ACC_Y, 2, (Joint *)joint, (void *)&data16, 50, NULL);
    uiSetAndHealth->accYLineEdit->setText(QString::number((short)data16) + "mg");
    jointGet(ACC_Z, 2, (Joint *)joint, (void *)&data16, 50, NULL);
    uiSetAndHealth->accZLineEdit->setText(QString::number((short)data16) + "mg");
    // 更新编码器,并显示
    jointGet(MOT_MT_DAT, 2, (Joint *)joint, (void *)&data16, 50, NULL);
    uiSetAndHealth->m_TurnLineEdit->setText(QString::number((short)data16));
    jointGet(MOT_ST_DAT, 2, (Joint *)joint, (void *)&data16, 50, NULL);
    uiSetAndHealth->s_TurnLineEdit->setText(QString::number((double)data16 * 360.0/65536.0) + "°");
    // 更新错误码，并显示
    jointGet(SYS_ERROR, 2, (Joint *)joint, (void *)&data_L, 50, NULL);
    // 按错误码显示文字
    uiSetAndHealth->errorTextEdit->clear();
    if (data_L & ERROR_MASK_OVER_CURRENT) {
      uiSetAndHealth->errorTextEdit->append("过流，错误码0x0001\n");
    }
    if (data_L & ERROR_MASK_OVER_VOLTAGE) {
      uiSetAndHealth->errorTextEdit->append("过压，错误码0x0002\n");
    }
    if (data_L & ERROR_MASK_UNDER_VOLTAGE) {
      uiSetAndHealth->errorTextEdit->append("欠压，错误码0x0004\n");
    }
    if (data_L & ERROR_MASK_OVER_TEMP) {
      uiSetAndHealth->errorTextEdit->append("过温，错误码0x0008\n");
    }
    if (data_L & ERROR_MASK_BATTERY) {
      uiSetAndHealth->errorTextEdit->append("编码器电池错误，错误码0x0010\n");
    }
    if (data_L & ERROR_MASK_ENCODER) {
      uiSetAndHealth->errorTextEdit->append("码盘错误，错误码0x0020\n");
    }
    if (data_L & ERROR_MASK_POTEN) {
      uiSetAndHealth->errorTextEdit->append("电位器错误，错误码0x0040\n");
    }
    if (data_L & ERROR_MASK_CURRENT_INIT) {
      uiSetAndHealth->errorTextEdit->append("电流检测错误，错误码0x0080\n");
    }
    if (data_L & ERROR_MASK_FUSE) {
      uiSetAndHealth->errorTextEdit->append("保险丝断开错误，错误码0x0100\n");
    }
    if (data_L == 0) {
      uiSetAndHealth->errorTextEdit->append("No Error\n");
    }
}

void SetAndHealth::on_IDPushButton_clicked()
{
#if LHDEBUG
    qDebug() <<__DATE__<<__TIME__<<__FILE__<<__LINE__<<__func__;
#endif
    // 不断电重启的话既不能检测到旧ID,也不能用新ID
    if(!joint) {
        return ;
    }
    uint16_t newID = uiSetAndHealth->IDLineEdit->text().toShort();
    jointSetID(joint, newID, 100, NULL);
    jointSetSave2Flash(joint,100,NULL);
//    emit IDChanged(newID);
    QMessageBox::information(this, "information", QObject::tr("请断电重启，不然新旧ID都不能用了！"), QMessageBox::Ok);
}

void SetAndHealth::on_setZeroPushButton_clicked()
{
#if LHDEBUG
    qDebug() <<__DATE__<<__TIME__<<__FILE__<<__LINE__<<__func__;
#endif
    if(!joint) {
        return ;
    }
    uint16_t workMode = 0;
    jointGetMode(joint, &workMode, 100, NULL);
    if(workMode == 3) {
#if  0
        qDebug() << tr("  尴尬了  ");
        uint32_t udata32 = 0;
        int32_t data32 = 0;
        jointGetTAG_SPEED_L(joint, &data32, 100, NULL);
        qDebug() << data32 << "dfdsf";
        jointGetSpeed(joint, &udata32, 100, NULL);
        qDebug() << udata32 << "dfdsf==============";
        jointGetTAG_POSITION_L(joint, &data32, 100, NULL);
        qDebug() << data32 * 360.0/65536.0 << "dfdsf====uuuuuuuuuuu==========";
        jointGetPosition(joint, &udata32, 100, NULL);
        qDebug() << (int)udata32 * 360.0/65536.0  << "dfdsf===8888888===========";
#endif
        emit ZeroPositionSeted();
        jointSetZero(joint,100,NULL);
        emit ZeroPositionSeted();
#if  0
        qDebug() << tr("  尴尬了  ");
//        uint32_t udata32 = 0;
//        int32_t data32 = 0;
        jointGetTAG_SPEED_L(joint, &data32, 100, NULL);
        qDebug() << data32 << "dfdsf";
        jointGetSpeed(joint, &udata32, 100, NULL);
        qDebug() << udata32 << "dfdsf==============";
        jointGetTAG_POSITION_L(joint, &data32, 100, NULL);
        qDebug() << data32 * 360.0/65536.0 << "dfdsf====uuuuuuuuuuu==========";
        jointGetPosition(joint, &udata32, 100, NULL);
        qDebug() << (int)udata32 * 360.0/65536.0 << "dfdsf===8888888===========";
#endif
    }
}

void SetAndHealth::on_ENonPPushButton_clicked()
{
    if(!joint) {
        return ;
    }
    jointSetClearError(joint, 100, NULL);
    qDebug() << tr("SetAndHealth::on_clearErrorButton_clicked(): done.");
}

void SetAndHealth::on_clearErrorButton_clicked()
{
    if(!joint) {
        return ;
    }
    jointSetClearError(joint, 100, NULL);
    qDebug() << tr("SetAndHealth::on_clearErrorButton_clicked(): done.");
}

void SetAndHealth::on_updateButton_clicked()
{
    this->myTimerSlot();
}
