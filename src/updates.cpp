#include "headers/updates.h"
#include "actors/Player.h"

#include <array>
#include <cstring>
#include <functional>

// texture manager
std::unordered_map<std::string, GLuint> TextureManager::textures;

static bool IsBoundsVisible(const glm::mat4 &clipMatrix, const glm::vec3 &min, const glm::vec3 &max) {

    const glm::vec3 corners[8] = {{min.x, min.y, min.z}, {max.x, min.y, min.z}, {min.x, max.y, min.z},
                                  {max.x, max.y, min.z}, {min.x, min.y, max.z}, {max.x, min.y, max.z},
                                  {min.x, max.y, max.z}, {max.x, max.y, max.z}};

    auto allOutside = [&](auto predicate) {

        for (const glm::vec3 &corner : corners) {

            const glm::vec4 clip = clipMatrix * glm::vec4(corner, 1.0f);

            if (!predicate(clip))
                return false;
        }

        return true;
    };

    return !allOutside([](const glm::vec4 &p) { return p.x < -p.w; }) &&
           !allOutside([](const glm::vec4 &p) { return p.x > p.w; }) &&
           !allOutside([](const glm::vec4 &p) { return p.y < -p.w; }) &&
           !allOutside([](const glm::vec4 &p) { return p.y > p.w; }) &&
           !allOutside([](const glm::vec4 &p) { return p.z < -p.w; }) &&
           !allOutside([](const glm::vec4 &p) { return p.z > p.w; });
}

//--------------------------------------------------------------------------
// this is crazy work from ai

static std::array<std::size_t, MAX_LIGHTS> shadowMapSignatures{};
static std::array<bool, MAX_LIGHTS> shadowMapSignatureValid{};

static void CombineShadowHash(std::size_t &seed, std::size_t value) {

    seed ^= value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
}

template <typename T> static void HashShadowValue(std::size_t &seed, const T &value) {

    CombineShadowHash(seed, std::hash<T>{}(value));
}

static void HashShadowVector(std::size_t &seed, const glm::vec3 &value) {

    HashShadowValue(seed, value.x);
    HashShadowValue(seed, value.y);
    HashShadowValue(seed, value.z);
}

static void HashShadowRotation(std::size_t &seed, const glm::quat &value) {

    HashShadowValue(seed, value.x);
    HashShadowValue(seed, value.y);
    HashShadowValue(seed, value.z);
    HashShadowValue(seed, value.w);
}

static std::size_t BuildShadowSceneSignature() {

    std::size_t signature = 0;

    HashShadowValue(signature, objects.size());
    HashShadowValue(signature, viewport_width);
    HashShadowValue(signature, viewport_height);
    HashShadowValue(signature, cam.nearPlane);
    HashShadowValue(signature, cam.farPlane);

    for (const Object &obj : objects) {

        HashShadowValue(signature, obj.id);
        HashShadowValue(signature, obj.parentID);
        HashShadowValue(signature, obj.visible);
        HashShadowValue(signature, obj.renderBothSides);
        HashShadowValue(signature, obj.shadowGeometryRevision);
        HashShadowValue(signature, obj.vao);
        HashShadowValue(signature, obj.vertices.size());
        HashShadowValue(signature, obj.indices.size());
        HashShadowValue(signature, obj.submeshes.size());
        HashShadowVector(signature, obj.position);
        HashShadowRotation(signature, obj.rotation);
        HashShadowVector(signature, obj.scale);
    }

    return signature;
}

static std::size_t BuildShadowLightSignature(size_t sceneSignature, size_t lightIndex) {

    std::size_t signature = sceneSignature;
    const Light &light = lights[lightIndex];
    const Object &lightObject = lightObjects[lightIndex];

    HashShadowValue(signature, light.typeID);
    HashShadowValue(signature, lightObject.id);
    HashShadowVector(signature, light.pos);
    HashShadowVector(signature, light.dir);
    HashShadowValue(signature, light.outer);
    HashShadowValue(signature, depthFBO[lightIndex]);
    HashShadowValue(signature, depth_texture[lightIndex]);

    return signature;
}

//--------------------------------------------------------------------------
// Update Grid

void update_grid(glm::vec3 cameraPos) {

    glUseProgram(program_grid);

    glm::mat4 model(1.0f);

    glm::vec3 gridPos(floor(cameraPos.x / STEP) * STEP, 0.0f, floor(cameraPos.z / STEP) * STEP);

    model = glm::translate(glm::mat4(1.0f), gridPos);

    glm::mat4 mvp = projection * view * model;

    glUniformMatrix4fv(shader_uniform.grid_MVP, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(gridVAO);

    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);

    glDrawArrays(GL_LINES, 0, gridVertexCount);

    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

//--------------------------------------------------------------------------
// Update Depth Map

void update_depthmap() {

    //--------------------------------------------------------------------

    const size_t shadowLightCount = std::min({lights.size(), lightObjects.size(), static_cast<size_t>(MAX_LIGHTS)});
    const GLboolean cullingEnabledAtStart = glIsEnabled(GL_CULL_FACE);
    size_t sceneSignature = 0;
    bool sceneSignatureBuilt = false;

    for (size_t i = 0; i < shadowLightCount; ++i) {

        lights[i].pos = lightObjects[i].position;
        lights[i].dir = glm::normalize(lightObjects[i].rotation * glm::vec3(0.0f, -1.0f, 0.0f));

        glm::vec3 forward = glm::normalize(lights[i].dir);

        glm::vec3 up = glm::vec3(0, 1, 0);

        if (abs(glm::dot(forward, up)) > 0.99f)
            up = glm::vec3(0, 0, 1);

        glm::mat4 lightView = glm::lookAt(lights[i].pos, lights[i].pos + forward, up);

        glm::mat4 lightProjection =
            glm::perspective(glm::radians(lights[i].outer * 2.0f), 1.0f, cam.nearPlane, cam.farPlane);

        lightVP[i] = lightProjection * lightView;

        if (lights[i].typeID != 2)
            continue;

        if (!sceneSignatureBuilt) {
            sceneSignature = BuildShadowSceneSignature();
            sceneSignatureBuilt = true;
        }

        const size_t signature = BuildShadowLightSignature(sceneSignature, i);
        if (shadowMapSignatureValid[i] && shadowMapSignatures[i] == signature)
            continue;

        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO[i]);

        glViewport(0, 0, viewport_width, viewport_height);

        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(program_depth);

        for (auto &obj : objects) {

            if (IsEmptyObject(obj) || !obj.visible)
                continue;

            const glm::mat4 model = GetModelMatrix(obj);

            glm::mat4 lightMVP = lightVP[i] * model;

            if (!IsBoundsVisible(lightMVP, obj.min, obj.max))
                continue;

            glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(lightMVP));

            glBindVertexArray(obj.vao);

            const bool disableBackfaceCulling = obj.renderBothSides && cullingEnabledAtStart;
            if (disableBackfaceCulling)
                glDisable(GL_CULL_FACE);

            if (obj.submeshes.empty()) {

                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(obj.indices.size()), GL_UNSIGNED_INT, nullptr);
            } else {

                for (const Submesh &submesh : obj.submeshes) {

                    if (!IsBoundsVisible(lightMVP, submesh.min, submesh.max))
                        continue;

                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(submesh.indexCount), GL_UNSIGNED_INT,
                        reinterpret_cast<const void *>(static_cast<uintptr_t>(submesh.firstIndex) * sizeof(GLuint)));
                }
            }

            if (disableBackfaceCulling)
                glEnable(GL_CULL_FACE);
        }

        shadowMapSignatures[i] = signature;
        shadowMapSignatureValid[i] = true;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //--------------------------------------------------------------------
}

void DrawVideo(const VideoPlayer &video) {

    glDisable(GL_DEPTH_TEST);

    glUseProgram(program_videoplayer);

    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, video.GetTexture());

    glUniform1i(glGetUniformLocation(program_videoplayer, "videoTexture"), 0);

    glBindVertexArray(videoQuad.vao);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(0);

    glEnable(GL_DEPTH_TEST);
}

//--------------------------------------------------------------------------
// Update Objects

void update_objects() {

    currentTexture = 0;

    glUseProgram(program);

    glUniform1i(shader_uniform.program_texture, 0);

    const int numLights = static_cast<int>(std::min({lights.size(), lightObjects.size(), static_cast<size_t>(MAX_LIGHTS)}));

    // Keep the render data in sync with the editor transform before uploading
    // uniforms, even when shadow rendering is disabled.
    for (int i = 0; i < numLights; ++i) {

        lights[i].pos = lightObjects[i].position;

        glm::vec3 direction = lightObjects[i].rotation * glm::vec3(0.0f, -1.0f, 0.0f);
        if (glm::length2(direction) > 0.000001f)
            lights[i].dir = glm::normalize(direction);
    }

    for (int i = 0; i < numLights; ++i) {

        glActiveTexture(GL_TEXTURE2 + static_cast<GLenum>(i));
        glBindTexture(GL_TEXTURE_2D, depth_texture[i]);

        if (!shader_uniform.class_uniforms_initialized) {

            init_class_uniforms();
            shader_uniform.class_uniforms_initialized = true;
        }

        glUniform1i(shader_uniform.depthdata[i], 2 + i);

        glUniform1i(shader_uniform.light_type[i], lights[i].typeID);
        glUniform3fv(shader_uniform.light_pos[i], 1, glm::value_ptr(lights[i].pos));
        glUniform3fv(shader_uniform.light_dir[i], 1, glm::value_ptr(lights[i].dir));
        glUniform3fv(shader_uniform.light_col[i], 1, glm::value_ptr(lights[i].col));
        glUniform3fv(shader_uniform.light_att[i], 1, glm::value_ptr(lights[i].att));
        glUniform1f(shader_uniform.light_ambient[i], lights[i].ambient);
        glUniform1f(shader_uniform.light_bias[i], lights[i].bias);
        glUniform1f(shader_uniform.light_pcf[i], lights[i].pcf);
        glUniform1f(shader_uniform.light_inner[i], lights[i].inner);
        glUniform1f(shader_uniform.light_outer[i], lights[i].outer);
        glUniform1f(shader_uniform.light_falloff[i], lights[i].falloff);
    }

    if (numLights > 0)
        glUniformMatrix4fv(shader_uniform.lightVP, numLights, GL_FALSE, glm::value_ptr(lightVP[0]));

    glUniform1i(shader_uniform.normalMap, 1);

    glUniformMatrix4fv(7, 1, GL_FALSE, glm::value_ptr(view));

    glUniform1i(shader_uniform.numLights, numLights);
    glUniform3fv(shader_uniform.cameraPos, 1, glm::value_ptr(cameraPos));

    glUniform1i(shader_uniform.light_active, st.light_active);
    glUniform1i(shader_uniform.normal_map_active, st.normal_map_active);
    glUniform1i(shader_uniform.shadow_active, st.shadow_active);

    const GLboolean cullingEnabledAtStart = glIsEnabled(GL_CULL_FACE);

    for (Object &obj : objects) {

        if (IsEmptyObject(obj))
            continue;

        if (currentViewMode == view_Mode::PLAY && !obj.visible)
            continue;

        glActiveTexture(GL_TEXTURE0);

        if (!obj.visible) {

            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            glBindTexture(GL_TEXTURE_2D, 0);
            currentTexture = 0;

            glUniform3f(shader_uniform.color, 0.2f, 0.8f, 1.0f);
            glUniform1i(shader_uniform.useTexture, false);
        } else {

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glm::mat4 model = GetModelMatrix(obj);

        obj.model = model;
        obj.mvp = projection * view * model;

        glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(obj.mvp));
        glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(obj.model));
        glUniform1i(shader_uniform.renderBothSides, obj.renderBothSides);

        const bool disableBackfaceCulling = obj.renderBothSides && cullingEnabledAtStart;
        if (disableBackfaceCulling)
            glDisable(GL_CULL_FACE);

        if (obj.tiling) {

            glm::vec3 size = obj.max - obj.min;

            glm::vec3 worldSize = (size / glm::vec3(100)) * obj.scale;

            obj.textureTiling = TextureTiling(obj, worldSize);

            glUniform3fv(shader_uniform.min, 1, glm::value_ptr(obj.min));
            glUniform3fv(shader_uniform.max, 1, glm::value_ptr(obj.max));

            glUniform1i(shader_uniform.mapping, static_cast<int>(obj.mapping));

            glUniform3fv(shader_uniform.worldSize, 1, glm::value_ptr(worldSize));

            glUniform2f(shader_uniform.textureTiling, obj.textureTiling.x, obj.textureTiling.y);
        }

        glUniform1i(shader_uniform.tiling, obj.tiling);

        glBindVertexArray(obj.vao);

        if (st.wireframe_active)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        constexpr GLint AlbedoTextureUnit = 0;
        constexpr GLint NormalTextureUnit = 1;
        constexpr GLint RoughnessTextureUnit = 2 + MAX_LIGHTS;
        constexpr GLint MetallicTextureUnit = 3 + MAX_LIGHTS;
        constexpr GLint EmissionTextureUnit = 4 + MAX_LIGHTS;
        constexpr GLint OpacityTextureUnit = 5 + MAX_LIGHTS;

        auto bindMaterial = [&](const Material &material) {
            const GLuint albedo = obj.obj_texture != 0 ? obj.obj_texture : material.albedoTexture;
            const GLuint normal = obj.normalTexture != 0 ? obj.normalTexture : material.normalTexture;

            glActiveTexture(GL_TEXTURE0 + AlbedoTextureUnit); glBindTexture(GL_TEXTURE_2D, albedo);
            glActiveTexture(GL_TEXTURE0 + NormalTextureUnit); glBindTexture(GL_TEXTURE_2D, normal);
            glActiveTexture(GL_TEXTURE0 + RoughnessTextureUnit); glBindTexture(GL_TEXTURE_2D, material.roughnessTexture);
            glActiveTexture(GL_TEXTURE0 + MetallicTextureUnit); glBindTexture(GL_TEXTURE_2D, material.metallicTexture);
            glActiveTexture(GL_TEXTURE0 + EmissionTextureUnit); glBindTexture(GL_TEXTURE_2D, material.emissionTexture);
            glActiveTexture(GL_TEXTURE0 + OpacityTextureUnit); glBindTexture(GL_TEXTURE_2D, material.opacityTexture);

            glUniform1i(shader_uniform.program_texture, AlbedoTextureUnit);
            glUniform1i(shader_uniform.normalMap, NormalTextureUnit);
            glUniform1i(shader_uniform.roughnessMap, RoughnessTextureUnit);
            glUniform1i(shader_uniform.metallicMap, MetallicTextureUnit);
            glUniform1i(shader_uniform.emissionMap, EmissionTextureUnit);
            glUniform1i(shader_uniform.opacityMap, OpacityTextureUnit);
            glUniform1i(shader_uniform.useTexture, albedo != 0);
            glUniform1i(shader_uniform.hasNormalMap, normal != 0);
            glUniform1i(shader_uniform.hasRoughnessMap, material.roughnessTexture != 0);
            glUniform1i(shader_uniform.hasMetallicMap, material.metallicTexture != 0);
            glUniform1i(shader_uniform.hasEmissionMap, material.emissionTexture != 0);
            glUniform1i(shader_uniform.hasOpacityMap, material.opacityTexture != 0);
            glUniform3fv(shader_uniform.color, 1, glm::value_ptr(material.diffuse));
            glUniform1f(shader_uniform.materialRoughness, material.roughness);
            glUniform1f(shader_uniform.materialMetallic, material.metallic);
            glUniform3fv(shader_uniform.materialEmission, 1, glm::value_ptr(material.emission));
            glUniform1f(shader_uniform.materialOpacity, material.opacity);
        };

        if (!obj.hasMaterial || obj.submeshes.empty()) {

            Material fallback{};

            fallback.normalTexture = obj.normalTexture;
            fallback.shininess = obj.shininess;
            fallback.roughness = glm::clamp(std::sqrt(2.0f / (obj.shininess + 2.0f)), 0.04f, 1.0f);

            bindMaterial(fallback);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(obj.indices.size()), GL_UNSIGNED_INT, nullptr);

        } else {

            for (const Submesh &submesh : obj.submeshes) {

                if (!IsBoundsVisible(obj.mvp, submesh.min, submesh.max))
                    continue;

                bindMaterial(submesh.material);

                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(submesh.indexCount), GL_UNSIGNED_INT,
                    reinterpret_cast<const void *>(static_cast<uintptr_t>(submesh.firstIndex) * sizeof(GLuint)));
            }
        }

        if (disableBackfaceCulling)
            glEnable(GL_CULL_FACE);

        glActiveTexture(GL_TEXTURE0);
    }

    if (!st.wireframe_active)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    //-----------------------------------------------------------------------------------------------
    // draw the lights and the normals, tangents and bitangents

    glUseProgram(program_line);

    GLint colorLoc = shader_uniform.program_line_color;

    if (normals_appear) {

        for (auto &obj : objects) {

            if (IsEmptyObject(obj))
                continue;

            glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(obj.mvp));

            glBindVertexArray(obj.normalTBVAO);
            glUniform3f(colorLoc, 0.0f, 0.0f, 1.0f); // blue
            glDrawArrays(GL_LINES, 0, (GLsizei)obj.normalLinesTBN.size());

            glBindVertexArray(obj.tangentVAO);
            glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f); // red
            glDrawArrays(GL_LINES, 0, (GLsizei)obj.tangentLines.size());

            glBindVertexArray(obj.bitangentVAO);
            glUniform3f(colorLoc, 0.0f, 1.0f, 0.0f); // green
            glDrawArrays(GL_LINES, 0, (GLsizei)obj.bitangentLines.size());
        }
    }

    const size_t drawableLightCount = std::min(lights.size(), lightObjects.size());

    for (size_t i = 0; i < drawableLightCount; ++i) {

        if (currentViewMode == view_Mode::PLAY && !lightObjects[i].visible)
            continue;

        if (!lightObjects[i].visible) {

            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            glUniform3f(colorLoc, 0.2f, 0.8f, 1.0f); // cyan
        } else {

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            glUniform3f(colorLoc, 1.f, 1.f, 1.f); // white
        }

        glm::mat4 model(1.0f);

        model = glm::translate(model, lightObjects[i].position);
        model *= glm::toMat4(lightObjects[i].rotation);
        model = glm::scale(model, lightObjects[i].scale);

        glm::mat4 mvp = projection * view * model;

        glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(mvp));
        glBindVertexArray(lightObjects[i].vao);

        if (st.wireframe_active)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glDrawElements(GL_TRIANGLES, (GLsizei)lightObjects[i].indices.size(), GL_UNSIGNED_INT, 0);
    }

    if (!st.wireframe_active)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

//--------------------------------------------------------------------------
// Update Skybox

void update_skybox() {

    glUseProgram(program_skybox);

    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

    glm::mat4 skyView = glm::mat4(glm::mat3(view));

    glUniformMatrix4fv(shader_uniform.skybox_view, 1, GL_FALSE, glm::value_ptr(skyView));

    glUniformMatrix4fv(shader_uniform.skybox_projection, 1, GL_FALSE, glm::value_ptr(projection));

    glUniform1f(shader_uniform.is_hdr, is_hdr);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glUniform1i(shader_uniform.program_skybox_skybox, 1);

    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
}

//--------------------------------------------------------------------------
// Update Physics

void PhysicsUpdate(float dt) {

    for (Object &obj : objects) {

        if (!obj.hasPhysics)
            continue;

        if (obj.physics.isStatic)
            continue;

        Physics &p = obj.physics;

        if (p.inverseMass > 0.0f && p.useGravity) {

            float mass = 1.0f / p.inverseMass;
            p.force += p.gravity * mass;
        }

        p.acceleration = p.force * p.inverseMass;

        p.velocity += p.acceleration * dt;

        float dragFactor = glm::clamp(1.0f - p.drag * dt, 0.0f, 1.0f);
        p.velocity.x *= dragFactor;
        p.velocity.z *= dragFactor;

        if (p.maxHorizontalSpeed > 0.0f) {

            glm::vec2 horizontalVelocity(p.velocity.x, p.velocity.z);
            float horizontalSpeed = glm::length(horizontalVelocity);

            if (horizontalSpeed > p.maxHorizontalSpeed) {

                glm::vec2 limitedVelocity = horizontalVelocity * (p.maxHorizontalSpeed / horizontalSpeed);

                p.velocity.x = limitedVelocity.x;
                p.velocity.z = limitedVelocity.y;
            }
        }

        obj.position += p.velocity * dt;

        p.force = glm::vec3(0.0f);
    }
}

//--------------------------------------------------------------------------
// animations

void UpdateAnimations(float dt) {

    for (auto &player : animationPlayers) {

        UpdateAnimationPlayer(player, dt);
    }
}

void UpdateAnimationPreview(float dt) {

    UpdateAnimationPlayer(animationPreviewPlayer, dt);
}

void UpdateAnimation(float dt) {

    UpdateAnimations(dt);

    UpdateAnimationPreview(dt);
}

void CollisionUpdate() {

    PlayerActor::BeginCollisionStep();

    for (size_t i = 0; i < objects.size(); i++) {

        for (size_t j = i + 1; j < objects.size(); j++) {

            Contact contact{};

            Object &a = objects[i];
            Object &b = objects[j];

            if (&a == &b)
                continue;

            if (!CanCollide(a, b))
                continue;

            if (a.collider.type == ColliderType::None || b.collider.type == ColliderType::None)
                continue;

            if (!a.hasCollider)
                continue;

            if (!b.hasCollider)
                continue;

            if (a.hasPhysics && b.hasPhysics && a.physics.isStatic && b.physics.isStatic)
                continue;

            Object colliderA = GetColliderProxy(a);
            Object colliderB = GetColliderProxy(b);

            switch (a.collider.type) {

            case ColliderType::Plane:

                switch (b.collider.type) {

                case ColliderType::Sphere:
                    contact = Sphere_vs_Plane(colliderB, colliderA);
                    contact.normal *= -1.0f;
                    break;

                case ColliderType::AABB:
                    contact = AABB_vs_Plane(colliderB, colliderA);
                    contact.normal *= -1.0f;
                    break;
                }

                break;

            case ColliderType::Sphere:

                switch (b.collider.type) {

                case ColliderType::Plane:
                    contact = Sphere_vs_Plane(colliderA, colliderB);
                    break;

                case ColliderType::Sphere:
                    contact = Sphere_vs_Sphere(colliderA, colliderB);
                    break;

                case ColliderType::AABB:
                    contact = Sphere_vs_AABB(colliderA, colliderB);
                    break;
                }

                break;

            case ColliderType::AABB:

                switch (b.collider.type) {

                case ColliderType::Plane:
                    contact = AABB_vs_Plane(colliderA, colliderB);
                    break;

                case ColliderType::Sphere:
                    contact = Sphere_vs_AABB(colliderB, colliderA);
                    contact.normal *= -1.0f;
                    break;

                case ColliderType::AABB:
                    contact = AABB_vs_AABB(colliderA, colliderB);
                    break;
                }

                break;

            default:
                break;
            }

            if (contact.hit) {

                PositionalCorrection(a, b, contact);
                ResolveVelocity(a, b, contact);

                PlayerActor::RegisterCollision(a, b, contact);

                //eventManager.Throw(std::make_unique<PlayAnimationEvent>(AnimationRefs::Anim_cubeMoves));
            }
        }
    }
}



//--------------------------------------------------------------------------
// Update Imgui and guizmo

void update_imgui_guizmo() {

    Editor();
}
