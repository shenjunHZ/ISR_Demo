#include "AppConfig.hpp"
#include "commonControl.hpp"
#include "Tools/Dir.h"
#include "Tools/File.h"
#include "GlobalDataCenter/GlobalDataCenter.h"
#include "MessageNotify/MessageNotify.h"
#include "MessageBox/MessageBox.h"
#include "DSGuiGlobalData.h"
#include "libdsl/DPrintLog.h"
#include <QtCore/QDir>
#include <QtGui/QIcon>
#include <QtGui/QScreen>
#include <QtGui/QFont>
#include <QtCore/QTime>

namespace applications
{

    AppConfig::AppConfig(QApplication& app)
        : m_sharedMenPtr(std::make_unique<QSharedMemory>("ISR Demo"))
    {
        // set path
        QString strCurPath = QCoreApplication::applicationDirPath();
        QDir::setCurrent(strCurPath);
        DSGUI::DSDir::SetCurrentPath(strCurPath);

        //set app
        app.setWindowIcon(QIcon("./monkey.ico"));
        app.setOrganizationName(QString("ISR Demo"));
        app.setApplicationName(QString("ISR Demo"));
        QScreen* screen = app.primaryScreen();
        qreal dpi = screen->logicalDotsPerInch() / 96;
        QFont chnFont("Microsoft YaHei");
        chnFont.setPixelSize(14 * dpi);
        app.setFont(chnFont);

        // global common component
        Common::CGlobalDataCenter::GetInstance().SetCurrentDirPath_UTF8(strCurPath.toUtf8().data());
        Common::CGlobalDataCenter::GetInstance().SetCurrentDirPath(strCurPath.toLocal8Bit().data());

        // set international
        QString strNotifyQmPath = QCoreApplication::applicationDirPath().append("/Language/Language_en/DSMessageNotify_en.qm");
        QString strLang("English");

        // set notify
        DSGUI::DSMessageNotify::Instance().SetParam(QStringList() << strNotifyQmPath, QStringList() << DSGUI::GetSkinVersion(), strLang);
        DSGUI::DSMessageNotify::Instance().StartMessageProcess();
        DSGUI::DSMessageNotify::Instance().SetMessageNotifyTitle(QObject::tr("Notify"));

        // set style
        QString strStyle = "";
        QFile fpStyle(applications::GetSkinPath("public", "style.qss"));
        if (fpStyle.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            strStyle = fpStyle.readAll();
            fpStyle.close();
        }
        QString strCSSFilePath = applications::GetSkinPath("") + QString("/DSGUI/CSS/");
        QStringList strList;
        strList.push_back("*.CSS");
        QStringList strFileList = QDir(strCSSFilePath).entryList(strList, QDir::Files);
        qApp->setStyleSheet(strStyle + DSGUI::DSFile::ReadAll(strFileList, strCSSFilePath));
    }

    bool AppConfig::runOnlyOne()
    {
        if (isRunning())
        {
            DSGUI::DSMessageBox::ShowInformation(NULL, QObject::tr("Notice"), QObject::tr("Application had runned!"), DSGUI::DSMessageBox::Ok);
            return true;
        }

        return false;
    }

    void AppConfig::releaseShareMemory()
    {
        if (m_sharedMenPtr && m_sharedMenPtr->isAttached())
        {
            m_sharedMenPtr->detach();
        }
    }

    bool AppConfig::isRunning()
    {
        volatile int i = 2;
        while (i--)
        {
            if (m_sharedMenPtr->attach(QSharedMemory::ReadOnly))
            {
                m_sharedMenPtr->detach();
            }
        }
        if (!m_sharedMenPtr->create(1))
        {
            return true;
        }

        return false;
    }

}