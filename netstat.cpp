/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
**
**  SysStat is a Qt-based interface to system statistics
**
**  Authors:
**       Copyright (c) 2009 - 2012 Kuzma Shapran <Kuzma.Shapran@gmail.com>
**
**  This library is free software; you can redistribute it and/or
**  modify it under the terms of the GNU Lesser General Public
**  License as published by the Free Software Foundation; either
**  version 2.1 of the License, or (at your option) any later version.
**
**  This library is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**  Lesser General Public License for more details.
**
**  You should have received a copy of the GNU Lesser General Public
**  License along with this library;
**  if not, write to the Free Software Foundation, Inc.,
**  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** END_COMMON_COPYRIGHT_HEADER */


#include "netstat.h"
#include "netstat_p.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_SYSCTL_H) && defined(HAVE_IF_H)
extern "C"
{
    #include <net/if.h>
    #include <net/if_mib.h>
    #include <net/if_types.h>
    #include <sys/socket.h> /* PF_LINK */
    #include <sys/types.h>
    #include <sys/sysctl.h>
}
#endif


namespace SysStat {

NetStatPrivate::NetStatPrivate(NetStat *parent)
    : BaseStatPrivate(parent)
{
    mSource = defaultSource();

    connect(mTimer, SIGNAL(timeout()), SLOT(timeout()));

#ifndef HAVE_SYSCTL_H

    QStringList rows(readAllFile("/proc/net/dev").split(QLatin1Char('\n'), Qt::SkipEmptyParts));

    rows.erase(rows.begin(), rows.begin() + 2);

    for (const QString &row : qAsConst(rows))
    {
        QStringList tokens = row.split(QLatin1Char(':'), Qt::SkipEmptyParts);
        if (tokens.size() != 2)
            continue;

        mSources.append(tokens[0].trimmed());
    }
#else
        int count;
        size_t len;
        int cntifmib[] = { CTL_NET, PF_LINK, NETLINK_GENERIC, IFMIB_SYSTEM, IFMIB_IFCOUNT };// net.link.generic.system.ifcount;
        len = sizeof(int);
        if (sysctl(cntifmib, 5, &count, &len, NULL, 0) < 0)
            perror("sysctl");


        struct ifmibdata ifmd;
        size_t len1 = sizeof(ifmd);
        for (int i=1; i<=count;i++) {
            int name[] = { CTL_NET, PF_LINK, NETLINK_GENERIC, IFMIB_IFDATA, i, IFDATA_GENERAL };

            if (sysctl(name, 6, &ifmd, &len1, NULL, 0) < 0) {
                perror("sysctl");
            }
            if ((ifmd.ifmd_data.ifi_type == IFT_ETHER) || (ifmd.ifmd_data.ifi_type == IFT_IEEE80211)) {
            const char *iface = ifmd.ifmd_name;
            mSources.append(QString::fromLatin1(iface));
            }
        }
#endif
}

NetStatPrivate::~NetStatPrivate() = default;

void NetStatPrivate::timeout()
{
#if defined(HAVE_IF_H) && defined(HAVE_SYSCTL_H)
    int count;
    size_t len;
    int name[] = { CTL_NET, PF_LINK, NETLINK_GENERIC, IFMIB_SYSTEM, IFMIB_IFCOUNT };
    struct ifmibdata ifmd;

    len = sizeof(int);
    if (sysctl(name, 5, &count, &len, NULL, 0) < 0)
        return;

    for (int i = 1; i <= count; i++)
    {
        len = sizeof(ifmd);
        int name[] = { CTL_NET, PF_LINK, NETLINK_GENERIC, IFMIB_IFDATA, i, IFDATA_GENERAL };

        if (sysctl(name, 6, &ifmd, &len, NULL, 0) < 0)
            break;

        if ((ifmd.ifmd_data.ifi_type == IFT_ETHER) || (ifmd.ifmd_data.ifi_type == IFT_IEEE80211))
        {
            const char *iface = ifmd.ifmd_name;
            QString interfaceName = QString::fromLatin1(iface);
            if ((ifmd.ifmd_data.ifi_link_state == LINK_STATE_UP) && (ifmd.ifmd_data.ifi_ipackets > 0))
            {


                Values current;
                current.received = ifmd.ifmd_data.ifi_ibytes;
                current.transmitted = ifmd.ifmd_data.ifi_obytes;

                if (!mPrevious.contains(interfaceName))
                    mPrevious.insert(interfaceName, Values());
                const Values &previous = mPrevious[interfaceName];

                if (interfaceName == mSource)
                    emit update((( current.received - previous.received ) * 1000 ) / mTimer->interval(), (( current.transmitted - previous.transmitted ) * 1000 ) / mTimer->interval());

                mPrevious[interfaceName] = current;
            } else if(interfaceName == mSource)
                emit(update(0,0));

        }
    }
#else
    QStringList rows(readAllFile("/proc/net/dev").split(QLatin1Char('\n'), Qt::SkipEmptyParts));


    if (rows.size() < 2)
        return;

    QStringList names = rows[1].split(QLatin1Char('|'));
    if (names.size() != 3)
        return;
    QStringList namesR = names[1].split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QStringList namesT = names[2].split(QLatin1Char(' '), Qt::SkipEmptyParts);
    int receivedIndex    =                 namesR.indexOf(QLatin1String("bytes"));
    int transmittedIndex = namesR.size() + namesT.indexOf(QLatin1String("bytes"));

    rows.erase(rows.begin(), rows.begin() + 2);

    for (const QString &row : qAsConst(rows))
    {
        QStringList tokens = row.split(QLatin1Char(':'), Qt::SkipEmptyParts);
        if (tokens.size() != 2)
            continue;

        QString interfaceName = tokens[0].trimmed();

        QStringList data = tokens[1].split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (data.size() < transmittedIndex)
            continue;

        Values current;
        current.received    = data[receivedIndex   ].toULongLong();
        current.transmitted = data[transmittedIndex].toULongLong();

        if (!mPrevious.contains(interfaceName))
            mPrevious.insert(interfaceName, Values());
        const Values &previous = mPrevious[interfaceName];

        if (interfaceName == mSource)
            emit update((( current.received - previous.received ) * 1000 ) / mTimer->interval(), (( current.transmitted - previous.transmitted ) * 1000 ) / mTimer->interval());

        mPrevious[interfaceName] = current;
    }
#endif
}

QString NetStatPrivate::defaultSource()
{
    return QLatin1String("lo");
}

NetStatPrivate::Values::Values()
    : received(0)
    , transmitted(0)
{
}

NetStat::NetStat(QObject *parent)
    : BaseStat(parent)
{
    impl = new NetStatPrivate(this);
    baseimpl = impl;

    connect(impl, SIGNAL(update(unsigned,unsigned)), this, SIGNAL(update(unsigned,unsigned)));
}

NetStat::~NetStat() = default;

}
