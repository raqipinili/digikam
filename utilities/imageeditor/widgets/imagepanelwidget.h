/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2005-07-01
 * Description : a widget to draw a control panel image tool.
 *
 * Copyright (C) 2005-2010 Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef IMAGEPANELWIDGET_H
#define IMAGEPANELWIDGET_H

// Qt includes

#include <QtGui/QPolygon>
#include <QtCore/QRect>
#include <QtCore/QString>
#include <QtGui/QWidget>

// Local includes

#include "dimg.h"
#include "digikam_export.h"

namespace Digikam
{

class ImageRegionWidget;
class ImagePanelWidgetPriv;

class DIGIKAM_EXPORT ImagePanelWidget : public QWidget
{
    Q_OBJECT

public:

    enum SeparateViewOptions
    {
        SeparateViewNormal=0,
        SeparateViewDuplicate,
        SeparateViewAll
    };

public:

    ImagePanelWidget(uint w, uint h, const QString& settingsSection, QWidget *parent=0);
    ~ImagePanelWidget();

    QRect  getOriginalImageRegion();
    QRect  getOriginalImageRegionToRender();
    DImg   getOriginalRegionImage();
    void   setPreviewImage(DImg img);
    void   setCenterImageRegionPosition();

    void   setHighLightPoints(const QPolygon& pt);

    void   ICCSettingsChanged();
    void   exposureSettingsChanged();

    void   writeSettings();

    ImageRegionWidget* previewWidget() const;

Q_SIGNALS:

    void signalOriginalClipFocusChanged();
    void signalResized();

private Q_SLOTS:

    void slotInitGui();

private:

    void readSettings();

private:

    ImagePanelWidgetPriv* const d;
};

}  // namespace Digikam

#endif /* IMAGEPANELWIDGET_H */
