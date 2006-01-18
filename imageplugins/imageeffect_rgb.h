/* ============================================================
 * Author: Gilles Caulier <caulier dot gilles at free.fr>
 * Date  : 2004-07-11
 * Description : digiKam image editor Color Balance tool.
 * 
 * Copyright 2004-2006 by Gilles Caulier
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

#ifndef IMAGEEFFECT_RGB_H
#define IMAGEEFFECT_RGB_H

// Digikam include.

#include "imagedlgbase.h"

class QComboBox;
class QHButtonGroup;

class QSpinBox;
class QSlider;

namespace Digikam
{
class HistogramWidget;
class ColorGradientWidget;
class ImageGuideWidget;
class DColor;
}

namespace DigikamImagesPluginCore
{

class ImageEffect_RGB : public Digikam::ImageDlgBase
{
    Q_OBJECT

public:

    ImageEffect_RGB(QWidget *parent);
    ~ImageEffect_RGB();

    enum HistogramScale
    {
        Linear=0,
        Logarithmic
    };

    enum ColorChannel
    {
        LuminosityChannel=0,
        RedChannel,
        GreenChannel,
        BlueChannel
    };

    uchar                        *m_destinationPreviewData;

    QComboBox                    *m_channelCB;    
    
    QHButtonGroup                *m_scaleBG;  

    QSpinBox                     *m_rInput;
    QSpinBox                     *m_gInput;
    QSpinBox                     *m_bInput;
    
    QSlider                      *m_rSlider;
    QSlider                      *m_gSlider;
    QSlider                      *m_bSlider;
    
    Digikam::ImageGuideWidget    *m_previewWidget;

    Digikam::ColorGradientWidget *m_hGradient;
    
    Digikam::HistogramWidget     *m_histogramWidget;
        
private:

    void adjustSliders(int r, int g, int b);
    
private slots:

    void slotDefault();
    void slotEffect();
    void slotOk();
    void slotChannelChanged(int channel);
    void slotScaleChanged(int scale);
    void slotColorSelectedFromTarget( const Digikam::DColor &color );
    
};

}  // NameSpace DigikamImagesPluginCore

#endif /* IMAGEEFFECT_RGB_H */
