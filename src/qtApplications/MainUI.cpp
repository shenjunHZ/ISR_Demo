#include "MainUI.hpp"
#include "ui_MainUI.h"
#include "applications/commonControl.hpp"
#include "LineEdit/LineEdit.h"
#include <libdsl/DStr.h>
#include <QtWidgets/QLineEdit>
#include <QtCore/QByteArray>
#include <QDesktopServices>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <libdsl/DNetUtil.h>
#include <Tools/Dir.h>
#include <QSettings>
#include <QProcess>
#include <iphlpapi.h>

using namespace DSGUI;

namespace applications
{
    MainUI::MainUI(IISRService& service, QWidget *parent)
        : QDialog(parent, Qt::FramelessWindowHint)
        , m_bmousePressed(false)
        , m_IISRService{service}
    {
        ui.reset( new Ui::MainUI() );
        ui->setupUi(this);
        setAttribute(Qt::WA_DeleteOnClose, false);
        setAttribute(Qt::WA_TranslucentBackground, true);
        // move to middle
        QDesktopWidget* pDeskWidget = QApplication::desktop();
        // use primary screen for show
        int iScreen = pDeskWidget->primaryScreen();
        QWidget* pPrimaryScreen = pDeskWidget->screen(iScreen);

        int iWidth = pPrimaryScreen->width();
        int iHeight = pPrimaryScreen->height();
        this->move((iWidth - width()) / 2, (iHeight - height()) / 2);
        
        QString oemAppname = "ISR Demo";
        this->setWindowTitle(oemAppname);

        m_msgLabel = ui->msgLabel;
        m_msgLabel->setObjectName("msgLabel");
        m_msgLabel->hide();


        ui->leResult->setTextMargins(24, 0, 20, 0);
        ui->leResult->installEventFilter(this);
        ui->leResult->setReadOnly(true);
        ui->ParserBg->installEventFilter(this);
        ui->ParserBg->setFocus();

        connect(ui->startRecordBtn,  SIGNAL(clicked()),     this, SLOT(onStartRecord()));
        connect(ui->endRecordBtn,    SIGNAL(clicked()),     this, SLOT(onEndRecord()));
        connect(ui->btnQuit,         SIGNAL(clicked()),     this, SLOT(onBtnQuit()));

        SET_MODULE_STYLE_BY_PATH("MainUI");

        this->setModal(true);
        this->activateWindow();

        m_IISRService.setRegisterNotify(std::bind(&MainUI::analysisResult, this, std::placeholders::_1, std::placeholders::_2));
    }

    MainUI::~MainUI()
    {

    }

    int MainUI::init()
    {
        initCommSkinInfo();
        return 0;
    }

    void MainUI::initCommSkinInfo()
    {	
        ui->leResult->setText("");
    }

    void MainUI::SetErrorMsg(const QString& tipMsg)
    {
        if (!this->isVisible())
        {
            QDialog::setVisible(true);
        }
        m_msgLabel->setText(tipMsg);
        if (tipMsg.isEmpty())
        {
            m_msgLabel->hide();
        }
        else
        {
            m_msgLabel->setText(tipMsg);
            m_msgLabel->show();
        }
    }

    void MainUI::UpdateLoginState(bool bLogin)
    {
        if (bLogin)
        {
            ui->ParserBg->setFocus();
        }
    }

    bool MainUI::eventFilter(QObject *obj, QEvent *event)
    {
        if (obj == ui->ParserBg)
        {
            if (event->type() == QEvent::MouseButtonPress)
            {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
                if (mouseEvent->button() == Qt::LeftButton)
                {
                    m_mousePos = mouseEvent->globalPos();
                    m_bmousePressed = true;
                    return true;
                }
            }
            else if (event->type() == QEvent::MouseMove)
            {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
                if ((mouseEvent->buttons() & Qt::LeftButton) && m_bmousePressed)
                {
                    int xDis = mouseEvent->globalPos().x() - m_mousePos.x();
                    int yDis = mouseEvent->globalPos().y() - m_mousePos.y();
                    m_mousePos = mouseEvent->globalPos();
                    QPoint pos = mapToGlobal(QPoint(0,0));
                    move(pos.x() + xDis,pos.y() + yDis);
                    return true;
                }
            }
            else if (event->type() == QEvent::MouseButtonRelease)
            {
                if(m_bmousePressed)
                {
                    ui->ParserBg->setFocus();
                }
                m_bmousePressed = false;

                QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
                if (mouseEvent->button() == Qt::LeftButton)
                {
                    return true;
                }
            }
            else if (event->type() == QEvent::KeyPress)
            {
                QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
                if (keyEvent->key() == Qt::Key_R)
                {
                    onStartRecord();
                    return true;
                }
                else if (keyEvent->key() == Qt::Key_E)
                {
                    onEndRecord();
                    return true;
                }
            }
        }

        return QDialog::eventFilter(obj, event);
    }

    void MainUI::moveEvent(QMoveEvent *event)
    {
        const int margin = 20;
        QPoint pt = event->pos();
        QRect rtWnd = QApplication::desktop()->availableGeometry(pt);
        if (pt.x()+this->width()<=margin)
        {
            pt = QPoint(-this->width()+margin, pt.y());
        }
        if (pt.x()>rtWnd.right()-margin)
        {
            pt = QPoint(rtWnd.right()-margin, pt.y());
        }
        if (pt.y()+this->height()<=margin)
        {
            pt = QPoint(pt.x(), -this->height()+margin);
        }
        if (pt.y()>rtWnd.bottom()-margin)
        {
            pt = QPoint(pt.x(), rtWnd.bottom()-margin);
        }
        move(pt);
    }

    void MainUI::onStartRecord()
    {
        ui->leResult->clear();

        ui->startRecordBtn->setDisabled(true);
        m_IISRService.srStartListening();
    }

    void MainUI::onBtnQuit()
    {
        UpdateLoginState(false);

        m_IISRService.srStopListening();
        this->accept();
    }

    void MainUI::onEndRecord()
    {
        QWidget *obj = (QWidget*)sender();
        
        m_IISRService.srStopListening();
        ui->startRecordBtn->setEnabled(true);
    }

    void MainUI::analysisResult(const std::string& result, const bool& isLast)
    {

        QString leResult = ui->leResult->text();
        ui->leResult->setText(QString::fromLocal8Bit(result.c_str()));

        if (isLast)
        {
            ui->startRecordBtn->setEnabled(true);
        }
    }

} // namespace operation