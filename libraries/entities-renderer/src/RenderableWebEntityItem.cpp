//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableWebEntityItem.h"

#include <glm/gtx/quaternion.hpp>

#include <gpu/GPUConfig.h>

#include <DeferredLightingEffect.h>
#include <GeometryCache.h>
#include <PerfStat.h>
#include <TextRenderer.h>
#include <OffscreenQmlSurface.h>
#include <AbstractViewStateInterface.h>
#include <GLMHelpers.h>
#include <PathUtils.h>
#include <TextureCache.h>
#include <gpu/GLBackend.h>

const int FIXED_FONT_POINT_SIZE = 40;
const float DPI = 30.47;
const float METERS_TO_INCHES = 39.3701;

EntityItem* RenderableWebEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderableWebEntityItem(entityID, properties);
}

RenderableWebEntityItem::RenderableWebEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    WebEntityItem(entityItemID, properties) {
}

RenderableWebEntityItem::~RenderableWebEntityItem() {
    if (_webSurface) {
        _webSurface->pause();
        _webSurface->disconnect(_connection);
        if (_texture) {
            _webSurface->releaseTexture(_texture);
            _texture = 0;
        }
        _webSurface.clear();
    }
}

void RenderableWebEntityItem::render(RenderArgs* args) {
    QOpenGLContext * currentContext = QOpenGLContext::currentContext();
    QSurface * currentSurface = currentContext->surface();
    if (!_webSurface) {
        _webSurface = QSharedPointer<OffscreenQmlSurface>(new OffscreenQmlSurface());
        _webSurface->create(currentContext);
        _webSurface->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/"));
        _webSurface->load("WebEntity.qml");
        _webSurface->resume();
        updateQmlSourceUrl();
        _connection = QObject::connect(_webSurface.data(), &OffscreenQmlSurface::textureUpdated, [&](GLuint textureId) {
            _webSurface->lockTexture(textureId);
            assert(!glGetError());
            std::swap(_texture, textureId);
            if (textureId) {
                _webSurface->releaseTexture(textureId);
            }
            if (_texture) {
                _webSurface->makeCurrent();
                glBindTexture(GL_TEXTURE_2D, _texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glBindTexture(GL_TEXTURE_2D, 0);
                _webSurface->doneCurrent();
            }
        });
    }

    glm::vec2 dims = glm::vec2(_dimensions);
    dims *= METERS_TO_INCHES * DPI;
    // The offscreen surface is idempotent for resizes (bails early
    // if it's a no-op), so it's safe to just call resize every frame 
    // without worrying about excessive overhead.
    _webSurface->resize(QSize(dims.x, dims.y));
    currentContext->makeCurrent(currentSurface);

    PerformanceTimer perfTimer("RenderableWebEntityItem::render");
    assert(getType() == EntityTypes::Web);
    glm::vec3 position = getPosition();
    glm::vec3 dimensions = getDimensions();
    glm::vec3 halfDimensions = dimensions / 2.0f;
    glm::quat rotation = getRotation();

    //qCDebug(entitytree) << "RenderableWebEntityItem::render() id:" << getEntityItemID() << "text:" << getText();

    glPushMatrix(); 
    {
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);

        float alpha = 1.0f; 
        static const glm::vec2 texMin(0);
        static const glm::vec2 texMax(1);
        glm::vec2 topLeft(-halfDimensions.x, -halfDimensions.y);
        glm::vec2 bottomRight(halfDimensions.x, halfDimensions.y);
        if (_texture) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, _texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        DependencyManager::get<GeometryCache>()->renderQuad(
            topLeft, bottomRight,  texMin, texMax, glm::vec4(1));
        if (_texture) {
            glBindTexture(GL_TEXTURE_2D, 0);
            glEnable(GL_TEXTURE_2D);
        }
    }
    glPopMatrix();
}

void RenderableWebEntityItem::setSourceUrl(const QString& value) {
    if (_sourceUrl != value) {
        _sourceUrl = value;
        AbstractViewStateInterface::instance()->postLambdaEvent([this] {
            // Update the offscreen display
            updateQmlSourceUrl();
        });
    }
}

void RenderableWebEntityItem::updateQmlSourceUrl() {
    if (!_webSurface) {
        return;
    }
    auto webView = _webSurface->getRootItem();
    if (!webView) {
        return;
    }
    if (!_sourceUrl.isEmpty()) {
        webView->setProperty("url", _sourceUrl);
    } else {
        webView->setProperty("url", QVariant());
    }
}