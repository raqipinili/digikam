/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 * 
 * Date        : 2004-06-15
 * Description : Albums manager interface.
 * 
 * Copyright (C) 2004 by Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Copyright (C) 2006-2008 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2006-2007 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
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

// C Ansi includes.

extern "C"
{
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>
}

// C++ includes.

#include <cstdlib>
#include <cstdio>
#include <cerrno> 

// Qt includes.

#include <Q3ValueList>
#include <Q3Dict>
#include <Q3IntDict>
#include <QList>
#include <QFile>
#include <QDir>
#include <QByteArray>
#include <QTextCodec>

// KDE includes.

#include <kconfig.h>
#include <klocale.h>
#include <kdeversion.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kio/netaccess.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kdirwatch.h>
#include <kconfiggroup.h>
// Local includes.

#include "ddebug.h"
#include "album.h"
#include "albumdb.h"
#include "albumitemhandler.h"
#include "albumsettings.h"
#include "collectionmanager.h"
#include "collectionlocation.h"
#include "databaseaccess.h"
#include "databaseurl.h"
#include "databaseparameters.h"
#include "dio.h"
#include "imagelister.h"
#include "scancontroller.h"
#include "upgradedb_sqlite2tosqlite3.h"
#include "albummanager.h"
#include "albummanager.moc"

namespace Digikam
{

class PAlbumPath
{
public:

    PAlbumPath()
        : albumRootId(-1)
    {
    }

    PAlbumPath(int albumRootId, const QString &albumPath)
       : albumRootId(albumRootId), albumPath(albumPath)
    {
    }

    PAlbumPath(PAlbum *album)
    {
        if (album->isRoot())
        {
            albumRootId = -1;
        }
        else
        {
            albumRootId = album->albumRootId();
            albumPath   = album->albumPath();
        }
    }

    bool operator==(const PAlbumPath &other) const
    {
        return other.albumRootId == albumRootId && other.albumPath == albumPath;
    }

    int     albumRootId;
    QString albumPath;
};

uint qHash(const PAlbumPath &id)
{
    return ::qHash(id.albumRootId) xor ::qHash(id.albumPath);
}

class AlbumManagerPriv
{

public:

    AlbumManagerPriv()
    {
        changed            = false;
        hasPriorizedDbPath = false;
        dateListJob        = 0;
        albumListJob       = 0;
        tagListJob         = 0;
        dirWatch           = 0;
        itemHandler        = 0;
        rootPAlbum         = 0;
        rootTAlbum         = 0;
        rootDAlbum         = 0;
        rootSAlbum         = 0;
        currentAlbum       = 0;
    }

    bool                        changed;
    bool                        hasPriorizedDbPath;

    QString                     dbPath;

    QList<QDateTime>            dbPathModificationDateList;

    KIO::TransferJob           *albumListJob;
    KIO::TransferJob           *dateListJob;
    KIO::TransferJob           *tagListJob;

    KDirWatch                  *dirWatch;

    AlbumItemHandler           *itemHandler;

    PAlbum                     *rootPAlbum;
    TAlbum                     *rootTAlbum;
    DAlbum                     *rootDAlbum;
    SAlbum                     *rootSAlbum;

    QHash<int,Album *>          allAlbumsIdHash;
    QHash<PAlbumPath, PAlbum*>  albumPathHash;

    Album                      *currentAlbum;
};

class AlbumManagerCreator { public: AlbumManager object; };
K_GLOBAL_STATIC(AlbumManagerCreator, creator)

AlbumManager* AlbumManager::instance()
{
    return &creator->object;
}

AlbumManager::AlbumManager()
{
    d = new AlbumManagerPriv;
}

AlbumManager::~AlbumManager()
{
    if (d->dateListJob)
    {
        d->dateListJob->kill();
        d->dateListJob = 0; 
    }

    if (d->albumListJob)
    {
        d->albumListJob->kill();
        d->albumListJob = 0;
    }

    if (d->tagListJob)
    {
        d->tagListJob->kill();
        d->tagListJob = 0;
    }

    delete d->rootPAlbum;
    delete d->rootTAlbum;
    delete d->rootDAlbum;
    delete d->rootSAlbum;

    delete d->dirWatch;

    delete d;
}

bool AlbumManager::setDatabase(const QString &dbPath, bool priority)
{
    // This is to ensure that the setup does not overrule the command line.
    // Replace with a better solution?
    if (priority)
    {
        d->hasPriorizedDbPath = true;
    }
    else if (d->hasPriorizedDbPath && !d->dbPath.isNull())
    {
        // ignore change without priority
        // true means, dont exit()
        return true;
    }

    d->dbPath  = dbPath;
    d->changed = true;

    disconnect(CollectionManager::instance(), 0, this, 0);
    d->dbPathModificationDateList.clear();

    if (d->dateListJob)
    {
        d->dateListJob->kill();
        d->dateListJob = 0;
    }

    if (d->albumListJob)
    {
        d->albumListJob->kill();
        d->albumListJob = 0;
    }

    if (d->tagListJob)
    {
        d->tagListJob->kill();
        d->tagListJob = 0;
    }

    delete d->dirWatch;
    d->dirWatch = 0;

    d->currentAlbum = 0;
    emit signalAlbumCurrentChanged(0);
    emit signalAlbumsCleared();

    d->albumPathHash.clear();
    d->allAlbumsIdHash.clear();

    delete d->rootPAlbum;
    delete d->rootTAlbum;
    delete d->rootDAlbum;

    d->rootPAlbum = 0;
    d->rootTAlbum = 0;
    d->rootDAlbum = 0;
    d->rootSAlbum = 0;

    // -- Database initialization -------------------------------------------------

    DatabaseAccess::setParameters(Digikam::DatabaseParameters::parametersForSQLiteDefaultFile(d->dbPath),
                                  DatabaseAccess::MainApplication);

    ScanController::Advice advice = ScanController::instance()->databaseInitialization();

    switch (advice)
    {
        case ScanController::Success:
            break;
        case ScanController::ContinueWithoutDatabase:
        {
            QString errorMsg = DatabaseAccess().lastError();
            if (errorMsg.isEmpty())
            {
                KMessageBox::error(0, i18n("<qt><p>Failed to open the database. "
                                        "</p><p>You cannot use digiKam without a working database. "
                                        "digiKam will attempt to start now, but it will <b>not</b> be functional. "
                                        "Please check the database settings in the <b>configuration menu</b>.</p></qt>"
                                        ));
            }
            else
            {
                KMessageBox::error(0, i18n("<qt><p>Failed to open the database. "
                                        " Error message from database: %1 "
                                        "</p><p>You cannot use digiKam without a working database. "
                                        "digiKam will attempt to start now, but it will <b>not</b> be functional. "
                                        "Please check the database settings in the <b>configuration menu</b>.</p></qt>",
                                        errorMsg));
            }
            return true;
        }
        case ScanController::AbortImmediately:
            return false;
    }

    // -- Locale Checking ---------------------------------------------------------

    QString currLocale(QTextCodec::codecForLocale()->name());
    QString dbLocale = DatabaseAccess().db()->getSetting("Locale");

    // guilty until proven innocent
    bool localeChanged = true;
    
    if (dbLocale.isNull())
    {
        DDebug() << "No locale found in database" << endl;

        // Copy an existing locale from the settings file (used < 0.8)
        // to the database.
        KSharedConfig::Ptr config = KGlobal::config();
        KConfigGroup group = config->group("General Settings");
        if (group.hasKey("Locale"))
        {
            DDebug() << "Locale found in configfile" << endl;
            dbLocale = group.readEntry("Locale", QString());

            // this hack is necessary, as we used to store the entire
            // locale info LC_ALL (for eg: en_US.UTF-8) earlier, 
            // we now save only the encoding (UTF-8)

            QString oldConfigLocale = ::setlocale(0, 0);

            if (oldConfigLocale == dbLocale)
            {
                dbLocale = currLocale;
                localeChanged = false;
                DatabaseAccess().db()->setSetting("Locale", dbLocale);
            }
        }
        else
        {
            DDebug() << "No locale found in config file"  << endl;
            dbLocale = currLocale;

            localeChanged = false;
            DatabaseAccess().db()->setSetting("Locale",dbLocale);
        }
    }
    else
    {
        if (dbLocale == currLocale)
            localeChanged = false;
    }

    if (localeChanged)
    {
        // TODO it would be better to replace all yes/no confirmation dialogs with ones that has custom
        // buttons that denote the actions directly, i.e.:  ["Ignore and Continue"]  ["Adjust locale"]
        int result =
            KMessageBox::warningYesNo(0,
                                      i18n("Your locale has changed from the previous time "
                                           "this album was opened.\n"
                                           "Old Locale : %1, New Locale : %2\n"
                                           "If you changed your locale lately, this is all right.\n"
                                           "Please notice that if you switched to a locale "
                                           "that does not support some of the file names in your collection, "
                                           "these files may no longer be found in the collection. "
                                           "If you are sure that you want to "
                                           "continue, click 'Yes'. "
                                           "Otherwise, click 'No' and correct your "
                                           "locale setting before restarting digiKam.",
                                           dbLocale, currLocale));
        if (result != KMessageBox::Yes)
            exit(0);

        DatabaseAccess().db()->setSetting("Locale",currLocale);
    }

    return true;
}

void AlbumManager::startScan()
{
    if (!d->changed)
        return;
    d->changed = false;

    // create dir watch
    d->dirWatch = new KDirWatch(this);
    connect(d->dirWatch, SIGNAL(dirty(const QString&)),
            this, SLOT(slotDirty(const QString&)));

    // listen to location status changes
    connect(CollectionManager::instance(), SIGNAL(locationStatusChanged(const CollectionLocation &, int)),
            this, SLOT(slotCollectionLocationStatusChanged(const CollectionLocation &, int)));

    // add album roots to dir watch
    QStringList albumRootPaths = CollectionManager::instance()->allAvailableAlbumRootPaths();
    for (QStringList::iterator it = albumRootPaths.begin(); it != albumRootPaths.end(); ++it)
    {
        if (!d->dirWatch->contains(*it))
            d->dirWatch->addDir(*it, KDirWatch::WatchSubDirs);
    }

    // create root albums
    d->rootPAlbum = new PAlbum(i18n("My Albums"));
    insertPAlbum(d->rootPAlbum);

    d->rootTAlbum = new TAlbum(i18n("My Tags"), 0, true);
    insertTAlbum(d->rootTAlbum);

    d->rootSAlbum = new SAlbum(0, KUrl(), true, true);

    d->rootDAlbum = new DAlbum(QDate(), true);

    // reload albums
    refresh();

    emit signalAllAlbumsLoaded();
}

void AlbumManager::slotCollectionLocationStatusChanged(const CollectionLocation &location, int oldStatus)
{
    if (location.status() == CollectionLocation::LocationAvailable)
    {
        if (!d->dirWatch->contains(location.albumRootPath()))
             d->dirWatch->addDir(location.albumRootPath(), KDirWatch::WatchSubDirs);
    }
    else if (oldStatus == CollectionLocation::LocationAvailable)
    {
        d->dirWatch->removeDir(location.albumRootPath());
    }
}

void AlbumManager::refresh()
{
    scanPAlbums();
    scanTAlbums();
    scanSAlbums();
    scanDAlbums();
}

void AlbumManager::scanPAlbums()
{
    // first insert all the current PAlbums into a map for quick lookup
    QHash<int, PAlbum *> oldAlbums;

    AlbumIterator it(d->rootPAlbum);
    while (it.current())
    {
        PAlbum* a = (PAlbum*)(*it);
        oldAlbums[a->id()] = a;
        ++it;
    }

    // scan db and get a list of all albums
    QList<AlbumInfo> currentAlbums = DatabaseAccess().db()->scanAlbums();

    qSort(currentAlbums);

    /*
    // cache location status
    QList<CollectionLocation> allLocations = CollectionManager::instance()->allLocations();
    QHash<int, CollectionLocation::Status> statusHash;
    foreach (CollectionLocation location, allLocations)
        statusHash[location->id()] = location.status();
    */

    QList<AlbumInfo> newAlbums;

    // go through all the Albums and see which ones are already present
    foreach (AlbumInfo info, currentAlbums)
    {
        // check that location of album is available
        if (CollectionManager::instance()->locationForAlbumRootId(info.albumRootId).isAvailable())
        //if (statusHash.value(info.albumRootId) == CollectionLocation::LocationAvailable)
        {
            if (oldAlbums.contains(info.id))
                oldAlbums.remove(info.id);
            else
                newAlbums << info;
        }
    }

    // now oldAlbums contains all the deleted albums and
    // newAlbums contains all the new albums

    // delete old albums, informing all frontends

    // The albums have to be removed with children being removed first,
    // removePAlbum takes care of that.
    // So we only feed it the albums from oldAlbums topmost in hierarchy.
    QSet<PAlbum *> topMostOldAlbums;
    foreach (PAlbum *album, oldAlbums)
    {
        if (!album->parent() || !oldAlbums.contains(album->parent()->id()))
            topMostOldAlbums << album;
    }

    foreach(PAlbum *album, topMostOldAlbums)
    {
        // this might look like there is memory leak here, since removePAlbum
        // doesn't delete albums and looks like child Albums don't get deleted.
        // But when the parent album gets deleted, the children are also deleted.
        removePAlbum(album);
        delete album;
    }

    qSort(newAlbums);

    foreach (AlbumInfo info, newAlbums)
    {
        if (info.relativePath.isEmpty() || info.relativePath == "/")
            continue;

        // last section, no slash
        QString name = info.relativePath.section('/', -1, -1);
        // all but last sections, leading slash, no trailing slash
        QString parentPath = info.relativePath.section('/', 0, -2);

        PAlbum* parent;
        if (parentPath.isEmpty())
            parent = d->rootPAlbum;
        else
            parent = d->albumPathHash.value(PAlbumPath(info.albumRootId, parentPath));

        if (!parent)
        {
            DWarning() <<  "Could not find parent with url: "
                       << parentPath << " for: " << info.relativePath << endl;
            continue;
        }

        // Create the new album
        PAlbum* album       = new PAlbum(info.albumRootId, parentPath, name, info.id);
        album->m_caption    = info.caption;
        album->m_collection = info.collection;
        album->m_date       = info.date;

        QString albumRootPath = CollectionManager::instance()->albumRootPath(info.iconAlbumRootId);
        if (!albumRootPath.isNull())
            album->m_icon = albumRootPath + info.iconRelativePath;

        album->setParent(parent);

        insertPAlbum(album);
    }

    getAlbumItemsCount();
}

void AlbumManager::getAlbumItemsCount()
{
    if (!AlbumSettings::instance()->getShowFolderTreeViewItemsCount())
        return;

    // List albums using kioslave

    if (d->albumListJob)
    {
        d->albumListJob->kill();
        d->albumListJob = 0;
    }

    DatabaseUrl u = DatabaseUrl::albumUrl();

    d->albumListJob = ImageLister::startListJob(u);
    d->albumListJob->addMetaData("folders", "true");

    connect(d->albumListJob, SIGNAL(result(KJob*)),
            this, SLOT(slotAlbumsJobResult(KJob*)));

    connect(d->albumListJob, SIGNAL(data(KIO::Job*, const QByteArray&)),
            this, SLOT(slotAlbumsJobData(KIO::Job*, const QByteArray&)));
}

void AlbumManager::scanTAlbums()
{
    // list TAlbums directly from the db
    // first insert all the current TAlbums into a map for quick lookup
    typedef QMap<int, TAlbum*> TagMap;
    TagMap tmap;

    tmap.insert(0, d->rootTAlbum);

    AlbumIterator it(d->rootTAlbum);
    while (it.current())
    {
        TAlbum* t = (TAlbum*)(*it);
        tmap.insert(t->id(), t);
        ++it;
    }

    // Retrieve the list of tags from the database
    TagInfo::List tList = DatabaseAccess().db()->scanTags();

    // sort the list. needed because we want the tags can be read in any order,
    // but we want to make sure that we are ensure to find the parent TAlbum
    // for a new TAlbum

    {
        Q3IntDict<TAlbum> tagDict;
        tagDict.setAutoDelete(false);

        // insert items into a dict for quick lookup
        for (TagInfo::List::iterator it = tList.begin(); it != tList.end(); ++it)
        {
            TagInfo info = *it;
            TAlbum* album = new TAlbum(info.name, info.id);
            if (info.icon.isNull())
            {
                // album image icon
                QString albumRootPath = CollectionManager::instance()->albumRootPath(info.iconAlbumRootId);
                album->m_icon = albumRootPath + info.iconRelativePath;
            }
            else
            {
                // system icon
                album->m_icon = info.icon;
            }
            album->m_pid  = info.pid;
            tagDict.insert(info.id, album);
        }
        tList.clear();

        // also add root tag
        TAlbum* rootTag = new TAlbum("root", 0, true);
        tagDict.insert(0, rootTag);

        // build tree
        Q3IntDictIterator<TAlbum> iter(tagDict);
        for ( ; iter.current(); ++iter )
        {
            TAlbum* album = iter.current();
            if (album->m_id == 0)
                continue;
            
            TAlbum* parent = tagDict.find(album->m_pid);
            if (parent)
            {
                album->setParent(parent);
            }
            else
            {
                DWarning() << "Failed to find parent tag for tag "
                            << iter.current()->m_title
                            << " with pid "
                            << iter.current()->m_pid << endl;
            }
        }

        // now insert the items into the list. becomes sorted
        AlbumIterator it(rootTag);
        while (it.current())
        {
            TagInfo info;
            TAlbum* album = (TAlbum*)it.current();
            info.id       = album->m_id;
            info.pid      = album->m_pid;
            info.name     = album->m_title;
            info.icon     = album->m_icon;
            tList.append(info);
            ++it;
        }

        // this will also delete all child albums
        delete rootTag;
    }
    
    for (TagInfo::List::iterator it = tList.begin(); it != tList.end(); ++it)
    {
        TagInfo info = *it;

        // check if we have already added this tag
        if (tmap.contains(info.id))
            continue;

        // Its a new album. Find the parent of the album
        TagMap::iterator iter = tmap.find(info.pid);
        if (iter == tmap.end())
        {
            DWarning() << "Failed to find parent tag for tag "
                       << info.name 
                       << " with pid "
                       << info.pid << endl;
            continue;
        }

        TAlbum* parent = iter.value();

        // Create the new TAlbum
        TAlbum* album = new TAlbum(info.name, info.id, false);
        album->m_icon = info.icon;
        album->setParent(parent);
        insertTAlbum(album);

        // also insert it in the map we are doing lookup of parent tags
        tmap.insert(info.id, album);
    }

    getTagItemsCount();
}

void AlbumManager::getTagItemsCount()
{
    if (!AlbumSettings::instance()->getShowFolderTreeViewItemsCount())
        return;

    // List tags using kioslave

    if (d->tagListJob)
    {
        d->tagListJob->kill();
        d->tagListJob = 0;
    }

    DatabaseUrl u = DatabaseUrl::fromTagIds(QList<int>());

    d->tagListJob = ImageLister::startListJob(u);
    d->tagListJob->addMetaData("folders", "true");

    connect(d->tagListJob, SIGNAL(result(KJob*)),
            this, SLOT(slotTagsJobResult(KJob*)));

    connect(d->tagListJob, SIGNAL(data(KIO::Job*, const QByteArray&)),
            this, SLOT(slotTagsJobData(KIO::Job*, const QByteArray&)));
}

void AlbumManager::scanSAlbums()
{
    // list SAlbums directly from the db
    // first insert all the current SAlbums into a map for quick lookup
    typedef QMap<int, SAlbum*> SearchMap;
    SearchMap sMap;

    AlbumIterator it(d->rootSAlbum);
    while (it.current())
    {
        SAlbum* t = (SAlbum*)(*it);
        sMap.insert(t->id(), t);
        ++it;
    }

    // Retrieve the list of searches from the database
    SearchInfo::List sList = DatabaseAccess().db()->scanSearches();

    for (SearchInfo::List::iterator it = sList.begin(); it != sList.end(); ++it)
    {
        SearchInfo info = *it;

        // check if we have already added this search
        if (sMap.contains(info.id))
            continue;

        bool simple = (info.url.queryItem("1.key") == QString::fromLatin1("keyword"));
        
        // Its a new album.
        SAlbum* album = new SAlbum(info.id, info.url, simple, false);
        album->setParent(d->rootSAlbum);
        d->allAlbumsIdHash[album->globalID()] = album;
        emit signalAlbumAdded(album);
    }
}

void AlbumManager::scanDAlbums()
{
    // List dates using kioslave:
    // The kioslave has a special mode listing the dates
    // for which there are images in the DB.

    if (d->dateListJob)
    {
        d->dateListJob->kill();
        d->dateListJob = 0;
    }

    DatabaseUrl u = DatabaseUrl::fromDate(QDate());

    d->dateListJob = ImageLister::startListJob(u);
    d->dateListJob->addMetaData("folders", "true");

    connect(d->dateListJob, SIGNAL(result(KJob*)),
            this, SLOT(slotDatesJobResult(KJob*)));

    connect(d->dateListJob, SIGNAL(data(KIO::Job*, const QByteArray&)),
            this, SLOT(slotDatesJobData(KIO::Job*, const QByteArray&)));
}

AlbumList AlbumManager::allPAlbums() const
{
    AlbumList list;
    if (d->rootPAlbum)
        list.append(d->rootPAlbum);

    AlbumIterator it(d->rootPAlbum);
    while (it.current())
    {
        list.append(*it);
        ++it;
    }
    
    return list;
}

AlbumList AlbumManager::allTAlbums() const
{
    AlbumList list;
    if (d->rootTAlbum)
        list.append(d->rootTAlbum);

    AlbumIterator it(d->rootTAlbum);
    while (it.current())
    {
        list.append(*it);
        ++it;
    }
    
    return list;
}

AlbumList AlbumManager::allSAlbums() const
{
    AlbumList list;
    if (d->rootSAlbum)
        list.append(d->rootSAlbum);

    AlbumIterator it(d->rootSAlbum);
    while (it.current())
    {
        list.append(*it);
        ++it;
    }
    
    return list;
}

AlbumList AlbumManager::allDAlbums() const
{
    AlbumList list;
    if (d->rootDAlbum)
        list.append(d->rootDAlbum);

    AlbumIterator it(d->rootDAlbum);
    while (it.current())
    {
        list.append(*it);
        ++it;
    }
    
    return list;
}

void AlbumManager::setCurrentAlbum(Album *album)
{
    d->currentAlbum = album;
    emit signalAlbumCurrentChanged(album);
}

Album* AlbumManager::currentAlbum() const
{
    return d->currentAlbum;
}

PAlbum* AlbumManager::findPAlbum(const KUrl& url) const
{
    CollectionLocation location = CollectionManager::instance()->locationForUrl(url);
    if (location.isNull())
        return 0;
    return d->albumPathHash.value(PAlbumPath(location.id(), CollectionManager::instance()->album(location, url)));
}

PAlbum* AlbumManager::findPAlbum(int id) const
{
    if (!d->rootPAlbum)
        return 0;

    int gid = d->rootPAlbum->globalID() + id;

    return (PAlbum*)(d->allAlbumsIdHash.value(gid));
}

TAlbum* AlbumManager::findTAlbum(int id) const
{
    if (!d->rootTAlbum)
        return 0;

    int gid = d->rootTAlbum->globalID() + id;

    return (TAlbum*)(d->allAlbumsIdHash.value(gid));
}

SAlbum* AlbumManager::findSAlbum(int id) const
{
    if (!d->rootTAlbum)
        return 0;

    int gid = d->rootSAlbum->globalID() + id;

    return (SAlbum*)(d->allAlbumsIdHash.value(gid));
}

DAlbum* AlbumManager::findDAlbum(int id) const
{
    if (!d->rootDAlbum)
        return 0;

    int gid = d->rootDAlbum->globalID() + id;

    return (DAlbum*)(d->allAlbumsIdHash.value(gid));
}

Album* AlbumManager::findAlbum(int gid) const
{
    return d->allAlbumsIdHash.value(gid);
}

TAlbum* AlbumManager::findTAlbum(const QString &tagPath) const
{
    // handle gracefully with or without leading slash
    bool withLeadingSlash = tagPath.startsWith("/");
    AlbumIterator it(d->rootTAlbum);
    while (it.current())
    {
        TAlbum *talbum = static_cast<TAlbum *>(*it);
        if (talbum->tagPath(withLeadingSlash) == tagPath)
            return talbum;
        ++it;
    }
    return 0;

}

PAlbum* AlbumManager::createPAlbum(PAlbum* parent,
                                   const QString& albumRoot,
                                   const QString& name,
                                   const QString& caption,
                                   const QDate& date,
                                   const QString& collection,
                                   QString& errMsg)
{
    if (!parent)
    {
        errMsg = i18n("No parent found for album.");
        return 0;
    }

    // sanity checks
    if (name.isEmpty())
    {
        errMsg = i18n("Album name cannot be empty.");
        return 0;
    }

    if (name.contains("/"))
    {
        errMsg = i18n("Album name cannot contain '/'.");
        return 0;
    }

    QString albumRootPath;
    QString albumPath;
    if (parent->isRoot())
    {
        if (albumRoot.isNull())
        {
            errMsg = i18n("No album root path supplied");
            return 0;
        }
        albumRootPath = albumRoot;
        albumPath     = "/" + name;
    }
    else
    {
        albumRootPath = parent->albumRootPath();
        albumPath     = parent->albumPath() + "/" + name;
    }

    int albumRootId = CollectionManager::instance()->locationForAlbumRootPath(albumRootPath).id();

    // first check if we have another album with the same name
    PAlbum *child = (PAlbum *)parent->m_firstChild;
    while (child)
    {
        if (child->albumRootId() == albumRootId && child->albumPath() == albumPath)
        {
            errMsg = i18n("An existing album has the same name.");
            return 0;
        }
        child = (PAlbum *)child->m_next;
    }

    DatabaseUrl url = parent->databaseUrl();
    url.addPath(name);
    KUrl fileUrl = url.fileUrl();

    //TODO: Use KIO::NetAccess?
    // make the directory synchronously, so that we can add the
    // album info to the database directly
    if (::mkdir(QFile::encodeName(fileUrl.path()), 0777) != 0)
    {
        if (errno == EEXIST)
            errMsg = i18n("Another file or folder with same name exists");
        else if (errno == EACCES)
            errMsg = i18n("Access denied to path");
        else if (errno == ENOSPC)
            errMsg = i18n("Disk is full");
        else
            errMsg = i18n("Unknown error"); // being lazy

        return 0;
    }

    int id = DatabaseAccess().db()->addAlbum(albumRootId, albumPath, caption, date, collection);

    if (id == -1)
    {
        errMsg = i18n("Failed to add album to database");
        return 0;
    }

    PAlbum *album       = new PAlbum(albumRootId, parent->albumPath(), name, id);
    album->m_caption    = caption;
    album->m_collection = collection;
    album->m_date       = date;

    album->setParent(parent);

    insertPAlbum(album);

    return album;
}

bool AlbumManager::renamePAlbum(PAlbum* album, const QString& newName,
                                QString& errMsg)
{
    if (!album)
    {
        errMsg = i18n("No such album");
        return false;
    }

    if (album == d->rootPAlbum)
    {
        errMsg = i18n("Cannot rename root album");
        return false;
    }

    if (newName.contains("/"))
    {
        errMsg = i18n("Album name cannot contain '/'");
        return false;
    }

    // first check if we have another sibling with the same name
    Album *sibling = album->m_parent->m_firstChild;
    while (sibling)
    {
        if (sibling->title() == newName)
        {
            errMsg = i18n("Another album with same name exists\n"
                          "Please choose another name");
            return false;
        }
        sibling = sibling->m_next;
    }

    QString oldAlbumPath = album->albumPath();

    KUrl u = album->fileUrl();
    u      = u.upUrl();
    u.addPath(newName);

    if (::rename(QFile::encodeName(album->folderPath()),
                 QFile::encodeName(u.path(KUrl::RemoveTrailingSlash))) != 0)
    {
        errMsg = i18n("Failed to rename Album");
        return false;
    }

    // now rename the album and subalbums in the database

    // all we need to do is set the title of the album which is being
    // renamed correctly and all the sub albums will automatically get
    // their url set correctly

    album->setTitle(newName);
    {
        DatabaseAccess access;
        access.db()->renameAlbum(album->id(), album->albumPath(), false);

        Album* subAlbum = 0;
        AlbumIterator it(album);
        while ((subAlbum = it.current()) != 0)
        {
            access.db()->renameAlbum(subAlbum->id(), ((PAlbum*)subAlbum)->albumPath(), false);
            ++it;
        }
    }

    // Update AlbumDict. basically clear it and rebuild from scratch
    {
        d->albumPathHash.clear();
        AlbumIterator it(d->rootPAlbum);
        PAlbum* subAlbum = 0;
        while ((subAlbum = (PAlbum*)it.current()) != 0)
        {
            d->albumPathHash[subAlbum] = subAlbum;
            ++it;
        }
    }

    emit signalAlbumRenamed(album);

    return true;
}

bool AlbumManager::updatePAlbumIcon(PAlbum *album, qlonglong iconID, QString& errMsg)
{
    if (!album)
    {
        errMsg = i18n("No such album");
        return false;
    }

    if (album == d->rootPAlbum)
    {
        errMsg = i18n("Cannot edit root album");
        return false;
    }

    {
        DatabaseAccess access;
        access.db()->setAlbumIcon(album->id(), iconID);
        QString iconRelativePath;
        int iconAlbumRootId;
        if (access.db()->getAlbumIcon(album->id(), &iconAlbumRootId, &iconRelativePath))
        {
            QString albumRootPath = CollectionManager::instance()->albumRootPath(iconAlbumRootId);
            album->m_icon = albumRootPath + iconRelativePath;
        }
        else
            album->m_icon = QString();
    }

    emit signalAlbumIconChanged(album);

    return true;
}

TAlbum* AlbumManager::createTAlbum(TAlbum* parent, const QString& name,
                                   const QString& iconkde, QString& errMsg)
{
    if (!parent)
    {
        errMsg = i18n("No parent found for tag");
        return 0;
    }

    // sanity checks
    if (name.isEmpty())
    {
        errMsg = i18n("Tag name cannot be empty");
        return 0;
    }
    
    if (name.contains("/"))
    {
        errMsg = i18n("Tag name cannot contain '/'");
        return 0;
    }
    
    // first check if we have another album with the same name
    Album *child = parent->m_firstChild;
    while (child)
    {
        if (child->title() == name)
        {
            errMsg = i18n("Tag name already exists");
            return 0;
        }
        child = child->m_next;
    }

    int id = DatabaseAccess().db()->addTag(parent->id(), name, iconkde, 0);
    if (id == -1)
    {
        errMsg = i18n("Failed to add tag to database");
        return 0;
    }
    
    TAlbum *album = new TAlbum(name, id, false);
    album->m_icon = iconkde;
    album->setParent(parent);

    insertTAlbum(album);
    
    return album;
}

AlbumList AlbumManager::findOrCreateTAlbums(const QStringList &tagPaths)
{
    // find tag ids for tag paths in list, create if they don't exist
    QList<int> tagIDs = DatabaseAccess().db()->getTagsFromTagPaths(tagPaths, true);

    // create TAlbum objects for the newly created tags
    scanTAlbums();

    AlbumList resultList;

    for (QList<int>::iterator it = tagIDs.begin() ; it != tagIDs.end() ; ++it)
    {
        resultList.append(findTAlbum(*it));
    }

    return resultList;
}

bool AlbumManager::deleteTAlbum(TAlbum* album, QString& errMsg)
{
    if (!album)
    {
        errMsg = i18n("No such album");
        return false;
    }

    if (album == d->rootTAlbum)
    {
        errMsg = i18n("Cannot delete Root Tag");
        return false;
    }

    {
        DatabaseAccess access;
        access.db()->deleteTag(album->id());

        Album* subAlbum = 0;
        AlbumIterator it(album);
        while ((subAlbum = it.current()) != 0)
        {
            access.db()->deleteTag(subAlbum->id());
            ++it;
        }
    }

    removeTAlbum(album);

    d->allAlbumsIdHash.remove(album->globalID());
    delete album;
    
    return true;
}

bool AlbumManager::renameTAlbum(TAlbum* album, const QString& name,
                                QString& errMsg)
{
    if (!album)
    {
        errMsg = i18n("No such album");
        return false;
    }

    if (album == d->rootTAlbum)
    {
        errMsg = i18n("Cannot edit root tag");
        return false;
    }

    if (name.contains("/"))
    {
        errMsg = i18n("Tag name cannot contain '/'");
        return false;
    }

    // first check if we have another sibling with the same name
    Album *sibling = album->m_parent->m_firstChild;
    while (sibling)
    {
        if (sibling->title() == name)
        {
            errMsg = i18n("Another tag with same name exists\n"
                          "Please choose another name");
            return false;
        }
        sibling = sibling->m_next;
    }

    DatabaseAccess().db()->setTagName(album->id(), name);
    album->setTitle(name);
    emit signalAlbumRenamed(album);

    return true;
}

bool AlbumManager::moveTAlbum(TAlbum* album, TAlbum *newParent, QString &errMsg)
{
    if (!album)
    {
        errMsg = i18n("No such album");
        return false;
    }

    if (album == d->rootTAlbum)
    {
        errMsg = i18n("Cannot move root tag");
        return false;
    }

    DatabaseAccess().db()->setTagParentID(album->id(), newParent->id());
    album->parent()->removeChild(album);
    album->setParent(newParent);

    emit signalTAlbumMoved(album, newParent);

    return true;
}

bool AlbumManager::updateTAlbumIcon(TAlbum* album, const QString& iconKDE,
                                    qlonglong iconID, QString& errMsg)
{
    if (!album)
    {
        errMsg = i18n("No such tag");
        return false;
    }

    if (album == d->rootTAlbum)
    {
        errMsg = i18n("Cannot edit root tag");
        return false;
    }

    {
        DatabaseAccess access;
        access.db()->setTagIcon(album->id(), iconKDE, iconID);
        QString albumRelativePath, iconKDE;
        int albumRootId;
        if (access.db()->getTagIcon(album->id(), &albumRootId, &albumRelativePath, &iconKDE))
        {
            if (iconKDE.isEmpty())
            {
                QString albumRootPath = CollectionManager::instance()->albumRootPath(albumRootId);
                album->m_icon = albumRootPath + albumRelativePath;
            }
            else
            {
                album->m_icon = iconKDE;
            }
        }
        else
            album->m_icon = QString();
    }

    emit signalAlbumIconChanged(album);

    return true;
}

AlbumList AlbumManager::getRecentlyAssignedTags() const
{
    QList<int> tagIDs = DatabaseAccess().db()->getRecentlyAssignedTags();

    AlbumList resultList;

    for (QList<int>::iterator it = tagIDs.begin() ; it != tagIDs.end() ; ++it)
    {
        resultList.append(findTAlbum(*it));
    }

    return resultList;
}

QStringList AlbumManager::tagPaths(const Q3ValueList<int> &tagIDs, bool leadingSlash) const
{
    QStringList tagPaths;

    for (Q3ValueList<int>::const_iterator it = tagIDs.begin(); it != tagIDs.end(); ++it)
    {
        TAlbum *album = findTAlbum(*it);
        if (album)
        {
            tagPaths.append(album->tagPath(leadingSlash));
        }
    }

    return tagPaths;
}

QStringList AlbumManager::tagNames(const QList<int> &tagIDs) const
{
    QStringList tagNames;

    foreach(int id, tagIDs)
    {
        TAlbum *album = findTAlbum(id);
        if (album)
        {
            tagNames << album->title();
        }
    }

    return tagNames;
}

SAlbum* AlbumManager::createSAlbum(const KUrl& url, bool simple)
{
    QString name = url.queryItem("name");

    // first iterate through all the search albums and see if there's an existing
    // SAlbum with same name. (Remember, SAlbums are arranged in a flat list)
    for (Album* album = d->rootSAlbum->firstChild(); album; album = album->next())
    {
        if (album->title() == name)
        {
            SAlbum* sa = (SAlbum*)album;
            sa->m_kurl = url;
            DatabaseAccess access;
            access.db()->updateSearch(sa->id(), url.queryItem("name"), url);
            return sa;
        }
    }

    int id = DatabaseAccess().db()->addSearch(url.queryItem("name"), url);;

    if (id == -1)
        return 0;

    SAlbum* album = new SAlbum(id, url, simple, false);
    album->setTitle(url.queryItem("name"));
    album->setParent(d->rootSAlbum);

    d->allAlbumsIdHash.insert(album->globalID(), album);
    emit signalAlbumAdded(album);

    return album;
}

bool AlbumManager::updateSAlbum(SAlbum* album, const KUrl& newURL)
{
    if (!album)
        return false;

    DatabaseAccess().db()->updateSearch(album->id(), newURL.queryItem("name"), newURL);

    QString oldName = album->title();

    album->m_kurl = newURL;
    album->setTitle(newURL.queryItem("name"));
    if (oldName != album->title())
        emit signalAlbumRenamed(album);

    return true;
}

bool AlbumManager::deleteSAlbum(SAlbum* album)
{
    if (!album)
        return false;

    emit signalAlbumDeleted(album);

    DatabaseAccess().db()->deleteSearch(album->id());

    d->allAlbumsIdHash.remove(album->globalID());
    delete album;

    return true;
}

void AlbumManager::insertPAlbum(PAlbum *album)
{
    if (!album)
        return;

    d->albumPathHash[album]  = album;
    d->allAlbumsIdHash[album->globalID()] = album;

    emit signalAlbumAdded(album);
}

void AlbumManager::removePAlbum(PAlbum *album)
{
    if (!album)
        return;

    // remove all children of this album
    Album* child = album->m_firstChild;
    while (child)
    {
        Album *next = child->m_next;
        removePAlbum((PAlbum*)child);
        child = next;
    }

    d->albumPathHash.remove(album);
    d->allAlbumsIdHash.remove(album->globalID());

    DatabaseUrl url = album->databaseUrl();

    if (album == d->currentAlbum)
    {
        d->currentAlbum = 0;
        emit signalAlbumCurrentChanged(0);
    }
    
    emit signalAlbumDeleted(album);
}

void AlbumManager::insertTAlbum(TAlbum *album)
{
    if (!album)
        return;

    d->allAlbumsIdHash.insert(album->globalID(), album);

    emit signalAlbumAdded(album);
}

void AlbumManager::removeTAlbum(TAlbum *album)
{
    if (!album)
        return;

    // remove all children of this album
    Album* child = album->m_firstChild;
    while (child)
    {
        Album *next = child->m_next;
        removeTAlbum((TAlbum*)child);
        child = next;
    }
    
    d->allAlbumsIdHash.remove(album->globalID());

    if (album == d->currentAlbum)
    {
        d->currentAlbum = 0;
        emit signalAlbumCurrentChanged(0);
    }
    
    emit signalAlbumDeleted(album);
}

void AlbumManager::emitAlbumItemsSelected(bool val)
{
    emit signalAlbumItemsSelected(val);    
}

void AlbumManager::setItemHandler(AlbumItemHandler *handler)
{
    d->itemHandler = handler;    
}

AlbumItemHandler* AlbumManager::getItemHandler()
{
    return d->itemHandler;
}

void AlbumManager::refreshItemHandler(const KUrl::List& itemList)
{
    if (itemList.empty())
        d->itemHandler->refresh();
    else
        d->itemHandler->refreshItems(itemList);
}

void AlbumManager::slotAlbumsJobResult(KJob* job)
{
    d->albumListJob = 0;

    if (job->error())
    {
        DWarning() << k_funcinfo << "Failed to list albums" << endl;
        return;
    }
}

void AlbumManager::slotAlbumsJobData(KIO::Job*, const QByteArray& data)
{
    if (data.isEmpty())
        return;

    QMap<int, int> albumsStatMap;
    QByteArray di(data);
    QDataStream ds(&di, QIODevice::ReadOnly);
    ds >> albumsStatMap;

    emit signalPAlbumsDirty(albumsStatMap);
}

void AlbumManager::slotTagsJobResult(KJob* job)
{
    d->tagListJob = 0;

    if (job->error())
    {
        DWarning() << k_funcinfo << "Failed to list tags" << endl;
        return;
    }
}

void AlbumManager::slotTagsJobData(KIO::Job*, const QByteArray& data)
{
    if (data.isEmpty())
        return;

    QMap<int, int> tagsStatMap;
    QByteArray di(data);
    QDataStream ds(&di, QIODevice::ReadOnly);
    ds >> tagsStatMap;

    emit signalTAlbumsDirty(tagsStatMap);
}

void AlbumManager::slotDatesJobResult(KJob* job)
{
    d->dateListJob = 0;

    if (job->error())
    {
        DWarning() << "Failed to list dates" << endl;
        return;
    }

    emit signalAllDAlbumsLoaded();
}

void AlbumManager::slotDatesJobData(KIO::Job*, const QByteArray& data)
{
    if (data.isEmpty())
        return;

    // insert all the DAlbums into a qmap for quick access
    QMap<QDate, DAlbum*> albumMap;

    AlbumIterator it(d->rootDAlbum);
    while (it.current())
    {
        DAlbum* a = (DAlbum*)(*it);
        albumMap.insert(a->date(), a);
        ++it;
    }

    QMap<QDateTime, int> datesStatMap;
    QByteArray di(data);
    QDataStream ds(&di, QIODevice::ReadOnly);
    ds >> datesStatMap;

    QMap<YearMonth, int> yearMonthMap;
    for ( QMap<QDateTime, int>::iterator it = datesStatMap.begin();
          it != datesStatMap.end(); ++it )
    {
        YearMonth yearMonth = YearMonth(it.key().date().year(), it.key().date().month());

        QMap<YearMonth, int>::iterator it2 = yearMonthMap.find(yearMonth);
        if ( it2 == yearMonthMap.end() )
        {
            yearMonthMap.insert( yearMonth, *it );
        }
        else
        {
            *it2 += *it;
        }
    }

    int year, month;
    for ( QMap<YearMonth, int>::iterator it = yearMonthMap.begin();
          it != yearMonthMap.end(); ++it )
    {
        year  = it.key().first;
        month = it.key().second;

        QDate date( year, month, 1 );

        // Do we already have this album
        if (albumMap.contains(date))
        {
            // already there. remove from map
            albumMap.remove(date);
            continue;
        }

        // new album. create one
        DAlbum* album = new DAlbum(date);
        album->setParent(d->rootDAlbum);
        d->allAlbumsIdHash.insert(album->globalID(), album);
        emit signalAlbumAdded(album);
    }

    // Now the items contained in the map are the ones which
    // have been deleted. 
    for (QMap<QDate, DAlbum*>::iterator it = albumMap.begin();
         it != albumMap.end(); ++it)
    {
        DAlbum* album = *it;
        emit signalAlbumDeleted(album);
        d->allAlbumsIdHash.remove(album->globalID());
        delete album;
    }

    emit signalDAlbumsDirty(yearMonthMap);
    emit signalDatesMapDirty(datesStatMap);
}

void AlbumManager::slotDirty(const QString& path)
{
    DDebug() << "AlbumManager::slotDirty" << path << endl;
    QDir dir(path);

    // Filter out dirty signals triggered by changes on the database file
    DatabaseParameters params = DatabaseAccess::parameters();
    if (params.isSQLite())
    {
        QFileInfo dbFile(params.SQLiteDatabaseFile());

        // is the signal for the directory containing the database file?
        if (dbFile.dir() == dir)
        {
            // retrieve modification dates
            QList<QDateTime> modList;
            QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

            // build list
            foreach (QFileInfo info, fileInfoList)
            {
                modList << info.lastModified();
            }

            // check for equality
            if (modList == d->dbPathModificationDateList)
            {
                DDebug() << "Filtering out db file triggered dir watch signal" << endl;
                // we can skip the signal
                return;
            }

            // set new list
            d->dbPathModificationDateList = modList;
        }
    }

    /*
    KUrl fileUrl;
    // we need to provide a trailing slash to DatabaseUrl to mark it as a directory
    fileUrl.setPath(QDir::cleanPath(path) + '/');
    DatabaseUrl url = DatabaseUrl::fromFileUrl(fileUrl, CollectionManager::instance()->albumRoot(fileUrl));

    if (d->dirtyAlbums.contains(url.url()))
        return;

    DDebug() << "Dirty: " << url.fileUrl().path() << endl;
    d->dirtyAlbums.append(url.url());

    if (DIO::running())
        return;

    KUrl u(d->dirtyAlbums.first());
    d->dirtyAlbums.pop_front();

    DIO::scan(u);
    */
}

}  // namespace Digikam
