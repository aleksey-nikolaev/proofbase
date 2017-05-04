#include "settings.h"

#include "proofobject_p.h"
#include "settingsgroup.h"

#include <QSettings>
#include <QSharedPointer>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QFileInfo>

namespace Proof {
class SettingsPrivate : public ProofObjectPrivate
{
    Q_DECLARE_PUBLIC(Settings)

    static QString filePath();
    void openSettings();
    void readSettings();
    void fillGroupFromSettings(SettingsGroup *groupToFill);
    void groupValueChanged(const QStringList &key, const QVariant &value);

    SettingsGroup *mainGroup;
    QSharedPointer<QSettings> settings;
};
}

using namespace Proof;

Settings::Settings(QObject *parent)
    : ProofObject(*new SettingsPrivate, parent)
{
    Q_D(Settings);
    d->mainGroup = new SettingsGroup(QLatin1String(""), this);
    connect(d->mainGroup, &SettingsGroup::valueChanged, this,
            [d](const QStringList &key, const QVariant &value){d->groupValueChanged(key, value);});
    d->readSettings();
}

Settings::~Settings()
{
    Q_D(Settings);
    d->settings.clear();
}

void Settings::sync()
{
    Q_D(Settings);
    d->settings->sync();
    d->readSettings();
}

SettingsGroup *Settings::mainGroup()
{
    Q_D(Settings);
    return d->mainGroup;
}

QStringList Settings::groups() const
{
    Q_D(const Settings);
    return d->mainGroup->groups();
}

SettingsGroup *Settings::group(const QString &groupName, NotFoundPolicy notFoundPolicy)
{
    Q_D(Settings);
    return d->mainGroup->group(groupName, notFoundPolicy);
}

SettingsGroup *Settings::addGroup(const QString &groupName)
{
    Q_D(Settings);
    return d->mainGroup->addGroup(groupName);
}

void Settings::deleteGroup(const QString &groupName)
{
    Q_D(Settings);
    d->mainGroup->deleteGroup(groupName);
}

QString Settings::filePath()
{
    return SettingsPrivate::filePath();
}

QString SettingsPrivate::filePath()
{
    if (qApp->arguments().contains(QStringLiteral("-c"))) {
        int index = qApp->arguments().indexOf(QStringLiteral("-c")) + 1;
        if (index < qApp->arguments().count()) {
            QString configPath = qApp->arguments().at(index);
            if (QFileInfo::exists(configPath))
                return configPath;
        }
    }
#if defined Q_OS_WIN
    //Windows already gives us org/app as part of conf location
    QString configPath = QString("%1/%2.conf")
            .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
            .arg(qApp->applicationName());
#elif defined Q_OS_ANDROID
    QString configPath = QString("%1/%2/%3.conf")
            .arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
            .arg(qApp->organizationName())
            .arg(qApp->applicationName());
#else
    QString configPath = QStringLiteral("%1/%2/%3.conf")
            .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation),
                 qApp->organizationName(),
                 qApp->applicationName());
#endif
    return configPath;
}

void SettingsPrivate::openSettings()
{
    QString configPath = filePath();
    settings = QSharedPointer<QSettings>::create(configPath, QSettings::IniFormat);
    qCDebug(proofCoreSettingsLog) << "Settings at:" << configPath;
}

void SettingsPrivate::readSettings()
{
    if (!settings)
        openSettings();
    fillGroupFromSettings(mainGroup);
}

void SettingsPrivate::fillGroupFromSettings(SettingsGroup *groupToFill)
{
    QSet<QString> toRemove = groupToFill->values().toSet();
    const auto childKeys = settings->childKeys();
    for (const QString &key : childKeys) {
        toRemove.remove(key);
        groupToFill->setValue(key, settings->value(key));
    }
    for (const QString &key : qAsConst(toRemove))
        groupToFill->deleteValue(key);

    toRemove = groupToFill->groups().toSet();
    const auto childGroups = settings->childGroups();
    for (const QString &key : childGroups) {
        toRemove.remove(key);
        SettingsGroup *group = groupToFill->addGroup(key);
        settings->beginGroup(key);
        fillGroupFromSettings(group);
        settings->endGroup();
    }
    for (const QString &key : qAsConst(toRemove))
        groupToFill->deleteGroup(key);
}

void SettingsPrivate::groupValueChanged(const QStringList &key, const QVariant &value)
{
    Q_ASSERT_X(key.count(), Q_FUNC_INFO, "key list can't be empty");

    QString groupPathToRestore = settings->group();
    QStringList groupsToRestore;
    while (!settings->group().isEmpty()) {
        settings->endGroup();
        QString newPath = settings->group();
        groupsToRestore.prepend(newPath.isEmpty()
                                ? groupPathToRestore
                                : groupPathToRestore.mid(newPath.length() + 1));
        groupPathToRestore = newPath;
    }

    for (int i = 0; i < key.count() - 1; ++i)
        settings->beginGroup(key[i]);

    if (value.isNull())
        settings->remove(key.last());
    else
        settings->setValue(key.last(), value);

    for (int i = 0; i < key.count() - 1; ++i)
        settings->endGroup();

    for (const QString &groupToRestore : groupsToRestore)
        settings->beginGroup(groupToRestore);
}
