// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include <memory>
#include <vector>

#include <GL/glew.h>

#include <visionaray/detail/platform.h>

#include <visionaray/camera.h>
#include <visionaray/cpu_buffer_rt.h>
#include <visionaray/kernels.h> // for make_params(...)
#include <visionaray/material.h>
#include <visionaray/point_light.h>
#include <visionaray/scheduler.h>

#include <common/manip/arcball_manipulator.h>
#include <common/manip/pan_manipulator.h>
#include <common/manip/zoom_manipulator.h>
#include <common/viewer_glut.h>

using namespace visionaray;


//-------------------------------------------------------------------------------------------------
// Cube geometry
//

static const vec3 verts[8] = {

        { -1.0f, -1.0f,  1.0f },
        {  1.0f, -1.0f,  1.0f },
        {  1.0f,  1.0f,  1.0f },
        { -1.0f,  1.0f,  1.0f },

        {  1.0f, -1.0f, -1.0f },
        { -1.0f, -1.0f, -1.0f },
        { -1.0f,  1.0f, -1.0f },
        {  1.0f,  1.0f, -1.0f }
        };


//-------------------------------------------------------------------------------------------------
// Stencil texture
//

static const unsigned char heart[16][16] = {
        { 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0 },
        { 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0 },
        { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
        { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
        { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
        { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
        };


//-------------------------------------------------------------------------------------------------
// struct with state variables
//

struct renderer : viewer_glut
{
    using host_ray_type = basic_ray<simd::float4>;
    using tex_ref       = texture_ref<unorm<8>, NormalizedFloat, 2>;

    renderer()
        : viewer_glut(512, 512, "Visionaray Custom Intersector Example")
        , bbox({ -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f })
        , host_sched(8)
    {
        make_cube();
        make_texture();
    }

    aabb                                        bbox;
    camera                                      cam;
    cpu_buffer_rt<PF_RGBA32F, PF_UNSPECIFIED>   host_rt;
    tiled_sched<host_ray_type>                  host_sched;


    // rendering data

    std::vector<basic_triangle<3, float>>       triangles;
    std::vector<vec3>                           normals;
    std::vector<vec2>                           tex_coords;
    std::vector<plastic<float>>                 materials;
    std::vector<tex_ref>                        textures;

    void add_tex_coords()
    {
        tex_coords.emplace_back( 0.0f, 0.0f );
        tex_coords.emplace_back( 1.0f, 0.0f );
        tex_coords.emplace_back( 1.0f, 1.0f );

        tex_coords.emplace_back( 0.0f, 0.0f );
        tex_coords.emplace_back( 1.0f, 1.0f );
        tex_coords.emplace_back( 0.0f, 1.0f );
    }

    void make_cube()
    {
        // triangles store one vert and two edges

        //
        //    v1    e2
        //     *-----------*
        //      \         /
        //       \       /
        //     e1 \     /
        //         \   /
        //          \ /
        //           *
        //


        // front

        triangles.emplace_back(
                verts[0],
                verts[1] - verts[0],
                verts[2] - verts[0]
                );

        triangles.emplace_back(
                verts[0],
                verts[2] - verts[0],
                verts[3] - verts[0]
                );

        add_tex_coords();


        // back

        triangles.emplace_back(
                verts[4],
                verts[5] - verts[4],
                verts[6] - verts[4]
                );

        triangles.emplace_back(
                verts[4],
                verts[6] - verts[4],
                verts[7] - verts[4]
                );

        add_tex_coords();


        // top

        triangles.emplace_back(
                verts[3],
                verts[2] - verts[3],
                verts[7] - verts[3]
                );

        triangles.emplace_back(
                verts[3],
                verts[7] - verts[3],
                verts[6] - verts[3]
                );

        add_tex_coords();


        // bottom

        triangles.emplace_back(
                verts[4],
                verts[5] - verts[4],
                verts[0] - verts[4]
                );

        triangles.emplace_back(
                verts[4],
                verts[0] - verts[4],
                verts[1] - verts[4]
                );

        add_tex_coords();


        // left

        triangles.emplace_back(
                verts[5],
                verts[0] - verts[5],
                verts[3] - verts[5]
                );

        triangles.emplace_back(
                verts[5],
                verts[3] - verts[5],
                verts[6] - verts[5]
                );

        add_tex_coords();


        // right

        triangles.emplace_back(
                verts[1],
                verts[4] - verts[1],
                verts[7] - verts[1]
                );

        triangles.emplace_back(
                verts[1],
                verts[7] - verts[1],
                verts[2] - verts[1]
                );

        add_tex_coords();


        // set a default material
        plastic<float> mat;
        mat.set_ca( from_rgb(0.0f, 0.0f, 0.0f) );
        mat.set_cd( from_rgb(0.8f, 0.2f, 0.1f) );
        mat.set_cs( from_rgb(1.0f, 0.9f, 0.7f) );
        mat.set_ka( 1.0f );
        mat.set_kd( 1.0f );
        mat.set_ks( 1.0f );
        mat.set_specular_exp( 10.0f );
        materials.push_back(mat);


        // set prim_id to identify the triangle
        unsigned prim_id = 0;

        // set geometry id to map to triangles to materials and textures
        for (auto& tri : triangles)
        {
            tri.prim_id = prim_id++;

            // all have the same material and texture
            tri.geom_id = 0;
        }


        // calculate normals
        for (auto const& tri : triangles)
        {
            normals.emplace_back( normalize(cross(tri.e1, tri.e2)) );
        }
    }


    void make_texture()
    {
        textures.emplace_back(16, 16);
        auto& tex = textures.back();
        tex.set_data(reinterpret_cast<unorm<8> const*>(heart));
        tex.set_filter_mode(Nearest);
        tex.set_address_mode(Clamp);
    }

protected:

    void on_display();

};

std::unique_ptr<renderer> rend(nullptr);


//-------------------------------------------------------------------------------------------------
// An intersector that cuts out parts of the geometry based on a texture lookup
// TODO: support for lambdas
// TODO: library support for obtaining the hit mask regardless of the ISA
//

struct mask_intersector : basic_intersector<mask_intersector>
{
    using basic_intersector<mask_intersector>::operator();

    template <typename R, typename S>
    auto operator()(R const& ray, basic_triangle<3, S> const& tri)
        -> decltype( intersect(ray, tri) )
    {
        assert( tex_coords );
        assert( textures );

        auto hr = intersect(ray, tri);

        if ( !any(hr.hit) )
        {
            return hr;
        }

        auto tc = get_tex_coord(tex_coords, hr);

        auto hr4 = simd::unpack( hr );
        auto tc4 = simd::unpack( tc );

        bool hit4[4] = { false, false, false, false };

        for (unsigned i = 0; i < 4; ++i)
        {
            if (!hr4[i].hit)
            {
                continue;
            }

            auto const& tex = textures[hr4[i].geom_id];
            hit4[i] = tex.width() > 0 && tex.height() > 0
                && static_cast<float>(tex2D(tex, tc4[i])) > 0.0f;
        }

        hr.hit &= simd::mask4(hit4[0], hit4[1], hit4[2], hit4[3]);

        return hr;
    }

    vec2 const*                 tex_coords;
    renderer::tex_ref const*    textures;
};


//-------------------------------------------------------------------------------------------------
// Display function, contains the rendering kernel
//

void renderer::on_display()
{
    // some setup

    using R = renderer::host_ray_type;
    using S = R::scalar_type;
    using V = vector<3, S>;

    auto sparams = make_sched_params<pixel_sampler::uniform_type>(
            rend->cam,
            rend->host_rt
            );



    // a headlight
    point_light<float> light;
    light.set_cl( vec3(1.0f, 1.0f, 1.0f) );
    light.set_kl( 1.0f );
    light.set_position( rend->cam.eye() );

    std::vector<point_light<float>> lights;
    lights.push_back(light);


    auto kparams = make_params<normals_per_face_binding>(
            rend->triangles.data(),
            rend->triangles.data() + rend->triangles.size(),
            rend->normals.data(),
            rend->materials.data(),
            lights.data(),
            lights.data() + lights.size(),
            1,          // num bounces - irrelevant for primary ray shading
            1E-3f       // a tiny number - also irrelevant for primary ray shading
            );

    mask_intersector intersector;
    intersector.tex_coords = rend->tex_coords.data();
    intersector.textures  = rend->textures.data();


    rend->host_sched.frame([&](R ray) -> result_record<S>
    {
        // basically reimplement the "simple" kernel

        result_record<S> result;

        result.color = kparams.bg_color;

        // use the closest_hit() intrinsic with a custom intersector
        auto hit_rec = closest_hit(
                ray,
                kparams.prims.begin,
                kparams.prims.end,
                intersector
                );

        if (any(hit_rec.hit))
        {
            hit_rec.isect_pos = ray.ori + ray.dir * hit_rec.t;

            // shade the surface
            auto surf = get_surface(hit_rec, kparams);

            auto sr = make_shade_record<decltype(kparams), S>();
            sr.active = hit_rec.hit;
            sr.isect_pos = hit_rec.isect_pos;
            // two-sided shading: if necessary, flip the normal
            sr.normal = faceforward( surf.normal, -ray.dir, surf.normal );
            sr.view_dir = -ray.dir;
            sr.light_dir = normalize( V(kparams.lights.begin->position()) - hit_rec.isect_pos );
            sr.light = *kparams.lights.begin;

            // only shade where ray hit
            result.color = select(
                    hit_rec.hit,
                    to_rgba( surf.shade(sr) ),
                    result.color
                    );

            result.isect_pos = hit_rec.isect_pos;
        }

        result.hit = hit_rec.hit;
        return result;
    }, sparams);


    // display the rendered image

    glClearColor(0.1, 0.4, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    rend->host_rt.display_color_buffer();

    swap_buffers();
}


//-------------------------------------------------------------------------------------------------
// Main function, performs initialization
//

int main(int argc, char** argv)
{
    rend = std::unique_ptr<renderer>(new renderer);

    try
    {
        rend->init(argc, argv);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    glewInit();

    float aspect = 1.0f;

    rend->cam.perspective(45.0f * constants::degrees_to_radians<float>(), aspect, 0.001f, 1000.0f);
    rend->cam.set_viewport(0, 0, 512, 512);
    rend->cam.view_all( rend->bbox );

    rend->add_manipulator( std::make_shared<arcball_manipulator>(rend->cam, mouse::Left) );
    rend->add_manipulator( std::make_shared<pan_manipulator>(rend->cam, mouse::Middle) );
    rend->add_manipulator( std::make_shared<zoom_manipulator>(rend->cam, mouse::Right) );

    glViewport(0, 0, 512, 512);
    rend->host_rt.resize(512, 512);

    rend->event_loop();
}
