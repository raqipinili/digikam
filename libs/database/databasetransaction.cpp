/* ============================================================
 * Authors: Marcel Wiesweg
 * Date   : 2007-03-23
 * Description : database interface.
 * 
 * Copyright 2007 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
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

#include "albumdb.h"
#include "databaseaccess.h"
#include "databasetransaction.h"

namespace Digikam
{

DatabaseTransaction::DatabaseTransaction()
    : m_access(0)
{
    DatabaseAccess access;
    access.db()->beginTransaction();
}

DatabaseTransaction::DatabaseTransaction(DatabaseAccess *access)
    : m_access(access)
{
    m_access->db()->beginTransaction();
}

DatabaseTransaction::~DatabaseTransaction()
{
    if (m_access)
    {
        m_access->db()->commitTransaction();
    }
    else
    {
        DatabaseAccess access;
        access.db()->commitTransaction();
    }
}

}


