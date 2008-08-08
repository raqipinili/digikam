/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2005-02-22
 * Description : a digiKam image editor plugin for simulate
 *               infrared film.
 *
 * Copyright (C) 2005-2008 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2006-2008 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
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

// Qt includes.

#include <qimage.h>
#include <qlabel.h>
#include <qwhatsthis.h>
#include <qlcdnumber.h>
#include <qslider.h>
#include <qlayout.h>
#include <qdatetime.h>
#include <qcheckbox.h>

// KDE includes.

#include <klocale.h>
#include <kaboutdata.h>
#include <kiconloader.h>
#include <kapplication.h>
#include <kstandarddirs.h>
#include <kconfig.h>

// Local includes.

#include "version.h"
#include "daboutdata.h"
#include "ddebug.h"
#include "dimg.h"
#include "imageiface.h"
#include "imagewidget.h"
#include "infrared.h"
#include "imageeffect_infrared.h"
#include "imageeffect_infrared.moc"

namespace DigikamInfraredImagesPlugin
{

ImageEffect_Infrared::ImageEffect_Infrared(QWidget* parent)
    : Digikam::CtrlPanelDlg(parent, i18n("Simulate Infrared Film to Photograph"),
                            "infrared", false, false, true,
                            Digikam::ImagePannelWidget::SeparateViewAll)
{
    QString whatsThis;

    KAboutData* about = new KAboutData("digikam",
                                       I18N_NOOP("Infrared Film"),
                                       digikam_version,
                                       I18N_NOOP("A digiKam image plugin to simulate infrared film."),
                                       KAboutData::License_GPL,
                                       "(c) 2005, Gilles Caulier\n"
                                       "(c) 2006-2008, Gilles Caulier and Marcel Wiesweg",
                                       0,
                                       Digikam::webProjectUrl());

    about->addAuthor("Gilles Caulier", I18N_NOOP("Author and maintainer"),
                     "caulier dot gilles at gmail dot com");

    about->addAuthor("Marcel Wiesweg", I18N_NOOP("Developer"),
                     "marcel dot wiesweg at gmx dot de");

    setAboutData(about);

    // -------------------------------------------------------------

    QWidget *gboxSettings     = new QWidget(m_imagePreviewWidget);
    QGridLayout* gridSettings = new QGridLayout( gboxSettings, 2, 1, 0, spacingHint());
    QLabel *label1            = new QLabel(i18n("Sensitivity (ISO):"), gboxSettings);

    m_sensibilitySlider = new QSlider(1, 25, 1, 1, Qt::Horizontal, gboxSettings);
    m_sensibilitySlider->setTracking ( false );
    m_sensibilitySlider->setTickInterval(1);
    m_sensibilitySlider->setTickmarks(QSlider::Below);

    m_sensibilityLCDValue = new QLCDNumber (4, gboxSettings);
    m_sensibilityLCDValue->setSegmentStyle ( QLCDNumber::Flat );
    m_sensibilityLCDValue->display( QString::number(200) );
    whatsThis = i18n("<p>Set here the ISO-sensitivity of the simulated infrared film. "
                     "Increasing this value will increase the proportion of green color in the mix. "
                     "It will also increase the halo effect on the hightlights, and the film "
                     "graininess (if that box is checked).</p>"
                     "<p>Note: to simulate an <b>Ilford SFX200</b> infrared film, use a sensitivity excursion of 200 to 800. "
                     "A sensitivity over 800 simulates <b>Kodak HIE</b> high-speed infrared film. This last one creates a more "
                     "dramatic photographic style.</p>");

    QWhatsThis::add( m_sensibilityLCDValue, whatsThis);
    QWhatsThis::add( m_sensibilitySlider, whatsThis);

    gridSettings->addMultiCellWidget(label1, 0, 0, 0, 1);
    gridSettings->addMultiCellWidget(m_sensibilitySlider, 1, 1, 0, 0);
    gridSettings->addMultiCellWidget(m_sensibilityLCDValue, 1, 1, 1, 1);

    // -------------------------------------------------------------

    m_addFilmGrain = new QCheckBox( i18n("Add film grain"), gboxSettings);
    m_addFilmGrain->setChecked( true );
    QWhatsThis::add( m_addFilmGrain, i18n("<p>This option adds infrared film grain to "
                                          "the image depending on ISO-sensitivity."));
    gridSettings->addMultiCellWidget(m_addFilmGrain, 2, 2, 0, 1);

    m_imagePreviewWidget->setUserAreaWidget(gboxSettings);

    // -------------------------------------------------------------

    connect( m_sensibilitySlider, SIGNAL(valueChanged(int)),
             this, SLOT(slotTimer()) );

    // this connection is necessary to change the LCD display when
    // the value is changed by single clicking on the slider
    connect( m_sensibilitySlider, SIGNAL(valueChanged(int)),
             this, SLOT(slotSliderMoved(int)) );

    connect( m_sensibilitySlider, SIGNAL(sliderMoved(int)),
             this, SLOT(slotSliderMoved(int)) );

    connect( m_addFilmGrain, SIGNAL(toggled (bool)),
             this, SLOT(slotEffect()) );
}

ImageEffect_Infrared::~ImageEffect_Infrared()
{
}

void ImageEffect_Infrared::renderingFinished()
{
    m_sensibilitySlider->setEnabled(true);
    m_addFilmGrain->setEnabled(true);
}

void ImageEffect_Infrared::readUserSettings()
{
    KConfig* config = kapp->config();
    config->setGroup("infrared Tool Dialog");
    m_sensibilitySlider->blockSignals(true);
    m_addFilmGrain->blockSignals(true);
    m_sensibilitySlider->setValue(config->readNumEntry("SensitivityAjustment", 1));
    m_addFilmGrain->setChecked(config->readBoolEntry("AddFilmGrain", false));
    m_sensibilitySlider->blockSignals(false);
    m_addFilmGrain->blockSignals(false);
    slotSliderMoved(m_sensibilitySlider->value());
}

void ImageEffect_Infrared::writeUserSettings()
{
    KConfig* config = kapp->config();
    config->setGroup("infrared Tool Dialog");
    config->writeEntry("SensitivityAjustment", m_sensibilitySlider->value());
    config->writeEntry("AddFilmGrain", m_addFilmGrain->isChecked());
    config->sync();
}

void ImageEffect_Infrared::resetValues()
{
    m_sensibilitySlider->blockSignals(true);
    m_addFilmGrain->blockSignals(true);
    m_sensibilitySlider->setValue(1);
    m_addFilmGrain->setChecked(false);
    m_sensibilitySlider->blockSignals(false);
    m_addFilmGrain->blockSignals(false);
}

void ImageEffect_Infrared::slotSliderMoved(int v)
{
    m_sensibilityLCDValue->display( QString::number(100 + 100 * v) );
}

void ImageEffect_Infrared::prepareEffect()
{
    m_addFilmGrain->setEnabled(false);
    m_sensibilitySlider->setEnabled(false);

    Digikam::DImg image = m_imagePreviewWidget->getOriginalRegionImage();
    int  s = 100 + 100 * m_sensibilitySlider->value();
    bool  g = m_addFilmGrain->isChecked();

    m_threadedFilter = dynamic_cast<Digikam::DImgThreadedFilter *>(
                       new Infrared(&image, this, s, g));
}

void ImageEffect_Infrared::prepareFinal()
{
    m_addFilmGrain->setEnabled(false);
    m_sensibilitySlider->setEnabled(false);

    int  s = 100 + 100 * m_sensibilitySlider->value();
    bool g = m_addFilmGrain->isChecked();

    Digikam::ImageIface iface(0, 0);

    m_threadedFilter = dynamic_cast<Digikam::DImgThreadedFilter *>(
                       new Infrared(iface.getOriginalImg(), this, s, g));
}

void ImageEffect_Infrared::putPreviewData(void)
{
    m_imagePreviewWidget->setPreviewImage(m_threadedFilter->getTargetImage());
}

void ImageEffect_Infrared::putFinalData(void)
{
    Digikam::ImageIface iface(0, 0);
    iface.putOriginalImage(i18n("Infrared"), m_threadedFilter->getTargetImage().bits());
}

}  // NameSpace DigikamInfraredImagesPlugin

