/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2008-12-26
 * Description : a tool to export items to Facebook web service
 *
 * Copyright (C) 2008-2009 by Luka Renko <lure at kubuntu dot org>
 * Copyright (C) 2008-2018 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2018      by Thanh Trung Dinh <dinhthanhtrung1996 at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#ifndef DIGIKAM_FB_TALKER_H
#define DIGIKAM_FB_TALKER_H

// Qt includes

#include <QList>
#include <QString>
#include <QTime>
#include <QObject>
#include <QUrl>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QMap>

// Local includes

#include "fbitem.h"

// O2 include

#include "o2.h"
#include "o0globals.h"

class QDomElement;

namespace Digikam
{

class FbTalker : public QObject
{
    Q_OBJECT

public:

    explicit FbTalker(QWidget* const parent);
    ~FbTalker();

    QString      getAccessToken()    const;
    unsigned int getSessionExpires() const;

    FbUser  getUser() const;

    bool    loggedIn() const;
    void    cancel();
    void    authenticate(bool imposed);
    void    reauthenticate();
    void    logout();
    void    link();
    void    unlink();

    void    removeUserAccount(const QString& account);
    
    void    listAlbums(long long userID = 0);

    void    createAlbum(const FbAlbum& album);

    bool    addPhoto(const QString& imgPath, const QString& albumID,
                     const QString& caption);

Q_SIGNALS:

    void    signalBusy(bool val);
    void    signalLoginProgress(int step, int maxStep = 0, const QString& label = QString());
    void    signalLoginDone(int errCode, const QString& errMsg);
    void    signalAddPhotoDone(int errCode, const QString& errMsg);
    void    signalCreateAlbumDone(int errCode, const QString& errMsg, const QString &newAlbumID);
    void    signalListAlbumsDone(int errCode, const QString& errMsg, const QList <FbAlbum>& albumsList);
    void    signalLinkingSucceeded();
    void    signalOpenBrowser(const QUrl& url);
    void    signalCloseBrowser();

private:

    //QString getApiSig(const QMap<QString, QString>& args);
    void    authenticationDone(int errCode, const QString& errMsg);
    void    getLoggedInUser();

    void    doOAuth();
    QString errorToText(int errCode, const QString& errMsg);
    int     parseErrorResponse(const QDomElement& e, QString& errMsg);
    void    parseResponseGetLoggedInUser(const QByteArray& data);
    void    parseResponseAddPhoto(const QByteArray& data);
    void    parseResponseCreateAlbum(const QByteArray& data);
    void    parseResponseListAlbums(const QByteArray& data);

private Q_SLOTS:
    
    void    slotFinished(QNetworkReply* reply);
    void    slotAccept();
    void    slotReject();
    void    slotResponseTokenReceived(const QMap<QString, QString>& rep);
    void    slotLinkingFailed();
    void    slotLinkingSucceeded();
    void    slotOpenBrowser(const QUrl& url);
    void    slotCloseBrowser();

private:

    class Private;
    Private* const d;
};

} // namespace Digikam

#endif // DIGIKAM_FB_TALKER_H
