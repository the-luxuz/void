#include "headers/updates.h"

//--------------------------------------------------------------------------
// global variables

// current mesh, texture and operation for ImGuizmo
int currentMesh = 0;
GLuint currentTexture = 0;
ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;

// snap imguizmo
bool useSnap;
float* snap;

// texture manager
std::unordered_map<std::string, GLuint> TextureManager::textures;

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

	for (int i = 0; i < lightObjects.size(); i++) {

		lights[i].dir = glm::normalize(lightObjects[i].rotation * glm::vec3(0.0f, -1.0f, 0.0f));

		glm::vec3 forward = glm::normalize(lights[i].dir);

		glm::vec3 up = glm::vec3(0, 1, 0);

		if (abs(glm::dot(forward, up)) > 0.99f)
			up = glm::vec3(0, 0, 1);

		glm::mat4 lightView = glm::lookAt(
			lights[i].pos,
			lights[i].pos + forward,
			up
		);

		glm::mat4 lightProjection = glm::perspective(
			glm::radians(lights[i].outer * 2.0f),
			1.0f,
			cam.nearPlane,
			cam.farPlane
		);

		lightVP[i] = lightProjection * lightView;

		if (lights[i].typeID != 2)
			continue;

		glBindFramebuffer(GL_FRAMEBUFFER, depthFBO[i]);

		glViewport(0, 0, viewport_width, viewport_height);

		glClear(GL_DEPTH_BUFFER_BIT);

		glUseProgram(program_depth);

		for (auto& obj : objects) {

			glm::mat4 model(1.0f);

			model = glm::translate(model, obj.position);

			model *= glm::toMat4(obj.rotation);

			model = glm::scale(model, obj.scale);

			glm::mat4 lightMVP = lightVP[i] * model;

			glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(lightMVP));

			glBindVertexArray(obj.vao);

			glDrawElements(
				GL_TRIANGLES,
				static_cast<GLsizei>(obj.indices.size()),
				GL_UNSIGNED_INT,
				0
			);
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//--------------------------------------------------------------------
}

//--------------------------------------------------------------------------
// Update Objects

void update_objects() {

	currentTexture = 0;
	
	glUseProgram(program);

	glUniform1i(shader_uniform.program_texture, 0);

	int numLights = lights.size(); // maximum of 8 lights

	for (int i = 0; i < lightObjects.size(); i++) {

		glActiveTexture(GL_TEXTURE2 + i);
		glBindTexture(GL_TEXTURE_2D, depth_texture[i]);

		std::string idx = std::to_string(i);

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

	glUniformMatrix4fv(shader_uniform.lightVP, numLights, GL_FALSE, glm::value_ptr(lightVP[0]));

	glUniform1i(shader_uniform.normalMap, 1);

	glUniformMatrix4fv(7, 1, GL_FALSE, glm::value_ptr(view));

	glUniform1i(shader_uniform.numLights, numLights);
	glUniform3fv(shader_uniform.cameraPos, 1, glm::value_ptr(cameraPos));

	glUniform1i(shader_uniform.tex_appear, tex_appear);
	glUniform1i(shader_uniform.normal_apper, normal_apper);
	glUniform1i(shader_uniform.tangent_apper, tangent_apper);
	glUniform1i(shader_uniform.bitangent_apper, bitangent_apper);

	glUniform1i(shader_uniform.light_active, st.light_active);
	glUniform1i(shader_uniform.normal_map_active, st.normal_map_active);
	glUniform1i(shader_uniform.shadow_active, st.shadow_active);

	if (st.wireframe_active) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	for (Object& obj : objects) {

		if(st.normal_map_active) {

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, obj.normalTexture);
		}

		glUniform1f(shader_uniform.shininess, obj.shininess);

		glm::mat4 model = GetModelMatrix(obj);

		obj.model = model;
		obj.mvp = projection * view * model;

		glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(obj.mvp));
		glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(obj.model));

		glActiveTexture(GL_TEXTURE0);

		if (currentTexture != obj.obj_texture) {
			
			glBindTexture(GL_TEXTURE_2D, obj.obj_texture);
			currentTexture = obj.obj_texture;
		}

		glUniform1i(shader_uniform.has_normal_map, obj.has_normal_map);


		glBindVertexArray(obj.vao);

		glDrawElements(
			GL_TRIANGLES,
			obj.indices.size(),
			GL_UNSIGNED_INT,
			nullptr
		);
	}

	//-----------------------------------------------------------------------------------------------
	// draw the lights and the normals, tangents and bitangents

	glUseProgram(program_line);

	GLint colorLoc = shader_uniform.program_line_color;

	if (normals_appear) {

		for (auto& obj : objects) {

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

	for (int i = 0; i < lightObjects.size(); i++) {

		lights[i].pos = lightObjects[i].position;

		glm::mat4 model(1.0f);

		model = glm::translate(model, lightObjects[i].position);
		model *= glm::toMat4(lightObjects[i].rotation);
		model = glm::scale(model, lightObjects[i].scale);

		glm::mat4 mvp = projection * view * model;

		glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(mvp));
		glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // white
		glBindVertexArray(lightObjects[i].vao);

		glDrawElements(GL_TRIANGLES, (GLsizei)lightObjects[i].indices.size(), GL_UNSIGNED_INT, 0);
	}

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
// Update Imgui and guizmo

void update_imgui_guizmo() {

	ImGui::Begin("ViewPort", nullptr);

	ImVec2 pos = ImGui::GetCursorScreenPos();

	viewportPos = ImGui::GetCursorScreenPos();
	viewportSize = ImGui::GetContentRegionAvail();

	ImGui::Image((ImTextureID)(intptr_t)viewport_texture, viewportSize, ImVec2(0, 1), ImVec2(1, 0));

	Object* obj = GetSelectedObject();
	Light* light = GetSelectedLight();

	if (obj) {

		ImGuizmo::BeginFrame();

		ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());

		ImGuizmo::SetOrthographic(false);

		ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

		float moveSnap[3] = { 12.5, 12.5, 12.5 };
		float scaleSnap[3] = { 0.125, 0.125, 0.125 };
		float rotateSnap[3] = { 11.25, 11.25, 11.25 };

		snap = nullptr;

		useSnap = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LSHIFT];

		if (useSnap) {

			switch (currentOperation) {

			case ImGuizmo::TRANSLATE:
				snap = moveSnap;
				break;

			case ImGuizmo::ROTATE:
				snap = rotateSnap;
				break;

			case ImGuizmo::SCALE:
				snap = scaleSnap;
				break;
			}
		}

		obj->model = GetModelMatrix(*obj);

		ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), currentOperation, ImGuizmo::WORLD, glm::value_ptr(obj->model), nullptr, snap);

		static bool wasUsing = false;

		bool usingGizmo = ImGuizmo::IsUsing();

		if (usingGizmo && !wasUsing) {

			prevState = CaptureState(*obj);
		}

		if (usingGizmo && !wasUsing && light) {

			prevLightState = CaptureLightState(*light);
		}

		if (ImGuizmo::IsUsing()) {

			glm::vec3 translation = glm::vec3(0.0f);
			glm::vec3 scale = glm::vec3(20.0f);
			glm::vec3 euler;

			ImGuizmo::DecomposeMatrixToComponents(
				glm::value_ptr(obj->model),
				glm::value_ptr(translation),
				glm::value_ptr(euler),
				glm::value_ptr(scale));

			if (useSnap && currentOperation == ImGuizmo::ROTATE) {

				auto Snap = [](float value, float step) {

					return roundf(value / step) * step;
					};

				euler.x = Snap(euler.x, rotateSnap[0]);
				euler.y = Snap(euler.y, rotateSnap[1]);
				euler.z = Snap(euler.z, rotateSnap[2]);
			}

			obj->position = translation;
			obj->scale = scale;
			obj->rotation = glm::quat(glm::radians(euler));

			if (light) {

				light->pos = translation;
			}

		}

		if (!usingGizmo && wasUsing) {

			obj->undoStack.push_back(prevState);
			obj->redoStack.clear();
		}

		if (!usingGizmo && wasUsing && light) {

			light->undoStack.push_back(prevLightState);
			light->redoStack.clear();
		}

		wasUsing = usingGizmo;

		if (currentOperation != ImGuizmo::SCALE && light)
			obj->scale = glm::vec3(20.0f);

	}

	ImGui::End();

	//----------------------------------------------------
	// browser

	ImGuiWindowFlags a_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	
	ImGui::Begin("Assets", nullptr, a_flags);

	if (currentFolder != rootFolder) {

		if (ImGui::Button("<-")) {
		
			currentFolder = currentFolder.parent_path();
			ReloadFolder();
		}
	}else {

		ImGui::BeginDisabled();
		ImGui::Button("<-");
		ImGui::EndDisabled();
	}

	ImGui::SameLine();

	if (ImGui::Button("Home")) {

		currentFolder = rootFolder;
		ReloadFolder();
	}

	ImGui::SameLine();

	ImGui::TextUnformatted(currentFolder.string().c_str());

	ImGui::Separator();

	int columns = std::max(1, (int)(ImGui::GetContentRegionAvail().x / cellSize));

	if (ImGui::BeginTable("AssetsTable", columns)) {

		for (auto& entry : folderEntries) {

			ImGui::TableNextColumn();

			ImGui::PushID(entry.path().string().c_str());

			if (entry.is_directory()) {

				if (ImGui::Button("", ImVec2(64, 64))) {

					newFolder = entry.path();
					reloadFolder = true;
				}

				ImGui::TextWrapped("%s", entry.path().filename().string().c_str());

			}else {

				auto ext = entry.path().extension().string();

				if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {

					ImGui::PushID(entry.path().string().c_str());

					std::string path = entry.path().string();

					if (path.rfind("../", 0) == 0)
						path.erase(0, 3);

					GLuint tex = TextureManager::Get(path);

					if (ImGui::ImageButton("", (ImTextureID)(intptr_t)tex, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0))) {

						if (selectedObject != -1) {

							objects[selectedObject].obj_texture = tex;
							objects[selectedObject].tex = path;
						}
					}

					if (ImGui::BeginDragDropSource()) {

						ImGui::SetDragDropPayload("IMAGE_FILE", path.c_str(), path.size() + 1);

						ImGui::Image((ImTextureID)(intptr_t)tex, ImVec2(64, 64));
						ImGui::Text("%s", entry.path().filename().string().c_str());

						ImGui::EndDragDropSource();
					}

					ImGui::TextWrapped("%s", entry.path().stem().string().c_str());

					ImGui::PopID();
				}

				if (ext == ".obj") {

					std::string path = entry.path().string();

					if (ImGui::Button("", ImVec2(64, 64))) {
						
						Object obj;
						
						init_obj(path.c_str(), obj);

						obj.name = GenerateObjectName(entry.path().stem().string());

						obj.mesh = path;

						buffer(obj);

						obj.position = glm::vec3(0.0f);
						obj.rotation = glm::quat(1.f, 0.f, 0.f, 0.f);
						obj.scale = glm::vec3(50.0f);

						obj.original_vertices = obj.vertices;
						obj.original_indices = obj.indices;

						obj.shininess = 20;

						obj.has_normal_map = false;

						obj.model = GetModelMatrix(obj);

						if (!objects.empty())
							obj.obj_texture = TextureManager::Get(objects[0].tex);
						else
							obj.obj_texture = 0;

						obj.tex = (objects.empty()) ? "" : objects[0].tex;

						objects.push_back(obj);
						selectedObject = (int)(objects.size() - 1);
					}

					if (ImGui::BeginDragDropSource()) {

						ImGui::SetDragDropPayload("OBJ_FILE", path.c_str(), path.size() + 1);

						ImGui::Image((ImTextureID)(intptr_t)nullptr, ImVec2(64, 64));
						ImGui::Text("%s", entry.path().filename().string().c_str());

						ImGui::EndDragDropSource();

					}

					ImGui::TextWrapped("%s", entry.path().stem().string().c_str());
				}

				if (ext == ".exr" || ext == ".hdr") {
				
					std::string path = entry.path().string();

					if (path.rfind("../", 0) == 0)
						path.erase(0, 3);


					if (ImGui::Button("", ImVec2(64, 64))) {
					
						init_skybox(path.c_str(), false);
						st.skybox = path;
					}

					ImGui::TextWrapped("%s", entry.path().stem().string().c_str());
				}

				if (ext == ".bin") {

					std::string path = entry.path().string();

					if (ImGui::Button("", ImVec2(64, 64))) {

						load_scene_ok(path.c_str());
					}

					ImGui::TextWrapped("%s", entry.path().stem().string().c_str());
				}
			}

			ImGui::PopID();
		}

		if (reloadFolder) {

			currentFolder = newFolder;

			ReloadFolder();

			reloadFolder = false;
		}

		ImGui::EndTable();
	}

	ImGui::End();

	//----------------------------------------------------
	// hierarchy

	ImGuiWindowFlags h_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

	ImGui::Begin("Hierarchy", nullptr, h_flags);

	for (size_t i = 0; i < objects.size(); i++) {

		ImGui::PushID((int)i);

		bool selected = (selectedObject == (int)i);

		if (ImGui::Selectable(objects[i].name.c_str(), selected))
			selectedObject = i;

		ImGui::PopID();
	}

	for (size_t i = 0; i < lightObjects.size(); i++) {

		ImGui::PushID((int)(objects.size() + i));

		bool selected = (selectedObject == (int)(objects.size() + i));

		if (ImGui::Selectable(lightObjects[i].name.c_str(), selected))
			selectedObject = objects.size() + i;

		ImGui::PopID();
	}

	ImGui::End();

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

	static const char* mappings[] = {
		"UV",
		"Planar",
		"Cube",
		"Cylinder",
		"Sphere"
	};

	static const char* planarCoords[] = {
		"XZ",
		"XY",
		"YZ"
	};
	
	static const char* cylinderAxes[] = {
		"X",
		"Y",
		"Z"
	};

	ImGui::Begin("Inspector");

	if (selectedObject != -1 && selectedObject < (int)objects.size()) {

		Object* obj = GetSelectedObject();

		if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen)) {

			static char name[256];
			strncpy_s(name, objects[selectedObject].name.c_str(), sizeof(name));
			name[255] = '\0';

			if (ImGui::InputText("Name", name, sizeof(name))) {

				objects[selectedObject].name = name;
			}

			ImGui::DragFloat3("Position", &obj->position.x, 0.1f);
			HandleInspectorUndo(*obj);

			glm::vec3 euler = glm::degrees(glm::eulerAngles(obj->rotation));
			if (ImGui::DragFloat3("Rotation", &euler.x, 1.0f))
				obj->rotation = glm::quat(glm::radians(euler));

			HandleInspectorUndo(*obj);

			ImGui::DragFloat3("Scale", &obj->scale.x, 0.1f);
			HandleInspectorUndo(*obj);
		}

		if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {

			ImGui::DragFloat("Shininess", &obj->shininess, 1.0f, 1.0f, 256.0f);
			HandleInspectorUndo(*obj);

			ImGui::Text("Texture:");

			ImGui::TextWrapped("%s", obj->tex.c_str());

			ImGui::Image((ImTextureID)(intptr_t)obj->obj_texture, ImVec2(80, 80));

			ImGui::Button("Drop texture here##Texture", ImVec2(80, 80));

			if (ImGui::BeginDragDropTarget()) {

				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("IMAGE_FILE")) {

					obj->undoStack.push_back(CaptureState(*obj));
					obj->redoStack.clear();

					const char* file = (const char*)payload->Data;

					objects[selectedObject].obj_texture = TextureManager::Get(file);

					objects[selectedObject].tex = file;
				}

				ImGui::EndDragDropTarget();
			}

			//------------------------------------------------------------

			ImGui::Text("Normal Map:");

			ImGui::TextWrapped("%s", obj->norm_map.c_str());

			ImGui::Image((ImTextureID)(intptr_t)obj->normalTexture, ImVec2(80, 80));

			ImGui::Button("Drop texture here##NormalMap", ImVec2(80, 80));

			if (ImGui::BeginDragDropTarget()) {

				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("IMAGE_FILE")) {

					obj->undoStack.push_back(CaptureState(*obj));
					obj->redoStack.clear();

					const char* file = (const char*)payload->Data;

					objects[selectedObject].normalTexture = TextureManager::Get(file);

					objects[selectedObject].norm_map = file;
					objects[selectedObject].has_normal_map = true;
				}

				ImGui::EndDragDropTarget();
			}

		}

		if (ImGui::CollapsingHeader("Mesh")) {

			ImGui::TextWrapped("%s", obj->mesh.c_str());

			ImGui::Text("Mesh:");

			ImGui::Button("Drop .obj here##Mesh", ImVec2(200, 80));

			if (ImGui::BeginDragDropTarget()) {

				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("OBJ_FILE")) {

					obj->undoStack.push_back(CaptureState(*obj));
					obj->redoStack.clear();

					DestroyBuffers(*obj);

					obj->vertices.clear();
					obj->indices.clear();

					obj->normalLines.clear();

					obj->tangentLines.clear();
					obj->bitangentLines.clear();
					obj->normalLinesTBN.clear();

					obj->original_vertices.clear();
					obj->original_indices.clear();

					ReplaceMesh(objects[selectedObject], (const char*)payload->Data);
				}

				ImGui::EndDragDropTarget();
			}

			ImGui::TextWrapped("%s", obj->mesh.c_str());

			ImGui::Text("Vertices: %d", (int)obj->vertices.size());
			ImGui::Text("Triangles: %d", (int)obj->indices.size() / 3);
		}

		if (ImGui::CollapsingHeader("Bounds")) {

			ImGui::DragFloat3("Min", &obj->min.x, 0.1f);
			HandleInspectorUndo(*obj);

			ImGui::DragFloat3("Max", &obj->max.x, 0.1f);
			HandleInspectorUndo(*obj);
		}

		ImGui::Separator();

		int current = (int)obj->mapping;

		if (ImGui::Combo("Mapping", &current, mappings, IM_ARRAYSIZE(mappings))) {

			obj->undoStack.push_back(CaptureState(*obj));
			obj->redoStack.clear();

			obj->mapping = (TextureMapping)current;
			UpdateMapping(*obj);
		}

		//------------------------------------------------------------
		// Planar

		if (obj->mapping == TextureMapping::Planar) {

			int coord = (int)obj->coordType;

			if (ImGui::Combo("Plane", &coord, planarCoords, IM_ARRAYSIZE(planarCoords))) {

				obj->undoStack.push_back(CaptureState(*obj));
				obj->redoStack.clear();

				obj->coordType = (coord_types)coord;
				UpdateMapping(*obj);
			}
		}
		

		//------------------------------------------------------------
		// Cubic

		if (obj->mapping == TextureMapping::Cubic) {

			if (ImGui::Button("Regenerate Cube UV")) {

				obj->undoStack.push_back(CaptureState(*obj));
				obj->redoStack.clear();

				UpdateMapping(*obj);
			}
		}

		//------------------------------------------------------------
		// Cylindrical

		if (obj->mapping == TextureMapping::Cylindrical) {

			int axis = (int)obj->cyaxisType;

			if (ImGui::Combo("Axis", &axis, cylinderAxes, IM_ARRAYSIZE(cylinderAxes))) {

				obj->undoStack.push_back(CaptureState(*obj));
				obj->redoStack.clear();

				obj->cyaxisType = (cylinder_axis)axis;
				UpdateMapping(*obj);
			}
		}

		//------------------------------------------------------------
		// Spherical

		if (obj->mapping == TextureMapping::Spherical) {

			if (ImGui::Button("Regenerate Sphere UV")) {

				obj->undoStack.push_back(CaptureState(*obj));
				obj->redoStack.clear();

				UpdateMapping(*obj);
			}
		}

		//------------------------------------------------------------
		// UV

		if (obj->mapping == TextureMapping::UV) {

			if (ImGui::Button("Restore Original UV")) {

				obj->undoStack.push_back(CaptureState(*obj));
				obj->redoStack.clear();

				UpdateMapping(*obj);
			}
		}

		ImGui::Separator();

		if (ImGui::Button("Delete Object")) {

			DestroyBuffers(objects[selectedObject]);

			objects[selectedObject].vertices.clear();
			objects[selectedObject].indices.clear();

			objects[selectedObject].normalLines.clear();

			objects[selectedObject].tangentLines.clear();
			objects[selectedObject].bitangentLines.clear();
			objects[selectedObject].normalLinesTBN.clear();

			objects[selectedObject].original_vertices.clear();
			objects[selectedObject].original_indices.clear();

			objects.erase(objects.begin() + selectedObject);

			selectedObject = -1;
		}

	} else {


		Light* light = GetSelectedLight();
		Object* obj = GetSelectedObject();

		if (light) {

			if(ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen)) {

				static char name[256];
				strncpy_s(name, lightObjects[selectedObject - objects.size()].name.c_str(), sizeof(name));
				name[255] = '\0';

				if (ImGui::InputText("Name", name, sizeof(name))) {

					lightObjects[selectedObject - objects.size()].name = name;
				}

				ImGui::ColorEdit3("Color", &light->col.x);
				HandleInspectorUndo(*light, lightObjects[selectedObject - objects.size()]);

				ImGui::DragFloat3("Position", &light->pos.x, 0.1f);
				HandleInspectorUndo(*light, lightObjects[selectedObject - objects.size()]);

				glm::vec3 euler = glm::degrees(glm::eulerAngles(obj->rotation));

				if (ImGui::DragFloat3("Rotation", &euler.x, 1.0f)) {

					obj->rotation = glm::quat(glm::radians(euler));
				}
				HandleInspectorUndo(*light, lightObjects[selectedObject - objects.size()]);

				ImGui::DragFloat3("Direction", &light->dir.x, 0.01f);
				HandleInspectorUndo(*light, lightObjects[selectedObject - objects.size()]);

				ImGui::DragFloat("Ambient", &light->ambient, 0.01f, 0.f, 1.f);
				HandleInspectorUndo(*light, lightObjects[selectedObject - objects.size()]);

				lightObjects[selectedObject - objects.size()].position = light->pos;
			}

			if (ImGui::CollapsingHeader("Attenuation", ImGuiTreeNodeFlags_DefaultOpen)) {

				ImGui::DragFloat("Constant", &light->att.x, 0.001f, 0.f, 10.f);
				HandleInspectorUndo(*light, lightObjects[selectedObject - objects.size()]);

				ImGui::DragFloat("Linear", &light->att.y, 0.0001f, 0.f, 1.f);
				HandleInspectorUndo(*light, lightObjects[selectedObject - objects.size()]);

				ImGui::DragFloat("Quadratic", &light->att.z, 0.00001f, 0.f, 1.f);
				HandleInspectorUndo(*light, lightObjects[selectedObject - objects.size()]);

				if (light->type == "SPOT") {

					ImGui::SeparatorText("Spot");

					ImGui::DragFloat("Inner", &light->inner, 0.25f, 0.f, 90.f);
					HandleInspectorUndo(*light, lightObjects[selectedObject - objects.size()]);

					ImGui::DragFloat("Outer", &light->outer, 0.25f, 0.f, 90.f);
					HandleInspectorUndo(*light, lightObjects[selectedObject - objects.size()]);

					ImGui::DragFloat("Falloff", &light->falloff, 0.05f, 0.f, 16.f);
					HandleInspectorUndo(*light, lightObjects[selectedObject - objects.size()]);
				}
			}

			if (ImGui::CollapsingHeader("Shadow", ImGuiTreeNodeFlags_DefaultOpen)) {

				ImGui::DragFloat("Bias", &light->bias, 0.000001f, 0.f, 0.01f, "%.6f");
				HandleInspectorUndo(*light, lightObjects[selectedObject - objects.size()]);

				ImGui::DragFloat("PCF", &light->pcf, 0.1f, 1.f, 16.f);
				HandleInspectorUndo(*light, lightObjects[selectedObject - objects.size()]);
			}

			if (ImGui::CollapsingHeader("Type", ImGuiTreeNodeFlags_DefaultOpen)) {

				static const char* types[] = {
					"Point",
					"Directional",
					"Spot"
				};

				int current = light->typeID;

				if (ImGui::Combo("Light Type", &current, types, IM_ARRAYSIZE(types))) {

					light->typeID = current;

					switch (current)
					{
					case 0: light->type = "POINT"; break;
					case 1: light->type = "DIR"; break;
					case 2: light->type = "SPOT"; break;
					}
				}
			}

			if (ImGui::Button("Delete Light")) {

				int index = selectedObject - objects.size();

				DestroyBuffers(objects[selectedObject]);

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

		}

	}

	ImGui::End();

	switch (currentTool) {

		case Tools::TRANSLATE:
			currentOperation = ImGuizmo::TRANSLATE;
			break;

		case Tools::ROTATE:
			currentOperation = ImGuizmo::ROTATE;
			break;
		case Tools::SCALE:
			currentOperation = ImGuizmo::SCALE;
			break;
	}
}