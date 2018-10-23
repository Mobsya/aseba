#include "request.h"

mobsya::AsebaVMDescriptionRequestResult::AsebaVMDescriptionRequestResult(
    const fb::NodeAsebaVMDescriptionT& description) {

    for(const auto& e : description.events) {
        if(!e)
            continue;
        QString name = QString::fromStdString(e->name);
        QString desc = QString::fromStdString(e->description);
        m_events << AsebaVMLocalEventDescription(name, desc);
    }

    for(const auto& f : description.functions) {
        if(!f)
            continue;
        QString name = QString::fromStdString(f->name);
        QString desc = QString::fromStdString(f->description);
        QVector<AsebaVMFunctionParameterDescription> params;
        for(const auto& p : f->parameters) {
            auto name = QString::fromStdString(p->name);
            params << AsebaVMFunctionParameterDescription(name, p->size);
        }
        m_functions << AsebaVMFunctionDescription(name, desc, params);
    }
}