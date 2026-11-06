#include "inspector.h"

static glm::quat LightRotationFromDirection(glm::vec3 direction) {

    const glm::vec3 localDirection(0.0f, -1.0f, 0.0f);
    direction = glm::normalize(direction);

    if (glm::dot(localDirection, direction) < -0.9999f)
        return glm::angleAxis(glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));

    return glm::normalize(glm::rotation(localDirection, direction));
}

void Inspector(){

    //----------------------------------------------------
    // lights

    ImGui::Begin("Lights");

    if (ImGui::Button("+ Point")) {

        if (lights.size() < MAX_LIGHTS) {

            CreateLight("POINT");
            selectedObject = (int)(objects.size() + lightObjects.size() - 1);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("+ Spot")) {

        if (lights.size() < MAX_LIGHTS) {

            CreateLight("SPOT");
            selectedObject = (int)(objects.size() + lightObjects.size() - 1);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("+ Directional")) {

        if (lights.size() < MAX_LIGHTS) {

            CreateLight("DIR");
            selectedObject = (int)(objects.size() + lightObjects.size() - 1);
        }
    }

    ImGui::End();

    //----------------------------------------------------
    // Inspector

    static const char *mappings[] = {"UV", "Planar", "Cube", "Cylinder", "Sphere"};

    static const char *planarCoords[] = {"XZ", "XY", "YZ"};

    static const char *cylinderAxes[] = {"X", "Y", "Z"};

    ImGui::Begin("Inspector");

    if (selectedObject != -1 && selectedObject < (int)objects.size()) {

        Object *obj = GetSelectedObject();
        const bool isEmpty = obj && IsEmptyObject(*obj);

        if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen)) {

            ImGui::PushID(reinterpret_cast<void *>(static_cast<uintptr_t>(obj->id)));

            static char name[256];
            strncpy_s(name, objects[selectedObject].name.c_str(), sizeof(name));
            name[255] = '\0';

            if (ImGui::InputText("Name", name, sizeof(name))) {

                Object *obj = GetSelectedObject();

                if (obj && obj == &objects[selectedObject])
                    objects[selectedObject].name = name;
            }

            ImGui::PopID();

            ImGui::DragFloat3("Position", &obj->position.x, 0.1f);

            HandleInspectorUndo(*obj);
            glm::vec3 euler = glm::degrees(glm::eulerAngles(obj->rotation));

            if (ImGui::DragFloat3("Rotation", &euler.x, 1.0f)) {
            
                obj->rotation = glm::quat(glm::radians(euler));

                HandleInspectorUndo(*obj);
            }

            ImGui::DragFloat3("Scale", &obj->scale.x, 0.1f);

            HandleInspectorUndo(*obj);
            ImGui::Checkbox("Visible", &obj->visible);

            HandleInspectorUndo(*obj);
            ImGui::Checkbox("Render both sides", &obj->renderBothSides);

            HandleInspectorUndo(*obj);
        }

        //------------------------------------------------------------

        if (obj->hasPhysics) {

            ImGui::Separator();

            if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {

                Physics &p = obj->physics;

                HandleInspectorUndo(*obj);
                ImGui::DragFloat3("Gravity", &obj->physics.gravity.x, 1.f);

                ImGui::Separator();

                ImGui::DragFloat3("Velocity", &obj->physics.velocity.x, 0.1f);

                HandleInspectorUndo(*obj);
                ImGui::DragFloat3("Acceleration", &obj->physics.acceleration.x, 0.1f);

                HandleInspectorUndo(*obj);
                ImGui::DragFloat("Drag", &obj->physics.drag, 0.1f);

                HandleInspectorUndo(*obj);
                ImGui::DragFloat("Max horizontal speed (0 = default)", &obj->physics.maxHorizontalSpeed, 10.0f, 0.0f, 100000.0f);

                HandleInspectorUndo(*obj);
                ImGui::DragFloat("Restitution", &obj->physics.restitution, 0.1f);

                HandleInspectorUndo(*obj);
                ImGui::DragFloat3("Force", &obj->physics.force.x, 0.1f);

                HandleInspectorUndo(*obj);
                float mass = (p.inverseMass == 0.0f) ? 0.0f : 1.0f / p.inverseMass;

                if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.0f, 1000.0f)) {

                    if (mass == 0.0f)
                        p.inverseMass = 0.0f;
                    else
                        p.inverseMass = 1.0f / mass;

                    HandleInspectorUndo(*obj);
                }

                ImGui::Checkbox("Use Gravity", &obj->physics.useGravity);

                HandleInspectorUndo(*obj);
                static float storedInverseMass = 1.f;
                static bool storedGravityUse = true;

                if (ImGui::Checkbox("Is Static", &obj->physics.isStatic)) {

                    if (obj->physics.isStatic) {

                        storedInverseMass = p.inverseMass;
                        storedGravityUse = obj->physics.useGravity;
                
                        obj->physics.useGravity = false;
                        p.inverseMass = 0.f;    

                    }else{
                    
                        obj->physics.useGravity = storedGravityUse;
                        p.inverseMass = storedInverseMass;  
                    }

                    HandleInspectorUndo(*obj);
                }


                if (ImGui::Button("Reset Velocities")) {

                    obj->physics.velocity = glm::vec3(0.0f);
                    obj->physics.acceleration = glm::vec3(0.0f);
                }
            }
        }

        //------------------------------------------------------------

        static const char *colliderTypes[] = {"None", "Plane", "Sphere", "AABB"};

        if (obj->hasCollider) {

            ImGui::Separator();

            if (ImGui::CollapsingHeader("Collision", ImGuiTreeNodeFlags_DefaultOpen)) {

                ImGui::Separator();

                ImGui::DragFloat3("Collider Offset", &obj->colliderTransform.offset.x, 0.1f);

                HandleInspectorUndo(*obj);
                glm::vec3 colliderRotation = glm::degrees(glm::eulerAngles(obj->colliderTransform.rotation));

                if (ImGui::DragFloat3("Collider Rotation", &colliderRotation.x, 1.0f)) {
                
                    obj->colliderTransform.rotation = glm::quat(glm::radians(colliderRotation));

                    HandleInspectorUndo(*obj);
                }

                ImGui::DragFloat3("Collider Size", &obj->colliderTransform.size.x, 0.01f, 0.01f, 1000.0f);

                HandleInspectorUndo(*obj);
                bool editCollider = editingColliderID == obj->id;

                if (ImGui::Checkbox("Edit Collider With Gizmo", &editCollider))
                    editingColliderID = editCollider ? obj->id : 0;

                ImGui::Separator();

                int currentType = (int)obj->collider.type;

                if (ImGui::Combo("Type", &currentType, colliderTypes, IM_ARRAYSIZE(colliderTypes))) {

                    HandleInspectorUndo(*obj);

                    obj->collider.type = (ColliderType)currentType;
                }

                ImGui::Separator();

                switch (obj->collider.type) {

                case ColliderType::None: {

                    ImGui::TextDisabled("No collider.");
                    break;
                }

                case ColliderType::Plane: {

                    ImGui::Text("Plane Collider");

                    ImGui::Separator();

                    ImGui::Checkbox("Trigger", &obj->collider.isTrigger);

                    ImGui::Separator();

                    if (ImGui::BeginCombo("Layers", "Select layers")) {

                        bool world = (obj->collider.layer & CollisionLayer::World) != 0;

                        bool player = (obj->collider.layer & CollisionLayer::Player) != 0;

                        bool enemy = (obj->collider.layer & CollisionLayer::Enemy) != 0;

                        bool bullet = (obj->collider.layer & CollisionLayer::Bullets) != 0;

                        if (ImGui::Checkbox("World", &world)) {

                            if (world)
                                obj->collider.layer |= CollisionLayer::World;
                            else
                                obj->collider.layer &= ~CollisionLayer::World;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Player", &player)) {

                            if (player)
                                obj->collider.layer |= CollisionLayer::Player;
                            else
                                obj->collider.layer &= ~CollisionLayer::Player;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Enemy", &enemy)) {

                            if (enemy)
                                obj->collider.layer |= CollisionLayer::Enemy;
                            else
                                obj->collider.layer &= ~CollisionLayer::Enemy;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Bullet", &bullet)) {

                            if (bullet)
                                obj->collider.layer |= CollisionLayer::Bullets;
                            else
                                obj->collider.layer &= ~CollisionLayer::Bullets;

                            HandleInspectorUndo(*obj);
                        }

                        ImGui::EndCombo();
                    }

                    ImGui::Separator();

                    if (ImGui::BeginCombo("Mask", "Collides With")) {

                        bool world = (obj->collider.mask & CollisionLayer::World) != 0;

                        bool player = (obj->collider.mask & CollisionLayer::Player) != 0;

                        bool enemy = (obj->collider.mask & CollisionLayer::Enemy) != 0;

                        bool bullet = (obj->collider.mask & CollisionLayer::Bullets) != 0;


                        if (ImGui::Checkbox("World", &world)) {

                            if (world)
                                obj->collider.mask |= CollisionLayer::World;
                            else
                                obj->collider.mask &= ~CollisionLayer::World;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Player", &player)) {

                            if (player)
                                obj->collider.mask |= CollisionLayer::Player;
                            else
                                obj->collider.mask &= ~CollisionLayer::Player;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Enemy", &enemy)) {

                            if (enemy)
                                obj->collider.mask |= CollisionLayer::Enemy;
                            else
                                obj->collider.mask &= ~CollisionLayer::Enemy;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Bullet", &bullet)) {

                            if (bullet)
                                obj->collider.mask |= CollisionLayer::Bullets;
                            else
                                obj->collider.mask &= ~CollisionLayer::Bullets;

                            HandleInspectorUndo(*obj);
                        }

                        ImGui::EndCombo();
                    }

                    ImGui::Separator();

                    break;
                }

                case ColliderType::Sphere: {

                    ImGui::Text("Sphere Collider");

                    ImGui::Separator();

                    ImGui::DragFloat("Radius", &obj->collider.radius, 0.01f, 0.0f);

                    ImGui::Separator();

                    ImGui::Checkbox("Trigger", &obj->collider.isTrigger);

                    ImGui::Separator();

                    if (ImGui::BeginCombo("Layers", "Select layers")) {

                        bool world = (obj->collider.layer & CollisionLayer::World) != 0;

                        bool player = (obj->collider.layer & CollisionLayer::Player) != 0;

                        bool enemy = (obj->collider.layer & CollisionLayer::Enemy) != 0;

                        bool bullet = (obj->collider.layer & CollisionLayer::Bullets) != 0;


                        if (ImGui::Checkbox("World", &world)) {

                            if (world)
                                obj->collider.layer |= CollisionLayer::World;
                            else
                                obj->collider.layer &= ~CollisionLayer::World;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Player", &player)) {

                            if (player)
                                obj->collider.layer |= CollisionLayer::Player;
                            else
                                obj->collider.layer &= ~CollisionLayer::Player;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Enemy", &enemy)) {

                            if (enemy)
                                obj->collider.layer |= CollisionLayer::Enemy;
                            else
                                obj->collider.layer &= ~CollisionLayer::Enemy;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Bullet", &bullet)) {

                            if (bullet)
                                obj->collider.layer |= CollisionLayer::Bullets;
                            else
                                obj->collider.layer &= ~CollisionLayer::Bullets;

                            HandleInspectorUndo(*obj);
                        }

                        ImGui::EndCombo();
                    }

                    ImGui::Separator();

                    if (ImGui::BeginCombo("Mask", "Collides With")) {

                        bool world = (obj->collider.mask & CollisionLayer::World) != 0;

                        bool player = (obj->collider.mask & CollisionLayer::Player) != 0;

                        bool enemy = (obj->collider.mask & CollisionLayer::Enemy) != 0;

                        bool bullet = (obj->collider.mask & CollisionLayer::Bullets) != 0;


                        if (ImGui::Checkbox("World", &world)) {

                            if (world)
                                obj->collider.mask |= CollisionLayer::World;
                            else
                                obj->collider.mask &= ~CollisionLayer::World;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Player", &player)) {

                            if (player)
                                obj->collider.mask |= CollisionLayer::Player;
                            else
                                obj->collider.mask &= ~CollisionLayer::Player;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Enemy", &enemy)) {

                            if (enemy)
                                obj->collider.mask |= CollisionLayer::Enemy;
                            else
                                obj->collider.mask &= ~CollisionLayer::Enemy;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Bullet", &bullet)) {

                            if (bullet)
                                obj->collider.mask |= CollisionLayer::Bullets;
                            else
                                obj->collider.mask &= ~CollisionLayer::Bullets;

                            HandleInspectorUndo(*obj);
                        }

                        ImGui::EndCombo();
                    }

                    ImGui::Separator();

                    break;
                }

                case ColliderType::AABB: {

                    ImGui::Text("AABB Collider");

                    ImGui::Separator();

                    // ImGui::DragFloat3("Half Extents", &obj->collider.halfExtents.x,
                    // 0.01f);

                    ImGui::Separator();

                    ImGui::Checkbox("Trigger", &obj->collider.isTrigger);

                    ImGui::Separator();

                    if (ImGui::BeginCombo("Layers", "Select layers")) {

                        bool world = (obj->collider.layer & CollisionLayer::World) != 0;

                        bool player = (obj->collider.layer & CollisionLayer::Player) != 0;

                        bool enemy = (obj->collider.layer & CollisionLayer::Enemy) != 0;

                        bool bullet = (obj->collider.layer & CollisionLayer::Bullets) != 0;

                        if (ImGui::Checkbox("World", &world)) {

                            if (world)
                                obj->collider.layer |= CollisionLayer::World;
                            else
                                obj->collider.layer &= ~CollisionLayer::World;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Player", &player)) {

                            if (player)
                                obj->collider.layer |= CollisionLayer::Player;
                            else
                                obj->collider.layer &= ~CollisionLayer::Player;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Enemy", &enemy)) {

                            if (enemy)
                                obj->collider.layer |= CollisionLayer::Enemy;
                            else
                                obj->collider.layer &= ~CollisionLayer::Enemy;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Bullet", &bullet)) {

                            if (bullet)
                                obj->collider.layer |= CollisionLayer::Bullets;
                            else
                                obj->collider.layer &= ~CollisionLayer::Bullets;

                            HandleInspectorUndo(*obj);
                        }

                        ImGui::EndCombo();
                    }

                    ImGui::Separator();

                    if (ImGui::BeginCombo("Mask", "Collides With")) {

                        bool world = (obj->collider.mask & CollisionLayer::World) != 0;

                        bool player = (obj->collider.mask & CollisionLayer::Player) != 0;

                        bool enemy = (obj->collider.mask & CollisionLayer::Enemy) != 0;

                        bool bullet = (obj->collider.mask & CollisionLayer::Bullets) != 0;


                        if (ImGui::Checkbox("World", &world)) {

                            if (world)
                                obj->collider.mask |= CollisionLayer::World;
                            else
                                obj->collider.mask &= ~CollisionLayer::World;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Player", &player)) {

                            if (player)
                                obj->collider.mask |= CollisionLayer::Player;
                            else
                                obj->collider.mask &= ~CollisionLayer::Player;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Enemy", &enemy)) {

                            if (enemy)
                                obj->collider.mask |= CollisionLayer::Enemy;
                            else
                                obj->collider.mask &= ~CollisionLayer::Enemy;

                            HandleInspectorUndo(*obj);
                        }

                        if (ImGui::Checkbox("Bullet", &bullet)) {

                            if (bullet)
                                obj->collider.mask |= CollisionLayer::Bullets;
                            else
                                obj->collider.mask &= ~CollisionLayer::Bullets;

                            HandleInspectorUndo(*obj);
                        }

                        ImGui::EndCombo();
                    }

                    ImGui::Separator();

                    break;
                }
                }
            }
        }

        if (!isEmpty && obj->hasMaterial) {

            ImGui::Separator();

            if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {

                ImGui::DragFloat("Shininess", &obj->shininess, 1.0f, 1.0f, 256.0f);

                HandleInspectorUndo(*obj);
                ImGui::Text("Texture:");

                ImGui::TextWrapped("%s", obj->tex.c_str());

                ImGui::Image((ImTextureID)(intptr_t)obj->obj_texture, ImVec2(80, 80), ImVec2(0, 1), ImVec2(1, 0));

                ImGui::Button("Drop texture here##Texture", ImVec2(80, 80));

                if (ImGui::BeginDragDropTarget()) {

                    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("IMAGE_FILE")) {

                        HandleInspectorUndo(*obj);

                        const char *file = (const char *)payload->Data;

                        objects[selectedObject].obj_texture = TextureManager::Get(file, false);

                        objects[selectedObject].tex = file;
                    }

                    ImGui::EndDragDropTarget();
                }

                ImGui::Checkbox("Tiling", &obj->tiling);

                HandleInspectorUndo(*obj);
                //------------------------------------------------------------

                ImGui::Text("Normal Map:");

                ImGui::TextWrapped("%s", obj->norm_map.c_str());

                ImGui::Image((ImTextureID)(intptr_t)obj->normalTexture, ImVec2(80, 80), ImVec2(0, 1), ImVec2(1, 0));

                ImGui::Button("Drop texture here##NormalMap", ImVec2(80, 80));

                if (ImGui::BeginDragDropTarget()) {

                    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("IMAGE_FILE")) {

                        HandleInspectorUndo(*obj);

                        const char *file = (const char *)payload->Data;

                        objects[selectedObject].normalTexture = TextureManager::Get(file);

                        objects[selectedObject].norm_map = file;
                        objects[selectedObject].hasNormalMap = true;
                    }

                    ImGui::EndDragDropTarget();
                }
            }
        }

        ImGui::Separator();

        if (isEmpty) {

            ImGui::Separator();
            ImGui::TextUnformatted("Empty object: transform-only hierarchy parent.");
            ImGui::TextUnformatted("Drag scene objects onto it in Hierarchy to parent them.");
        }

        if (!isEmpty && ImGui::CollapsingHeader("Mesh"), ImGuiTreeNodeFlags_DefaultOpen) {

            ImGui::TextWrapped("%s", obj->mesh.c_str());

            ImGui::Text("Mesh:");

            ImGui::Button("Drop model here##Mesh", ImVec2(200, 80));

            if (ImGui::BeginDragDropTarget()) {

                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("MODEL_FILE")) {

                    HandleInspectorUndo(*obj);

                    DestroyBuffers(*obj);

                    obj->vertices.clear();
                    obj->indices.clear();

                    obj->normalLines.clear();

                    obj->tangentLines.clear();
                    obj->bitangentLines.clear();
                    obj->normalLinesTBN.clear();

                    obj->original_vertices.clear();
                    obj->original_indices.clear();

                    ReplaceMesh(objects[selectedObject], (const char *)payload->Data);
                }

                ImGui::EndDragDropTarget();
            }

            ImGui::TextWrapped("%s", obj->mesh.c_str());

            ImGui::Text("Vertices: %d", (int)obj->vertices.size());
            ImGui::Text("Triangles: %d", (int)obj->indices.size() / 3);
        }

        ImGui::Separator();

        if (!isEmpty && ImGui::CollapsingHeader("Bounds"), ImGuiTreeNodeFlags_DefaultOpen) {

            ImGui::DragFloat3("Min", &obj->min.x, 0.1f);

            HandleInspectorUndo(*obj);
            ImGui::DragFloat3("Max", &obj->max.x, 0.1f);

            HandleInspectorUndo(*obj);
        }

        if (animationWindow) {

            if (obj)
                DrawObjectAnimationButtons(*obj);
        }

        if (!isEmpty && obj->hasMapping) {

            ImGui::Separator();

            int current = (int)obj->mapping;

            if (ImGui::CollapsingHeader("Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {

                if (ImGui::Combo("Mapping##M", &current, mappings, IM_ARRAYSIZE(mappings))) {

                    HandleInspectorUndo(*obj);

                    obj->mapping = (TextureMapping)current;
                    UpdateMapping(*obj);
                }

                //------------------------------------------------------------
                // Planar

                if (obj->mapping == TextureMapping::Planar) {

                    int coord = (int)obj->coordType;

                    if (ImGui::Combo("Plane", &coord, planarCoords, IM_ARRAYSIZE(planarCoords))) {

                        HandleInspectorUndo(*obj);

                        obj->coordType = (coord_types)coord;
                        UpdateMapping(*obj);
                    }
                }

                //------------------------------------------------------------
                // Cubic

                if (obj->mapping == TextureMapping::Cubic) {

                    if (ImGui::Button("Regenerate Cube UV")) {

                        HandleInspectorUndo(*obj);

                        UpdateMapping(*obj);
                    }
                }

                //------------------------------------------------------------
                // Cylindrical

                if (obj->mapping == TextureMapping::Cylindrical) {

                    int axis = (int)obj->cyaxisType;

                    if (ImGui::Combo("Axis", &axis, cylinderAxes, IM_ARRAYSIZE(cylinderAxes))) {

                        HandleInspectorUndo(*obj);

                        obj->cyaxisType = (cylinder_axis)axis;
                        UpdateMapping(*obj);
                    }
                }

                //------------------------------------------------------------
                // Spherical

                if (obj->mapping == TextureMapping::Spherical) {

                    if (ImGui::Button("Regenerate Sphere UV")) {

                        HandleInspectorUndo(*obj);

                        UpdateMapping(*obj);
                    }
                }

                //------------------------------------------------------------
                // UV

                if (obj->mapping == TextureMapping::UV) {

                    if (ImGui::Button("Restore Original UV")) {

                        HandleInspectorUndo(*obj);

                        UpdateMapping(*obj);
                    }
                }
            }
        }

        ImGui::Separator();

        if (ImGui::BeginCombo("Add Module", "Modules")) {

            bool collision = obj->hasCollider;

            if (ImGui::Checkbox("Collision", &collision)) {

                obj->hasCollider = collision;

                HandleInspectorUndo(*obj);
            }

            bool physics = obj->hasPhysics;

            if (ImGui::Checkbox("Physics", &physics)) {

                obj->hasPhysics = physics;

                HandleInspectorUndo(*obj);
            }

            if (!isEmpty) {
                bool material = obj->hasMaterial;
                bool mapping = obj->hasMapping;
                bool animation = obj->hasAnimation;

                if (ImGui::Checkbox("Material", &material)) {

                    obj->hasMaterial = material;

                    HandleInspectorUndo(*obj);
                }

                if (ImGui::Checkbox("Mapping", &mapping)) {

                    obj->hasMapping = mapping;

                    HandleInspectorUndo(*obj);
                }

                if (ImGui::Checkbox("Animation", &animation)) {

                    obj->hasAnimation = animation;

                    HandleInspectorUndo(*obj);
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Separator();

        if (ImGui::Button("Delete Object")) {

            DeleteObjectHierarchy(objects[selectedObject].id);

            selectedObject = -1;
        }
    } else {

        Light *light = GetSelectedLight();
        Object *obj = GetSelectedObject();

        if (light) {

            const int lightIndex = selectedObject - static_cast<int>(objects.size());
            Object &lightObject = lightObjects[lightIndex];

            ImGui::PushID(reinterpret_cast<void *>(static_cast<uintptr_t>(lightObject.id)));

            if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen)) {

                static EntityID nameBufferLightID = 0;
                static char lightName[256] = {};

                if (nameBufferLightID != lightObject.id) {
                    strncpy_s(lightName, lightObject.name.c_str(), sizeof(lightName) - 1);
                    lightName[sizeof(lightName) - 1] = '\0';
                    nameBufferLightID = lightObject.id;
                }

                if (ImGui::InputText("Name", lightName, sizeof(lightName)))
                    lightObject.name = lightName;

                static const char *types[] = {"Point", "Directional", "Spot"};
                int currentType = static_cast<int>(light->typeID);

                if (ImGui::Combo("Type", &currentType, types, IM_ARRAYSIZE(types))) {

                    light->undoStack.push_back(CaptureLightState(*light));
                    light->redoStack.clear();
                    lightObject.undoStack.push_back(CaptureState(lightObject));
                    lightObject.redoStack.clear();

                    light->typeID = static_cast<GLuint>(currentType);
                    light->type = currentType == 0 ? "POINT" : currentType == 1 ? "DIR" : "SPOT";
                }

                ImGui::ColorEdit3("Color", &light->col.x);
                HandleInspectorUndo(*light, lightObject);

                ImGui::DragFloat3("Position", &lightObject.position.x, 1.0f, 0.0f, 0.0f, "%.1f");
                HandleInspectorUndo(*light, lightObject);

                if (light->typeID != 0) {

                    glm::vec3 direction = glm::normalize(lightObject.rotation * glm::vec3(0.0f, -1.0f, 0.0f));

                    if (ImGui::DragFloat3("Direction", &direction.x, 0.01f, -1.0f, 1.0f, "%.3f") &&
                        glm::length2(direction) > 0.000001f) {

                        direction = glm::normalize(direction);
                        lightObject.rotation = LightRotationFromDirection(direction);
                        light->dir = direction;
                    }

                    HandleInspectorUndo(*light, lightObject);
                }

                ImGui::DragFloat("Ambient", &light->ambient, 0.01f, 0.0f, 1.0f, "%.2f");
                HandleInspectorUndo(*light, lightObject);

                ImGui::Checkbox("Show gizmo", &lightObject.visible);
                HandleInspectorUndo(*light, lightObject);
            }

            if (light->typeID != 1 && ImGui::CollapsingHeader("Range", ImGuiTreeNodeFlags_DefaultOpen)) {

                float range = GetLightRange(*light);
                const float dragSpeed = std::max(range * 0.005f, 0.1f);

                if (ImGui::DragFloat("Distance", &range, dragSpeed, 1.0f, 10000.0f, "%.1f units"))
                    SetLightRange(*light, range);

                HandleInspectorUndo(*light, lightObject);
            }

            if (light->typeID == 2 && ImGui::CollapsingHeader("Spot cone", ImGuiTreeNodeFlags_DefaultOpen)) {

                const float maxSpotAngle = glm::clamp(light->outer, 0.1f, 89.0f);
                const float minOuterAngle = glm::clamp(light->inner, 0.1f, 89.0f);

                ImGui::SliderFloat("Inner angle", &light->inner, 0.0f, maxSpotAngle, "%.1f deg");
                HandleInspectorUndo(*light, lightObject);

                ImGui::SliderFloat("Outer angle", &light->outer, minOuterAngle, 89.0f, "%.1f deg");
                HandleInspectorUndo(*light, lightObject);

                ImGui::SliderFloat("Falloff", &light->falloff, 0.1f, 16.0f, "%.2f");
                HandleInspectorUndo(*light, lightObject);
            }

            if (ImGui::CollapsingHeader("Advanced attenuation")) {

                ImGui::DragFloat("Constant", &light->att.x, 0.001f, 0.001f, 10.0f, "%.4f");
                HandleInspectorUndo(*light, lightObject);

                ImGui::DragFloat("Linear", &light->att.y, 0.0001f, 0.0f, 1.0f, "%.5f");
                HandleInspectorUndo(*light, lightObject);

                ImGui::DragFloat("Quadratic", &light->att.z, 0.00001f, 0.0f, 1.0f, "%.7f");
                HandleInspectorUndo(*light, lightObject);
            }

            if (light->typeID == 2 && ImGui::CollapsingHeader("Shadows")) {

                ImGui::DragFloat("Bias", &light->bias, 0.000001f, 0.0f, 0.01f, "%.7f");
                HandleInspectorUndo(*light, lightObject);

                ImGui::SliderFloat("PCF radius", &light->pcf, 1.0f, 8.0f, "%.0f");
                HandleInspectorUndo(*light, lightObject);
            }

            ImGui::Separator();

            if (animationWindow) {

                if (light)
                    DrawLightAnimationButtons(*light);

                if (obj)
                    DrawObjectAnimationButtons(*obj);
            }

            if (ImGui::Button("Delete Light")) {

                int index = selectedObject - static_cast<int>(objects.size());

                DestroyBuffers(lightObjects[index]);

                lightObjects[index].vertices.clear();
                lightObjects[index].indices.clear();

                lightObjects[index].normalLines.clear();

                lightObjects[index].tangentLines.clear();
                lightObjects[index].bitangentLines.clear();
                lightObjects[index].normalLinesTBN.clear();

                lightObjects[index].original_vertices.clear();
                lightObjects[index].original_indices.clear();

                lights.erase(lights.begin() + index);
                lightObjects.erase(lightObjects.begin() + index);

                selectedObject = -1;
            }

            ImGui::PopID();
        }
    }

    ImGui::End();

}
