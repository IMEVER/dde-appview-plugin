/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -p launcherdbusinterface -c LauncherDbus com.deepin.dde.daemon.Launcher.xml com.deepin.dde.daemon.Launcher
 *
 * qdbusxml2cpp is Copyright (C) 2020 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef LAUNCHERDBUSINTERFACE_H
#define LAUNCHERDBUSINTERFACE_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

struct ItemInfo
{
    QString s1;
    QString s2;
    QString s3;
    QString s4;
    int i1;
    int i2;
};

typedef QList<ItemInfo> ItemInfoList;

Q_DECLARE_METATYPE(ItemInfo)
Q_DECLARE_METATYPE(ItemInfoList)

/*
 * Proxy class for interface com.deepin.dde.daemon.Launcher
 */
class LauncherDbus: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "com.deepin.dde.daemon.Launcher"; }

public:
    LauncherDbus(const QDBusConnection &connection, QObject *parent = nullptr);

    ~LauncherDbus();

    Q_PROPERTY(int DisplayMode READ displayMode WRITE setDisplayMode)
    inline int displayMode() const
    { return qvariant_cast< int >(property("DisplayMode")); }
    inline void setDisplayMode(int value)
    { setProperty("DisplayMode", QVariant::fromValue(value)); }

    Q_PROPERTY(bool Fullscreen READ fullscreen WRITE setFullscreen)
    inline bool fullscreen() const
    { return qvariant_cast< bool >(property("Fullscreen")); }
    inline void setFullscreen(bool value)
    { setProperty("Fullscreen", QVariant::fromValue(value)); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<ItemInfoList> GetAllItemInfos()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("GetAllItemInfos"), argumentList);
    }

    inline QDBusPendingReply<QStringList> GetAllNewInstalledApps()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("GetAllNewInstalledApps"), argumentList);
    }

    inline QDBusPendingReply<bool> GetDisableScaling(const QString &id)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(id);
        return asyncCallWithArgumentList(QStringLiteral("GetDisableScaling"), argumentList);
    }

    inline QDBusPendingReply<ItemInfo> GetItemInfo(const QString &id)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(id);
        return asyncCallWithArgumentList(QStringLiteral("GetItemInfo"), argumentList);
    }

    inline QDBusPendingReply<bool> GetUseProxy(const QString &id)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(id);
        return asyncCallWithArgumentList(QStringLiteral("GetUseProxy"), argumentList);
    }

    inline QDBusPendingReply<bool> IsItemOnDesktop(const QString &id)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(id);
        return asyncCallWithArgumentList(QStringLiteral("IsItemOnDesktop"), argumentList);
    }

    inline QDBusPendingReply<> MarkLaunched(const QString &id)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(id);
        return asyncCallWithArgumentList(QStringLiteral("MarkLaunched"), argumentList);
    }

    inline QDBusPendingReply<bool> RequestRemoveFromDesktop(const QString &id)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(id);
        return asyncCallWithArgumentList(QStringLiteral("RequestRemoveFromDesktop"), argumentList);
    }

    inline QDBusPendingReply<bool> RequestSendToDesktop(const QString &id)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(id);
        return asyncCallWithArgumentList(QStringLiteral("RequestSendToDesktop"), argumentList);
    }

    inline QDBusPendingReply<> RequestUninstall(const QString &id, bool purge)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(id) << QVariant::fromValue(purge);
        return asyncCallWithArgumentList(QStringLiteral("RequestUninstall"), argumentList);
    }

    inline QDBusPendingReply<> Search(const QString &key)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(key);
        return asyncCallWithArgumentList(QStringLiteral("Search"), argumentList);
    }

    inline QDBusPendingReply<> SetDisableScaling(const QString &id, bool value)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(id) << QVariant::fromValue(value);
        return asyncCallWithArgumentList(QStringLiteral("SetDisableScaling"), argumentList);
    }

    inline QDBusPendingReply<> SetUseProxy(const QString &id, bool value)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(id) << QVariant::fromValue(value);
        return asyncCallWithArgumentList(QStringLiteral("SetUseProxy"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void ItemChanged(const QString &status, ItemInfo itemInfo, qlonglong categoryID);
    void NewAppLaunched(const QString &appID);
    void SearchDone(const QStringList &apps);
    void UninstallFailed(const QString &appId, const QString &errMsg);
    void UninstallSuccess(const QString &appID);
};

namespace com {
  namespace deepin {
    namespace dde {
      namespace daemon {
        typedef ::LauncherDbus Launcher;
      }
    }
  }
}
#endif