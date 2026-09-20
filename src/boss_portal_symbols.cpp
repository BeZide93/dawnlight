#include "boss_portal_symbols.hpp"
#include "service_imports.hpp"
#include "generated/boss_portal_art.hpp"
#include "f_op/f_op_view.h"
#include "d/d_com_inf_game.h"
#include "mods/svc/hook.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace dawnlight {
namespace {
DEFINE_HOOK(&dComIfGd_drawXluListDark, MirrorTranslucentEndHook);
static_assert(kBossPortalSymbolCount == portal_art::kCount);
struct Vertex { float clip[4], uv[2], color[4], face[2]; };
struct Draw {
    GfxRange vertices;
    uint32_t count;
    WGPURenderPipeline pipeline;
    WGPUBindGroup bindings;
};
static_assert(sizeof(Draw) <= GFX_INLINE_DRAW_PAYLOAD_SIZE);
struct Pipeline { uint64_t layout; bool reversed; WGPURenderPipeline handle; };
GfxDrawTypeHandle sDraw = 0;
GfxStageHookHandle sStage = 0;
GfxStageHookHandle sFrameEnd = 0;
bool sTranslucentHook = false;
Draw sPendingDraw{};
bool sHasPendingDraw = false;
WGPUTexture sTexture = nullptr;
WGPUTextureView sTextureView = nullptr;
WGPUSampler sSampler = nullptr;
WGPUBindGroupLayout sBindingLayout = nullptr;
WGPUPipelineLayout sPipelineLayout = nullptr;
WGPUBindGroup sBindings = nullptr;
WGPUShaderModule sShader = nullptr;
std::vector<Pipeline> sPipelines;
bool sFailed = false;

constexpr char kShader[] = R"(
struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
    @location(1) color: vec4f,
    @location(2) face: vec2f,
};
@group(0) @binding(0) var mask: texture_2d<f32>;
@group(0) @binding(1) var maskSampler: sampler;
@vertex fn vs(@location(0) position: vec4f, @location(1) uv: vec2f,
              @location(2) color: vec4f, @location(3) face: vec2f) -> VertexOut {
    var out: VertexOut;
    out.position = position;
    out.uv = uv;
    out.color = color;
    out.face = face;
    return out;
}
@fragment fn fs(in: VertexOut) -> @location(0) vec4f {
    let ink = textureSample(mask, maskSampler, in.uv).r;
    let radius = length(in.face - vec2f(0.5));
    let disc = 1.0 - smoothstep(0.485, 0.5, radius);
    if (disc < 0.001) { discard; }
    // Opaque black ink OVER translucent colored glass. Composite locally in
    // premultiplied form, then return straight alpha for the pipeline blend.
    // This keeps the silhouette black while the mirror shows through its gaps.
    let glassAlpha = (1.0 - ink) * in.color.a;
    let alpha = ink + glassAlpha;
    return vec4f(in.color.rgb * glassAlpha / max(alpha, 0.001), disc * alpha);
}
)";

WGPUStringView label(const char* value) { return {value, std::strlen(value)}; }

// Decode bounded count/value pairs. Invalid artwork disables only this feature.
bool decode_mask(std::vector<uint8_t>& pixels) {
    constexpr size_t size = portal_art::kWidth * portal_art::kHeight;
    pixels.clear();
    pixels.reserve(size);
    uint32_t bits = 0;
    unsigned bitCount = 0, run = 0;
    bool hasCount = false;
    for (const char* chunk : portal_art::kMaskRleBase64) {
        for (const char* p = chunk; *p; ++p) {
            const char c = *p;
            if (c == '=') continue;
            int value = c >= 'A' && c <= 'Z' ? c-'A' :
                        c >= 'a' && c <= 'z' ? c-'a'+26 :
                        c >= '0' && c <= '9' ? c-'0'+52 : c == '+' ? 62 : c == '/' ? 63 : -1;
            if (value < 0) return false;
            bits = (bits << 6) | static_cast<unsigned>(value);
            bitCount += 6;
            if (bitCount < 8) continue;
            bitCount -= 8;
            const uint8_t byte = static_cast<uint8_t>(bits >> bitCount);
            if (!hasCount) {
                run = byte;
                if (run == 0) return false;
            } else {
                if (pixels.size() + run > size) return false;
                pixels.insert(pixels.end(), run, byte);
            }
            hasCount = !hasCount;
        }
    }
    return !hasCount && pixels.size() == size;
}

bool create_art(const GfxDeviceInfo& device) {
    std::vector<uint8_t> pixels;
    if (!decode_mask(pixels)) return false;
    WGPUTextureDescriptor texture = WGPU_TEXTURE_DESCRIPTOR_INIT;
    texture.label = label("Dawnlight supplied boss icons");
    texture.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    texture.dimension = WGPUTextureDimension_2D;
    texture.size = {portal_art::kWidth, portal_art::kHeight, 1};
    texture.format = WGPUTextureFormat_R8Unorm;
    texture.mipLevelCount = 5;
    sTexture = wgpuDeviceCreateTexture(device.device, &texture);
    if (!sTexture) return false;
    unsigned width = portal_art::kWidth, height = portal_art::kHeight;
    for (unsigned level = 0; level < 5; ++level) {
        WGPUTexelCopyTextureInfo target = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
        target.texture = sTexture;
        target.mipLevel = level;
        WGPUTexelCopyBufferLayout source = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
        source.bytesPerRow = width;
        source.rowsPerImage = height;
        const WGPUExtent3D extent = {width, height, 1};
        wgpuQueueWriteTexture(device.queue, &target, pixels.data(), pixels.size(), &source, &extent);
        if (level == 4) break;
        std::vector<uint8_t> next(width/2 * (height/2));
        for (unsigned y = 0; y < height/2; ++y) {
            for (unsigned x = 0; x < width/2; ++x) {
                const unsigned p = y*2*width + x*2;
                next[y*(width/2)+x] = (pixels[p]+pixels[p+1]+pixels[p+width]+pixels[p+width+1]+2)/4;
            }
        }
        width /= 2;
        height /= 2;
        pixels.swap(next);
    }
    sTextureView = wgpuTextureCreateView(sTexture, nullptr);
    WGPUSamplerDescriptor sampler = WGPU_SAMPLER_DESCRIPTOR_INIT;
    sampler.magFilter = sampler.minFilter = WGPUFilterMode_Linear;
    sampler.mipmapFilter = WGPUMipmapFilterMode_Linear;
    sampler.lodMaxClamp = 4;
    sSampler = wgpuDeviceCreateSampler(device.device, &sampler);
    WGPUBindGroupLayoutEntry entries[2] = {WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT, WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT};
    entries[0].binding = 0;
    entries[0].visibility = WGPUShaderStage_Fragment;
    entries[0].texture.sampleType = WGPUTextureSampleType_Float;
    entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[1].binding = 1;
    entries[1].visibility = WGPUShaderStage_Fragment;
    entries[1].sampler.type = WGPUSamplerBindingType_Filtering;
    WGPUBindGroupLayoutDescriptor layout = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
    layout.entryCount = 2;
    layout.entries = entries;
    sBindingLayout = wgpuDeviceCreateBindGroupLayout(device.device, &layout);
    WGPUPipelineLayoutDescriptor pipelineLayout = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    pipelineLayout.bindGroupLayoutCount = 1;
    pipelineLayout.bindGroupLayouts = &sBindingLayout;
    sPipelineLayout = wgpuDeviceCreatePipelineLayout(device.device, &pipelineLayout);
    WGPUBindGroupEntry bindings[2] = {WGPU_BIND_GROUP_ENTRY_INIT, WGPU_BIND_GROUP_ENTRY_INIT};
    bindings[0].binding = 0;
    bindings[0].textureView = sTextureView;
    bindings[1].binding = 1;
    bindings[1].sampler = sSampler;
    WGPUBindGroupDescriptor group = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    group.layout = sBindingLayout;
    group.entryCount = 2;
    group.entries = bindings;
    sBindings = wgpuDeviceCreateBindGroup(device.device, &group);
    WGPUShaderSourceWGSL wgsl = WGPU_SHADER_SOURCE_WGSL_INIT;
    wgsl.code = label(kShader);
    WGPUShaderModuleDescriptor shader = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    shader.nextInChain = &wgsl.chain;
    sShader = wgpuDeviceCreateShaderModule(device.device, &shader);
    return sTextureView && sSampler && sBindingLayout && sPipelineLayout && sBindings && sShader;
}

WGPURenderPipeline pipeline_for(const GfxDeviceInfo& device, const GfxRenderTargetLayout& layout) {
    for (const auto& pipeline : sPipelines) {
        if (pipeline.layout == layout.key && pipeline.reversed == device.uses_reversed_z) return pipeline.handle;
    }
    WGPUVertexAttribute attributes[4] = {WGPU_VERTEX_ATTRIBUTE_INIT, WGPU_VERTEX_ATTRIBUTE_INIT, WGPU_VERTEX_ATTRIBUTE_INIT, WGPU_VERTEX_ATTRIBUTE_INIT};
    attributes[0].format = WGPUVertexFormat_Float32x4;
    attributes[0].offset = offsetof(Vertex, clip);
    attributes[1].format = WGPUVertexFormat_Float32x2;
    attributes[1].offset = offsetof(Vertex, uv);
    attributes[2].format = WGPUVertexFormat_Float32x4;
    attributes[2].offset = offsetof(Vertex, color);
    attributes[3].format = WGPUVertexFormat_Float32x2;
    attributes[3].offset = offsetof(Vertex, face);
    for (unsigned i = 0; i < 4; ++i) attributes[i].shaderLocation = i;
    WGPUVertexBufferLayout buffer = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
    buffer.arrayStride = sizeof(Vertex);
    buffer.attributeCount = 4;
    buffer.attributes = attributes;
    WGPUBlendState blend = WGPU_BLEND_STATE_INIT;
    blend.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blend.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blend.alpha.srcFactor = WGPUBlendFactor_One;
    blend.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    WGPUColorTargetState targets[GFX_MAX_COLOR_ATTACHMENTS];
    WGPUFragmentState fragment = WGPU_FRAGMENT_STATE_INIT;
    fragment.module = sShader;
    fragment.entryPoint = label("fs");
    fragment.targetCount = gfx_init_color_target_states(&layout, targets, &blend, WGPUColorWriteMask_All);
    fragment.targets = targets;
    WGPUDepthStencilState depth = WGPU_DEPTH_STENCIL_STATE_INIT;
    depth.format = layout.depth_stencil_format;
    // A translucent tint must not occlude later game materials/effects.
    depth.depthWriteEnabled = WGPUOptionalBool_False;
    depth.depthCompare = device.uses_reversed_z ? WGPUCompareFunction_GreaterEqual : WGPUCompareFunction_LessEqual;
    WGPURenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    desc.label = label("Dawnlight boss portal symbols");
    desc.layout = sPipelineLayout;
    desc.vertex.module = sShader;
    desc.vertex.entryPoint = label("vs");
    desc.vertex.bufferCount = 1;
    desc.vertex.buffers = &buffer;
    desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    desc.primitive.cullMode = WGPUCullMode_None;
    desc.depthStencil = depth.format == WGPUTextureFormat_Undefined ? nullptr : &depth;
    desc.multisample.count = layout.sample_count;
    desc.fragment = &fragment;
    auto pipeline = wgpuDeviceCreateRenderPipeline(device.device, &desc);
    if (pipeline) sPipelines.push_back({layout.key, device.uses_reversed_z, pipeline});
    return pipeline;
}

void draw(ModContext*, const GfxDrawContext* context, const void* data, size_t size, void*) {
    if (size != sizeof(Draw)) return;
    Draw batch;
    std::memcpy(&batch, data, sizeof(batch));
    wgpuRenderPassEncoderSetPipeline(context->pass, batch.pipeline);
    wgpuRenderPassEncoderSetBindGroup(context->pass, 0, batch.bindings, 0, nullptr);
    wgpuRenderPassEncoderSetVertexBuffer(context->pass, 0, context->vertex_buffer, batch.vertices.offset, batch.vertices.size);
    wgpuRenderPassEncoderDraw(context->pass, batch.count, 1, 0, 0);
}

void discard_pending_draw(ModContext*, const GfxStageContext*, void*) {
    // Stream ranges are frame-local. Never retain one if a debug/alternate
    // scene path skipped the normal translucent pass.
    sHasPendingDraw = false;
}

void after_translucent(ModContext*, void*, void*, void*) {
    if (!sHasPendingDraw) return;
    sHasPendingDraw = false;
    // The pinned SDK's last world-camera stage is BEFORE the XLU lists. Submit
    // after both normal and dark XLU materials, before post-processing/HUD.
    // GfxService::push_draw flushes the GX queue before inserting our draw.
    svc_gfx->push_draw(mod_ctx, sDraw, &sPendingDraw, sizeof(sPendingDraw));
}

void stage(ModContext*, const GfxStageContext* context, void*) {
    sHasPendingDraw = false;
    if (sFailed || !context->game_view) return;
    const auto& view = *static_cast<const view_class*>(context->game_view);
    struct Symbol {
        unsigned index;
        BossPortalSymbolSurface surface;
        float cameraZ;
        bool defeated;
    };
    std::array<Symbol, kBossPortalSymbolCount> symbols;
    unsigned count = 0;
    for (unsigned index = 0; index < kBossPortalSymbolCount; ++index) {
        BossPortalSymbolSurface surface;
        bool defeated;
        if (!get_boss_portal_symbol(index, surface, defeated)) continue;
        const auto& n = surface.normal;
        const cXyz eye = view.lookat.eye - surface.center;
        // The label is permanently mounted on the inward-facing front plane.
        // Looking from behind hides it; the camera never changes its transform.
        if (eye.x*n.x + eye.y*n.y + eye.z*n.z <= 0.0f) continue;
        const auto& c = surface.center;
        const float z = view.viewMtx[2][0]*c.x + view.viewMtx[2][1]*c.y +
                        view.viewMtx[2][2]*c.z + view.viewMtx[2][3];
        const auto& r = surface.right;
        const auto& u = surface.up;
        const float radius = 0.5f*(std::abs(view.viewMtx[2][0]*r.x + view.viewMtx[2][1]*r.y + view.viewMtx[2][2]*r.z) +
                                  std::abs(view.viewMtx[2][0]*u.x + view.viewMtx[2][1]*u.y + view.viewMtx[2][2]*u.z));
        if (z-radius >= -view.near_) continue;
        symbols[count++] = {index, surface, z, defeated};
    }
    if (!count) return;
    GfxDeviceInfo device = GFX_DEVICE_INFO_INIT;
    GfxRenderTargetLayout layout = GFX_RENDER_TARGET_LAYOUT_INIT;
    if (svc_gfx->get_device_info(mod_ctx, &device) != MOD_OK ||
        svc_gfx->get_scene_target_layout(mod_ctx, &layout) != MOD_OK || !device.device) return;
    if (!sTexture && !create_art(device)) {
        sFailed = true;
        svc_log->warn(mod_ctx, "Boss portal symbols: artwork initialization failed");
        return;
    }
    const auto pipeline = pipeline_for(device, layout);
    if (!pipeline) {
        sFailed = true;
        svc_log->warn(mod_ctx, "Boss portal symbols: pipeline initialization failed");
        return;
    }
    std::sort(symbols.begin(), symbols.begin()+count, [](const Symbol& a, const Symbol& b) { return a.cameraZ < b.cameraZ; });
    std::array<Vertex, kBossPortalSymbolCount*6> vertices;
    unsigned vertexCount = 0;
    constexpr float corners[6][2] = {{0,0},{1,0},{0,1},{0,1},{1,0},{1,1}};
    for (unsigned i = 0; i < count; ++i) {
        const auto& symbol = symbols[i];
        for (const auto& corner : corners) {
            auto& vertex = vertices[vertexCount++];
            const auto& face = symbol.surface;
            const cXyz world = face.center + face.right*(corner[0]-0.5f) + face.up*(0.5f-corner[1]);
            float camera[3];
            for (unsigned row = 0; row < 3; ++row) {
                camera[row] = view.viewMtx[row][0]*world.x + view.viewMtx[row][1]*world.y +
                              view.viewMtx[row][2]*world.z + view.viewMtx[row][3];
            }
            for (unsigned row = 0; row < 4; ++row) {
                vertex.clip[row] = view.projMtx[row][0]*camera[0] + view.projMtx[row][1]*camera[1] +
                                   view.projMtx[row][2]*camera[2] + view.projMtx[row][3];
            }
            vertex.face[0] = corner[0];
            vertex.face[1] = corner[1];
            // GX clip depth is [-w, 0]. Match Aurora's projection conversion.
            vertex.clip[2] = device.uses_reversed_z ? -vertex.clip[2] : vertex.clip[2]+vertex.clip[3];
            vertex.uv[0] = (symbol.index%portal_art::kColumns + corner[0])*portal_art::kCell / portal_art::kWidth;
            vertex.uv[1] = (symbol.index/portal_art::kColumns + corner[1])*portal_art::kCell / portal_art::kHeight;
            vertex.color[0] = symbol.defeated ? 1.0f : 0.0f;
            vertex.color[1] = symbol.defeated ? 0.30f : 0.0f;
            vertex.color[2] = symbol.defeated ? 0.25f : 1.0f;
            vertex.color[3] = 0.30f;
        }
    }
    Draw batch = {{}, vertexCount, pipeline, sBindings};
    if (svc_gfx->push_verts(mod_ctx, vertices.data(), vertexCount*sizeof(Vertex), alignof(Vertex), &batch.vertices) == MOD_OK) {
        sPendingDraw = batch;
        sHasPendingDraw = true;
    }
}
} // namespace

void initialize_boss_portal_symbols() {
    if (sStage || !svc_gfx || !svc_hook) return;
    const GfxDrawTypeDesc drawDesc = {sizeof(GfxDrawTypeDesc), "Dawnlight boss symbols", draw, nullptr};
    if (svc_gfx->register_draw_type(mod_ctx, &drawDesc, &sDraw) != MOD_OK) {
        svc_log->warn(mod_ctx, "Boss portal symbols: unable to register drawing");
        return;
    }
    const GfxStageHookDesc stageDesc = {sizeof(GfxStageHookDesc), stage, nullptr};
    if (svc_gfx->register_stage_hook(mod_ctx, GFX_STAGE_SCENE_AFTER_OPAQUE, &stageDesc, &sStage) != MOD_OK) {
        shutdown_boss_portal_symbols();
        svc_log->warn(mod_ctx, "Boss portal symbols: unable to register scene hook");
        return;
    }
    const GfxStageHookDesc endDesc = {sizeof(GfxStageHookDesc), discard_pending_draw, nullptr};
    if (svc_gfx->register_stage_hook(mod_ctx, GFX_STAGE_FRAME_BEFORE_HUD, &endDesc, &sFrameEnd) != MOD_OK) {
        shutdown_boss_portal_symbols();
        svc_log->warn(mod_ctx, "Boss portal symbols: unable to register frame cleanup");
        return;
    }
    if (mods::hook::add_post<MirrorTranslucentEndHook>(svc_hook, after_translucent) != MOD_OK) {
        // add_post may have installed its trampoline before failing.
        mods::hook::uninstall<MirrorTranslucentEndHook>(svc_hook);
        shutdown_boss_portal_symbols();
        svc_log->warn(mod_ctx, "Boss portal symbols: unable to register translucent draw hook");
        return;
    }
    sTranslucentHook = true;
}

void shutdown_boss_portal_symbols() {
    sHasPendingDraw = false;
    if (sTranslucentHook) mods::hook::uninstall<MirrorTranslucentEndHook>(svc_hook);
    sTranslucentHook = false;
    if (sFrameEnd) svc_gfx->unregister_stage_hook(mod_ctx, sFrameEnd);
    if (sStage) svc_gfx->unregister_stage_hook(mod_ctx, sStage);
    if (sDraw) svc_gfx->unregister_draw_type(mod_ctx, sDraw);
    sFrameEnd = sStage = sDraw = 0;
    for (const auto& pipeline : sPipelines) wgpuRenderPipelineRelease(pipeline.handle);
    sPipelines.clear();
    if (sBindings) wgpuBindGroupRelease(sBindings);
    if (sPipelineLayout) wgpuPipelineLayoutRelease(sPipelineLayout);
    if (sBindingLayout) wgpuBindGroupLayoutRelease(sBindingLayout);
    if (sShader) wgpuShaderModuleRelease(sShader);
    if (sSampler) wgpuSamplerRelease(sSampler);
    if (sTextureView) wgpuTextureViewRelease(sTextureView);
    if (sTexture) wgpuTextureRelease(sTexture);
    sBindings = nullptr;
    sPipelineLayout = nullptr;
    sBindingLayout = nullptr;
    sShader = nullptr;
    sSampler = nullptr;
    sTextureView = nullptr;
    sTexture = nullptr;
    sFailed = false;
}
} // namespace dawnlight
