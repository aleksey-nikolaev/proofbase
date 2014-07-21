#ifndef NETWORKDATAENTITYQMLWRAPPER_H
#define NETWORKDATAENTITYQMLWRAPPER_H

#include "proofcore/proofobject.h"
#include "proofnetwork/proofnetwork_global.h"

#include <QSharedPointer>

#define PROOF_NDE_WRAPPED_PROPERTY_IMPL_R(Nde, Type, Getter) \
    Type Nde##QmlWrapper::Getter() const \
    { \
        Q_D(const Nde##QmlWrapper); \
        const QSharedPointer<Nde> entity = d->entity<Nde>(); \
        Q_ASSERT(entity); \
        return entity->Getter(); \
    }

#define PROOF_NDE_WRAPPED_PROPERTY_IMPL_RW(Nde, Type, Getter, Setter) \
    PROOF_NDE_WRAPPED_PROPERTY_IMPL_R(Nde, Type, Getter) \
    void Nde##QmlWrapper::Setter(const Type &arg) \
    { \
        Q_D(const Nde##QmlWrapper); \
        QSharedPointer<Nde> entity = d->entity<Nde>(); \
        Q_ASSERT(entity); \
        if (arg != entity->Getter()) { \
            entity->Setter(arg); \
            emit Getter##Changed(arg); \
        } \
    }

namespace Proof {
class NetworkDataEntity;
class NetworkDataEntityQmlWrapperPrivate;
class PROOF_NETWORK_EXPORT NetworkDataEntityQmlWrapper : public ProofObject
{
    Q_OBJECT
    Q_PROPERTY(bool isFetched READ isFetched NOTIFY isFetchedChanged)
    Q_DECLARE_PRIVATE(NetworkDataEntityQmlWrapper)
public:
    bool isFetched() const;

signals:
    void isFetchedChanged(bool arg);

protected:
    NetworkDataEntityQmlWrapper() = delete;
    explicit NetworkDataEntityQmlWrapper(const QSharedPointer<NetworkDataEntity> &networkDataEntity,
                                         NetworkDataEntityQmlWrapperPrivate &dd, QObject *parent = 0);
};
}

#endif // NETWORKDATAENTITYQMLWRAPPER_H
