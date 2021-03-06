/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Device Utilities module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qnetworksettingsmanager_p.h"
#include "connman_manager_interface.cpp"
#include "moc_connman_manager_interface.cpp"
#include "qnetworksettingsinterface.h"
#include "qnetworksettingsinterface_p.h"
#include "qnetworksettingsservicemodel.h"
#include "qnetworksettingsuseragent.h"
#include <QFile>
#include <QNetworkInterface>

QT_BEGIN_NAMESPACE

const QString ConnManServiceName(QStringLiteral("net.connman"));
const QString QdbdFileName(QStringLiteral("/etc/default/qdbd"));
const char* SearchKeyword("USB_ETHERNET_PROTOCOL=");

QNetworkSettingsManagerPrivate::QNetworkSettingsManagerPrivate(QNetworkSettingsManager *parent)
    :QObject(parent)
    ,q_ptr(parent)
    , m_interfaceModel(Q_NULLPTR)
    , m_serviceModel(Q_NULLPTR)
    , m_serviceFilter(Q_NULLPTR)
    , m_manager(Q_NULLPTR)
    , m_agent(Q_NULLPTR)
    , m_serviceWatcher(Q_NULLPTR)
    , m_currentWifiConnection(Q_NULLPTR)
    , m_currentWiredConnection(Q_NULLPTR)
    , m_initialized(false)
{
    qDBusRegisterMetaType<ConnmanMapStruct>();
    qDBusRegisterMetaType<ConnmanMapStructList>();

    QNetworkSettingsUserAgent* userAgent = new QNetworkSettingsUserAgent(this);
    this->setUserAgent(userAgent);

    m_serviceModel = new QNetworkSettingsServiceModel(this);
    m_serviceFilter = new QNetworkSettingsServiceFilter(this);
    m_serviceFilter->setSourceModel(m_serviceModel);

    QDBusConnectionInterface* bus = QDBusConnection::systemBus().interface();
    if (bus->isServiceRegistered(ConnManServiceName)) {
        if (!initialize())
            qWarning("Failed to initialize connman connection");
    } else {
        m_serviceWatcher = new QDBusServiceWatcher(this);
        m_serviceWatcher->setConnection(QDBusConnection::systemBus());
        m_serviceWatcher->setWatchedServices(QStringList({ConnManServiceName}));
        connect(m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &QNetworkSettingsManagerPrivate::onConnmanServiceRegistered);
    }
}

bool QNetworkSettingsManagerPrivate::initialize()
{
    if (m_initialized && m_manager)
        return m_initialized;

    m_manager = new NetConnmanManagerInterface(ConnManServiceName, QStringLiteral("/"),
            QDBusConnection::systemBus(), this);

    if (m_manager->isValid()) {
        //List technologies
        QDBusPendingReply<ConnmanMapStructList> reply = m_manager->GetTechnologies();
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
        connect(watcher, &QDBusPendingCallWatcher::finished,
                this, &QNetworkSettingsManagerPrivate::getTechnologiesFinished);

        reply = m_manager->GetServices();
        watcher = new QDBusPendingCallWatcher(reply, this);
        connect(watcher, &QDBusPendingCallWatcher::finished,
               this, &QNetworkSettingsManagerPrivate::getServicesFinished);

        connect(m_manager, &NetConnmanManagerInterface::ServicesChanged, this, &QNetworkSettingsManagerPrivate::onServicesChanged);
        connect(m_manager, &NetConnmanManagerInterface::TechnologyAdded, this, &QNetworkSettingsManagerPrivate::onTechnologyAdded);
        connect(m_manager, &NetConnmanManagerInterface::TechnologyRemoved, this, &QNetworkSettingsManagerPrivate::onTechnologyRemoved);

        m_manager->RegisterAgent(QDBusObjectPath(AgentPath));
        m_initialized = true;
    } else {
        delete m_manager;
        m_manager = nullptr;
        m_initialized = false;
    }
    return m_initialized;
}

void QNetworkSettingsManagerPrivate::requestInput(const QString& service, const QString& type)
{
    Q_UNUSED(service)
    Q_UNUSED(type)
    emit m_agent->showUserCredentialsInput();
}

void QNetworkSettingsManagerPrivate::connectBySsid(const QString &name)
{
    m_unnamedServicesForSsidConnection = m_unnamedServices;
    tryNextConnection();
    m_currentSsid = name;
}

void QNetworkSettingsManagerPrivate::clearConnectionState()
{
    m_unnamedServicesForSsidConnection.clear();
    m_currentSsid.clear();
}

void QNetworkSettingsManagerPrivate::tryNextConnection()
{
    Q_Q(QNetworkSettingsManager);
    QNetworkSettingsService* service = nullptr;
    if (!m_currentSsid.isEmpty()) {
        service = m_serviceModel->getByName(m_currentSsid);
        m_currentSsid.clear();
    }
    if (!service) {
        if (!m_unnamedServicesForSsidConnection.isEmpty()) {
            service = *m_unnamedServicesForSsidConnection.begin();
            m_unnamedServicesForSsidConnection.erase(m_unnamedServicesForSsidConnection.begin());
        } else {
            q->clearConnectionState();
        }
    }
    if (service) {
        service->doConnectService();
    }
}

void QNetworkSettingsManagerPrivate::onConnmanServiceRegistered(const QString &serviceName)
{
    if (serviceName == ConnManServiceName) {
        if (!initialize())
            qWarning("Failed to initialize connman connection");
    }
}

void QNetworkSettingsManagerPrivate::onTechnologyAdded(const QDBusObjectPath &technology, const QVariantMap &properties)
{
    Q_Q(QNetworkSettingsManager);

    foreach (QNetworkSettingsInterface* item, m_interfaceModel.getModel()) {
        ConnmanSettingsInterface* tech = qobject_cast<ConnmanSettingsInterface*>(item);
        if (tech->path() != technology.path()) {
            ConnmanSettingsInterface *interface = new ConnmanSettingsInterface(technology.path(), properties, this);
            interface->scanServices();

            if (interface->type() == QNetworkSettingsType::Wired) {
                m_interfaceModel.insert(0, interface);
            }
            else if (interface->type() == QNetworkSettingsType::Wifi) {
                m_interfaceModel.append(interface);
            }
            emit q->interfacesChanged();
        }
    }
}

void QNetworkSettingsManagerPrivate::onTechnologyRemoved(const QDBusObjectPath &technology)
{
    Q_Q(QNetworkSettingsManager);

    foreach (QNetworkSettingsInterface* item, m_interfaceModel.getModel()) {
        ConnmanSettingsInterface* tech = qobject_cast<ConnmanSettingsInterface*>(item);
        if (tech->path() == technology.path()) {
            m_interfaceModel.removeInterface(technology.path());
            emit q->interfacesChanged();
        }
    }
}

void QNetworkSettingsManagerPrivate::getServicesFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<ConnmanMapStructList> reply = *watcher;
    watcher->deleteLater();

    if (reply.isError())
        return;

    foreach (const ConnmanMapStruct &object, reply.value()) {
        const QString servicePath = object.objectPath.path();
        handleNewService(servicePath);
    }
}

void QNetworkSettingsManagerPrivate::getTechnologiesFinished(QDBusPendingCallWatcher *watcher)
{
    Q_Q(QNetworkSettingsManager);

    QDBusPendingReply<ConnmanMapStructList> reply = *watcher;
    watcher->deleteLater();
    if (reply.isError())
        return;

    foreach (const ConnmanMapStruct &object, reply.value()) {
        ConnmanSettingsInterface *item = new ConnmanSettingsInterface(object.objectPath.path(), object.propertyMap, this);
        item->scanServices();

        if (item->type() == QNetworkSettingsType::Wired) {
            m_interfaceModel.insert(0, item);
        }
        else if (item->type() == QNetworkSettingsType::Wifi) {
            m_interfaceModel.append(item);
        }
        emit q->interfacesChanged();
    }
}

void QNetworkSettingsManagerPrivate::onServicesChanged(ConnmanMapStructList changed, const QList<QDBusObjectPath> &removed)
{
    foreach (QDBusObjectPath path, removed) {
        m_serviceModel->removeService(path.path());
        auto serviceIter = m_unnamedServices.find(path.path());
        if (serviceIter != m_unnamedServices.end()) {
            serviceIter.value()->deleteLater();
            m_unnamedServices.erase(serviceIter);
        }
    }

    QStringList newServices;
    foreach (ConnmanMapStruct map, changed) {
        bool found = false;
        foreach (QNetworkSettingsService* service, m_serviceModel->getModel()) {
            if (service->id() == map.objectPath.path() && service->placeholderState() == false) {
                found =true;
                break;
            }
        }
        if (!found)
            newServices.append(map.objectPath.path());
    }

    foreach (QString newService, newServices) {
        handleNewService(newService);
    }

}

void QNetworkSettingsManagerPrivate::handleNewService(const QString &servicePath)
{
    Q_Q(QNetworkSettingsManager);

    QNetworkSettingsService *service = new QNetworkSettingsService(servicePath, this);

    connect(service, &QNetworkSettingsService::connectionStateCleared,
            q, &QNetworkSettingsManager::clearConnectionState);

    connect(service, &QNetworkSettingsService::serviceConnected,
            q, &QNetworkSettingsManager::setCurrentConnection);
    connect(service, &QNetworkSettingsService::serviceDisconnected,
            q, &QNetworkSettingsManager::clearCurrentConnection);

    if (service->name().length() > 0 && service->type() != QNetworkSettingsType::Unknown) {
        m_serviceModel->append(service);
        emit q->servicesChanged();
        if (service->type() == QNetworkSettingsType::Wired) {
            m_serviceFilter->setWiredNetworksAvailable(true);
        }
    }
    else {
        //Service name or type not set, wait for update
        connect(service, &QNetworkSettingsService::nameChanged, this, &QNetworkSettingsManagerPrivate::serviceReady);
        connect(service, &QNetworkSettingsService::typeChanged, this, &QNetworkSettingsManagerPrivate::serviceReady);
    }
}

void QNetworkSettingsManagerPrivate::setUserAgent(QNetworkSettingsUserAgent *agent)
{
    m_agent = agent;
}

void QNetworkSettingsManagerPrivate::serviceReady()
{
    Q_Q(QNetworkSettingsManager);

    QNetworkSettingsService* service = qobject_cast<QNetworkSettingsService*>(sender());

    if (service->type() != QNetworkSettingsType::Unknown
            && service->type() == QNetworkSettingsType::Wifi) {
        m_unnamedServices.insert(service->id(), service);
    }

    if (service->name().length() > 0 && service->type() != QNetworkSettingsType::Unknown) {
        service->disconnect(this);
        m_unnamedServices.remove(service->id());
        m_serviceModel->append(service);
        emit q->servicesChanged();
        if (service->type() == QNetworkSettingsType::Wired) {
            m_serviceFilter->setWiredNetworksAvailable(true);
        }

        //Update the interface state accordingly
        foreach (QNetworkSettingsInterface* item, m_interfaceModel.getModel()) {
            ConnmanSettingsInterface* technology = qobject_cast<ConnmanSettingsInterface*>(item);
            if (technology->name() == service->name() && technology->type() == service->type()) {
                technology->setState(technology->state());
            }
        }
    }
}

QString QNetworkSettingsManagerPrivate::usbEthernetInternetProtocolAddress()
{
    QString usbEthernetIp = QLatin1String("Not connected");
    QNetworkInterface interface = QNetworkInterface::interfaceFromName(QLatin1String("usb0"));
    if (interface.flags().testFlag(QNetworkInterface::IsUp)) {
        for (QNetworkAddressEntry &entry:interface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol){
                usbEthernetIp = entry.ip().toString();
                break;
            }
        }
    }
    return usbEthernetIp;
}

QString QNetworkSettingsManagerPrivate::usbVirtualEthernetLinkProtocol()
{
    QByteArray line(QNetworkSettingsManagerPrivate::readUsbEthernetProtocolLine());
    QString protocol;
    if (line.size()) {
        int keywordStartIndex(line.indexOf("="));
        line = line.trimmed();
        protocol = QString::fromLatin1( line.mid(keywordStartIndex + 1, (line.length() - 1)).toUpper() );
    }
    return protocol;
}

bool QNetworkSettingsManagerPrivate::hasUsbEthernetProtocolConfiguration()
{
    return !(QNetworkSettingsManagerPrivate::readUsbEthernetProtocolLine().isEmpty());
}

void QNetworkSettingsManagerPrivate::setUsbVirtualEthernetLinkProtocol(const QString &protocol)
{
    if (QLatin1String("RNDIS") == protocol || QLatin1String("CDCECM") == protocol) {
        QByteArray fileContent(QNetworkSettingsManagerPrivate::readQdbdFileContent());
        writeUsbEthernetProtocolToFileContent(fileContent, protocol);
    } else{
        qWarning("Unsupported USB Ethernet protocol");
    }
}

QByteArray QNetworkSettingsManagerPrivate::readQdbdFileContent()
{
    QFile qdbdFile(QdbdFileName);
    QByteArray fileContent;
    if (qdbdFile.open(QIODevice::ReadOnly | QIODevice::Text))
        fileContent = qdbdFile.readAll();

    return fileContent;
}

QByteArray QNetworkSettingsManagerPrivate::readUsbEthernetProtocolLine()
{
    QByteArray fileContent(QNetworkSettingsManagerPrivate::readQdbdFileContent());
    int keywordStartIndex(fileContent.indexOf(SearchKeyword, 0));
    int keywordLineEndIndex(fileContent.indexOf("\n", keywordStartIndex));
    QByteArray keywordLine = fileContent.mid(keywordStartIndex, keywordLineEndIndex);
    return keywordLine;
}

void QNetworkSettingsManagerPrivate::writeUsbEthernetProtocolToFileContent(QByteArray &fileContent, const QString &protocol)
{
    int keywordStartIndex(fileContent.indexOf(SearchKeyword));
    QByteArray previousLines = fileContent.mid(0, keywordStartIndex);
    int keywordLineEndIndex(fileContent.indexOf("\n", keywordStartIndex));
    QByteArray keywordLine = fileContent.mid(keywordStartIndex, keywordLineEndIndex);
    QByteArray followingLines = fileContent.mid((keywordLineEndIndex), (fileContent.length() - 1));
    QByteArray updatedLines = previousLines.append(SearchKeyword);
    updatedLines.append(protocol.toLatin1().toLower());
    updatedLines.append(followingLines);
    QFile qdbdFile(QdbdFileName);
    if (qdbdFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        int result = qdbdFile.write(updatedLines);
        if (-1 == result)
            qDebug("USB Ethernet protocol write to file failed");

    } else {
        qDebug("USB Ethernet protocol file open failed");
    }
}

QT_END_NAMESPACE
