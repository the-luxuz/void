#include "headers/inits.h"

//--------------------------------------------------------------------------
// global variables

// window and context
SDL_Window* window;
SDL_GLContext context;

Shader_Uniform shader_uniform;

//--------------------------------------------------------------------------
// init functions

void init_uniforms() {
	
	// grid shader uniforms

	shader_uniform.grid_MVP = glGetUniformLocation(program_grid, "uMVP");

	// main shader uniforms

	shader_uniform.program_texture = glGetUniformLocation(program, "tex");

	shader_uniform.color = glGetUniformLocation(program, "color");

	shader_uniform.useTexture = glGetUniformLocation(program, "useTexture");

	shader_uniform.tiling = glGetUniformLocation(program, "tiling");

	shader_uniform.mapping = glGetUniformLocation(program, "mapping");

	shader_uniform.min = glGetUniformLocation(program, "objMin");
	shader_uniform.max = glGetUniformLocation(program, "objMax");

	shader_uniform.worldSize = glGetUniformLocation(program, "worldSize");

	shader_uniform.textureTiling = glGetUniformLocation(program, "textureTiling");

	shader_uniform.lightVP = glGetUniformLocation(program, "lightVP");

	shader_uniform.normalMap = glGetUniformLocation(program, "normalMap");

	shader_uniform.numLights = glGetUniformLocation(program, "numLights");

	shader_uniform.light_active = glGetUniformLocation(program, "light_active");
	shader_uniform.normal_map_active = glGetUniformLocation(program, "normal_map_active");
	shader_uniform.shadow_active = glGetUniformLocation(program, "shadow_active");

	shader_uniform.cameraPos = glGetUniformLocation(program, "cameraPos");

	shader_uniform.shininess = glGetUniformLocation(program, "shininess");

	shader_uniform.hasNormalMap = glGetUniformLocation(program, "hasNormalMap");

	// line shader uniforms

	shader_uniform.program_line_color = glGetUniformLocation(program_line, "color");

	//skybox uniforms

	shader_uniform.skybox_view = glGetUniformLocation(program_skybox, "view");

	shader_uniform.skybox_projection = glGetUniformLocation(program_skybox, "projection");

	shader_uniform.is_hdr = glGetUniformLocation(program_skybox, "is_hdr");

	shader_uniform.program_skybox_skybox = glGetUniformLocation(program_skybox, "skybox");
}

void init_class_uniforms() {

	for (int i = 0; i < MAX_LIGHTS; i++) {

		std::string idx = std::to_string(i);

		shader_uniform.depthdata[i] = glGetUniformLocation(program, ("depthData[" + idx + "]").c_str());

		shader_uniform.light_type[i] = glGetUniformLocation(program, ("lights[" + idx + "].type").c_str());
		shader_uniform.light_pos[i] = glGetUniformLocation(program, ("lights[" + idx + "].position").c_str());
		shader_uniform.light_dir[i] = glGetUniformLocation(program, ("lights[" + idx + "].direction").c_str());
		shader_uniform.light_col[i] = glGetUniformLocation(program, ("lights[" + idx + "].color").c_str());
		shader_uniform.light_att[i] = glGetUniformLocation(program, ("lights[" + idx + "].attenuation").c_str());
		shader_uniform.light_ambient[i] = glGetUniformLocation(program, ("lights[" + idx + "].ambient").c_str());
		shader_uniform.light_bias[i] = glGetUniformLocation(program, ("lights[" + idx + "].bias").c_str());
		shader_uniform.light_pcf[i] = glGetUniformLocation(program, ("lights[" + idx + "].pcf").c_str());
		shader_uniform.light_inner[i] = glGetUniformLocation(program, ("lights[" + idx + "].inner").c_str());
		shader_uniform.light_outer[i] = glGetUniformLocation(program, ("lights[" + idx + "].outer").c_str());
		shader_uniform.light_falloff[i] = glGetUniformLocation(program, ("lights[" + idx + "].falloff").c_str());
	}
}

void init_viewport() {

	for (int i = 0; i < MAX_LIGHTS; i++) {

		glGenFramebuffers(1, &depthFBO[i]);
		glBindFramebuffer(GL_FRAMEBUFFER, depthFBO[i]);

		Create_depth_tex(viewport_width, viewport_height, i);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	glGenFramebuffers(1, &viewportFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, viewportFBO);

	Create_viewport_tex(viewport_width, viewport_height);

	CreateViewportDepthTexture(st.w, st.h);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer incompleto\n";

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void init_grid() {
	
	Create_Grid();
}

void init_skybox(const char* skyboxFile, bool create_vaos) {
	
	if (create_vaos)
		GenSkybox();

	Load_Skybox(skyboxFile);

	if (create_vaos)
		gen_cubemap();

	ConvertHDRToCubemap();
}

void init_camera() {

	cam.fov = st.fov;

	glm::vec3 offset = cam.cam_position - cam.cam_target;

	cam.radius = glm::length(offset);

	cam.alpha = acos(offset.y / cam.radius);
	cam.beta = atan2(offset.z, offset.x);
}

void init_obj(const char* objFile, Object& obj) {

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, objFile, "../data/objects/");

	if (!err.empty())
		std::cout << err << std::endl;

	if (!success)
		return;

	obj.vertices.clear();
	obj.indices.clear();

	glm::vec3 min(FLT_MAX);
	glm::vec3 max(-FLT_MAX);

	for (auto& shape : shapes) {

		auto& idx = shape.mesh.indices;

		for (size_t i = 0; i + 2 < idx.size(); i += 3) {

			auto i0 = idx[i + 0];
			auto i1 = idx[i + 1];
			auto i2 = idx[i + 2];

			glm::vec3 p0(attrib.vertices[3 * i0.vertex_index + 0], attrib.vertices[3 * i0.vertex_index + 1], attrib.vertices[3 * i0.vertex_index + 2]);

			glm::vec3 p1(attrib.vertices[3 * i1.vertex_index + 0], attrib.vertices[3 * i1.vertex_index + 1], attrib.vertices[3 * i1.vertex_index + 2]);

			glm::vec3 p2(attrib.vertices[3 * i2.vertex_index + 0], attrib.vertices[3 * i2.vertex_index + 1], attrib.vertices[3 * i2.vertex_index + 2]);

			glm::vec2 uv0(0.0f), uv1(0.0f), uv2(0.0f);

			

			// make sure the texcoord index is valid before accessing the array
			if (i0.texcoord_index >= 0)
				uv0 = { attrib.texcoords[2 * i0.texcoord_index + 0], attrib.texcoords[2 * i0.texcoord_index + 1] };
			if (i1.texcoord_index >= 0)
				uv1 = { attrib.texcoords[2 * i1.texcoord_index + 0], attrib.texcoords[2 * i1.texcoord_index + 1] };
			if (i2.texcoord_index >= 0)
				uv2 = { attrib.texcoords[2 * i2.texcoord_index + 0], attrib.texcoords[2 * i2.texcoord_index + 1] };

			Add_Tri(obj, Vertex{ p0, glm::vec3(0), uv0 }, Vertex{ p1, glm::vec3(0), uv1 }, Vertex{ p2, glm::vec3(0), uv2 });

			//printf("%i", uv0.x);
		}
	}

	for (const Vertex& v : obj.vertices) {

		min = glm::min(min, v.position);
		max = glm::max(max, v.position);
	}

	obj.min = min;
	obj.max = max;

	glm::vec3 size = max - min;

	obj.collider.radius = std::max(size.x, std::max(size.y, size.z)) * 0.5f;
}

bool init_lights(Object& obj) {

	init_obj("../data/objects/sphere.obj", obj);

	return true;
}

void init() {

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	//if(!dont_reload_cam)
		//init_camera();

	init_uniforms();

	init_grid();

	init_skybox(st.skybox.c_str(), true);

	init_viewport();

	read_textures_path();

	ReloadFolder();

	objects.clear();

	lightObjects.clear();

	lights.clear();

	for (auto& p : par.objects) {

		Object obj;

		init_obj(p.mesh.c_str(), obj);

		Build_Normals(obj);
		Build_TBN(obj);
		CreateNormalBuffers(obj);

		buffer(obj);

		obj.id = GenerateEntityID();

		obj.name = p.obj_name;
		obj.mesh = p.mesh;
		obj.tex = p.tex;

		obj.position = p.position;
		obj.rotation = p.rotation;
		obj.scale = p.scale;

		obj.coordType = p.coordType;
		obj.cyaxisType = p.cyaxisType;
		obj.mapping = p.mapping;

		obj.hasNormalMap = p.hasNormalMap;

		obj.max = p.max;
		obj.min = p.min;

		obj.obj_texture = TextureManager::Get(obj.tex);

		//buffer(obj);

		obj.original_vertices = obj.vertices;
		obj.original_indices = obj.indices;

		obj.shininess = p.shininess;

		obj.norm_map = p.normal_map;

		obj.physics = p.physics;
		obj.hasPhysics = p.hasPhysics;

		obj.collider = p.collider;
		obj.hasCollider = p.hasCollider;

		obj.hasMaterial = p.hasMaterial;
		obj.hasMapping = p.hasMapping;

		obj.tiling = p.tiling;

		obj.visible = p.visible;

		obj.model = GetModelMatrix(obj);

		UpdateMapping(obj);

		LoadNormalMap(obj);
		objects.push_back(std::move(obj));
	}

	for (int i = 0; i < par.lights.size() && i < MAX_LIGHTS; i++) { // up to 8 lights

		Light light;

		Object obj;

		if (init_lights(obj)) {

			light.id = GenerateEntityID();

			light.name = par.lights[i].name;

			light.pos = par.lights[i].pos;
			light.dir = par.lights[i].dir;
			light.col = par.lights[i].col;
			light.att = par.lights[i].att;

			light.rotation = par.lights[i].rotation;

			light.ambient = par.lights[i].amb;

			light.bias = par.lights[i].bias;
			light.pcf = par.lights[i].pcf;

			light.inner = par.lights[i].inner;
			light.outer = par.lights[i].outer;
			light.falloff = par.lights[i].falloff;

			light.type = par.lights[i].type;

			if (light.type == "POINT")
				light.typeID = 0;

			else if (light.type == "DIR")
				light.typeID = 1;

			else if (light.type == "SPOT")
				light.typeID = 2;

			obj.id = GenerateEntityID();

			obj.name = light.name;

			obj.position = light.pos;
			obj.rotation = par.lights[i].rotation;
			obj.scale = glm::vec3(20.f);

			obj.visible = par.lights_objects[i].visible;

			obj.model = GetModelMatrix(obj);

			buffer(obj);

			lights.push_back(std::move(light));

			lightObjects.push_back(std::move(obj));
		}
	}
}

bool open_init(const char* settings, const char* scene, Settings& st) {

	selectedObject = -1;

	for (auto& o : objects) {

		o.undoStack.clear();
		o.redoStack.clear();

		DestroyBuffers(o);

		o.vao = 0;
		o.vbo = 0;
		o.ebo = 0;

		o.normalVAO = 0;
		o.normalVBO = 0;

		o.normalTBVAO = 0;
		o.normalTBVBO = 0;

		o.tangentVAO = 0;
		o.tangentVBO = 0;

		o.bitangentVAO = 0;
		o.bitangentVBO = 0;
	}

	for (auto& o : lightObjects) {

		o.undoStack.clear();
		o.redoStack.clear();

		DestroyBuffers(o);

		o.vao = 0;
		o.vbo = 0;
		o.ebo = 0;

		o.normalVAO = 0;
		o.normalVBO = 0;

		o.normalTBVAO = 0;
		o.normalTBVBO = 0;

		o.tangentVAO = 0;
		o.tangentVBO = 0;

		o.bitangentVAO = 0;
		o.bitangentVBO = 0;
	}

	for (auto& o : lights) {

		o.undoStack.clear();
		o.redoStack.clear();
	}

	objects.clear();
	lightObjects.clear();
	lights.clear();

	objectNameCounters.clear();

	if (par.load(settings, scene, st, cam, objects))
		init();

	return true;
}

//--------------------------------------------------------------------------
// main init function

void init_(const char* settings, const char* scene) {

	//st.win_name = "void";
	//st.win_mode = window_mode::WINDOWED;
	//st.w = 1280;
	//st.h = 720;
	//st.VSync = 0;
	//st.fov = 95;
	//
	//par.save(settings, scene, st, cam, objects, lightObjects, lights);

	if (par.load(settings, scene,  st, cam, objects)) {

		windowedSize = { st.w, st.h };

		SDL_Init(SDL_INIT_VIDEO);

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		window = SDL_CreateWindow(st.win_name.c_str(), st.w, st.h, SDL_WINDOW_OPENGL);

		ApplyWindowMode(window);

		context = SDL_GL_CreateContext(window);

		if (!st.VSync)
			SDL_GL_SetSwapInterval(0);
		else
			SDL_GL_SetSwapInterval(1);

		glewExperimental = GL_TRUE;
		GLenum err = glewInit();

		if (err != GLEW_OK) {

			std::cout << glewGetErrorString(err) << '\n';
			return;
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		(void)io;

		io.ConfigWindowsMoveFromTitleBarOnly = true;

		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGuiStyle& style = ImGui::GetStyle();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {

			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui::StyleColorsDark();

		ImGui_ImplSDL3_InitForOpenGL(window, context);
		ImGui_ImplOpenGL3_Init("#version 330");

		create_programs();

		init();
	}
}