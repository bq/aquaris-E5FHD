#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
#include "camera_AE_PLineTable_ov8825raw.h"
#include "camera_info_ov8825raw.h"
#include "camera_custom_AEPlinetable.h"
#include "camera_custom_tsf_tbl.h"
const NVRAM_CAMERA_ISP_PARAM_STRUCT CAMERA_ISP_DEFAULT_VALUE =
{{
    //Version
    Version: NVRAM_CAMERA_PARA_FILE_VERSION,
    //SensorId
    SensorId: SENSOR_ID,
    ISPComm:{
        {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        }
    },
    ISPPca:{
        #include INCLUDE_FILENAME_ISP_PCA_PARAM
    },
    ISPRegs:{
        #include INCLUDE_FILENAME_ISP_REGS_PARAM
        },
    ISPMfbMixer:{{
        {//00: MFB mixer for ISO 100
            0x00000000, 0x00000000
        },
        {//01: MFB mixer for ISO 200
            0x00000000, 0x00000000
        },
        {//02: MFB mixer for ISO 400
            0x00000000, 0x00000000
        },
        {//03: MFB mixer for ISO 800
            0x00000000, 0x00000000
        },
        {//04: MFB mixer for ISO 1600
            0x00000000, 0x00000000
        },
        {//05: MFB mixer for ISO 2400
            0x00000000, 0x00000000
        },
        {//06: MFB mixer for ISO 3200
            0x00000000, 0x00000000
        }
    }},
    ISPCcmPoly22:{
        66475,    // i4R_AVG
        14135,    // i4R_STD
        92825,    // i4B_AVG
        23889,    // i4B_STD
        {  // i4P00[9]
            5027500, -2032500, -440000, -687500, 3290000, -42500, 125000, -2220000, 4652500
        },
        {  // i4P10[9]
            889458, -985274, 115349, -86517, 17752, 68765, -6011, 663225, -666980
        },
        {  // i4P01[9]
            537883, -701716, 182779, -207822, -92890, 300712, -75984, -48949, 115461
        },
        {  // i4P20[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        {  // i4P11[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        {  // i4P02[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        }
    }
}};

const NVRAM_CAMERA_3A_STRUCT CAMERA_3A_NVRAM_DEFAULT_VALUE =
{
    NVRAM_CAMERA_3A_FILE_VERSION, // u4Version
    SENSOR_ID, // SensorId

    // AE NVRAM
    {
        // rDevicesInfo
        {
            1144,    // u4MinGain, 1024 base = 1x
            10240,    // u4MaxGain, 16x
            100,    // u4MiniISOGain, ISOxx  
            128,    // u4GainStepUnit, 1x/8 
            26355,    // u4PreExpUnit 
            30,    // u4PreMaxFrameRate
            17763,    // u4VideoExpUnit  
            30,    // u4VideoMaxFrameRate 
            512,   // u4Video2PreRatio, 1024 base = 1x
            17763,    // u4CapExpUnit 
            23,    // u4CapMaxFrameRate
            512,   // u4Cap2PreRatio, 1024 base = 1x
            24,      // u4LensFno, Fno = 2.8
            350     // u4FocusLength_100x
        },
        // rHistConfig
        {
            2,    // u4HistHighThres
            40,    // u4HistLowThres
            2,    // u4MostBrightRatio
            1,    // u4MostDarkRatio
            160,    // u4CentralHighBound
            20,    // u4CentralLowBound
            {240, 230, 220, 210, 200},    // u4OverExpThres[AE_CCT_STRENGTH_NUM] 
            {75, 108, 128, 148, 170},    // u4HistStretchThres[AE_CCT_STRENGTH_NUM] 
            {18, 22, 26, 30, 34}    // u4BlackLightThres[AE_CCT_STRENGTH_NUM] 
        },
        // rCCTConfig
        {
            TRUE,    // bEnableBlackLight
            TRUE,    // bEnableHistStretch
            FALSE,    // bEnableAntiOverExposure
            TRUE,    // bEnableTimeLPF
            FALSE,    // bEnableCaptureThres
            FALSE,    // bEnableVideoThres
            FALSE,    // bEnableStrobeThres
            47,    // u4AETarget
            47,    // u4StrobeAETarget
            50,    // u4InitIndex
            4,    // u4BackLightWeight
            32,    // u4HistStretchWeight
            4,    // u4AntiOverExpWeight
            2,    // u4BlackLightStrengthIndex
            2,    // u4HistStretchStrengthIndex
            2,    // u4AntiOverExpStrengthIndex
            2,    // u4TimeLPFStrengthIndex
            {1, 3, 5, 7, 8},    // u4LPFConvergeTable[AE_CCT_STRENGTH_NUM] 
            90,    // u4InDoorEV = 9.0, 10 base 
            -2,    // i4BVOffset delta BV = value/10 
            64,    // u4PreviewFlareOffset
            64,    // u4CaptureFlareOffset
            5,    // u4CaptureFlareThres
            64,    // u4VideoFlareOffset
            5,    // u4VideoFlareThres
            64,    // u4StrobeFlareOffset
            2,    // u4StrobeFlareThres
            8,    // u4PrvMaxFlareThres
            0,    // u4PrvMinFlareThres
            8,    // u4VideoMaxFlareThres
            0,    // u4VideoMinFlareThres
            18,    // u4FlatnessThres    // 10 base for flatness condition.
            50    // u4FlatnessStrength
        }
    },
    // AWB NVRAM
    {
        // AWB calibration data
        {
            // rUnitGain (unit gain: 1.0 = 512)
            {
                0,    // i4R
                0,    // i4G
                0    // i4B
            },
            // rGoldenGain (golden sample gain: 1.0 = 512)
            {
                0,    // i4R
                0,    // i4G
                0    // i4B
            },
            // rTuningUnitGain (Tuning sample unit gain: 1.0 = 512)
            {
                0,    // i4R
                0,    // i4G
                0    // i4B
            },
            // rD65Gain (D65 WB gain: 1.0 = 512)
            {
                829,    // i4R
                512,    // i4G
                591    // i4B
            }
        },
        // Original XY coordinate of AWB light source
        {
           // Strobe
            {
                55,    // i4X
                -344    // i4Y
            },
            // Horizon
            {
                -425,    // i4X
                -286    // i4Y
            },
            // A
            {
                -298,    // i4X
                -288    // i4Y
            },
            // TL84
            {
                -164,    // i4X
                -307    // i4Y
            },
            // CWF
            {
                -91,    // i4X
                -399    // i4Y
            },
            // DNP
            {
                -8,    // i4X
                -261    // i4Y
            },
            // D65
            {
                125,    // i4X
                -231    // i4Y
            },
            // DF
            {
                78,    // i4X
                -303    // i4Y
            }
        },
        // Rotated XY coordinate of AWB light source
        {
            // Strobe
            {
                6,    // i4X
                -349    // i4Y
            },
            // Horizon
            {
                -462,    // i4X
                -224    // i4Y
            },
            // A
            {
                -336,    // i4X
                -244    // i4Y
            },
            // TL84
            {
                -206,    // i4X
                -282    // i4Y
            },
            // CWF
            {
                -146,    // i4X
                -383    // i4Y
            },
            // DNP
            {
                -45,    // i4X
                -258    // i4Y
            },
            // D65
            {
                92,    // i4X
                -247    // i4Y
            },
            // DF
            {
                35,    // i4X
                -312    // i4Y
            }
        },
        // AWB gain of AWB light source
        {
            // Strobe 
            {
                878,    // i4R
                512,    // i4G
                757    // i4B
            },
            // Horizon 
            {
                512,    // i4R
                618,    // i4G
                1616    // i4B
            },
            // A 
            {
                512,    // i4R
                519,    // i4G
                1148    // i4B
            },
            // TL84 
            {
                621,    // i4R
                512,    // i4G
                969    // i4B
            },
            // CWF 
            {
                777,    // i4R
                512,    // i4G
                993    // i4B
            },
            // DNP 
            {
                721,    // i4R
                512,    // i4G
                737    // i4B
            },
            // D65 
            {
                829,    // i4R
                512,    // i4G
                591    // i4B
            },
            // DF 
            {
                857,    // i4R
                512,    // i4G
                694    // i4B
            }
        },
        // Rotation matrix parameter
        {
            8,    // i4RotationAngle
            254,    // i4Cos
            36    // i4Sin
        },
        // Daylight locus parameter
        {
            -167,    // i4SlopeNumerator
            128    // i4SlopeDenominator
        },
        // AWB light area
        {
            // Strobe:FIXME
            {
            0,    // i4RightBound
            0,    // i4LeftBound
            0,    // i4UpperBound
            0    // i4LowerBound
            },
            // Tungsten
            {
            -256,    // i4RightBound
            -906,    // i4LeftBound
            -184,    // i4UpperBound
            -284    // i4LowerBound
            },
            // Warm fluorescent
            {
            -256,    // i4RightBound
            -906,    // i4LeftBound
            -284,    // i4UpperBound
            -404    // i4LowerBound
            },
            // Fluorescent
            {
            -95,    // i4RightBound
            -256,    // i4LeftBound
            -175,    // i4UpperBound
            -332    // i4LowerBound
            },
            // CWF
            {
            -95,    // i4RightBound
            -256,    // i4LeftBound
            -332,    // i4UpperBound
            -433    // i4LowerBound
            },
            // Daylight
            {
            117,    // i4RightBound
            -95,    // i4LeftBound
            -167,    // i4UpperBound
            -327    // i4LowerBound
            },
            // Shade
            {
            477,    // i4RightBound
            117,    // i4LeftBound
            -167,    // i4UpperBound
            -327    // i4LowerBound
            },
            // Daylight Fluorescent
            {
            117,    // i4RightBound
            -95,    // i4LeftBound
            -327,    // i4UpperBound
            -433    // i4LowerBound
            }
        },
        // PWB light area
        {
            // Reference area
            {
            477,    // i4RightBound
            -906,    // i4LeftBound
            0,    // i4UpperBound
            -433    // i4LowerBound
            },
            // Daylight
            {
            142,    // i4RightBound
            -95,    // i4LeftBound
            -167,    // i4UpperBound
            -327    // i4LowerBound
            },
            // Cloudy daylight
            {
            242,    // i4RightBound
            67,    // i4LeftBound
            -167,    // i4UpperBound
            -327    // i4LowerBound
            },
            // Shade
            {
            342,    // i4RightBound
            67,    // i4LeftBound
            -167,    // i4UpperBound
            -327    // i4LowerBound
            },
            // Twilight
            {
            -95,    // i4RightBound
            -255,    // i4LeftBound
            -167,    // i4UpperBound
            -327    // i4LowerBound
            },
            // Fluorescent
            {
            142,    // i4RightBound
            -306,    // i4LeftBound
            -197,    // i4UpperBound
            -433    // i4LowerBound
            },
            // Warm fluorescent
            {
            -236,    // i4RightBound
            -436,    // i4LeftBound
            -197,    // i4UpperBound
            -433    // i4LowerBound
            },
            // Incandescent
            {
            -236,    // i4RightBound
            -436,    // i4LeftBound
            -167,    // i4UpperBound
            -327    // i4LowerBound
            },
            // Gray World
            {
            5000,    // i4RightBound
            -5000,    // i4LeftBound
            5000,    // i4UpperBound
            -5000    // i4LowerBound
            }
        },
        // PWB default gain	
        {
            // Daylight
            {
            767,    // i4R
            512,    // i4G
            656    // i4B
            },
            // Cloudy daylight
            {
            891,    // i4R
            512,    // i4G
            537    // i4B
            },
            // Shade
            {
            944,    // i4R
            512,    // i4G
            497    // i4B
            },
            // Twilight
            {
            611,    // i4R
            512,    // i4G
            888    // i4B
            },
            // Fluorescent
            {
            754,    // i4R
            512,    // i4G
            833    // i4B
            },
            // Warm fluorescent
            {
            563,    // i4R
            512,    // i4G
            1228    // i4B
            },
            // Incandescent
            {
            508,    // i4R
            512,    // i4G
            1136    // i4B
            },
            // Gray World
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            }
        },
        // AWB preference color	
        {
            // Tungsten
            {
            0,    // i4SliderValue
            6617    // i4OffsetThr
            },
            // Warm fluorescent	
            {
            0,    // i4SliderValue
            5784    // i4OffsetThr
            },
            // Shade
            {
            0,    // i4SliderValue
            1344    // i4OffsetThr
            },
            // Daylight WB gain
            {
            709,    // i4R
            512,    // i4G
            728    // i4B
            },
            // Preference gain: strobe
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: tungsten
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: warm fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: CWF
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: daylight
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: shade
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: daylight fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            }
        },
        {// CCT estimation
            {// CCT
                2300,    // i4CCT[0]
                2850,    // i4CCT[1]
                4100,    // i4CCT[2]
                5100,    // i4CCT[3]
                6500    // i4CCT[4]
            },
            {// Rotated X coordinate
                -554,    // i4RotatedXCoordinate[0]
                -428,    // i4RotatedXCoordinate[1]
                -298,    // i4RotatedXCoordinate[2]
                -137,    // i4RotatedXCoordinate[3]
                0    // i4RotatedXCoordinate[4]
            }
        }
    },
    {0}
};

#include INCLUDE_FILENAME_ISP_LSC_PARAM
//};  //  namespace

const CAMERA_TSF_TBL_STRUCT CAMERA_TSF_DEFAULT_VALUE =
{
    #include INCLUDE_FILENAME_TSF_PARA
    #include INCLUDE_FILENAME_TSF_DATA
};


typedef NSFeature::RAWSensorInfo<SENSOR_ID> SensorInfoSingleton_T;


namespace NSFeature {
template <>
UINT32
SensorInfoSingleton_T::
impGetDefaultData(CAMERA_DATA_TYPE_ENUM const CameraDataType, VOID*const pDataBuf, UINT32 const size) const
{
    UINT32 dataSize[CAMERA_DATA_TYPE_NUM] = {sizeof(NVRAM_CAMERA_ISP_PARAM_STRUCT),
                                             sizeof(NVRAM_CAMERA_3A_STRUCT),
                                             sizeof(NVRAM_CAMERA_SHADING_STRUCT),
                                             sizeof(NVRAM_LENS_PARA_STRUCT),
                                             sizeof(AE_PLINETABLE_T),
                                             0,
                                             sizeof(CAMERA_TSF_TBL_STRUCT)};

    if (CameraDataType > CAMERA_DATA_TSF_TABLE || NULL == pDataBuf || (size < dataSize[CameraDataType]))
    {
        return 1;
    }

    switch(CameraDataType)
    {
        case CAMERA_NVRAM_DATA_ISP:
            memcpy(pDataBuf,&CAMERA_ISP_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_ISP_PARAM_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_3A:
            memcpy(pDataBuf,&CAMERA_3A_NVRAM_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_3A_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_SHADING:
            memcpy(pDataBuf,&CAMERA_SHADING_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_SHADING_STRUCT));
            break;
        case CAMERA_DATA_AE_PLINETABLE:
            memcpy(pDataBuf,&g_PlineTableMapping,sizeof(AE_PLINETABLE_T));
            break;
        case CAMERA_DATA_TSF_TABLE:
            memcpy(pDataBuf,&CAMERA_TSF_DEFAULT_VALUE,sizeof(CAMERA_TSF_TBL_STRUCT));
            break;
        default:
            break;
    }
    return 0;
}}; // NSFeature


