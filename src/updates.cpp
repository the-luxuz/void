#include "headers/updates.h"

int currentMesh = 0;

GLuint currentTexture = 0;

bool useSnap;

float* snap;

glm::mat4 m;

const float cellSize = 90.0f;

ObjectState prevState;

ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;

std::unordered_map<std::string, GLuint> TextureManager::textures;

void update_grid(glm::vec3 cameraPos) {

	glUseProgram(program_grid);

	glm::mat4 model(1.0f);

	glm::vec3 gridPos(floor(cameraPos.x / STEP) * STEP, 0.0f, floor(cameraPos.z / STEP) * STEP);

	model = glm::translate(glm::mat4(1.0f), gridPos);

	glm::mat4 mvp = projection * view * model;

	glUniformMatrix4fv(
		glGetUniformLocation(program_grid, "uMVP"),
		1,
		GL_FALSE,
		glm::value_ptr(mvp)
	);

	glBindVertexArray(gridVAO);

	glBindBuffer(GL_ARRAY_BUFFER, gridVBO);

	glDrawArrays(GL_LINES, 0, gridVertexCount);

	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(glm::vec3),
		(void*)0);

	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

void update_depthmap() {
	
	//--------------------------------------------------------------------

	for (int i = 0; i < lightObjects.size(); i++) {

		// this is bcs if i put the up vector like (0, 1, 0), the light directions like (0, -1, 0), will not work
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
	}

	for (int i = 0; i < lightObjects.size(); i++) {

		/*if (lights[i].typeID != 2)
			continue;*/

		glBindFramebuffer(GL_FRAMEBUFFER, depthFBO[i]);

		glViewport(0, 0, viewport_width, viewport_height);

		glClear(GL_DEPTH_BUFFER_BIT);

		glUseProgram(program_depth);

		for (auto& obj : objects) {

			// excludes the planes from the depth map
			if (obj.type == "PLANE")
				continue;

			glm::mat4 model(1.0f);

			model = glm::translate(model, obj.position);

			model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1, 0, 0));
			model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0, 1, 0));
			model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0, 0, 1));

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

void update_objects() {

	currentTexture = 0;
	
	glUseProgram(program);

	glUniform1i(glGetUniformLocation(program, "tex"), 0);

	for (int i = 0; i < lightObjects.size(); i++) {

		glActiveTexture(GL_TEXTURE2 + i);
		glBindTexture(GL_TEXTURE_2D, depth_texture[i]);

		std::string idx = std::to_string(i);

		glUniform1i(glGetUniformLocation(program, ("depthData[" + idx + "]").c_str()), 2 + i);
	}

	int numLights = lights.size(); // maximum of 8 lights
	glUniform1i(glGetUniformLocation(program, "numLights"), numLights);

	for (int i = 0; i < numLights; i++) {

		std::string idx = std::to_string(i);

		glUniform1i(glGetUniformLocation(program, ("lights[" + idx + "].type").c_str()), lights[i].typeID);
		glUniform3fv(glGetUniformLocation(program, ("lights[" + idx + "].position").c_str()), 1, glm::value_ptr(lights[i].pos));
		glUniform3fv(glGetUniformLocation(program, ("lights[" + idx + "].direction").c_str()), 1, glm::value_ptr(lights[i].dir));
		glUniform3fv(glGetUniformLocation(program, ("lights[" + idx + "].attenuation").c_str()), 1, glm::value_ptr(lights[i].att));
		glUniform1f(glGetUniformLocation(program, ("lights[" + idx + "].ambient").c_str()), lights[i].ambient);
		glUniform1f(glGetUniformLocation(program, ("lights[" + idx + "].bias").c_str()), lights[i].bias);
		glUniform1f(glGetUniformLocation(program, ("lights[" + idx + "].pcf").c_str()), lights[i].pcf);
		glUniform1f(glGetUniformLocation(program, ("lights[" + idx + "].inner").c_str()), lights[i].inner);
		glUniform1f(glGetUniformLocation(program, ("lights[" + idx + "].outer").c_str()), lights[i].outer);
		glUniform1f(glGetUniformLocation(program, ("lights[" + idx + "].falloff").c_str()), lights[i].falloff);
		glUniform3fv(glGetUniformLocation(program, ("lights[" + idx + "].color").c_str()), 1, glm::value_ptr(lights[i].col));

		GLint loc = glGetUniformLocation(program, "lightVP");
		glUniformMatrix4fv(loc, numLights, GL_FALSE, glm::value_ptr(lightVP[0]));

	}

	glUniform1i(glGetUniformLocation(program, "normalMap"), 1);

	glUniformMatrix4fv(7, 1, GL_FALSE, glm::value_ptr(view));

	glUniform1i(glGetUniformLocation(program, "numLights"), numLights);

	for (Object& obj : objects) {

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, obj.normalTexture);

		glUniform3fv(glGetUniformLocation(program, "cameraPos"), 1, glm::value_ptr(cameraPos));
		glUniform1f(glGetUniformLocation(program, "shininess"), obj.shininess);

		glm::mat4 model = GetModelMatrix(obj);

		glm::mat4 mvp = projection * view * model;

		obj.model = model;
		obj.mvp = projection * view * model;

		glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(obj.mvp));
		glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(obj.model));

		glActiveTexture(GL_TEXTURE0);

		if (currentTexture != obj.obj_texture) {
			
			glBindTexture(GL_TEXTURE_2D, obj.obj_texture);
			currentTexture = obj.obj_texture;
		}

		glUniform1i(glGetUniformLocation(program, "tex_appear"), tex_appear);
		glUniform1i(glGetUniformLocation(program, "normal_apper"), normal_apper);
		glUniform1i(glGetUniformLocation(program, "tangent_apper"), tangent_apper);
		glUniform1i(glGetUniformLocation(program, "bitangent_apper"), bitangent_apper);

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

	GLint colorLoc = glGetUniformLocation(program_line, "color");

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
		model = glm::scale(model, lightObjects[i].scale);

		glm::mat4 mvp = projection * view * model;

		glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(mvp));
		glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // white
		glBindVertexArray(lightObjects[i].vao);

		glDrawElements(GL_TRIANGLES, (GLsizei)lightObjects[i].indices.size(), GL_UNSIGNED_INT, 0);
	}
}

void update_skybox() {
	
	glUseProgram(program_skybox);

	glDepthFunc(GL_LEQUAL);
	glDisable(GL_CULL_FACE);

	glm::mat4 skyView = glm::mat4(glm::mat3(view));

	glUniformMatrix4fv(glGetUniformLocation(program_skybox, "view"), 1, GL_FALSE, glm::value_ptr(skyView));

	glUniformMatrix4fv(glGetUniformLocation(program_skybox, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glUniform1f(glGetUniformLocation(program_skybox, "is_hdr"), is_hdr);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glUniform1i(glGetUniformLocation(program_skybox, "skybox"), 1);

	glBindVertexArray(skyboxVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
}

void update_imgui_guizmo() {

	ImGui::Begin("ViewPort", nullptr);

	ImVec2 pos = ImGui::GetCursorScreenPos();

	viewportPos = ImGui::GetCursorScreenPos();
	viewportSize = ImGui::GetContentRegionAvail();

	ImGui::Image((ImTextureID)(intptr_t)viewport_texture, viewportSize, ImVec2(0, 1), ImVec2(1, 0));


	Object* obj = GetSelectedObject();
	Light* light = GetSelectedLight();

	if (obj) {

		m = GetModelMatrix(*obj);

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

		ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), currentOperation, ImGuizmo::LOCAL, glm::value_ptr(m), nullptr, snap);

		static bool wasUsing = false;

		bool usingGizmo = ImGuizmo::IsUsing();

		if (usingGizmo && !wasUsing) {

			prevState = CaptureState(*obj);
		}

		if (ImGuizmo::IsUsing()) {

			glm::vec3 translation = glm::vec3(0.0f);
			glm::vec3 scale = glm::vec3(50.0f);
			glm::vec3 rotation = glm::vec3(0.0f);

			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(m), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));

			if (useSnap && currentOperation == ImGuizmo::ROTATE) {

				auto Snap = [](float value, float step) {

					return roundf(value / step) * step;
					};

				rotation.x = Snap(rotation.x, rotateSnap[0]);
				rotation.y = Snap(rotation.y, rotateSnap[1]);
				rotation.z = Snap(rotation.z, rotateSnap[2]);
			}

			obj->position = translation;
			obj->scale = scale;
			obj->rotation = rotation;

			if (light)
			{
				light->pos = translation;
			}

		}

		if (!usingGizmo && wasUsing) {

			obj->undoStack.push_back(prevState);
			obj->redoStack.clear();
		}

		wasUsing = usingGizmo;

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

					ImGui::TextWrapped("%s", entry.path().stem().string().c_str());
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
						obj.rotation = glm::vec3(0.0f);
						obj.scale = glm::vec3(50.0f);

						obj.original_vertices = obj.vertices;
						obj.original_indices = obj.indices;

						if (!objects.empty())
							obj.obj_texture = TextureManager::Get(objects[0].tex);
						else
							obj.obj_texture = 0;

						objects.push_back(obj);
						//selectedObject = (int)(objects.size() - 1);
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
	// tools

	if (selectedObject != -1) {
	
		ImGui::Begin("Tools");

		if (ImGui::RadioButton("Move", currentOperation == ImGuizmo::TRANSLATE))
			currentOperation = ImGuizmo::TRANSLATE;

		ImGui::SameLine();

		if (ImGui::RadioButton("Rotate", currentOperation == ImGuizmo::ROTATE))
			currentOperation = ImGuizmo::ROTATE;

		ImGui::SameLine();

		if (ImGui::RadioButton("Scale", currentOperation == ImGuizmo::SCALE))
			currentOperation = ImGuizmo::SCALE;

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

		if (selectedObject != -1) {

			if (selectedObject < objects.size()) {

				Object* obj = GetSelectedObject();

				int current = (int)obj->mapping;

				if (ImGui::Combo("Mapping", &current, mappings, IM_ARRAYSIZE(mappings))) {

					obj->mapping = (TextureMapping)current;
					UpdateMapping(*obj);
				}

				if (obj->mapping == TextureMapping::Planar) {

					int coord = (int)obj->coordType;

					if (ImGui::Combo("Plane", &coord, planarCoords, IM_ARRAYSIZE(planarCoords))) {

						obj->coordType = (coord_types)coord;
						UpdateMapping(*obj);
					}
				}

				if (obj->mapping == TextureMapping::Cylindrical) {

					int coord = (int)obj->cyaxisType;

					if (ImGui::Combo("Cylinder", &coord, cylinderAxes, IM_ARRAYSIZE(cylinderAxes))) {

						obj->cyaxisType = (cylinder_axis)coord;
						UpdateMapping(*obj);
					}
				}
			}
			else {

				Light* light = GetSelectedLight();

				ImGui::ColorEdit3("Color", &light->col.x);
				ImGui::DragFloat3("Position", &light->pos.x, 0.1f);

				// Sincronizar la esfera
				lightObjects[selectedObject - objects.size()].position = light->pos;
			}
		}

		ImGui::End();
	}
}