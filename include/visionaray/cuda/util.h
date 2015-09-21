// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#ifndef VSNRAY_CUDA_UTIL_H
#define VSNRAY_CUDA_UTIL_H 1

#include <cuda_runtime_api.h>

#include <visionaray/detail/macros.h>
#include <visionaray/math/math.h>
#include <visionaray/texture/forward.h>

namespace visionaray
{
namespace cuda
{

//-------------------------------------------------------------------------------------------------
// cuda texel type <-> visionaray texel type
//

template <typename Type, tex_read_mode ReadMode>
struct map_texel_type
{
    using cuda_type             = Type;
    using vsnray_type           = Type;

    using cuda_return_type      = Type;
    using vsnray_return_type    = Type;

    VSNRAY_FUNC static Type convert_return_type(Type const& value)
    {
        return value;
    }
};


//-------------------------------------------------------------------------------------------------
// cuda -> visionaray
//

template <>
struct map_texel_type<uchar4, ElementType>
{
    using cuda_type             = uchar4;
    using vsnray_type           = vector<4, unorm<8>>;

    using cuda_return_type      = uchar4;
    using vsnray_return_type    = vector<4, unorm<8>>;

    VSNRAY_FUNC static vector<4, unorm<8>> convert_return_type(uchar4 const& value)
    {
        return vector<4, unorm<8>>( value.x, value.y, value.z, value.w );
    }
};

template <>
struct map_texel_type<unsigned char, NormalizedFloat>
{
    using cuda_type             = unsigned char;
    using vsnray_type           = unorm<8>;

    using cuda_return_type      = float;
    using vsnray_return_type    = float;

    VSNRAY_FUNC static float convert_return_type(float value)
    {
        return value;
    }
};

template <>
struct map_texel_type<uchar2, NormalizedFloat>
{
    using cuda_type             = uchar2;
    using vsnray_type           = vector<2, unorm<8>>;

    using cuda_return_type      = float2;
    using vsnray_return_type    = vector<2, float>;

    VSNRAY_FUNC static vector<2, float> convert_return_type(float2 const& value)
    {
        return vector<2, float>( value.x, value.y );
    }
};

template <>
struct map_texel_type<uchar3, NormalizedFloat>
{
    using cuda_type             = uchar3;
    using vsnray_type           = vector<3, unorm<8>>;

    using cuda_return_type      = float3;
    using vsnray_return_type    = vector<3, float>;

    VSNRAY_FUNC static vector<3, float> convert_return_type(float3 const& value)
    {
        return vector<3, float>( value.x, value.y, value.z );
    }
};

template <>
struct map_texel_type<uchar4, NormalizedFloat>
{
    using cuda_type             = uchar4;
    using vsnray_type           = vector<4, unorm<8>>;

    using cuda_return_type      = float4;
    using vsnray_return_type    = vector<4, float>;

    VSNRAY_FUNC static vector<4, float> convert_return_type(float4 const& value)
    {
        return vector<4, float>( value.x, value.y, value.z, value.w );
    }
};

template <tex_read_mode ReadMode>
struct map_texel_type<float2, ReadMode>
{
    using cuda_type             = float2;
    using vsnray_type           = vector<2, float>;

    using cuda_return_type      = float2;
    using vsnray_return_type    = vector<2, float>;

    VSNRAY_FUNC static vector<2, float> convert_return_type(float2 const& value)
    {
        return vector<2, float>( value.x, value.y );
    }
};

template <tex_read_mode ReadMode>
struct map_texel_type<float4, ReadMode>
{
    using cuda_type             = float4;
    using vsnray_type           = vector<4, float>;

    using cuda_return_type      = float4;
    using vsnray_return_type    = vector<4, float>;

    VSNRAY_FUNC static vector<4, float> convert_return_type(float4 const& value)
    {
        return vector<4, float>( value.x, value.y, value.z, value.w );
    }
};


//-------------------------------------------------------------------------------------------------
// cuda <- visionaray
//

template <tex_read_mode ReadMode>
struct map_texel_type<unorm<8>, ReadMode>
{
    using cuda_type   = unsigned char;
    using vsnray_type = unorm<8>;

    VSNRAY_FUNC static unsigned char convert_return_type(unorm<8> value)
    {
        return static_cast<unsigned char>(value);
    }
};

template <tex_read_mode ReadMode>
struct map_texel_type<vector<2, unorm<8>>, ReadMode>
{
    using cuda_type   = uchar2;
    using vsnray_type = vector<2, unorm<8>>;

    VSNRAY_FUNC static uchar2 convert_return_type(vector<2, unorm<8>> const& value)
    {
        return make_uchar2(
                static_cast<unsigned char>(value.x),
                static_cast<unsigned char>(value.y)
                );
    }
};

template <tex_read_mode ReadMode>
struct map_texel_type<vector<3, unorm<8>>, ReadMode>
{
    using cuda_type   = uchar3;
    using vsnray_type = vector<3, unorm<8>>;

    VSNRAY_FUNC static uchar3 convert_return_type(vector<3, unorm<8>> const& value)
    {
        return make_uchar3(
                static_cast<unsigned char>(value.x),
                static_cast<unsigned char>(value.y),
                static_cast<unsigned char>(value.z)
                );
    }
};

template <tex_read_mode ReadMode>
struct map_texel_type<vector<4, unorm<8>>, ReadMode>
{
    using cuda_type   = uchar4;
    using vsnray_type = vector<4, unorm<8>>;

    VSNRAY_FUNC static uchar4 convert_return_type(vector<4, unorm<8>> const& value)
    {
        return make_uchar4(
                static_cast<unsigned char>(value.x),
                static_cast<unsigned char>(value.y),
                static_cast<unsigned char>(value.z),
                static_cast<unsigned char>(value.w)
                );
    }
};

template <tex_read_mode ReadMode>
struct map_texel_type<vector<2, float>, ReadMode>
{
    using cuda_type   = float2;
    using vsnray_type = vector<2, float>;

    VSNRAY_FUNC static float2 convert_return_type(vector<2, float> const& value)
    {
        return make_float2( value.x, value.y );
    }
};

template <tex_read_mode ReadMode>
struct map_texel_type<vector<4, float>, ReadMode>
{
    using cuda_type   = float4;
    using vsnray_type = vector<4, float>;

    VSNRAY_FUNC static float4 convert_return_type(vector<4, float> const& value)
    {
        return make_float4( value.x, value.y, value.z, value.w );
    }
};

} // cuda
} // visionaray

#endif // VSNRAY_CUDA_UTIL_H
