// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include <type_traits>
#include <utility>

#include "../make_generator.h"
#include "range.h"
#include "sched_common.h"

namespace visionaray
{
namespace basic_sched_impl
{

//-------------------------------------------------------------------------------------------------
// Generate primary ray and sample pixel
//

template <typename R, typename K, typename SP, typename Generator, typename ...Args>
void call_sample_pixel(
        std::false_type /* has intersector */,
        R               /* */,
        K               kernel,
        SP              sparams,
        Generator&      gen,
        unsigned        frame_num,
        Args&&...       args
        )
{
    auto r = detail::make_primary_rays(
            R{},
            typename SP::pixel_sampler_type{},
            gen,
            std::forward<Args>(args)...
            );

    sample_pixel(
            kernel,
            typename SP::pixel_sampler_type(),
            r,
            gen,
            frame_num,
            sparams.rt.ref(),
            std::forward<Args>(args)...
            );
}

template <typename R, typename K, typename SP, typename Generator, typename ...Args>
void call_sample_pixel(
        std::true_type  /* has intersector */,
        R               /* */,
        K               kernel,
        SP              sparams,
        Generator&      gen,
        unsigned        frame_num,
        Args&&...       args
        )
{
    auto r = detail::make_primary_rays(
            R{},
            typename SP::pixel_sampler_type{},
            gen,
            std::forward<Args>(args)...
            );

    sample_pixel(
            detail::have_intersector_tag(),
            sparams.intersector,
            kernel,
            typename SP::pixel_sampler_type(),
            r,
            gen,
            frame_num,
            sparams.rt.ref(),
            std::forward<Args>(args)...
            );
}

} // basic_sched_impl


//-------------------------------------------------------------------------------------------------
// basic_sched implementation
//

template <typename B, typename R>
template <typename ...Args>
basic_sched<B, R>::basic_sched(Args&&... args)
    : backend_(std::forward<Args>(args)...)
{
}

template <typename B, typename R>
template <typename K, typename SP>
void basic_sched<B, R>::frame(K kernel, SP sched_params, unsigned frame_num)
{
    sched_params.cam.begin_frame();

    sched_params.rt.begin_frame();

    int pw = packet_size<typename R::scalar_type>::w;
    int ph = packet_size<typename R::scalar_type>::h;

    // Tile size must be be a multiple of packet size.
    int dx = round_up(16, pw);
    int dy = round_up(16, ph);

    int x0 = sched_params.scissor_box.x;
    int y0 = sched_params.scissor_box.y;

    int nx = x0 + sched_params.scissor_box.w;
    int ny = y0 + sched_params.scissor_box.h;

    backend_.for_each_packet(
        tiled_range2d<int>(x0, nx, dx, y0, ny, dy), pw, ph,
        [=](int x, int y)
        {
            auto gen = make_generator(
                    typename R::scalar_type{},
                    typename SP::pixel_sampler_type{},
                    detail::tic(typename R::scalar_type{})
                    );

            basic_sched_impl::call_sample_pixel(
                    typename detail::sched_params_has_intersector<SP>::type(),
                    R{},
                    kernel,
                    sched_params,
                    gen,
                    frame_num,
                    x,
                    y,
                    sched_params.rt.width(),
                    sched_params.rt.height(),
                    sched_params.cam
                    );
        });

    sched_params.rt.end_frame();

    sched_params.cam.end_frame();
}

template <typename B, typename R>
template <typename ...Args>
void basic_sched<B, R>::reset(Args&&... args)
{
    backend_.reset(std::forward<Args>(args)...);
}

} // visionaray
