#include "main.h"
#include "Context.h"
#include "Model.h"
#include <memory>

Context::Context(bool hasPreview) 
    : m_hasPreview(hasPreview) {

    if (m_hasPreview) {
        m_previewChain = QSharedPointer<Chain>(new Chain(m_previewSize)); // TODO setSize
    }
}

Context::~Context() {
}

Model *Context::model() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_model;
}

void Context::setModel(Model *model) {
    Q_ASSERT(QThread::currentThread() == thread());
    m_model = model;
    emit modelChanged(model);
}

QSize Context::previewSize() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_previewSize;
}

void Context::setPreviewSize(QSize size) {
    Q_ASSERT(QThread::currentThread() == thread());
    if (size != m_previewSize) {
        m_previewSize = size;
        QSharedPointer<Chain> previewChain(new Chain(size));
        m_previewChain = previewChain;
        chainsChanged();
        emit previewSizeChanged(size);
    }
}

void Context::onRenderRequested() {
    auto output = qobject_cast<Output *>(sender());
    auto chain = output->chain();
    auto modelCopy = m_model->createCopyForRendering();
    auto result = modelCopy.render(chain);
    m_model->copyBackRenderStates(chain, &modelCopy);
}

QList<QSharedPointer<Chain>> Context::chains() {
    auto o = outputs();
    QList<QSharedPointer<Chain>> chains;
    if (m_hasPreview) {
        chains.append(m_previewChain);
    }
    for (int i=0; i<o.count(); i++) {
        chains.append(o.at(i)->chain());
    }
    return chains;
}

void Context::chainsChanged() {
    m_model->setChains(chains());
}

QList<Output *> Context::outputs() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_outputs;
}

void Context::setOutputs(QList<Output *> outputs) {
    Q_ASSERT(QThread::currentThread() == thread());
    if (m_outputs != outputs) {
        // TODO I am lazy
        for (int i=0; i<m_outputs.count(); i++) {
            disconnect(m_outputs.at(i), &Output::renderRequested, this, &Context::onRenderRequested);
            disconnect(m_outputs.at(i), &Output::chainChanged, this, &Context::chainsChanged);
        }
        m_outputs = outputs;
        for (int i=0; i<outputs.count(); i++) {
            connect(outputs.at(i), &Output::renderRequested, this, &Context::onRenderRequested, Qt::DirectConnection);
            connect(outputs.at(i), &Output::chainChanged, this, &Context::chainsChanged);
        }
        chainsChanged();
        emit outputsChanged(outputs);
    }
}
