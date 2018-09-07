/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2010-06-23
 * Description : widget for displaying an item in view with used filters on current image
 *
 * Copyright (C) 2010 by Martin Klapetek <martin dot klapetek at gmail dot com>
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

#ifndef DIGIKAM_IMAGE_FILTERS_HISTORY_TREE_ITEM_H
#define DIGIKAM_IMAGE_FILTERS_HISTORY_TREE_ITEM_H

// Qt includes

#include <QList>
#include <QVariant>

// Local includes

#include "digikam_export.h"

namespace Digikam
{

class ImageFiltersHistoryTreeItem
{
public:

    explicit ImageFiltersHistoryTreeItem(const QList<QVariant>& data, ImageFiltersHistoryTreeItem* const parent = 0);
    explicit ImageFiltersHistoryTreeItem(const QString& data, ImageFiltersHistoryTreeItem* const parent = 0);
    ~ImageFiltersHistoryTreeItem();

    void appendChild(ImageFiltersHistoryTreeItem* const child);
    void removeChild(int row);

    ImageFiltersHistoryTreeItem* child(int row) const;
    int childCount()                      const;
    int columnCount()                     const;
    QVariant data(int column)             const;
    int row()                             const;
    ImageFiltersHistoryTreeItem* parent() const;
    void setDisabled(bool disabled)       const;
    bool isDisabled()                     const;

private:

    ImageFiltersHistoryTreeItem(const ImageFiltersHistoryTreeItem&); // disable

private:

    class Private;
    Private* const d;
};

} // namespace Digikam

#endif // DIGIKAM_IMAGE_FILTERS_HISTORY_TREE_ITEM_H
