/*
@tool    common interface
@author  sj
@data    2017-9-8
@version 1.0
*/
#pragma once

#include <QtCore/QString>

namespace applications
{
    class commonControl
    {
    public:

    };

    bool loadStyleSheet(const QString &sheetName, QString &styleSheet);

    QString GetSkinPath(const QString& strFolder, const QString& strSkinName = QString());

#define SET_MODULE_STYLE_BY_PATH(Folder)                                                                        \
	QString strStyleSheet;																					    \
	bool bRetLoadStyle = applications::loadStyleSheet(applications::GetSkinPath(Folder, objectName() + ".qss"), strStyleSheet);	\
	if(bRetLoadStyle)																							\
	{																											\
		setStyleSheet(strStyleSheet);																			\
	}

#define SET_MODULE_SKIN_FLODER_PATH(Floder) \
	(DSGUI::DSDir::GetCurrentPath() + "/skin/" + DSGUI::GetSkinVersion() + "/" +QString(Floder) + "/")
} // namespace common