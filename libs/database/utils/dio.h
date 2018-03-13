/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2005-05-17
 * Description : low level files management interface.
 *
 * Copyright (C) 2005      by Renchi Raju <renchi dot raju at gmail dot com>
 * Copyright (C) 2012-2013 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#ifndef _DIGIKAM_IO_H_
#define _DIGIKAM_IO_H_

// Qt includes

#include <QObject>

// Local includes

#include "digikam_export.h"

class QUrl;

namespace Digikam
{

class PAlbum;
class ImageInfo;
class IOJobData;
class ProgressItem;

class DIGIKAM_EXPORT DIO : public QObject
{
    Q_OBJECT

public:

    static void cleanUp();

    /**
     * All DIO methods will take care for sidecar files, if they exist
     */

    /// Copy an album to another album
    static void copy(PAlbum* const src, PAlbum* const dest);

    /// Copy items to another album
    static void copy(const QList<ImageInfo>& infos, PAlbum* const dest);

    /// Copy an external file to another album
    static void copy(const QUrl& src, PAlbum* const dest);

    /// Copy external files to another album
    static void copy(const QList<QUrl>& srcList, PAlbum* const dest);

    /// Move an album into another album
    static void move(PAlbum* const src, PAlbum* const dest);

    /// Move items to another album
    static void move(const QList<ImageInfo>& infos, PAlbum* const dest);

    /// Move external files another album
    static void move(const QUrl& src, PAlbum* const dest);

    /// Move external files into another album
    static void move(const QList<QUrl>& srcList, PAlbum* const dest);

    static void del(const QList<ImageInfo>& infos, bool useTrash);
    static void del(const ImageInfo& info, bool useTrash);
    static void del(PAlbum* const album, bool useTrash);

    /// Rename item to new name
    static void rename(const ImageInfo& info, const QString& newName);

    static DIO* instance();

Q_SIGNALS:

    void signalRenameSucceeded(const QUrl&);
    void signalRenameFailed(const QUrl&);

private:

enum Operation
{
    CopyAlbum           = 1 << 0,
    CopyImage           = 1 << 1,
    CopyFiles           = 1 << 2,
    MoveAlbum           = 1 << 3,
    MoveImage           = 1 << 4,
    MoveFiles           = 1 << 5,
    Rename              = 1 << 6,
    Delete              = 1 << 7,
    Trash               = 1 << 8,
};

    DIO();
    ~DIO();

    void renameFile(IOJobData* const data);

    void processRename(IOJobData* const data);
    void processJob(IOJobData* const data);
    void createJob(IOJobData* const data);

    ProgressItem* getProgressItem(int operation);

private Q_SLOTS:

    void slotResult();
    void slotCancel(ProgressItem* item);
    void slotOneProccessed(int operation);
    void slotRenamed(const QUrl& oldUrl, const QUrl& newUrl);

private:

    friend class DIOCreator;
};

// -----------------------------------------------------------------------------------------

class SidecarFinder
{
public:

    explicit SidecarFinder(const QList<QUrl>& files);
    explicit SidecarFinder(const QUrl& file);

    QList<QUrl> localFiles;

    QList<QString> localFileSuffixes;

private:

    void process(const QList<QUrl>&);
};

// -----------------------------------------------------------------------------------------

class GroupedImagesFinder
{
public:

    explicit GroupedImagesFinder(const QList<ImageInfo>& source);

    QList<ImageInfo> infos;

private:

    void process(const QList<ImageInfo>& source);
};

} // namespace Digikam

#endif // _DIGIKAM_IO_H_
