// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include <cstddef>

#include <visionaray/math/simd/type_traits.h>
#include <visionaray/math/array.h>
#include <visionaray/math/forward.h>
#include <visionaray/math/vector.h>
#include <visionaray/texture/texture_traits.h>

namespace visionaray
{

//-------------------------------------------------------------------------------------------------
// Implement Visionaray's texturing interface for Ptex textures
//

namespace ptex
{

// tex2D
inline vector<4, unorm<8>> tex2D(PtexPtr<PtexTexture> const& tex, coordinate<float> const& coord)
{
    // Older versions of Ptex have only non-const PtexPtr accessors
    PtexPtr<Ptex::PtexTexture>& mutable_tex = const_cast<PtexPtr<PtexTexture>&>(tex);

    if (mutable_tex == nullptr)
    {
        return vector<4, unorm<8>>(1.0f);
    }

    Ptex::PtexFilter::Options opts(Ptex::PtexFilter::FilterType::f_bspline);
    PtexPtr<Ptex::PtexFilter> filter(Ptex::PtexFilter::getFilter(mutable_tex.get(), opts));

    vec3 rgb;
    filter->eval(
            rgb.data(),
            0,
            mutable_tex->numChannels(),
            coord.face_id,
            coord.u,
            coord.v,
            1.0f,
            1.0f,
            1.0f,
            1.0f
            );

    return vector<4, unorm<8>>(rgb.x, rgb.y, rgb.z, 1.0f);
}

} // ptex


namespace simd
{

template <
    size_t N,
    typename T = float_from_simd_width<N>
    >
inline ptex::coordinate<T> pack(array<ptex::coordinate<float>, N> const& coords)
{
    ptex::coordinate<T> result;

    int* face_id = reinterpret_cast<int*>(&result.face_id);
    float* u = reinterpret_cast<float*>(&result.u);
    float* v = reinterpret_cast<float*>(&result.v);
    float* du = reinterpret_cast<float*>(&result.du);
    float* dv = reinterpret_cast<float*>(&result.dv);

    for (size_t i = 0; i < N; ++i)
    {
        face_id[i] = coords[i].face_id;
        u[i]       = coords[i].u;
        v[i]       = coords[i].v;
        du[i]      = coords[i].du;
        dv[i]      = coords[i].dv;
    }

    return result;
}

} // simd


// get_tex_coord() non-simd
template <
    typename HR,
    typename T,
    typename = typename std::enable_if<!simd::is_simd_vector<typename HR::scalar_type>::value>::type
    >
inline auto get_tex_coord(
        ptex::face_id_t const*  face_ids,
        HR const&               hr,
        basic_triangle<3, T>    /* */
        )
    -> ptex::coordinate<typename HR::scalar_type>
{
    ptex::coordinate<typename HR::scalar_type> result;

    result.face_id = face_ids[hr.prim_id];
    result.u = hr.u;
    result.v = hr.v;

    return result;
}

// get_tex_coord() simd
template <
    typename HR,
    typename T,
    typename = typename std::enable_if<simd::is_simd_vector<typename HR::scalar_type>::value>::type,
    typename = void
    >
inline auto get_tex_coord(
        ptex::face_id_t const*  face_ids,
        HR const&               hr,
        basic_triangle<3, T>    /* */
        )
    -> ptex::coordinate<typename HR::scalar_type>
{
    using U = typename HR::scalar_type;

    auto hrs = unpack(hr);

    array<ptex::coordinate<float>, simd::num_elements<U>::value> coords;

    for (int i = 0; i < simd::num_elements<U>::value; ++i)
    {
        coords[i].face_id = face_ids[hrs[i].prim_id];
        coords[i].u = hrs[i].u;
        coords[i].v = hrs[i].v;
    }

    return simd::pack(coords);
}

template <>
struct texture_dimensions<PtexPtr<PtexTexture>>
{
    enum { value = 2 };
};

} // visionaray
