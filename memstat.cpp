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


#include "memstat.h"
#include "memstat_p.h"
#if defined(HAVE_KVM_H) && defined(HAVE_SYSCTL_H)
extern "C"
{
    #include <paths.h>
    #include <unistd.h>
    #include <fcntl.h>

    #include <kvm.h>
    #include <sys/types.h>
    #include <sys/sysctl.h>
}
#endif

namespace SysStat {
#ifdef HAVE_SYSCTL_H
int SwapDevices()
{
    int buf;
    size_t len = sizeof(int);

    if (sysctlbyname("vm.nswapdev", &buf, &len, NULL, 0) < 0)
        return 0;
    else
        return buf;
}

qulonglong MemGetByBytes(QString property)
{
    qulonglong buf=0;
    size_t len = sizeof(qulonglong);

    std::string s = property.toStdString();
    const char *name = s.c_str();

    if (sysctlbyname(name, &buf, &len, NULL, 0) < 0)
        return 0;
    else
        return buf;
}

qulonglong MemGetByPages(QString name)
{
    qulonglong res = 0;


    res = MemGetByBytes(name);
    if (res > 0)
        res = res * getpagesize();

    return res;
}
#endif
MemStatPrivate::MemStatPrivate(MemStat *parent)
    : BaseStatPrivate(parent)
{
    mSource = defaultSource();

    connect(mTimer, SIGNAL(timeout()), SLOT(timeout()));

    mSources << QLatin1String("memory") << QLatin1String("swap");
}

MemStatPrivate::~MemStatPrivate() = default;

void MemStatPrivate::timeout()
{
    qulonglong memTotal = 0;
    qulonglong memFree = 0;
    qulonglong memBuffers = 0;
    qulonglong memCached = 0;
    qulonglong swapTotal = 0;
#ifdef HAVE_SYSCTL_H
    memTotal = MemGetByBytes(QLatin1String("hw.physmem"));
    memFree = MemGetByPages(QLatin1String("vm.stats.vm.v_free_count"));
    memBuffers = MemGetByBytes(QLatin1String("vfs.bufspace"));
    memCached = MemGetByPages(QLatin1String("vm.stats.vm.v_inactive_count"));

#endif
#ifdef HAVE_KVM_H
    qulonglong swapUsed = 0;
    kvm_t *kd;
    struct kvm_swap kswap[16]; /* size taken from pstat/pstat.c */

    kd = kvm_open(NULL, _PATH_DEVNULL, NULL, O_RDONLY, "kvm_open");
    if (kd == NULL)
        kvm_close(kd);

    if (kvm_getswapinfo(kd, kswap, (sizeof(kswap) / sizeof(kswap[0])), SWIF_DEV_PREFIX) > 0)
    {
        int swapd = SwapDevices();
        /* TODO: loop over swap devives */
        if (swapd >= 1)
        {
            swapTotal = static_cast<qulonglong>(kswap[0].ksw_total * getpagesize());
            swapUsed = static_cast<qulonglong>(kswap[0].ksw_used * getpagesize());
        }

        kvm_close(kd);
    }
    else
        kvm_close(kd);
#endif
#ifndef HAVE_SYSCTL_H
    qulonglong swapFree = 0;
    const QStringList rows = readAllFile("/proc/meminfo").split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &row : rows)
    {
        QStringList tokens = row.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (tokens.size() != 3)
            continue;

        if (tokens[0] == QLatin1String("MemTotal:"))
            memTotal = tokens[1].toULong();
        else if(tokens[0] == QLatin1String("MemFree:"))
            memFree = tokens[1].toULong();
        else if(tokens[0] == QLatin1String("Buffers:"))
            memBuffers = tokens[1].toULong();
        else if(tokens[0] == QLatin1String("Cached:"))
            memCached = tokens[1].toULong();
        else if(tokens[0] == QLatin1String("SwapTotal:"))
            swapTotal = tokens[1].toULong();
        else if(tokens[0] == QLatin1String("SwapFree:"))
            swapFree = tokens[1].toULong();
    }

#endif
    if (mSource == QLatin1String("memory"))
    {
        if (memTotal)
        {
            float memTotal_d     = static_cast<float>(memTotal);
            float applications_d = static_cast<float>(memTotal - memFree - memBuffers - memCached) / memTotal_d;
            float buffers_d      = static_cast<float>(memBuffers) / memTotal_d;
            float cached_d       = static_cast<float>(memCached) / memTotal_d;

            emit memoryUpdate(applications_d, buffers_d, cached_d);
        }
    }
    else if (mSource == QLatin1String("swap"))
    {
        if (swapTotal)
        {
#ifndef HAVE_KVM_H
            float swapUsed_d = static_cast<float>(swapTotal - swapFree) / static_cast<float>(swapTotal);
#else
            float swapUsed_d = static_cast<float>(swapUsed) / static_cast<float>(swapTotal);
#endif
            emit swapUpdate(swapUsed_d);
        }
    }
}

QString MemStatPrivate::defaultSource()
{
    return QLatin1String("memory");
}

MemStat::MemStat(QObject *parent)
    : BaseStat(parent)
{
    impl = new MemStatPrivate(this);
    baseimpl = impl;

    connect(impl, SIGNAL(memoryUpdate(float,float,float)), this, SIGNAL(memoryUpdate(float,float,float)));
    connect(impl, SIGNAL(swapUpdate(float)), this, SIGNAL(swapUpdate(float)));
}

MemStat::~MemStat() = default;

}
