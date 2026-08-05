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

void update_objects() {

	currentTexture = 0;
	
	glUseProgram(program);

	glActiveTexture(GL_TEXTURE0);

	glUniform1i(glGetUniformLocation(program, "uTexture"), 0);

	for (Object& obj : objects) {

		glm::mat4 model = GetModelMatrix(obj);

		glm::mat4 mvp = projection * view * model;

		glUniformMatrix4fv(
			glGetUniformLocation(program, "uMVP"),
			1,
			GL_FALSE,
			glm::value_ptr(mvp)
		);


		if (currentTexture != obj.obj_texture) {
			
			glBindTexture(GL_TEXTURE_2D, obj.obj_texture);
			currentTexture = obj.obj_texture;
		}

		glBindVertexArray(obj.vao);

		glDrawElements(
			GL_TRIANGLES,
			obj.indices.size(),
			GL_UNSIGNED_INT,
			nullptr
		);
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

						if(selectedObject != -1)
							objects[selectedObject].obj_texture = tex;
					}

					ImGui::TextWrapped("%s", entry.path().stem().string().c_str());
				}

				if (ext == ".obj") {

					std::string path = entry.path().string();

					if (ImGui::Button("", ImVec2(64, 64))) {
						
						Object obj;
						
						init_obj(path.c_str(), obj);

						obj.name = GenerateObjectName(entry.path().stem().string());

						buffer(obj);

						obj.position = glm::vec3(0.0f);
						obj.rotation = glm::vec3(0.0f);
						obj.scale = glm::vec3(50.0f);

						obj.original_vertices = obj.vertices;
						obj.original_indices = obj.indices;

						obj.obj_texture = LoadObjTexture(par.objects[0].tex);

						objects.push_back(obj);
						selectedObject = (int)(objects.size() - 1);
					}

					ImGui::TextWrapped("%s", entry.path().stem().string().c_str());
				}

				if (ext == ".exr" || ext == ".hdr") {
				
					std::string path = entry.path().string();

					if (path.rfind("../", 0) == 0)
						path.erase(0, 3);

					if (ImGui::Button("", ImVec2(64, 64)))
						init_skybox(path.c_str());

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
	
			obj = GetSelectedObject();

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

		ImGui::End();
	}
}