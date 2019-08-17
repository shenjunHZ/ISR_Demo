#pragma once
#include <QtWidgets/QDialog>
#include <QtGui/QPalette>
#include <QtCore/QString>
#include <QtGui/QMouseEvent>
#include <QtGui/QImage>
#include <QtGui/QBitmap>
#include <QtGui/QPixmap>
#include <QtGui/QIcon>
#include <QtCore/QThread>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <memory>
#include "applications/ISRService.hpp"

namespace Ui 
{
    class MainUI;
};
namespace common
{
    class CLocalIniFile;
}

namespace applications
{
    class NoticeFrame : public QFrame
    {
        Q_OBJECT

        Q_PROPERTY(bool notice READ notice WRITE setNotice)
    public:
        NoticeFrame(QWidget*parent = 0):QFrame(parent)
            ,m_bNotice(false)
        {
        }
        bool notice() const{return m_bNotice;}
        void setNotice(bool n){m_bNotice = n;}
    private:
        bool  m_bNotice;
    };

    class MainUI : public QDialog
    {
        Q_OBJECT

    public:
        MainUI(IISRService& service, QWidget *parent = 0);
        ~MainUI();

        int init();
        void initCommSkinInfo();

    protected:
        virtual bool eventFilter(QObject *obj, QEvent *event);
        virtual void moveEvent(QMoveEvent *event);

    private slots:
        void onStartRecord();
        void onBtnQuit();
        void onEndRecord();

    private:
        void SetErrorMsg(const QString& tipMsg);  
        void UpdateLoginState(bool bLogin);
        void analysisResult(const std::string& result, const bool& isLast);

    private:
        std::unique_ptr<Ui::MainUI>		ui;
        QLabel*                         m_msgLabel;
        NoticeFrame*					frUserName;
        NoticeFrame*					frUserPwd;
        NoticeFrame*					frAddress;
        NoticeFrame*					frPort;
        QPoint							m_mousePos;
        bool							m_bmousePressed;
        IISRService& m_IISRService;
    };
}

