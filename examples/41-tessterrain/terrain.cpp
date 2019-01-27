/*
 * Copyright 2015 Andrew Mac. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "common.h"
#include "bgfx_utils.h"
#include "imgui/imgui.h"
#include "camera.h"
#include "bounds.h"
#include <bx/allocator.h>
#include <bx/debug.h>
#include <bx/math.h>
#include <vector>
#include <iostream>

#include "./terrain_definitions.h"
#include "./terrain_extra.h"
// #include "./debug_cube.h"
namespace
{

// static const uint16_t s_terrainSize = 256;

struct PosTexCoord0Vertex {
	float m_x;
	float m_y;
	float m_z;
	float m_u;
	float m_v;


	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
	}

	static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl PosTexCoord0Vertex::ms_decl;

struct TerrainData
{
	int pingPong{0}; // can be 1 or 0 for tracking subdivision buffers.
	void flipPingPong() {
		pingPong = 1 - pingPong;
	}
	PosTexCoord0Vertex   m_vertices[4]{
    {-1.0f, -1.0f, 0.0f, 0.f, 0.0f},
    {+1.0f, -1.0f, 0.0f, 1.f, 0.0f},
    {+1.0f, +1.0f, 0.0f, 0.f, 1.1f},
    {-1.0f, +1.0f, 0.0f, 0.f, 1.0f}
	};
	// uint32_t             m_vertexCount;
	uint16_t             m_indices[6]{
      0, 1, 3,
      2, 3, 1
  };
	float                m_transform[16];

	TerrainData() {
		bx::mtxSRT(m_transform, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	}
	// uint32_t             m_indexCount;
};

// struct BrushData
// {
// 	bool    m_raise;
// 	int32_t m_size;
// 	float   m_power;
// };

// union IndirectCommand {
//   struct {
//       uint32_t num_groups_x,
//           num_groups_y,
//           num_groups_z,
//           align[5];
//   } dispatchIndirect;
//   struct {
//       uint32_t count,
//           primCount,
//           first,
//           baseInstance,
//           align[4];
//   } drawArraysIndirect;
//   struct {
//       uint32_t count,
//           primCount,
//           firstIndex,
//           baseVertex,
//           baseInstance,
//           align[3];
//   } drawElementsIndirect;
// };

class ExampleTerrain : public entry::AppI
{
public:
	ExampleTerrain(const char* _name, const char* _description)
		: entry::AppI(_name, _description)
	{
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);

		m_width  = _width;
		m_height = _height;
		m_debug  = BGFX_DEBUG_TEXT | BGFX_DEBUG_WIREFRAME;
		m_reset  = BGFX_RESET_VSYNC;

		bgfx::Init init;
		init.type     = args.m_type;
		init.vendorId = args.m_pciId;
		init.resolution.width  = m_width;
		init.resolution.height = m_height;
		init.resolution.reset  = m_reset;
		bgfx::init(init);

		// Enable m_debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		bgfx::setViewClear(0
			, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
			, 0x303030ff
			, 1.0f
			, 0
			);

		// Create program from shaders.
		// m_terrainProgram              = loadProgram("vs_terrain",                "fs_terrain");
		// m_terrainHeightTextureProgram = loadProgram("vs_terrain_height_texture", "fs_terrain");

		// Imgui.
		initTerrain();
		// init_debug_cube();
		imguiCreate();



		m_oldWidth  = 0;
		m_oldHeight = 0;
		m_oldReset  = m_reset;

		cameraCreate();

		cameraSetPosition({ 0.0f, 15.0f, -30.0f });
		cameraSetVerticalAngle(-0.40);
		cameraSetHorizontalAngle(0.0);
	}

	void initIndirectBuffers() {
    // atomic_data = { 0, 0, 0, 0, 0, 0, 0, 0 };
    // IndirectCommand cmd = {
    //     2u / COMPUTE_THREAD_COUNT + 1u,
    //     1u, 1u, 0u, 0u, 0u, 0u, 2u
    // };

    const int gpuSubd = 3;
    const int subdLevel = 2 * gpuSubd - 1;
    const uint32_t cnt = subdLevel > 0 ? 6u << subdLevel : 3u;
    // IndirectCommand drawElements = { cnt, 0u, 0u, 0u, 0u, 0u, 0u, 0u };

    auto atomicRef = bgfx::makeRef(&atomic_data, sizeof(atomic_data));
    // auto cmdRef = bgfx::makeRef(&cmd, sizeof(cmd));
    // auto drawRef = bgfx::makeRef(&drawElements, sizeof(drawElements));

    m_dispatchIndirectBuffer = bgfx::createIndirectBuffer(1);
    assert(cnt == 192);
    m_drawIndirectBuffer = bgfx::createIndirectBuffer(cnt);

    atomic_buffer_handle = bgfx::createIndexBuffer(/*BUFFER_BINDING_SUBD_COUNTER*/ atomicRef);
    // bgfx::createIndexBuffer(/*BUFFER_BINDING_INDIRECT_COMMAND*/ cmdRef);
    // bgfx::createIndexBuffer(/*BUFFER_BINDING_CULLED_SUBD_COUNTER*/ drawRef);
	}

	void initTextures() {
		// m_heightTexture.idx = bgfx::kInvalidHandle;
		s_heightTexture = bgfx::createUniform("u_DmapSampler", bgfx::UniformType::Sampler);
		s_textureSmap = bgfx::createUniform("u_SmapSampler", bgfx::UniformType::Sampler);

		loadTextures();

		bgfx::setTexture(0, s_heightTexture, m_heightTexture);
		bgfx::setTexture(1, s_textureSmap, m_textureSmap);
	}

	void initStaticVertexBuffer() {		// Create static vertex buffer.
		static_assert(sizeof(m_terrain.m_vertices) == sizeof(PosTexCoord0Vertex) * 4);
		m_vbh = bgfx::createVertexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(m_terrain.m_vertices, sizeof(m_terrain.m_vertices) )
			, PosTexCoord0Vertex::ms_decl
			);

		// Create static index buffer.
		static_assert(sizeof(m_terrain.m_indices) == sizeof(uint16_t) * 6);
		m_ibh = bgfx::createIndexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(m_terrain.m_indices, sizeof(m_terrain.m_indices) )
			);
	}

	void initTerrain() {
		// Create vertex stream declaration.
		PosTexCoord0Vertex::init();
		Vec2::init();

		loadInstancedGeometryBuffers();
		initStaticVertexBuffer();
		loadSubdivisionBuffers();
		initIndirectBuffers();

		const bgfx::Caps* caps = bgfx::getCaps();
		m_computeSupported  = !!(caps->supported & BGFX_CAPS_COMPUTE);
		m_indirectSupported = !!(caps->supported & BGFX_CAPS_DRAW_INDIRECT);

		assert(m_computeSupported);
		assert(m_indirectSupported);
		if (!m_computeSupported) return;


		// Create tesselation program from shaders.
		cs_lod_program = loadProgram("cs_terrain_lod", NULL);
		cs_indirect_program = loadProgram("cs_terrain_updateIndirect", NULL);
		terrain_render_program = loadProgram("vs_terrain_cs_render", "fs_terrain_cs_render");

		initTextures();
	}

	void renderCompute() {

		if (m_terrain.pingPong == 0) {
			// flip the two subd buffer handles back and forth.
	    bgfx::setBuffer(BUFFER_BINDING_SUBD1, subd1_buffer_handle, bgfx::Access::ReadWrite);
			bgfx::setBuffer(BUFFER_BINDING_SUBD2, subd2_buffer_handle, bgfx::Access::ReadWrite);
		} else {
			bgfx::setBuffer(BUFFER_BINDING_SUBD2, subd1_buffer_handle, bgfx::Access::ReadWrite);
	    bgfx::setBuffer(BUFFER_BINDING_SUBD1, subd2_buffer_handle, bgfx::Access::ReadWrite);
		}
		bgfx::setBuffer(BUFFER_BINDING_CULLED_SUBD, culled_buffer_handle, bgfx::Access::ReadWrite);
		bgfx::setBuffer(BUFFER_BINDING_INDIRECT_COMMAND, m_dispatchIndirectBuffer, bgfx::Access::ReadWrite);
		bgfx::setBuffer(BUFFER_BINDING_SUBD_COUNTER, atomic_buffer_handle, bgfx::Access::ReadWrite);
		bgfx::setBuffer(BUFFER_BINDING_CULLED_SUBD_COUNTER, m_drawIndirectBuffer, bgfx::Access::ReadWrite);
		bgfx::setBuffer(BUFFER_BINDING_GEOMETRY_VERTICES, m_vbh, bgfx::Access::Read);
		bgfx::setBuffer(BUFFER_BINDING_GEOMETRY_INDEXES, m_ibh, bgfx::Access::Read);

		// bgfx::dispatch(0, cs_lod_program, m_dispatchIndirectBuffer);
		bgfx::dispatch(0, cs_lod_program, COMPUTE_THREAD_COUNT);
	}

	void renderRender() {

		// debug simple
		// bgfx::setVertexBuffer(0, m_vbh);
		// bgfx::setIndexBuffer(m_ibh);

		bgfx::setVertexBuffer(0, instanced_vbh);
		bgfx::setIndexBuffer(instanced_ibh);
		// bgfx::submit(0, terrain_render_program);
		// std::cout << "Submit " << std::endl;

		bgfx::submit(0, terrain_render_program, m_drawIndirectBuffer, 0 /* start*/);
	}

	void renderIndirect() {

		bgfx::setBuffer(BUFFER_BINDING_SUBD_COUNTER, atomic_buffer_handle, bgfx::Access::ReadWrite);
		bgfx::setBuffer(BUFFER_BINDING_CULLED_SUBD_COUNTER, m_drawIndirectBuffer, bgfx::Access::ReadWrite);
		bgfx::setBuffer(BUFFER_BINDING_INDIRECT_COMMAND, m_dispatchIndirectBuffer, bgfx::Access::ReadWrite);
		bgfx::setBuffer(BUFFER_BINDING_GEOMETRY_VERTICES, m_vbh, bgfx::Access::Read);
		bgfx::setBuffer(BUFFER_BINDING_GEOMETRY_INDEXES, m_ibh, bgfx::Access::Read);

		bgfx::dispatch(0, cs_indirect_program, 1, 1, 1);
	}

	void updateTerrain() {

		m_params[0] = 5.f; // dmap factor
		m_params[1] = 0.0247889;  // lod factor

		u_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4);
		bgfx::setUniform(u_params, m_params);

		renderCompute();
		renderRender();
		// renderIndirect();
		m_terrain.flipPingPong();


		// Submit primitive for rendering to view 0.
		// if (m_useIndirect)
		// {
		// 	bgfx::submit(0, terrain_render_program, m_dispatchIndirectBuffer, 0);
		// }
		// else
		// {
		// 	bgfx::submit(0, terrain_render_program);
		// }
	}

	virtual int shutdown() override
	{
		// Cleanup.
		cameraDestroy();
		imguiDestroy();

		if (bgfx::isValid(m_ibh) )
		{
			bgfx::destroy(m_ibh);
		}

		if (bgfx::isValid(m_vbh) )
		{
			bgfx::destroy(m_vbh);
		}

		// if (bgfx::isValid(m_dibh) )
		// {
		// 	bgfx::destroy(m_dibh);
		// }

		// if (bgfx::isValid(m_dvbh) )
		// {
		// 	bgfx::destroy(m_dvbh);
		// }

		bgfx::destroy(s_heightTexture);

		if (bgfx::isValid(m_heightTexture) )
		{
			bgfx::destroy(m_heightTexture);
		}

		// bgfx::destroy(m_terrainProgram);
		// bgfx::destroy(m_terrainHeightTextureProgram);

		/// When data is passed to bgfx via makeRef we need to make
		/// sure library is done with it before freeing memory blocks.
		bgfx::frame();

		// bx::AllocatorI* allocator = entry::getAllocator();
		// BX_FREE(allocator, m_terrain.m_vertices);
		// BX_FREE(allocator, m_terrain.m_indices);
		// BX_FREE(allocator, m_terrain.m_heightMap);

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}


void loadSmapTexture(const uint16_t* texels, int w = 4096, int h = 4096) {
  std::vector<float> smap(w * h * 2);

	for (int j = 0; j < h; ++j) {
	  for (int i = 0; i < w; ++i) {
	    int i1 = std::max(0, i - 1);
	    int i2 = std::min(w - 1, i + 1);
	    int j1 = std::max(0, j - 1);
	    int j2 = std::min(h - 1, j + 1);
	    uint16_t px_l = texels[i1 + w * j]; // in [0,2^16-1]
	    uint16_t px_r = texels[i2 + w * j]; // in [0,2^16-1]
	    uint16_t px_b = texels[i + w * j1]; // in [0,2^16-1]
	    uint16_t px_t = texels[i + w * j2]; // in [0,2^16-1]
	    float z_l = (float)px_l / 65535.0f; // in [0, 1]
	    float z_r = (float)px_r / 65535.0f; // in [0, 1]
	    float z_b = (float)px_b / 65535.0f; // in [0, 1]
	    float z_t = (float)px_t / 65535.0f; // in [0, 1]
	    float slope_x = (float)w * 0.5f * (z_r - z_l);
	    float slope_y = (float)h * 0.5f * (z_t - z_b);

	    smap[2 * (i + w * j)] = slope_x;
	    smap[1 + 2 * (i + w * j)] = slope_y;
	  }
	}
}

void loadInstancedGeometryBuffers() {
	   // auto vertices = verticesL3;
	   // auto indexes = indexesL3;

	   static_assert(instancedMeshVertexCount * sizeof(float[2]) == sizeof(verticesL3));
	   static_assert(instancedMeshPrimitiveCount * 3 * sizeof(uint16_t) == sizeof(indexesL3));
	   // auto verticesMem = bgfx::makeRef(vertices, sizeof(vertices));
	   // auto indicesMem = bgfx::makeRef(vertices, sizeof(indexes));

	   auto flags = BGFX_BUFFER_COMPUTE_READ_WRITE;

	   // std::cout << "? " << std::endl;

	   instanced_vbh = bgfx::createVertexBuffer(
	   	bgfx::makeRef(verticesL3, sizeof(verticesL3)),
	   	Vec2::ms_decl, flags);

	   instanced_ibh = bgfx::createIndexBuffer(
	   	bgfx::makeRef(indexesL3, sizeof(indexesL3)),
	   	flags);

	   // std::cout << "aa? " << std::endl;
	   // bgfx::update(instanced_ibh, 0, verticesMem);
	   // bgfx::update(instanced_vbh, 0, indicesMem);
}

void loadTextures() {
	// m_heightTexture = loadTexture("textures/heightmap.png");

  // std::vector<uint16_t>	m_downloadBuffer(4096 * 4096 * 4);
  // bgfx::readTexture(m_heightTexture, &m_downloadBuffer[0]);

	const uint64_t tsFlags = 0
		| BGFX_TEXTURE_RT
		| BGFX_SAMPLER_U_CLAMP
		| BGFX_SAMPLER_V_CLAMP
		;

  m_heightTexture = bgfx::createTexture2D(4096, 4096, false, 1, bgfx::TextureFormat::BGRA8, tsFlags);

  m_textureSmap = bgfx::createTexture2D(4096, 4096, false, 1, bgfx::TextureFormat::BGRA8, tsFlags);

	// auto mem = bgfx::makeRef(&m_terrain.m_heightMap[0], sizeof(uint8_t) * s_terrainSize * s_terrainSize);
  // bgfx::updateTexture2D(m_textureSmap, 0, 0, 0, 4096, 4096, mem);
}

auto loadSubdBuffer(size_t bufferCapacity) {
	const uint32_t initial_data[] = { 0, 1, 1, 1 };

	auto handle = bgfx::createDynamicIndexBuffer(bufferCapacity);
	auto mem = bgfx::makeRef(initial_data, sizeof(initial_data));
	bgfx::update(handle, 0, mem);
	return handle;
	// bgfx::setBuffer(id, handle, bgfx::Access::Write);
}

void loadSubdivisionBuffers() {
  // LOG("Loading {Subd-Buffer}\n");
  const size_t bufferCapacity = 1 << 28;

  subd1_buffer_handle = loadSubdBuffer(bufferCapacity);
  subd2_buffer_handle = loadSubdBuffer(bufferCapacity);
  // if (g_terrain.method == METHOD_CS) {
  if (true) {
    // LOG("Loading {Culled-Subd-Buffer}\n");
    culled_buffer_handle = loadSubdBuffer(bufferCapacity);
    //loadSubdBuffer(BUFFER_CULLED_SUBD2, bufferCapacity);
  }
}

	bool update() override
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
		{
			int64_t now = bx::getHPCounter();
			static int64_t last = now;
			const int64_t frameTime = now - last;
			last = now;
			const double freq = double(bx::getHPFrequency() );
			const float deltaTime = float(frameTime/freq);

			imguiBeginFrame(m_mouseState.m_mx
				,  m_mouseState.m_my
				, (m_mouseState.m_buttons[entry::MouseButton::Left  ] ? IMGUI_MBUT_LEFT   : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Right ] ? IMGUI_MBUT_RIGHT  : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
				,  m_mouseState.m_mz
				, uint16_t(m_width)
				, uint16_t(m_height)
				);

			showExampleDialog(this);

			ImGui::SetNextWindowPos(
				  ImVec2(m_width - m_width / 5.0f - 10.0f, 10.0f)
				, ImGuiCond_FirstUseEver
				);
			ImGui::SetNextWindowSize(
				  ImVec2(m_width / 5.0f, m_height / 3.0f)
				, ImGuiCond_FirstUseEver
				);
			ImGui::Begin("Settings"
				, NULL
				, 0
				);

			ImGui::Separator();

			ImGui::Separator();

			// ImGui::Checkbox("Raise Terrain", &m_brush.m_raise);

			// ImGui::SliderInt("Brush Size", &m_brush.m_size, 1, 50);
			// ImGui::SliderFloat("Brush Power", &m_brush.m_power, 0.0f, 1.0f);

			ImGui::End();
			imguiEndFrame();

			if (!ImGui::MouseOverArea() )
			{
				// Update camera.
				cameraUpdate(deltaTime, m_mouseState);

			}


			// Set view 0 default viewport.
			bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height) );

			cameraGetViewMtx(m_viewMtx);
			bx::mtxProj(m_projMtx, 60.0f, float(m_width) / float(m_height), 0.1f, 2000.0f, bgfx::getCaps()->homogeneousDepth);

			bgfx::setViewTransform(0, m_viewMtx, m_projMtx);
			bgfx::setTransform(m_terrain.m_transform);

			updateTerrain();
			// render_debug_cube();
			bgfx::touch(0);
			bgfx::dbgTextClear();
			bgfx::dbgTextPrintf(0, 1, 0x0f, "Color can be changed with ANSI \x1b[9;me\x1b[10;ms\x1b[11;mc\x1b[12;ma\x1b[13;mp\x1b[14;me\x1b[0m code too.");
			// Advance to next frame. Rendering thread will be kicked to
			// process submitted rendering primitives.
			bgfx::frame();

			return true;
		}

		return false;
	}

	bgfx::VertexBufferHandle m_vbh;
	bgfx::IndexBufferHandle  m_ibh;
	bgfx::VertexBufferHandle instanced_vbh;
	bgfx::IndexBufferHandle instanced_ibh;

	bgfx::DynamicIndexBufferHandle subd1_buffer_handle;
	bgfx::DynamicIndexBufferHandle subd2_buffer_handle;
	bgfx::DynamicIndexBufferHandle culled_buffer_handle;

	int atomic_data[8]{ 0, 0, 0, 0, 0, 0, 0, 0 };
	bgfx::IndexBufferHandle atomic_buffer_handle;

	bgfx::UniformHandle u_params;
	bgfx::UniformHandle s_heightTexture;
	bgfx::TextureHandle m_heightTexture;
	bgfx::UniformHandle s_textureSmap;
	bgfx::TextureHandle m_textureSmap;

	bgfx::IndirectBufferHandle m_dispatchIndirectBuffer;
	bgfx::IndirectBufferHandle m_drawIndirectBuffer;

	bgfx::ProgramHandle cs_lod_program;
	bgfx::ProgramHandle cs_indirect_program;
	bgfx::ProgramHandle terrain_render_program;

	bool m_useIndirect;
	bool m_computeSupported;
	bool m_indirectSupported;

	float m_params[4];

	float m_viewMtx[16];
	float m_projMtx[16];

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;

	uint32_t m_oldWidth;
	uint32_t m_oldHeight;
	uint32_t m_oldReset;

	TerrainData m_terrain;
	// BrushData m_brush;

	entry::MouseState m_mouseState;

};

} // namespace

ENTRY_IMPLEMENT_MAIN(ExampleTerrain, "41-tessterrain", "Terrain compute example.");
