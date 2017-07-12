/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2017-07-04
 * Description : Autodetect kmail binary program
 *
 * Copyright (C) 2017 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef KMAIL_BINARY_H
#define KMAIL_BINARY_H

// Local includes

#include "dbinaryiface.h"

namespace Digikam
{

class KmailBinary : public DBinaryIface
{
public:

    KmailBinary()
        : DBinaryIface(
                       QLatin1String("kmail"),
                       QLatin1String("KMail"),
                       QLatin1String("https://www.kde.org/applications/internet/kmail/"),
                       QLatin1String("SendByMail"),
                       QStringList(QLatin1String("-v")),
                       i18n("KDE Mail Client.")
                      )
        {
            setup();
        }

    ~KmailBinary()
    {
    }
};

} // namespace Digikam

#endif // KMAIL_BINARY_H
