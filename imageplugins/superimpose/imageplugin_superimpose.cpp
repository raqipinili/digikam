/* ============================================================
 * Authors: Gilles Caulier <caulier dot gilles at gmail dot com>
 * Date   : 2005-01-04
 * Description : 
 * 
 * Copyright 2005-2007 by Gilles Caulier 
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

// KDE includes.
  
#include <klocale.h>
#include <kgenericfactory.h>
#include <klibloader.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kcursor.h>

// Local includes.

#include "ddebug.h"
#include "imageeffect_superimpose.h"
#include "imageplugin_superimpose.h"
#include "imageplugin_superimpose.moc"

K_PLUGIN_FACTORY( SuperImposeFactory, registerPlugin<ImagePlugin_SuperImpose>(); )
K_EXPORT_PLUGIN ( SuperImposeFactory("digikamimageplugin_superimpose") )

ImagePlugin_SuperImpose::ImagePlugin_SuperImpose(QObject *parent, const QVariantList &)
                        : Digikam::ImagePlugin(parent, "ImagePlugin_SuperImpose")
{
    m_superimposeAction  = new KAction(KIcon("superimpose"), i18n("Template Superimpose..."), this);
    actionCollection()->addAction("imageplugin_superimpose", m_superimposeAction );
 
    connect(m_superimposeAction, SIGNAL(triggered(bool)), 
            this, SLOT(slotSuperImpose()));

    setXMLFile("digikamimageplugin_superimpose_ui.rc");        
                                    
    DDebug() << "ImagePlugin_SuperImpose plugin loaded" << endl;
}

ImagePlugin_SuperImpose::~ImagePlugin_SuperImpose()
{
}

void ImagePlugin_SuperImpose::setEnabledActions(bool enable)
{
    m_superimposeAction->setEnabled(enable);
}

void ImagePlugin_SuperImpose::slotSuperImpose()
{
    DigikamSuperImposeImagesPlugin::ImageEffect_SuperImpose dlg(parentWidget());
    dlg.exec();
}
