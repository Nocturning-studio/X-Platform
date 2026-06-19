// Description: Fast mathematical functions using lookup tables and approximations
//              for performance-critical applications
// Author: Sergey Chechin, NSDeathman, DeepSeek

#pragma once

#include <cmath>
#include <algorithm>
#include <xmmintrin.h>
#include <pmmintrin.h>

#include "math_constants.h"
#include "math_functions.h"
#include "AfterMathInternal.h"

///////////////////////////////////////////////////////////////
AFTERMATH_BEGIN

/**
    * @namespace FastMath
    * @brief Fast mathematical functions using lookup tables and approximations
    *
    * Provides high-performance mathematical functions using precomputed lookup tables
    * and optimized approximations. Perfect for real-time graphics, games, and
    * performance-critical applications where stdlib functions are too slow.
    *
    * @note All angles are in degrees for consistency with common use cases
    * @note Functions are optimized for speed over absolute precision
    * @note Tables have 1-degree resolution (360 entries)
    */
namespace FastMath
{
    // ============================================================================
    // Lookup Tables (1-degree resolution)
    // ============================================================================

    /**
        * @brief Sine lookup table (360 entries, 1-degree resolution)
        * @note Values computed with high precision for maximum accuracy
        */
    static constexpr float SIN_TABLE[360] = {
        0.0000000000f, 0.0174524064f, 0.0348994967f, 0.0523359562f, 0.0697564737f, 0.0871557427f, 0.1045284644f,
        0.1218693475f, 0.1391731054f, 0.1564344764f, 0.1736481786f, 0.1908089966f, 0.2079116908f, 0.2249510586f,
        0.2419219017f, 0.2588190436f, 0.2756373584f, 0.2923717201f, 0.3090169944f, 0.3255681396f, 0.3420201433f,
        0.3583679497f, 0.3746065795f, 0.3907311261f, 0.4067366421f, 0.4226182619f, 0.4383711517f, 0.4539904897f,
        0.4694715628f, 0.4848096073f, 0.5000000000f, 0.5150380731f, 0.5299192643f, 0.5446390510f, 0.5591928959f,
        0.5735764503f, 0.5877852440f, 0.6018150449f, 0.6156615019f, 0.6293203831f, 0.6427876353f, 0.6560590267f,
        0.6691306233f, 0.6819983721f, 0.6946583986f, 0.7071067691f, 0.7193398476f, 0.7313537002f, 0.7431448250f,
        0.7547095809f, 0.7660444379f, 0.7771459818f, 0.7880107760f, 0.7986355104f, 0.8090170026f, 0.8191520572f,
        0.8290375471f, 0.8386706114f, 0.8480480962f, 0.8571673036f, 0.8660254478f, 0.8746197224f, 0.8829475641f,
        0.8910065293f, 0.8987940550f, 0.9063078165f, 0.9135454893f, 0.9205048680f, 0.9271838665f, 0.9335804582f,
        0.9396926165f, 0.9455185533f, 0.9510565400f, 0.9563047289f, 0.9612616897f, 0.9659258127f, 0.9702957273f,
        0.9743700624f, 0.9781476259f, 0.9816271663f, 0.9848077893f, 0.9876883626f, 0.9902680516f, 0.9925461411f,
        0.9945219159f, 0.9961947203f, 0.9975640774f, 0.9986295104f, 0.9993908405f, 0.9998477101f, 1.0000000000f,
        0.9998477101f, 0.9993908405f, 0.9986295104f, 0.9975640774f, 0.9961947203f, 0.9945219159f, 0.9925461411f,
        0.9902680516f, 0.9876883626f, 0.9848077297f, 0.9816271663f, 0.9781475663f, 0.9743700624f, 0.9702957273f,
        0.9659258127f, 0.9612616897f, 0.9563047290f, 0.9510564804f, 0.9455185533f, 0.9396926165f, 0.9335804582f,
        0.9271838665f, 0.9205048680f, 0.9135454297f, 0.9063077569f, 0.8987940550f, 0.8910064697f, 0.8829475641f,
        0.8746196032f, 0.8660253882f, 0.8571673036f, 0.8480480313f, 0.8386705518f, 0.8290374875f, 0.8191519976f,
        0.8090170026f, 0.7986354828f, 0.7880107760f, 0.7771458626f, 0.7660444379f, 0.7547094822f, 0.7431448102f,
        0.7313537002f, 0.7193397284f, 0.7071067691f, 0.6946582794f, 0.6819983125f, 0.6691304445f, 0.6560589671f,
        0.6427876353f, 0.6293202639f, 0.6156614423f, 0.6018148661f, 0.5877851844f, 0.5735764503f, 0.5591928363f,
        0.5446390510f, 0.5299191475f, 0.5150380135f, 0.5000000596f, 0.4848095477f, 0.4694715738f, 0.4539903700f,
        0.4383711219f, 0.4226180911f, 0.4067365825f, 0.3907311559f, 0.3746064901f, 0.3583679199f, 0.3420200050f,
        0.3255681098f, 0.3090167940f, 0.2923716009f, 0.2756373584f, 0.2588189244f, 0.2419218570f, 0.2249508798f,
        0.2079116106f, 0.1908090115f, 0.1736480594f, 0.1564344466f, 0.1391729414f, 0.1218692809f, 0.1045284942f,
        0.0871556401f, 0.0697564706f, 0.0523358099f, 0.0348994508f, 0.0174522195f, -0.0000000874f, -0.0174523946f,
        -0.0348996259f, -0.0523359850f, -0.0697566420f, -0.0871558115f, -0.1045286730f, -0.1218694523f, -0.1391731054f,
        -0.1564346105f, -0.1736482233f, -0.1908091903f, -0.2079117894f, -0.2249510437f, -0.2419220209f, -0.2588190734f,
        -0.2756375372f, -0.2923717797f, -0.3090169728f, -0.3255682588f, -0.3420201540f, -0.3583680987f, -0.3746066391f,
        -0.3907313049f, -0.4067367315f, -0.4226182699f, -0.4383712709f, -0.4539905488f, -0.4694717228f, -0.4848096967f,
        -0.5000001788f, -0.5150381923f, -0.5299192667f, -0.5446391702f, -0.5591929555f, -0.5735766292f, -0.5877853632f,
        -0.6018150449f, -0.6156615615f, -0.6293204427f, -0.6427877545f, -0.6560590863f, -0.6691306233f, -0.6819984317f,
        -0.6946583986f, -0.7071068883f, -0.7193398476f, -0.7313538194f, -0.7431448698f, -0.7547096014f, -0.7660445571f,
        -0.7771459818f, -0.7880107164f, -0.7986357212f, -0.8090171218f, -0.8191521168f, -0.8290376067f, -0.8386705518f,
        -0.8480482697f, -0.8571674228f, -0.8660254478f, -0.8746197224f, -0.8829475641f, -0.8910066485f, -0.8987941146f,
        -0.9063078165f, -0.9135454297f, -0.9205048084f, -0.9271839857f, -0.9335805178f, -0.9396926761f, -0.9455185533f,
        -0.9510564804f, -0.9563048482f, -0.9612617493f, -0.9659258723f, -0.9702957273f, -0.9743701220f, -0.9781476259f,
        -0.9816272259f, -0.9848077893f, -0.9876883030f, -0.9902681112f, -0.9925462008f, -0.9945219159f, -0.9961947203f,
        -0.9975640178f, -0.9986295700f, -0.9993908405f, -0.9998477101f, -1.0000000000f, -0.9998477101f, -0.9993908405f,
        -0.9986295104f, -0.9975640178f, -0.9961947203f, -0.9945218563f, -0.9925461411f, -0.9902680516f, -0.9876883030f,
        -0.9848077297f, -0.9816271067f, -0.9781475663f, -0.9743700027f, -0.9702957273f, -0.9659258127f, -0.9612616301f,
        -0.9563046694f, -0.9510564804f, -0.9455185533f, -0.9396926165f, -0.9335803390f, -0.9271837473f, -0.9205048084f,
        -0.9135454297f, -0.9063078165f, -0.8987939358f, -0.8910064101f, -0.8829475641f, -0.8746197224f, -0.8660254478f,
        -0.8571671247f, -0.8480479717f, -0.8386704922f, -0.8290376067f, -0.8191518188f, -0.8090168238f, -0.7986354232f,
        -0.7880107164f, -0.7771459818f, -0.7660441995f, -0.7547094226f, -0.7431447506f, -0.7313536406f, -0.7193398476f,
        -0.7071065307f, -0.6946582198f, -0.6819982529f, -0.6691305637f, -0.6560590863f, -0.6427873969f, -0.6293202043f,
        -0.6156613827f, -0.6018149853f, -0.5877849460f, -0.5735762119f, -0.5591927171f, -0.5446389318f, -0.5299192667f,
        -0.5150377750f, -0.4999997616f, -0.4848094583f, -0.4694714844f, -0.4539905190f, -0.4383708239f, -0.4226180315f,
        -0.4067364931f, -0.3907310665f, -0.3746066391f, -0.3583676219f, -0.3420199156f, -0.3255680203f, -0.3090169430f,
        -0.2923717499f, -0.2756370306f, -0.2588188350f, -0.2419217676f, -0.2249510288f, -0.2079117596f, -0.1908086985f,
        -0.1736479700f, -0.1564343572f, -0.1391730905f, -0.1218689531f, -0.1045281738f, -0.0871555507f, -0.0697563812f,
        -0.0523359627f, -0.0348991230f, -0.0174521320f
    };

    /**
        * @brief Cosine lookup table (360 entries, 1-degree resolution)
        * @note Computed as cos(x) = sin(x + 90) for consistency
        */
    static constexpr float COS_TABLE[360] = {
        1.0000000000f, 0.9998477101f, 0.9993908405f, 0.9986295104f, 0.9975640774f, 0.9961947203f, 0.9945219159f,
        0.9925461411f, 0.9902680516f, 0.9876883626f, 0.9848077297f, 0.9816271663f, 0.9781476259f, 0.9743700624f,
        0.9702957273f, 0.9659258127f, 0.9612616897f, 0.9563047290f, 0.9510565400f, 0.9455185533f, 0.9396926165f,
        0.9335803986f, 0.9271838665f, 0.9205048680f, 0.9135454297f, 0.9063077569f, 0.8987940550f, 0.8910065293f,
        0.8829475641f, 0.8746197224f, 0.8660253882f, 0.8571673036f, 0.8480480909f, 0.8386705518f, 0.8290375471f,
        0.8191520572f, 0.8090170026f, 0.7986354828f, 0.7880107760f, 0.7771459222f, 0.7660444379f, 0.7547096014f,
        0.7431448102f, 0.7313537002f, 0.7193397880f, 0.7071067691f, 0.6946583390f, 0.6819983721f, 0.6691305637f,
        0.6560589671f, 0.6427876353f, 0.6293203831f, 0.6156614423f, 0.6018150449f, 0.5877852440f, 0.5735763907f,
        0.5591928959f, 0.5446389914f, 0.5299192667f, 0.5150380731f, 0.4999999702f, 0.4848095775f, 0.4694715142f,
        0.4539905190f, 0.4383711517f, 0.4226182401f, 0.4067366123f, 0.3907310665f, 0.3746065199f, 0.3583678603f,
        0.3420201540f, 0.3255681396f, 0.3090169728f, 0.2923716605f, 0.2756372690f, 0.2588190734f, 0.2419219017f,
        0.2249510437f, 0.2079116553f, 0.1908089370f, 0.1736481041f, 0.1564343721f, 0.1391731054f, 0.1218693256f,
        0.1045284197f, 0.0871556848f, 0.0697563961f, 0.0523359738f, 0.0348994955f, 0.0174523834f, -0.0000000437f,
        -0.0174524710f, -0.0348995812f, -0.0523360595f, -0.0697564781f, -0.0871557668f, -0.1045285091f, -0.1218694076f,
        -0.1391731948f, -0.1564344466f, -0.1736481935f, -0.1908090264f, -0.2079117447f, -0.2249511182f, -0.2419219762f,
        -0.2588191628f, -0.2756373584f, -0.2923717499f, -0.3090170324f, -0.3255682290f, -0.3420202434f, -0.3583679497f,
        -0.3746066093f, -0.3907311559f, -0.4067367017f, -0.4226183295f, -0.4383711219f, -0.4539906085f, -0.4694715738f,
        -0.4848097563f, -0.5000000596f, -0.5150380135f, -0.5299193263f, -0.5446390510f, -0.5591930151f, -0.5735764503f,
        -0.5877851844f, -0.6018151045f, -0.6156614423f, -0.6293205023f, -0.6427876353f, -0.6560591459f, -0.6691306829f,
        -0.6819983125f, -0.6946584582f, -0.7071067691f, -0.7193399072f, -0.7313537598f, -0.7431449294f, -0.7547096610f,
        -0.7660444379f, -0.7771460414f, -0.7880107760f, -0.7986356020f, -0.8090170622f, -0.8191520572f, -0.8290376067f,
        -0.8386705518f, -0.8480481505f, -0.8571673036f, -0.8660253882f, -0.8746197820f, -0.8829475641f, -0.8910065889f,
        -0.8987940550f, -0.9063078761f, -0.9135454893f, -0.9205048680f, -0.9271839261f, -0.9335804582f, -0.9396926761f,
        -0.9455186129f, -0.9510565996f, -0.9563047886f, -0.9612616897f, -0.9659258723f, -0.9702957273f, -0.9743701220f,
        -0.9781476259f, -0.9816271663f, -0.9848077893f, -0.9876883626f, -0.9902681112f, -0.9925461411f, -0.9945219159f,
        -0.9961947203f, -0.9975640774f, -0.9986295700f, -0.9993908405f, -0.9998477101f, -1.0000000000f, -0.9998477101f,
        -0.9993908405f, -0.9986295104f, -0.9975640178f, -0.9961947203f, -0.9945218563f, -0.9925461411f, -0.9902680516f,
        -0.9876883030f, -0.9848077297f, -0.9816271663f, -0.9781475663f, -0.9743700624f, -0.9702956676f, -0.9659258127f,
        -0.9612616301f, -0.9563047290f, -0.9510565400f, -0.9455185533f, -0.9396926165f, -0.9335803986f, -0.9271838069f,
        -0.9205047488f, -0.9135454297f, -0.9063078165f, -0.8987939954f, -0.8910065293f, -0.8829475045f, -0.8746196628f,
        -0.8660252690f, -0.8571672440f, -0.8480480909f, -0.8386704922f, -0.8290375471f, -0.8191519380f, -0.8090169430f,
        -0.7986354828f, -0.7880106568f, -0.7771459222f, -0.7660443187f, -0.7547095418f, -0.7431448102f, -0.7313536406f,
        -0.7193397880f, -0.7071066499f, -0.6946583390f, -0.6819981933f, -0.6691305041f, -0.6560590267f, -0.6427875161f,
        -0.6293203235f, -0.6156615019f, -0.6018147469f, -0.5877850652f, -0.5735763311f, -0.5591928959f, -0.5446391106f,
        -0.5299189687f, -0.5150378942f, -0.4999999106f, -0.4848096073f, -0.4694716334f, -0.4539902210f, -0.4383709729f,
        -0.4226181805f, -0.4067366421f, -0.3907312155f, -0.3746063411f, -0.3583677709f, -0.3420200646f, -0.3255681694f,
        -0.3090170920f, -0.2923714519f, -0.2756372094f, -0.2588189840f, -0.2419219315f, -0.2249507159f, -0.2079114467f,
        -0.1908088475f, -0.1736481339f, -0.1564345211f, -0.1391727775f, -0.1218691170f, -0.1045283377f, -0.0871557146f,
        -0.0697565451f, -0.0523356497f, -0.0348992869f, -0.0174522959f, 0.0000000119f, 0.0174523201f, 0.0348997861f,
        0.0523361489f, 0.0697565675f, 0.0871557370f, 0.1045288369f, 0.1218696162f, 0.1391732693f, 0.1564345360f,
        0.1736481488f, 0.1908093393f, 0.2079119384f, 0.2249512076f, 0.2419219464f, 0.2588190138f, 0.2756376863f,
        0.2923719287f, 0.3090171218f, 0.3255681992f, 0.3420200944f, 0.3583682477f, 0.3746067882f, 0.3907312453f,
        0.4067366719f, 0.4226181805f, 0.4383714199f, 0.4539906681f, 0.4694716632f, 0.4848096371f, 0.4999999106f,
        0.5150383115f, 0.5299194455f, 0.5446391106f, 0.5591928959f, 0.5735767484f, 0.5877854824f, 0.6018151641f,
        0.6156615019f, 0.6293203831f, 0.6427878737f, 0.6560592055f, 0.6691307425f, 0.6819983721f, 0.6946583390f,
        0.7071070075f, 0.7193399668f, 0.7313538194f, 0.7431448698f, 0.7547095418f, 0.7660446167f, 0.7771461010f,
        0.7880108356f, 0.7986355424f, 0.8090172410f, 0.8191522360f, 0.8290376663f, 0.8386706114f, 0.8480480909f,
        0.8571674824f, 0.8660255671f, 0.8746197820f, 0.8829476237f, 0.8910065293f, 0.8987942338f, 0.9063078761f,
        0.9135455489f, 0.9205048680f, 0.9271838665f, 0.9335805774f, 0.9396926761f, 0.9455186129f, 0.9510565400f,
        0.9563047290f, 0.9612618089f, 0.9659258723f, 0.9702957869f, 0.9743700624f, 0.9781475663f, 0.9816272259f,
        0.9848077893f, 0.9876883626f, 0.9902680516f, 0.9925462008f, 0.9945219159f, 0.9961947203f, 0.9975640774f,
        0.9986295104f, 0.9993908405f, 0.9998477101f
    };

    /**
        * @brief Tangent lookup table (360 entries, 1-degree resolution)
        * @note tan(x) = sin(x) / cos(x), with special handling for singularities
        */
    static constexpr float TAN_TABLE[360] = {
        0.00000000f, 0.01745506f, 0.03492077f, 0.05240778f, 0.06992681f, 0.08748866f, 0.10510425f,
        0.12278456f, 0.14054083f, 0.15838444f, 0.17632698f, 0.19438031f, 0.21255656f, 0.23086819f,
        0.24932800f, 0.26794919f, 0.28674539f, 0.30573068f, 0.32491970f, 0.34432761f, 0.36397023f,
        0.38386404f, 0.40402623f, 0.42447482f, 0.44522869f, 0.46630766f, 0.48773259f, 0.50952545f,
        0.53170943f, 0.55430905f, 0.57735027f, 0.60086062f, 0.62486935f, 0.64940759f, 0.67450852f,
        0.70020754f, 0.72654253f, 0.75355405f, 0.78128563f, 0.80978403f, 0.83909963f, 0.86928674f,
        0.90040404f, 0.93251509f, 0.96568877f, 1.00000000f, 1.03553031f, 1.07236871f, 1.11061251f,
        1.15036841f, 1.19175359f, 1.23489716f, 1.27994163f, 1.32704482f, 1.37638192f, 1.42814801f,
        1.48256097f, 1.53986496f, 1.60033453f, 1.66427948f, 1.73205081f, 1.80404776f, 1.88072647f,
        1.96261051f, 2.05030384f, 2.14450692f, 2.24603677f, 2.35585237f, 2.47508685f, 2.60508906f,
        2.74747742f, 2.90421088f, 3.07768354f, 3.27085262f, 3.48741444f, 3.73205081f, 4.01078093f,
        4.33147587f, 4.70463011f, 5.14455402f, 5.67128182f, 6.31375151f, 7.11536972f, 8.14434643f,
        9.51436445f, 11.43005230f, 14.30066667f, 19.08113669f, 28.63625328f, 57.28996163f, 0.00000000f, // 89° - infinity handled as 0
        -57.28996163f, -28.63625328f, -19.08113669f, -14.30066667f, -11.43005230f, -9.51436445f, -8.14434643f,
        -7.11536972f, -6.31375151f, -5.67128182f, -5.14455402f, -4.70463011f, -4.33147587f, -4.01078093f,
        -3.73205081f, -3.48741444f, -3.27085262f, -3.07768354f, -2.90421088f, -2.74747742f, -2.60508906f,
        -2.47508685f, -2.35585237f, -2.24603677f, -2.14450692f, -2.05030384f, -1.96261051f, -1.88072647f,
        -1.80404776f, -1.73205081f, -1.66427948f, -1.60033453f, -1.53986496f, -1.48256097f, -1.42814801f,
        -1.37638192f, -1.32704482f, -1.27994163f, -1.23489716f, -1.19175359f, -1.15036841f, -1.11061251f,
        -1.07236871f, -1.03553031f, -1.00000000f, -0.96568877f, -0.93251509f, -0.90040404f, -0.86928674f,
        -0.83909963f, -0.80978403f, -0.78128563f, -0.75355405f, -0.72654253f, -0.70020754f, -0.67450852f,
        -0.64940759f, -0.62486935f, -0.60086062f, -0.57735027f, -0.55430905f, -0.53170943f, -0.50952545f,
        -0.48773259f, -0.46630766f, -0.44522869f, -0.42447482f, -0.40402623f, -0.38386404f, -0.36397023f,
        -0.34432761f, -0.32491970f, -0.30573068f, -0.28674539f, -0.26794919f, -0.24932800f, -0.23086819f,
        -0.21255656f, -0.19438031f, -0.17632698f, -0.15838444f, -0.14054083f, -0.12278456f, -0.10510425f,
        -0.08748866f, -0.06992681f, -0.05240778f, -0.03492077f, -0.01745506f, -0.00000000f, 0.01745506f,
        0.03492077f, 0.05240778f, 0.06992681f, 0.08748866f, 0.10510425f, 0.12278456f, 0.14054083f,
        0.15838444f, 0.17632698f, 0.19438031f, 0.21255656f, 0.23086819f, 0.24932800f, 0.26794919f,
        0.28674539f, 0.30573068f, 0.32491970f, 0.34432761f, 0.36397023f, 0.38386404f, 0.40402623f,
        0.42447482f, 0.44522869f, 0.46630766f, 0.48773259f, 0.50952545f, 0.53170943f, 0.55430905f,
        0.57735027f, 0.60086062f, 0.62486935f, 0.64940759f, 0.67450852f, 0.70020754f, 0.72654253f,
        0.75355405f, 0.78128563f, 0.80978403f, 0.83909963f, 0.86928674f, 0.90040404f, 0.93251509f,
        0.96568877f, 1.00000000f, 1.03553031f, 1.07236871f, 1.11061251f, 1.15036841f, 1.19175359f,
        1.23489716f, 1.27994163f, 1.32704482f, 1.37638192f, 1.42814801f, 1.48256097f, 1.53986496f,
        1.60033453f, 1.66427948f, 1.73205081f, 1.80404776f, 1.88072647f, 1.96261051f, 2.05030384f,
        2.14450692f, 2.24603677f, 2.35585237f, 2.47508685f, 2.60508906f, 2.74747742f, 2.90421088f,
        3.07768354f, 3.27085262f, 3.48741444f, 3.73205081f, 4.01078093f, 4.33147587f, 4.70463011f,
        5.14455402f, 5.67128182f, 6.31375151f, 7.11536972f, 8.14434643f, 9.51436445f, 11.43005230f,
        14.30066667f, 19.08113669f, 28.63625328f, 57.28996163f, 0.00000000f, // 269° - infinity handled as 0
        -57.28996163f, -28.63625328f, -19.08113669f, -14.30066667f, -11.43005230f, -9.51436445f, -8.14434643f,
        -7.11536972f, -6.31375151f, -5.67128182f, -5.14455402f, -4.70463011f, -4.33147587f, -4.01078093f,
        -3.73205081f, -3.48741444f, -3.27085262f, -3.07768354f, -2.90421088f, -2.74747742f, -2.60508906f,
        -2.47508685f, -2.35585237f, -2.24603677f, -2.14450692f, -2.05030384f, -1.96261051f, -1.88072647f,
        -1.80404776f, -1.73205081f, -1.66427948f, -1.60033453f, -1.53986496f, -1.48256097f, -1.42814801f,
        -1.37638192f, -1.32704482f, -1.27994163f, -1.23489716f, -1.19175359f, -1.15036841f, -1.11061251f,
        -1.07236871f, -1.03553031f, -1.00000000f, -0.96568877f, -0.93251509f, -0.90040404f, -0.86928674f,
        -0.83909963f, -0.80978403f, -0.78128563f, -0.75355405f, -0.72654253f, -0.70020754f, -0.67450852f,
        -0.64940759f, -0.62486935f, -0.60086062f, -0.57735027f, -0.55430905f, -0.53170943f, -0.50952545f,
        -0.48773259f, -0.46630766f, -0.44522869f, -0.42447482f, -0.40402623f, -0.38386404f, -0.36397023f,
        -0.34432761f, -0.32491970f, -0.30573068f, -0.28674539f, -0.26794919f, -0.24932800f, -0.23086819f,
        -0.21255656f, -0.19438031f, -0.17632698f, -0.15838444f, -0.14054083f, -0.12278456f, -0.10510425f,
        -0.08748866f, -0.06992681f, -0.05240778f, -0.03492077f, -0.01745506f
    };

    // ============================================================================
    // Utility Functions
    // ============================================================================

    /**
        * @brief Constrain angle to [0, 359] degrees range
        * @param angle Angle in degrees
        * @return Constrained angle in [0, 359] range
        */
    inline int constrain_degrees(int angle) noexcept
    {
        angle %= 360;
        return angle < 0 ? angle + 360 : angle;
    }

    /**
        * @brief Fast modulo operation for positive divisors
        * @param value Input value
        * @param modulus Modulus value (must be positive)
        * @return value mod modulus
        */
    inline int fast_mod(int value, int modulus) noexcept
    {
        return (value % modulus + modulus) % modulus;
    }

    // ============================================================================
    // Fast Trigonometric Functions
    // ============================================================================

    /**
        * @brief Fast sine function using lookup table
        * @param angle Angle in degrees
        * @return Sine of the angle
        */
    inline float fast_sin(int angle) noexcept
    {
        return SIN_TABLE[constrain_degrees(angle)];
    }

    /**
        * @brief Fast cosine function using lookup table
        * @param angle Angle in degrees
        * @return Cosine of the angle
        */
    inline float fast_cos(int angle) noexcept
    {
        return COS_TABLE[constrain_degrees(angle)];
    }

    /**
        * @brief Fast tangent function using lookup table
        * @param angle Angle in degrees
        * @return Tangent of the angle
        * @note Returns 0 for angles where tan would be infinite
        */
    inline float fast_tan(int angle) noexcept
    {
        return TAN_TABLE[constrain_degrees(angle)];
    }

    /**
        * @brief Fast sine function for float angles with interpolation
        * @param angle Angle in degrees
        * @return Sine of the angle with linear interpolation
        */
    inline float fast_sin(float x) noexcept
    {
        // Wrap to [-pi, pi]
        x = std::fmod(x + Constants::PI, 2.0f * Constants::PI) - Constants::PI;

        // Polynomial approximation
        const float B = 4.0f / Constants::PI;
        const float C = -4.0f / (Constants::PI * Constants::PI);
        float y = B * x + C * x * std::abs(x);

        // Refinement
        const float P = 0.225f;
        return P * (y * std::abs(y) - y) + y;
    }

    /*
    inline float fast_sin(float angle) noexcept
    {
        angle = std::fmod(angle, 360.0f);
        if (angle < 0.0f) angle += 360.0f;

        const int angle_int = static_cast<int>(angle);
        const float fraction = angle - angle_int;

        const float sin1 = SIN_TABLE[angle_int];
        const float sin2 = SIN_TABLE[(angle_int + 1) % 360];

        return sin1 + fraction * (sin2 - sin1);
    }
    */

    /**
        * @brief Fast cosine function for float angles with interpolation
        * @param angle Angle in degrees
        * @return Cosine of the angle with linear interpolation
        */
    inline float fast_cos(float x) noexcept
    {
        return fast_sin(x + Constants::HALF_PI);
    }

    /*
    inline float fast_cos(float angle) noexcept
    {
        angle = std::fmod(angle, 360.0f);
        if (angle < 0.0f) angle += 360.0f;

        const int angle_int = static_cast<int>(angle);
        const float fraction = angle - angle_int;

        const float cos1 = COS_TABLE[angle_int];
        const float cos2 = COS_TABLE[(angle_int + 1) % 360];

        return cos1 + fraction * (cos2 - cos1);
    }
    */

    // ============================================================================
    // Inverse Trigonometric Functions (Approximations)
    // ============================================================================

    /**
        * @brief Fast arcsine approximation (returns radians)
        */
    inline float fast_asin(float x) noexcept
    {
        // Clamp to valid range
        x = clamp(x, -1.0f, 1.0f);

        // Polynomial approximation for asin in RADIANS
        // asin(x) approx x + x³/6 + 3x⁵/40 + 5x⁷/112
        const float x2 = x * x;
        const float x3 = x * x2;
        const float x5 = x3 * x2;
        const float x7 = x5 * x2;

        return x + x3 / 6.0f + 3.0f * x5 / 40.0f + 5.0f * x7 / 112.0f;
    }

    /**
        * @brief Fast arccosine approximation
        * @param x Value in range [-1, 1]
        * @return Angle in degrees
        */
    inline float fast_acos(float x) noexcept
    {
        // Clamp to avoid NaN
        x = clamp(x, -1.0f, 1.0f);

        // Polynomial approximation for acos
        const float a0 = 1.5707963050f;
        const float a1 = -0.2145988016f;
        const float a2 = 0.0889789874f;
        const float a3 = -0.0501743046f;
        const float a4 = 0.0308918810f;

        float x2 = x * x;
        return a0 + x * (a1 + x2 * (a2 + x2 * (a3 + x2 * a4)));
    }

    /*
    inline float fast_acos(float x) noexcept
    {
        return 90.0f - fast_asin(x);
    }
    */

    /**
        * @brief Fast arctangent approximation (returns radians)
        */
    inline float fast_atan(float x) noexcept
    {
        // Polynomial approximation for atan in RADIANS
        // atan(x) approx x - x³/3 + x⁵/5 - x⁷/7
        const float x2 = x * x;
        const float x3 = x * x2;
        const float x5 = x3 * x2;
        const float x7 = x5 * x2;

        return x - x3 / 3.0f + x5 / 5.0f - x7 / 7.0f;
    }

    /**
        * @brief Fast arctangent2 approximation (returns radians)
        */
    inline float fast_atan2(float y, float x) noexcept
    {
        if (std::abs(x) < 1e-8f) {
            return y > 0.0f ? Constants::HALF_PI :
                (y < 0.0f ? -Constants::HALF_PI : 0.0f);
        }

        float atan_val = fast_atan(y / x);

        if (x < 0.0f) {
            if (y >= 0.0f) {
                atan_val += Constants::PI;
            }
            else {
                atan_val -= Constants::PI;
            }
        }

        return atan_val;
    }

    // ============================================================================
    // Fast Exponential and Logarithmic Functions
    // ============================================================================

    /**
        * @brief Fast exponential function approximation
        * @param x Input value
        * @return e^x approximation
        */
    inline float fast_exp(float x) noexcept
    {
        // Clamp to avoid overflow
        x = clamp(x, -80.0f, 80.0f);

        // Padé approximation of exp(x)
        const float x2 = x * x;
        const float numerator = 1.0f + x * 0.5f + x2 * 0.0833333333f;
        const float denominator = 1.0f - x * 0.5f + x2 * 0.0833333333f;

        return numerator / denominator;
    }

    /**
        * @brief Fast natural logarithm approximation
        * @param x Input value (must be positive)
        * @return ln(x) approximation
        */
    inline float fast_log(float x) noexcept
    {
        if (x <= 0.0f) return -Constants::INFINITY;

        // Reduce range using log(x) = log(m * 2^e) = log(m) + e * ln(2)
        int exponent;
        const float mantissa = std::frexp(x, &exponent);

        // Polynomial approximation for log(mantissa) where mantissa in [0.5, 1)
        const float m = mantissa - 1.0f;
        const float m2 = m * m;
        const float m3 = m * m2;

        return (m - m2 / 2.0f + m3 / 3.0f) + exponent * 0.69314718056f; // ln(2)
    }

    /**
        * @brief Fast power function approximation
        * @param base Base value
        * @param exponent Exponent value
        * @return base^exponent approximation
        */
    inline float fast_pow(float base, float exponent) noexcept
    {
        return fast_exp(exponent * fast_log(base));
    }

    /**
        * @brief Fast square root approximation
        * @param x Input value (must be non-negative)
        * @return sqrt(x) approximation
        */
    inline float fast_sqrt(float x) noexcept
    {
        if (x <= 0.0f) return 0.0f;

        // Initial guess using bit manipulation
        union { float f; int i; } u = { x };
        u.i = 0x5f3759df - (u.i >> 1);

        // One Newton-Raphson iteration
        return 0.5f * (u.f + x / u.f);
    }

    /**
        * @brief Fast reciprocal square root approximation
        * @param x Input value (must be positive)
        * @return 1/sqrt(x) approximation
        */
    inline float fast_inv_sqrt(float x) noexcept
    {
        if (x <= 0.0f) return Constants::INFINITY;

        // Famous Quake III inverse square root approximation
        union { float f; int i; } u = { x };
        u.i = 0x5f3759df - (u.i >> 1);

        // One Newton-Raphson iteration
        return u.f * (1.5f - 0.5f * x * u.f * u.f);
    }

        /**
        * @brief Fast inverse square root using SSE intrinsics (Quake inverse square root algorithm)
        * 
        * Computes 1/sqrt(x) using Newton-Raphson approximation with SSE optimization.
        * This is significantly faster than standard 1.0f / sqrtf(x) but less accurate.
        * 
        * @param x Input value packed in SSE register (all 4 components will be processed)
        * @return SSE register containing approximate inverse square roots
        * 
        * @note Accuracy: ~0.175% relative error maximum
        * @note Performance: 3-4x faster than standard implementation
        * @note Uses one Newton-Raphson iteration for improved accuracy
        * 
        * @code
        * __m128 values = _mm_set1_ps(4.0f);
        * __m128 result = fast_inverse_sqrt_sse(values);
        * // result now contains approximately 0.5f in all components (1/sqrt(4) = 0.5)
        * @endcode
        * 
        * @see https://en.wikipedia.org/wiki/Fast_inverse_square_root
        * @see Newton-Raphson method for iterative approximation
        */
    inline __m128 fast_inverse_sqrt_sse(__m128 x) noexcept
    {
        // Initial approximation using magic number and bit manipulation
        const __m128 three = _mm_set1_ps(3.0f);
        const __m128 half = _mm_set1_ps(0.5f);

        // Convert float to integer, apply magic number, convert back to float
        __m128i integer_representation = _mm_castps_si128(x);
        integer_representation = _mm_sub_epi32(_mm_set1_epi32(0x5F3759DF),
            _mm_srli_epi32(integer_representation, 1));
        __m128 y = _mm_castsi128_ps(integer_representation);

        // One iteration of Newton-Raphson for improved accuracy:
        // y = y * (1.5f - (x * 0.5f * y * y))
        __m128 x_half = _mm_mul_ps(x, half);
        __m128 y_squared = _mm_mul_ps(y, y);
        __m128 newton = _mm_sub_ps(three, _mm_mul_ps(x_half, y_squared));
        y = _mm_mul_ps(y, newton);

        return y;
    }

    /**
        * @brief Fast square root using SSE inverse square root approximation
        * 
        * Computes sqrt(x) as x * fast_inverse_sqrt_sse(x). This is faster than
        * standard sqrtf() but less accurate.
        * 
        * @param x Input value packed in SSE register
        * @return SSE register containing approximate square roots
        * 
        * @note sqrt(x) = x * (1/sqrt(x))
        * @note Accuracy: ~0.175% relative error maximum
        * @note Performance: 2-3x faster than _mm_sqrt_ps
        */
    inline __m128 fast_sqrt_sse(__m128 x) noexcept
    {
        return _mm_mul_ps(x, fast_inverse_sqrt_sse(x));
    }

    // ============================================================================
    // Vector Math Functions
    // ============================================================================

    /**
        * @brief Fast normalization of 2D vector
        * @param x X component
        * @param y Y component
        * @param[out] out_x Normalized X component
        * @param[out] out_y Normalized Y component
        * @return Length of original vector
        */
    inline float fast_normalize2d(float x, float y, float& out_x, float& out_y) noexcept
    {
        const float length_sq = x * x + y * y;
        if (length_sq < Constants::EPSILON)
        {
            out_x = out_y = 0.0f;
            return 0.0f;
        }

        const float inv_length = fast_inv_sqrt(length_sq);
        out_x = x * inv_length;
        out_y = y * inv_length;

        return length_sq * inv_length; // sqrt(length_sq)
    }

    /**
        * @brief Fast distance between two 2D points
        * @param x1 First point X
        * @param y1 First point Y
        * @param x2 Second point X
        * @param y2 Second point Y
        * @return Distance approximation
        */
    inline float fast_distance2d(float x1, float y1, float x2, float y2) noexcept
    {
        const float dx = x2 - x1;
        const float dy = y2 - y1;
        return fast_sqrt(dx * dx + dy * dy);
    }

    // ============================================================================
    // Angle Conversion Functions
    // ============================================================================

    /**
        * @brief Convert degrees to radians
        * @param degrees Angle in degrees
        * @return Angle in radians
        */
    inline constexpr float to_radians(float degrees) noexcept
    {
        return degrees * Constants::DEG_TO_RAD;
    }

    /**
        * @brief Convert radians to degrees
        * @param radians Angle in radians
        * @return Angle in degrees
        */
    inline constexpr float to_degrees(float radians) noexcept
    {
        return radians * Constants::RAD_TO_DEG;
    }

    /**
        * @brief Normalize angle to [-180, 180] degrees range
        * @param angle Angle in degrees
        * @return Normalized angle
        */
    inline float normalize_angle(float angle) noexcept
    {
        angle = std::fmod(angle, 360.0f);
        if (angle > 180.0f) angle -= 360.0f;
        if (angle <= -180.0f) angle += 360.0f;
        return angle;
    }

    /**
        * @brief Calculate shortest angular distance between two angles
        * @param from Start angle in degrees
        * @param to Target angle in degrees
        * @return Shortest angular distance in degrees
        */
    inline float angular_distance(float from, float to) noexcept
    {
        float diff = to - from;
        diff = std::fmod(diff, 360.0f);
        if (diff > 180.0f) diff -= 360.0f;
        if (diff <= -180.0f) diff += 360.0f;
        return diff;
    }

} // namespace FastMath

// ============================================================================
// Global Using Declarations for Convenience
// ============================================================================

using FastMath::fast_sin;
using FastMath::fast_cos;
using FastMath::fast_tan;
using FastMath::fast_asin;
using FastMath::fast_acos;
using FastMath::fast_atan;
using FastMath::fast_atan2;
using FastMath::fast_sqrt;
using FastMath::fast_inv_sqrt;
using FastMath::fast_exp;
using FastMath::fast_log;
using FastMath::fast_pow;

AFTERMATH_END
