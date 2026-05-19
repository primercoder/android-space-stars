#include <SDL.h>
#include <GLES2/gl2.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <vector>
#include <algorithm>
#include <random>
#include <cmath>
#include <cstdio>
#include <cstring>

// ====================== PHYSICS CONSTANTS ======================
static const float G = 0.1f;
static size_t initial_nstars = 0;
static float aspect = 1.0f;
static constexpr size_t INTEGRATOR_SUBSTEPS = 50;
static const float speed_scale = 0.85f;
static const float merge_distance = 0.035f;
static float mouse_speed_factor = 1.2f;
static const float trail_max_age = 0.6f;
static const int trail_max_points_per_star = 2000;
static const float trail_min_dist = 0.006f;

static std::mt19937 rng;
static size_t total_stars_ever = 0;

// ====================== DATA STRUCTURES ======================
struct Star {
    glm::vec2 pos;
    glm::vec2 vel;
    float mass;
    glm::vec3 color;
};

using StarVector = std::vector<Star>;
static StarVector stars;

struct TrailPoint {
    glm::vec2 pos;
    float age;
};

struct Trail {
    std::vector<TrailPoint> points;
    glm::vec3 color;
};

static std::vector<Trail> trails;
static std::vector<Trail> orphan_trails;

// ====================== SIMULATION STATE ======================
static bool paused = false;
static bool show_grid = false;
static bool running = true;
static glm::vec2 camera_offset = glm::vec2(0.0f, 0.0f);

enum class Mode { LAUNCH, MOVE };
static Mode current_mode = Mode::LAUNCH;

// ====================== 5x7 BITMAP FONT ======================
static const uint8_t glyphs[128][7] = {
    {0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},
    {0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x04,0x04,0x04,0x04,0x00,0x00,0x04},
    {0},{0},{0},
    {0x10,0x12,0x04,0x08,0x12,0x02,0x00},
    {0},{0},{0},{0},
    {0x00,0x04,0x15,0x0E,0x15,0x04,0x00},
    {0x00,0x04,0x04,0x1F,0x04,0x04,0x00},
    {0x00,0x00,0x00,0x00,0x04,0x04,0x08},
    {0x00,0x00,0x00,0x1F,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x04},
    {0x01,0x02,0x02,0x04,0x08,0x08,0x10},
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x1F},
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
    {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E},
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
    {0x00,0x00,0x04,0x00,0x00,0x04,0x00},
    {0},{0},{0},{0},
    {0x0E,0x11,0x01,0x02,0x04,0x00,0x04},
    {0x0E,0x11,0x01,0x0D,0x15,0x15,0x0E},
    {0x04,0x0A,0x11,0x11,0x1F,0x11,0x11},
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
    {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E},
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x1F},
    {0x0F,0x02,0x02,0x02,0x02,0x12,0x0C},
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
    {0x11,0x1B,0x15,0x11,0x11,0x11,0x11},
    {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
    {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
    {0x11,0x11,0x11,0x11,0x11,0x0A,0x04},
    {0x11,0x11,0x11,0x15,0x15,0x1B,0x11},
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
    {0x0E,0x08,0x08,0x08,0x08,0x08,0x0E},
    {0},{0},
    {0x04,0x0A,0x11,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x1F},
    {0x08,0x04,0x02,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F},
    {0x10,0x10,0x1E,0x11,0x11,0x11,0x1E},
    {0x00,0x00,0x0E,0x11,0x10,0x11,0x0E},
    {0x01,0x01,0x0D,0x13,0x11,0x11,0x0F},
    {0x00,0x00,0x0E,0x11,0x1F,0x10,0x0E},
    {0x06,0x09,0x08,0x1E,0x08,0x08,0x08},
    {0x00,0x0E,0x11,0x11,0x0F,0x01,0x0E},
    {0x10,0x10,0x16,0x19,0x11,0x11,0x11},
    {0x00,0x04,0x00,0x0C,0x04,0x04,0x0E},
    {0x00,0x04,0x00,0x0C,0x04,0x14,0x08},
    {0x10,0x10,0x12,0x14,0x18,0x14,0x12},
    {0x0C,0x04,0x04,0x04,0x04,0x04,0x0E},
    {0x00,0x00,0x1A,0x15,0x15,0x11,0x11},
    {0x00,0x00,0x16,0x19,0x11,0x11,0x11},
    {0x00,0x00,0x0E,0x11,0x11,0x11,0x0E},
    {0x00,0x00,0x1E,0x11,0x1E,0x10,0x10},
    {0x00,0x00,0x0D,0x13,0x0F,0x01,0x01},
    {0x00,0x00,0x16,0x19,0x10,0x10,0x10},
    {0x00,0x00,0x0F,0x10,0x0E,0x01,0x1E},
    {0x08,0x08,0x1E,0x08,0x08,0x09,0x06},
    {0x00,0x00,0x11,0x11,0x11,0x13,0x0D},
    {0x00,0x00,0x11,0x11,0x11,0x0A,0x04},
    {0x00,0x00,0x11,0x11,0x15,0x15,0x0A},
    {0x00,0x00,0x11,0x0A,0x04,0x0A,0x11},
    {0x00,0x00,0x11,0x11,0x0F,0x01,0x0E},
    {0x00,0x00,0x1F,0x02,0x04,0x08,0x1F},
    {0},{0},{0},{0},
};

// ====================== SHADERS (GLES 2.0) ======================
static GLuint solid_prog = 0;
static GLuint circle_prog = 0;
static GLuint vcolor_prog = 0;
static GLuint text_prog = 0;
static GLuint font_tex = 0;

static GLint solid_aPos = 0, solid_uProj = 0, solid_uColor = 0;
static GLint circle_aPos = 0, circle_aUV = 0, circle_uProj = 0, circle_uColor = 0, circle_uGlow = 0;
static GLint vcolor_aPos = 0, vcolor_aColor = 0, vcolor_uProj = 0;
static GLint text_aPos = 0, text_aTex = 0, text_uProj = 0, text_uColor = 0, text_uTex = 0;

static GLuint compile_shader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        SDL_Log("Shader compile: %s", log);
    }
    return s;
}

static GLuint link_program(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(p, 512, nullptr, log);
        SDL_Log("Program link: %s", log);
    }
    return p;
}

static void init_shaders() {
    // --- solid color shader ---
    {
        const char* vs = "attribute vec2 aPos;\n"
            "uniform mat4 uProj;\n"
            "void main(){gl_Position=uProj*vec4(aPos,0.0,1.0);}";
        const char* fs = "precision mediump float;\n"
            "uniform vec4 uColor;\n"
            "void main(){gl_FragColor=uColor;}";
        GLuint vs1 = compile_shader(GL_VERTEX_SHADER, vs);
        GLuint fs1 = compile_shader(GL_FRAGMENT_SHADER, fs);
        solid_prog = link_program(vs1, fs1);
        glDeleteShader(vs1); glDeleteShader(fs1);
        glBindAttribLocation(solid_prog, 0, "aPos");
        glLinkProgram(solid_prog);
        solid_aPos = 0;
        solid_uProj = glGetUniformLocation(solid_prog, "uProj");
        solid_uColor = glGetUniformLocation(solid_prog, "uColor");
    }

    // --- circle shader (quads with UV for circular rendering) ---
    {
        const char* vs = "attribute vec2 aPos;\n"
            "attribute vec2 aUV;\n"
            "uniform mat4 uProj;\n"
            "varying vec2 vUV;\n"
            "void main(){vUV=aUV;gl_Position=uProj*vec4(aPos,0.0,1.0);}";
        const char* fs = "precision mediump float;\n"
            "uniform vec4 uColor;\n"
            "uniform float uGlow;\n"
            "varying vec2 vUV;\n"
            "void main(){\n"
            "  float d=length(vUV);\n"
            "  if(d>1.0)discard;\n"
            "  float alpha;\n"
            "  if(uGlow>0.5){alpha=1.0-smoothstep(0.0,1.0,d);}\n"
            "  else{alpha=1.0-smoothstep(0.7,1.0,d);}\n"
            "  gl_FragColor=vec4(uColor.rgb,uColor.a*alpha);\n"
            "}";
        GLuint vs1 = compile_shader(GL_VERTEX_SHADER, vs);
        GLuint fs1 = compile_shader(GL_FRAGMENT_SHADER, fs);
        circle_prog = link_program(vs1, fs1);
        glDeleteShader(vs1); glDeleteShader(fs1);
        glBindAttribLocation(circle_prog, 0, "aPos");
        glBindAttribLocation(circle_prog, 1, "aUV");
        glLinkProgram(circle_prog);
        circle_aPos = 0;
        circle_aUV = 1;
        circle_uProj = glGetUniformLocation(circle_prog, "uProj");
        circle_uColor = glGetUniformLocation(circle_prog, "uColor");
        circle_uGlow = glGetUniformLocation(circle_prog, "uGlow");
    }

    // --- per-vertex color shader (trails) ---
    {
        const char* vs = "attribute vec2 aPos;\n"
            "attribute vec4 aColor;\n"
            "uniform mat4 uProj;\n"
            "varying vec4 vColor;\n"
            "void main(){vColor=aColor;gl_Position=uProj*vec4(aPos,0.0,1.0);}";
        const char* fs = "precision mediump float;\n"
            "varying vec4 vColor;\n"
            "void main(){gl_FragColor=vColor;}";
        GLuint vs1 = compile_shader(GL_VERTEX_SHADER, vs);
        GLuint fs1 = compile_shader(GL_FRAGMENT_SHADER, fs);
        vcolor_prog = link_program(vs1, fs1);
        glDeleteShader(vs1); glDeleteShader(fs1);
        glBindAttribLocation(vcolor_prog, 0, "aPos");
        glBindAttribLocation(vcolor_prog, 1, "aColor");
        glLinkProgram(vcolor_prog);
        vcolor_aPos = 0;
        vcolor_aColor = 1;
        vcolor_uProj = glGetUniformLocation(vcolor_prog, "uProj");
    }

    // --- textured shader (font) ---
    {
        const char* vs = "attribute vec2 aPos;\n"
            "attribute vec2 aTex;\n"
            "uniform mat4 uProj;\n"
            "varying vec2 vTex;\n"
            "void main(){vTex=aTex;gl_Position=uProj*vec4(aPos,0.0,1.0);}";
        const char* fs = "precision mediump float;\n"
            "uniform vec4 uColor;\n"
            "uniform sampler2D uTex;\n"
            "varying vec2 vTex;\n"
            "void main(){float a=texture2D(uTex,vTex).r;gl_FragColor=vec4(uColor.rgb,uColor.a*a);}";
        GLuint vs1 = compile_shader(GL_VERTEX_SHADER, vs);
        GLuint fs1 = compile_shader(GL_FRAGMENT_SHADER, fs);
        text_prog = link_program(vs1, fs1);
        glDeleteShader(vs1); glDeleteShader(fs1);
        glBindAttribLocation(text_prog, 0, "aPos");
        glBindAttribLocation(text_prog, 1, "aTex");
        glLinkProgram(text_prog);
        text_aPos = 0;
        text_aTex = 1;
        text_uProj = glGetUniformLocation(text_prog, "uProj");
        text_uColor = glGetUniformLocation(text_prog, "uColor");
        text_uTex = glGetUniformLocation(text_prog, "uTex");
    }
}

// ====================== FONT TEXTURE ======================
static void init_font_texture() {
    static const int FW = 128, FH = 64;
    uint8_t pixels[FW * FH];
    memset(pixels, 0, sizeof(pixels));
    for (int c = 0; c < 128; c++) {
        int col = c % 16, row = c / 16;
        int bx = col * 8, by = row * 8;
        for (int y = 0; y < 7; y++) {
            uint8_t bits = glyphs[c][y];
            // flip vertically: glyph[y=0]=top → pixel row by+7 (top of cell)
            int py = by + (7 - y);
            for (int x = 0; x < 5; x++)
                if (bits & (1 << (4 - x)))
                    pixels[py * FW + (bx + x + 1)] = 255;
        }
    }
    glGenTextures(1, &font_tex);
    glBindTexture(GL_TEXTURE_2D, font_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, FW, FH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// ====================== RENDER HELPERS ======================
static void draw_quad(float x1, float y1, float x2, float y2) {
    float verts[] = { x1,y1, x2,y1, x1,y2, x2,y1, x2,y2, x1,y2 };
    glVertexAttribPointer(solid_aPos, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(solid_aPos);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

static void draw_circle_quad(float cx, float cy, float hw, float hh, float glow) {
    glUniform1f(circle_uGlow, glow);
    float verts[] = {
        cx-hw,cy-hh, -1,-1,
        cx+hw,cy-hh,  1,-1,
        cx-hw,cy+hh, -1, 1,
        cx+hw,cy+hh,  1, 1,
    };
    glVertexAttribPointer(circle_aPos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), verts);
    glEnableVertexAttribArray(circle_aPos);
    glVertexAttribPointer(circle_aUV, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), verts + 2);
    glEnableVertexAttribArray(circle_aUV);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// ====================== PHYSICS ======================
static glm::vec3 generate_light_color() {
    std::uniform_real_distribution<float> d(0.6f, 1.0f);
    return glm::vec3(d(rng), d(rng), d(rng));
}

static glm::vec3 distinct_color_from(const StarVector& existing) {
    const float min_dist = 0.45f;
    for (int tries = 0; tries < 20; ++tries) {
        glm::vec3 c = generate_light_color();
        bool ok = true;
        for (auto& s : existing)
            if (glm::length(c - s.color) < min_dist) { ok = false; break; }
        if (ok) return c;
    }
    return generate_light_color();
}

static void init_sim() {
    auto seed = std::random_device{}();
    SDL_Log("Seed: %u", seed);
    rng = std::mt19937(seed);
    auto unif = std::uniform_real_distribution<float>(-1.0f, 1.0f);
    auto unif2 = std::uniform_real_distribution<float>(-0.3f, 0.3f);
    stars.clear(); trails.clear();
    stars.reserve(initial_nstars + 8);
    trails.reserve(initial_nstars + 8);
    for (size_t i = 0; i < initial_nstars; ++i) {
        Star s;
        s.pos = glm::vec2(unif(rng) * aspect, unif(rng));
        s.vel = glm::vec2(unif2(rng), unif2(rng)) * speed_scale;
        s.mass = 1.0f;
        s.color = distinct_color_from(stars);
        stars.push_back(s);
        trails.push_back(Trail{{}, s.color});
    }
    total_stars_ever += initial_nstars;
}

static void derivative(const StarVector& src, StarVector& dst) {
    size_t n = src.size();
    dst.assign(n, Star{});
    for (size_t i = 0; i < n; ++i) {
        dst[i].pos = src[i].vel;
        glm::vec2 acc(0.0f);
        for (size_t j = 0; j < n; ++j) {
            if (i == j) continue;
            auto r12 = src[j].pos - src[i].pos;
            float r2 = glm::dot(r12, r12);
            const float min_r2 = 0.05f * 0.05f;
            if (r2 < min_r2) r2 = min_r2;
            float inv_r = 1.0f / std::sqrt(r2);
            acc += float(G * src[j].mass) * r12 * (inv_r * inv_r * inv_r);
        }
        dst[i].vel = acc;
    }
}

static void integrate_to(StarVector& out, const StarVector& base, const StarVector& k, float dt) {
    size_t n = base.size();
    out.resize(n);
    for (size_t i = 0; i < n; ++i) {
        out[i].pos = base[i].pos + k[i].pos * dt;
        out[i].vel = base[i].vel + k[i].vel * dt;
        out[i].mass = base[i].mass;
        out[i].color = base[i].color;
    }
}

static void integrate_inplace(StarVector& base, const StarVector& k, float dt) {
    size_t n = base.size();
    for (size_t i = 0; i < n; ++i) {
        base[i].pos += k[i].pos * dt;
        base[i].vel += k[i].vel * dt;
    }
}

static void handle_collisions(StarVector& starsv) {
    bool merged = true;
    while (merged) {
        merged = false;
        size_t n = starsv.size();
        for (size_t i = 0; i < n; ++i)
            for (size_t j = i + 1; j < n; ++j) {
                float dist = glm::length(starsv[j].pos - starsv[i].pos);
                if (dist >= merge_distance) continue;
                Star s;
                s.mass = starsv[i].mass + starsv[j].mass;
                s.pos = (starsv[i].pos * starsv[i].mass + starsv[j].pos * starsv[j].mass) / s.mass;
                s.vel = (starsv[i].vel * starsv[i].mass + starsv[j].vel * starsv[j].mass) / s.mass;
                s.color = distinct_color_from(starsv);
                orphan_trails.push_back(std::move(trails[i]));
                orphan_trails.push_back(std::move(trails[j]));
                trails.erase(trails.begin() + j);
                trails.erase(trails.begin() + i);
                trails.push_back(Trail{{}, s.color});
                starsv.erase(starsv.begin() + j);
                starsv.erase(starsv.begin() + i);
                starsv.push_back(s);
                merged = true;
                goto next_round;
            }
        next_round:;
    }
}

static void sample_and_age_trails(float dt) {
    size_t n = stars.size();
    while (trails.size() < n)
        trails.push_back(Trail{{}, stars[trails.size()].color});
    for (size_t i = 0; i < n; ++i) {
        auto& trail = trails[i];
        trail.color = stars[i].color;
        for (auto& p : trail.points) p.age += dt;
        if (trail.points.empty()) {
            trail.points.push_back({stars[i].pos, 0.0f});
        } else {
            float dist = glm::length(stars[i].pos - trail.points.back().pos);
            if (dist >= trail_min_dist)
                trail.points.push_back({stars[i].pos, 0.0f});
        }
        size_t write = 0;
        for (size_t j = 0; j < trail.points.size(); ++j)
            if (trail.points[j].age < trail_max_age) {
                if (write != j) trail.points[write] = trail.points[j];
                write++;
            }
        trail.points.resize(write);
        if (trail.points.size() > size_t(trail_max_points_per_star))
            trail.points.erase(trail.points.begin(), trail.points.end() - trail_max_points_per_star);
    }
    while (trails.size() > n) trails.pop_back();
    for (auto it = orphan_trails.begin(); it != orphan_trails.end(); ) {
        for (auto& p : it->points) p.age += dt;
        size_t write = 0;
        for (size_t j = 0; j < it->points.size(); ++j)
            if (it->points[j].age < trail_max_age) {
                if (write != j) it->points[write] = it->points[j];
                write++;
            }
        it->points.resize(write);
        if (it->points.empty()) it = orphan_trails.erase(it);
        else ++it;
    }
}

static void substep_rk4(float dt) {
    StarVector k1, k2, k3, k4, tmp;
    derivative(stars, k1);
    integrate_to(tmp, stars, k1, dt / 2.0f);
    derivative(tmp, k2);
    integrate_to(tmp, stars, k2, dt / 2.0f);
    derivative(tmp, k3);
    integrate_to(tmp, stars, k3, dt);
    derivative(tmp, k4);
    size_t n = stars.size();
    StarVector k(n);
    for (size_t i = 0; i < n; ++i) {
        k[i].pos = (k1[i].pos + k2[i].pos * 2.0f + k3[i].pos * 2.0f + k4[i].pos) * (1.0f / 6.0f);
        k[i].vel = (k1[i].vel + k2[i].vel * 2.0f + k3[i].vel * 2.0f + k4[i].vel) * (1.0f / 6.0f);
        k[i].mass = stars[i].mass;
        k[i].color = stars[i].color;
    }
    integrate_inplace(stars, k, dt);
    handle_collisions(stars);
}

static void advance(float dt) {
    constexpr size_t n = INTEGRATOR_SUBSTEPS;
    float subdt = dt / float(n);
    constexpr size_t SAMPLE_EVERY = 5;
    for (size_t i = 0; i < n; ++i) {
        substep_rk4(subdt);
        if ((i + 1) % SAMPLE_EVERY == 0)
            sample_and_age_trails(subdt * float(SAMPLE_EVERY));
    }
}

// ====================== RENDERER ======================
static int g_fb_w = 0, g_fb_h = 0;

static void render_simulation() {
    glm::mat4 proj = glm::ortho(
        -aspect + camera_offset.x, aspect + camera_offset.x,
        -1.0f + camera_offset.y, 1.0f + camera_offset.y);

    float px_to_world = 2.0f / g_fb_h;
    float star_size = 12.0f * px_to_world;   // bigger than before
    float glow_size = 20.0f * px_to_world;

    // --- Trails (glow + core passes) ---
    glUseProgram(vcolor_prog);
    glUniformMatrix4fv(vcolor_uProj, 1, GL_FALSE, glm::value_ptr(proj));

    auto draw_trails = [&](const std::vector<Trail>& tv, float base_w, float a_scale, float a_pow) {
        for (auto& t : tv) {
            auto& pts = t.points;
            if (pts.size() < 2) continue;
            std::vector<float> vbuf;
            vbuf.reserve(pts.size() * 10);
            for (size_t j = 0; j < pts.size(); ++j) {
                float age_t = 1.0f - (pts[j].age / trail_max_age);
                if (age_t < 0.0f) age_t = 0.0f;
                glm::vec2 dir;
                if (j == 0) dir = pts[1].pos - pts[0].pos;
                else if (j == pts.size() - 1) dir = pts[j].pos - pts[j - 1].pos;
                else dir = pts[j + 1].pos - pts[j - 1].pos;
                float len = glm::length(dir);
                glm::vec2 perp(0.0f);
                if (len > 0.0001f) { dir /= len; perp = glm::vec2(-dir.y, dir.x); }
                float w = age_t * base_w * 0.5f;
                float a = age_t * age_t;
                if (a_pow > 2.5f) a *= age_t;
                a *= a_scale;
                glm::vec3 c = t.color * a;
                vbuf.push_back(pts[j].pos.x + perp.x * w);
                vbuf.push_back(pts[j].pos.y + perp.y * w);
                vbuf.push_back(c.x); vbuf.push_back(c.y); vbuf.push_back(c.z);
                vbuf.push_back(pts[j].pos.x - perp.x * w);
                vbuf.push_back(pts[j].pos.y - perp.y * w);
                vbuf.push_back(c.x); vbuf.push_back(c.y); vbuf.push_back(c.z);
            }
            if (vbuf.empty()) continue;
            glVertexAttribPointer(vcolor_aPos, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), vbuf.data());
            glEnableVertexAttribArray(vcolor_aPos);
            glVertexAttribPointer(vcolor_aColor, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), vbuf.data() + 2);
            glEnableVertexAttribArray(vcolor_aColor);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, GLsizei(vbuf.size() / 5));
        }
    };

    draw_trails(trails, glow_size, 0.12f, 2.0f);
    draw_trails(orphan_trails, glow_size, 0.12f, 2.0f);
    draw_trails(trails, star_size, 1.0f, 3.0f);
    draw_trails(orphan_trails, star_size, 1.0f, 3.0f);

    // --- Stars (circular, using circle shader) ---
    glUseProgram(circle_prog);
    glUniformMatrix4fv(circle_uProj, 1, GL_FALSE, glm::value_ptr(proj));

    // glow layer
    glUniform4f(circle_uColor, 1, 1, 1, 0.1f);
    for (auto& s : stars)
        draw_circle_quad(s.pos.x, s.pos.y, glow_size, glow_size, 1.0f);

    // core layer
    for (auto& s : stars) {
        glUniform4f(circle_uColor, s.color.x, s.color.y, s.color.z, 1.0f);
        draw_circle_quad(s.pos.x, s.pos.y, star_size, star_size, 0.0f);
    }

    // --- Grid ---
    if (show_grid) {
        glUseProgram(solid_prog);
        glUniformMatrix4fv(solid_uProj, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform4f(solid_uColor, 0.6f, 0.6f, 0.6f, 0.25f);
        const float grid_step = 0.1f;
        float left = camera_offset.x - aspect, right = camera_offset.x + aspect;
        float bottom = camera_offset.y - 1.0f, top = camera_offset.y + 1.0f;
        float x0 = std::floor(left / grid_step) * grid_step;
        float y0 = std::floor(bottom / grid_step) * grid_step;
        std::vector<float> lv;
        for (float x = x0; x <= right; x += grid_step) {
            lv.push_back(x); lv.push_back(bottom);
            lv.push_back(x); lv.push_back(top);
        }
        for (float y = y0; y <= top; y += grid_step) {
            lv.push_back(left); lv.push_back(y);
            lv.push_back(right); lv.push_back(y);
        }
        glVertexAttribPointer(solid_aPos, 2, GL_FLOAT, GL_FALSE, 0, lv.data());
        glEnableVertexAttribArray(solid_aPos);
        glDrawArrays(GL_LINES, 0, GLsizei(lv.size() / 2));
    }
}

// ====================== UI ======================
struct Button {
    float x, y, w, h;
    const char* label;
    bool pressed;
    bool active;
    bool toggle;    // true = stays pressed (toggle button)
};

static Button btn_pause =  {0,0,0,0, "Pause", false, true, false};
static Button btn_reset =  {0,0,0,0, "Reset", false, true, false};
static Button btn_grid  =  {0,0,0,0, "Grid",  false, true, false};
static Button btn_mode  =  {0,0,0,0, "MOVE",  false, true, true};

static void render_text(const char* text, float sx, float sy, float scale, const glm::vec4& color) {
    if (!text || !*text) return;
    glUseProgram(text_prog);
    glm::mat4 proj = glm::ortho(0.0f, float(g_fb_w), 0.0f, float(g_fb_h));
    glUniformMatrix4fv(text_uProj, 1, GL_FALSE, glm::value_ptr(proj));
    glUniform4fv(text_uColor, 1, &color.x);
    glUniform1i(text_uTex, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font_tex);

    float ch_w = 8.0f * scale, ch_h = 8.0f * scale;
    float cx = sx, cy = sy;

    for (const char* p = text; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        int col = c % 16, row = c / 16;
        float u0 = col * 8.0f / 128.0f, v0 = row * 8.0f / 64.0f;
        float u1 = (col + 1) * 8.0f / 128.0f, v1 = (row + 1) * 8.0f / 64.0f;
        float verts[] = {
            cx,     cy,      u0, v0,
            cx+ch_w, cy,      u1, v0,
            cx,     cy+ch_h, u0, v1,
            cx+ch_w, cy,      u1, v0,
            cx+ch_w, cy+ch_h, u1, v1,
            cx,     cy+ch_h, u0, v1,
        };
        glVertexAttribPointer(text_aPos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), verts);
        glEnableVertexAttribArray(text_aPos);
        glVertexAttribPointer(text_aTex, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), verts + 2);
        glEnableVertexAttribArray(text_aTex);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        cx += ch_w;
    }
}

static void render_button(Button& btn) {
    bool active = btn.toggle ? btn.pressed : btn.pressed;
    glm::vec4 bg = active ? glm::vec4(0.9f, 0.9f, 0.9f, 0.9f) : glm::vec4(0.15f, 0.15f, 0.15f, 0.85f);
    glm::vec4 fg = active ? glm::vec4(0, 0, 0, 1) : glm::vec4(0.85f, 0.85f, 0.85f, 1);

    glUseProgram(solid_prog);
    glm::mat4 proj = glm::ortho(0.0f, float(g_fb_w), 0.0f, float(g_fb_h));
    glUniformMatrix4fv(solid_uProj, 1, GL_FALSE, glm::value_ptr(proj));
    glUniform4fv(solid_uColor, 1, &bg.x);
    draw_quad(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);

    float len = float(strlen(btn.label));
    float max_font_w = btn.w * 0.9f / (len * 8.0f);
    float font_sz = std::min(btn.h / 36.0f * 3.0f, max_font_w);
    float tw = len * 8.0f * font_sz, th = 8.0f * font_sz;
    float tx = btn.x + (btn.w - tw) * 0.5f;
    float ty = btn.y + (btn.h - th) * 0.5f;
    render_text(btn.label, tx, ty, font_sz, fg);
}

static void layout_buttons(int fb_w, int fb_h) {
    g_fb_w = fb_w; g_fb_h = fb_h;

    // Bottom button row spans 80% of screen width
    float row_w = fb_w * 0.8f;
    float gap = row_w * 0.02f;
    float bw = (row_w - gap * 3.0f) / 4.0f;
    float bh = bw * 0.45f;
    float bot = fb_h * 0.015f;
    float start_x = (fb_w - row_w) * 0.5f;

    btn_pause = {start_x,                bot, bw, bh, "Pause", false, true, false};
    btn_reset = {start_x + bw + gap,     bot, bw, bh, "Reset", false, true, false};
    btn_grid  = {start_x + (bw+gap)*2,   bot, bw, bh, "Grid",  false, true, false};
    btn_mode  = {start_x + (bw+gap)*3,   bot, bw, bh, "MOVE",  false, true, true};
}

static void render_ui() {
    btn_pause.label = paused ? "Go on" : "Pause";
    btn_grid.label  = show_grid ? "Hide" : "Grid";
    render_button(btn_pause);
    render_button(btn_reset);
    render_button(btn_grid);
    render_button(btn_mode);

    char hud[128];
    snprintf(hud, sizeof(hud), "Stars: %zu  total: %zu%s",
        stars.size(), total_stars_ever, paused ? "  PAUSED" : "");
    render_text(hud, 18.0f, float(g_fb_h) - 36.0f, 3.5f, glm::vec4(0.85f, 0.85f, 0.85f, 1));

    const char* mode_hint = (current_mode == Mode::LAUNCH)
        ? "Mode: LAUNCH  (drag to create stars)"
        : "Mode: MOVE  (drag to pan camera)";
    render_text(mode_hint, 18.0f, float(g_fb_h) - 72.0f, 2.0f, glm::vec4(0.55f, 0.55f, 0.55f, 1));
}

// ====================== INPUT ======================
static bool touch_active = false;
static bool first_finger_down = false;
static int active_finger = -1;
static glm::vec2 touch_start_world, touch_end_world;
static glm::vec2 touch_start_screen;
static glm::vec2 pan_cam_start;

static glm::vec2 screen_to_world(float sx, float sy) {
    float nx = sx / g_fb_w, ny = sy / g_fb_h;
    return glm::vec2(
        (nx * 2.0f - 1.0f) * aspect + camera_offset.x,
        (1.0f - ny * 2.0f) + camera_offset.y);
}

static void add_star_world(const glm::vec2& pos, const glm::vec2& vel, float mass = 1.0f) {
    Star s;
    s.pos = pos; s.vel = vel; s.mass = mass;
    s.color = distinct_color_from(stars);
    stars.push_back(s);
    trails.push_back(Trail{{}, s.color});
    ++total_stars_ever;
}

static bool hit_test(Button& btn, float sx, float sy) {
    return sx >= btn.x && sx <= btn.x + btn.w && sy >= btn.y && sy <= btn.y + btn.h;
}

static void on_touch(const SDL_TouchFingerEvent& e, bool down) {
    float sx = e.x * float(g_fb_w);
    // buttons use OpenGL pixel coords (y=0 at bottom)
    float sy_btn = (1.0f - e.y) * float(g_fb_h);
    // world uses desktop coords (y=0 at top, matching screen_to_world)
    float sy_world = e.y * float(g_fb_h);
    int fid = int(e.fingerId);

    if (down) {
        if (hit_test(btn_mode, sx, sy_btn)) {
            btn_mode.pressed = !btn_mode.pressed;
            current_mode = (current_mode == Mode::LAUNCH) ? Mode::MOVE : Mode::LAUNCH;
            btn_mode.label = (current_mode == Mode::LAUNCH) ? "MOVE" : "LAUNCH";
            return;
        }
        if (hit_test(btn_pause, sx, sy_btn)) { btn_pause.pressed = true; return; }
        if (hit_test(btn_reset, sx, sy_btn)) { btn_reset.pressed = true; return; }
        if (hit_test(btn_grid, sx, sy_btn))  { btn_grid.pressed = true; return; }

        if (!touch_active) {
            touch_active = true;
            active_finger = fid;
            touch_start_screen = glm::vec2(sx, sy_world);
            touch_start_world = screen_to_world(sx, sy_world);
            touch_end_world = touch_start_world;
            pan_cam_start = camera_offset;
        }
    } else { // up
        if (btn_pause.pressed) { btn_pause.pressed = false; paused = !paused; return; }
        if (btn_reset.pressed) { btn_reset.pressed = false;
            stars.clear(); trails.clear(); orphan_trails.clear();
            total_stars_ever = 0; paused = false; return; }
        if (btn_grid.pressed)  { btn_grid.pressed = false; show_grid = !show_grid; return; }

        if (touch_active && fid == active_finger) {
            touch_active = false;
            active_finger = -1;
            float pixel_dx = (touch_end_world.x - touch_start_world.x) / (2.0f * aspect / g_fb_w);
            float pixel_dy = (touch_end_world.y - touch_start_world.y) / (2.0f / g_fb_h);
            float pixel_dist = std::sqrt(pixel_dx * pixel_dx + pixel_dy * pixel_dy);

            if (current_mode == Mode::LAUNCH) {
                if (pixel_dist > 25.0f) {
                    glm::vec2 vel = (touch_end_world - touch_start_world) * mouse_speed_factor;
                    add_star_world(touch_start_world, vel);
                } else {
                    add_star_world(touch_start_world, glm::vec2(0.0f));
                }
            }
        }
    }
}

static void on_touch_move(const SDL_TouchFingerEvent& e) {
    float sx = e.x * float(g_fb_w);
    float sy = e.y * float(g_fb_h);
    int fid = int(e.fingerId);

    if (!touch_active || fid != active_finger) return;

    if (current_mode == Mode::LAUNCH) {
        touch_end_world = screen_to_world(sx, sy);
    } else { // MOVE
        glm::vec2 cur = screen_to_world(sx, sy);
        glm::vec2 start = screen_to_world(touch_start_screen.x, touch_start_screen.y);
        camera_offset = pan_cam_start - (cur - start);
    }
}

// ====================== MAIN ======================
extern "C" int SDL_main(int, char**) {
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        SDL_Log("SDL init failed: %s", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Trisolar",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        0, 0,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!window) {
        SDL_Log("Window: %s", SDL_GetError()); SDL_Quit(); return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    SDL_GLContext glc = SDL_GL_CreateContext(window);
    if (!glc) {
        SDL_Log("GL context: %s", SDL_GetError());
        SDL_DestroyWindow(window); SDL_Quit(); return -1;
    }

    SDL_GL_SetSwapInterval(1);

    int w, h, fb_w, fb_h;
    SDL_GetWindowSize(window, &w, &h);
    SDL_GL_GetDrawableSize(window, &fb_w, &fb_h);
    aspect = float(fb_w) / float(fb_h);
    glViewport(0, 0, fb_w, fb_h);
    layout_buttons(fb_w, fb_h);

    init_shaders();
    init_font_texture();
    init_sim();

    glClearColor(0, 0, 0, 1);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    Uint64 lastTime = SDL_GetPerformanceCounter();
    bool run = true;

    while (run) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT: run = false; break;
                case SDL_FINGERDOWN: on_touch(ev.tfinger, true); break;
                case SDL_FINGERUP: on_touch(ev.tfinger, false); break;
                case SDL_FINGERMOTION: on_touch_move(ev.tfinger); break;
                case SDL_KEYDOWN:
                    if (ev.key.keysym.sym == SDLK_AC_BACK || ev.key.keysym.sym == SDLK_ESCAPE)
                        run = false;
                    break;
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        float dt = float((now - lastTime) / double(SDL_GetPerformanceFrequency()));
        lastTime = now;
        if (dt > 0.05f) dt = 0.05f;

        if (!paused) advance(dt);

        glClear(GL_COLOR_BUFFER_BIT);
        render_simulation();
        render_ui();
        SDL_GL_SwapWindow(window);
    }

    glDeleteTextures(1, &font_tex);
    glDeleteProgram(solid_prog);
    glDeleteProgram(circle_prog);
    glDeleteProgram(vcolor_prog);
    glDeleteProgram(text_prog);
    SDL_GL_DeleteContext(glc);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
